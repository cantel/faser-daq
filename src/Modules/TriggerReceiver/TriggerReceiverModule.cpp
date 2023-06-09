/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#include "TriggerReceiverModule.hpp"
#include "EventFormats/DAQFormats.hpp"
#include "EventFormats/TLBDataFragment.hpp"
#include "EventFormats/TLBMonitoringFragment.hpp"

#include <Utils/Binary.hpp>

using namespace DAQFormats;
using namespace TLBDataFormat;
using namespace TLBMonFormat;
using namespace daqling::utilities;
using namespace TLBDataFormat;
using namespace TLBMonFormat;
using namespace TriggerReceiver;
#define _ms 1000 // used for usleep

TriggerReceiverModule::TriggerReceiverModule(const std::string& n):FaserProcess(n) {
  auto cfg = getModuleSettings();

  INFO("In TriggerReceiverModule()");
  m_status = STATUS_OK;

  if (cfg.contains("BoardID") && cfg.contains("SCIP") && cfg.contains("DAQIP")){
    INFO("Using ethernet communication interface.");
    m_tlb = new TLBAccess(cfg["SCIP"].get<std::string>(), cfg["DAQIP"].get<std::string>(), false, cfg["BoardID"].get<int>() );
  }
  else{
    INFO("Some, or possibly all, parametrs SCIP, DAQIP, BoardID have not been specified. Assuming we're communicating via USB!"); 
    m_tlb = new TLBAccess();
  }
  m_tlb->DisableTrigger(); // make sure TLB not sending triggers
}

TriggerReceiverModule::~TriggerReceiverModule() { 
  INFO("Shutdown"); 
 
  delete m_tlb;
}

// optional (configuration can be handled in the constructor)
void TriggerReceiverModule::configure() {
  FaserProcess::configure();

  registerVariable(m_physicsEventCount, "PhysicsEvents");
  registerVariable(m_physicsEventCount, "PhysicsRate", metrics::RATE);
  registerVariable(m_monitoringEventCount, "MonitoringEvents");
  registerVariable(m_monitoringEventCount, "MonitoringRate", metrics::RATE);
  registerVariable(m_badFragmentsCount, "BadFragments");
  registerVariable(m_badFragmentsCount, "BadFragmentsRate", metrics::RATE);
  registerVariable(m_fragment_status, "FragmentStatus");
  registerVariable(m_trigger_payload_size, "TriggerPayloadSize");
  registerVariable(m_monitoring_payload_size, "MonitoringPayloadSize");
  registerVariable(m_dataRate, "DataRate", metrics::LAST_VALUE); // kB/s read via network socket
  registerVariable(m_missedL1, "MissedEventIDError");
  
  auto cfg = getModuleSettings();

  auto log_level = (m_config.getConfig())["loglevel"]["module"];
  m_tlb->SetDebug((log_level=="TRACE"?1:0)); //Set to 0 for no debug, to 1 for debug

  json cfg_LUTconfig = cfg["LUTConfig"];

  // fixed configs
  cfg["Reset"] = true;
  cfg["ECR"] = true;
  cfg["TriggerEnable"] = false;
  cfg["SoftwareTrigger"] = false;

  if (cfg["EnableMonitoringData"].get<bool>()) m_enable_monitoringdata = true;
  else m_enable_monitoringdata = false;
  if (cfg["EnableTriggerData"].get<bool>()) m_enable_triggerdata = true;
  else m_enable_triggerdata = false;

  INFO("Configuring TLB");
  if ( cfg_LUTconfig==nullptr ) {
    m_status=STATUS_ERROR;
    sleep(1); // wait for error state to appear in RC GUI.
    throw TriggerReceiverIssue(ERS_HERE,"No LUT configuration provided. TLB Configuration failed.");
  }
  
  // attempt configuration with cfg and appropriate LUT
  try {
    m_tlb->ConfigureAndVerifyTLB(cfg);
    m_tlb->ConfigureLUT(cfg_LUTconfig);
    INFO("Done.");  
  } catch ( TLBAccessException &e ){
      m_status=STATUS_ERROR;
      sleep(1);
      throw TLBAccesIssue(ERS_HERE,e);
  }

}

void TriggerReceiverModule::enableTrigger(const std::string &arg) {
  INFO("Got enableTrigger command with argument "<<arg);
  //auto myjson = getModuleSettings(); //Temporary while using USB.
  //int WhatToRead=0x0; //Temp
  //if ( m_enable_triggerdata ) readout_param |= TLBReadoutParameters::EnableTriggerData;
  //if ( m_enable_monitoringdata ) readout_param |= TLBReadoutParameters::EnableMonitoringData;
  //m_tlb->StartReadout(WhatToRead); //Temp
  m_status = TLBAccess::GPIOCheck(m_tlb, &TLBAccess::EnableTrigger, false,false); //Only enables trigger. Doesn't send ECR nor Reset 
  if (m_status) {
    ERROR("Issue encountered while enabling trigger. If trigger rate remains 0, try enabling trigger again.");
  }
}

void TriggerReceiverModule::disableTrigger(const std::string &arg) { //run with "command disableTrigger"
  INFO("Got disableTrigger command with argument "<<arg);
  m_status = TLBAccess::GPIOCheck(m_tlb, &TLBAccess::DisableTrigger);
  if (m_status) {
    ERROR("Issue encountered while disabling trigger. If still see trigger rate, try disabling trigger again.");
  }
  //m_tlb->StopReadout(); //Temporary while using USB. Should empty USB buffer.
  usleep(100); //Once ethernet is implemented you should either check if data is pushed or if timeout (100musec).  
}

void TriggerReceiverModule::sendECR() { //run with "command ECR"
  INFO("Received ECR command.");
  m_status = TLBAccess::GPIOCheck(m_tlb, &TLBAccess::SendECR);
  if (m_status){
    ERROR("Issue encountered while resetting the TLB L1 counter. Try resend ECR?");
  }
  else INFO("ECR successful.");
  INFO("ECR count = " << m_ECRcount);
  m_prev_event_id = 0;
}


void TriggerReceiverModule::start(unsigned run_num) {
  INFO("Starting...");
  uint16_t readout_param = TLBReadoutParameters::ReadoutFIFOReset;
  if ( m_enable_triggerdata ) readout_param |= TLBReadoutParameters::EnableTriggerData;
  if ( m_enable_monitoringdata ) readout_param |= TLBReadoutParameters::EnableMonitoringData;
  m_status = TLBAccess::GPIOCheck(m_tlb, &TLBAccess::StartReadout, readout_param );
  if (m_status) {
    THROW(TLBAccessException, "Board communication issue encountered while starting readout. Will not enable trigger.");
  }
  usleep(2000*_ms);//temporary - wait for all modules
  INFO("Enabling trigger...");
  m_status = TLBAccess::GPIOCheck(m_tlb, &TLBAccess::EnableTrigger, true,true); //sends ECR and Reset
  if (m_status){
    THROW(TLBAccessException, "Board communication issue encountered while enabling trigger.");
  }
  INFO("Starting software DAQ processing.");
  FaserProcess::start(run_num);
}

void TriggerReceiverModule::stop() {  
  INFO("Stopping readout.");
  m_status = TLBAccess::GPIOCheck(m_tlb, &TLBAccess::DisableTrigger);
  if (m_status){
    ERROR("Issue encountered while disabling trigger. Continuing tentatively...");
  }
  usleep(100*_ms);
  m_status = TLBAccess::GPIOCheck(m_tlb, &TLBAccess::StopReadout);
  if (m_status){
    ERROR("Issue encountered while stopping readout. Continuing tentatively...");
  }
  usleep(1000*_ms); //value to be tweaked. Should be large enough to empty the buffer.
  FaserProcess::stop(); //this turns m_run to false
}

void TriggerReceiverModule::runner() noexcept {
  INFO("Running...");
  
  std::vector<std::vector<uint32_t>> vector_of_raw_events;
  uint8_t  local_fragment_tag;
  uint32_t local_source_id    = SourceIDs::TriggerSourceID;
  uint64_t local_event_id;
  uint16_t local_bc_id;
  uint16_t local_trigger_bits(0);
  m_prev_event_id = 0;


  while (m_run || vector_of_raw_events.size()) {
    m_dataRate = m_tlb->GetDataRate();
    vector_of_raw_events = m_tlb->GetTLBEventData();
      
    if (vector_of_raw_events.size()==0){
      usleep(100); //this is to make sure we don't occupy CPU resources if no data is on output
    }
    else {
      for(std::vector<std::vector<uint32_t>>::size_type i=0; i<vector_of_raw_events.size(); i++){
        size_t total_size = vector_of_raw_events[i].size() * sizeof(uint32_t); //Event size in byte
        if (!total_size) continue;
        auto event = vector_of_raw_events[i].data();

        m_fragment_status = 0;  
        local_trigger_bits = 0;
     
        if (TLBDecode::IsMonitoringHeader(*event)){ // tlb monitoring data event
          local_fragment_tag=EventTags::TLBMonitoringTag;
          TLBMonitoringFragment tlb_fragment = TLBMonitoringFragment(event, total_size);
          local_event_id = tlb_fragment.event_id();
          local_bc_id = tlb_fragment.bc_id();
          if (!tlb_fragment.valid()) {
            WARNING("Corrupted trigger monitoring data fragment at triggered event count "<<m_physicsEventCount);
            m_fragment_status = EventStatus::CorruptedFragment;
            m_status = STATUS_WARN;
          }
          DEBUG("Monitoring fragment:\n"<<tlb_fragment<<"fragment size: "<<total_size<<", fragment status: "<<m_fragment_status<<", ECRcount: "<<m_ECRcount);
          m_monitoringEventCount+=1;
          m_monitoring_payload_size = total_size;
        }
        else { // trigger data event
          local_fragment_tag=EventTags::PhysicsTag;
          TLBDataFragment tlb_fragment = TLBDataFragment(event, total_size);
          local_event_id = tlb_fragment.event_id();
          local_bc_id = tlb_fragment.bc_id();
          if (tlb_fragment.valid()) {
            auto expected_event_id = m_prev_event_id+1;
            if (local_event_id != (expected_event_id&0xffffff)){
              WARNING("Unexpected L1 ID! Current L1 ID = "<<local_event_id<<" but was expecting "<<expected_event_id);
              m_missedL1+=(local_event_id-(expected_event_id&0xffffff));
              if (m_missedL1) m_status = STATUS_WARN;
              else m_status = STATUS_OK;
            }
            m_prev_event_id = local_event_id;
            local_trigger_bits = tlb_fragment.tap();
          }
          else {
            WARNING("Corrupted trigger physics data fragment at triggered event count "<<m_physicsEventCount);
            m_fragment_status = EventStatus::CorruptedFragment;
            m_status = STATUS_WARN;
            m_prev_event_id++;
          }
          DEBUG("Data fragment:\n"<<tlb_fragment<<"fragment size: "<<total_size<<", fragment status: "<<m_fragment_status<<", ECRcount: "<<m_ECRcount);
          m_physicsEventCount+=1;
          m_trigger_payload_size = total_size;
        }

        local_event_id = (m_ECRcount<<24) + local_event_id;

        std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, 
                                              local_event_id, local_bc_id, event, total_size));
        fragment->set_status(m_fragment_status);
        fragment->set_trigger_bits(local_trigger_bits);

        if (m_fragment_status){
          m_badFragmentsCount+=1;
        }

        std::unique_ptr<const byteVector> bytestream(fragment->raw());

        DataFragment<daqling::utilities::Binary> binData(bytestream->data(),bytestream->size());
        m_connections.send(0,binData); // place the raw binary event fragment on the output port
      }
    } 
  }
  INFO("Runner stopped");
}
