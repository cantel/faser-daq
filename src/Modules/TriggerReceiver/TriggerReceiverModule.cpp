/**
 * Copyright (C) 2019 CERN
 * 
 * DAQling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * DAQling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with DAQling. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TriggerReceiverModule.hpp"
#include "EventFormats/DAQFormats.hpp"
#include "EventFormats/TLBDataFragment.hpp"
#include "EventFormats/TLBMonitoringFragment.hpp"

#include <Utils/Binary.hpp>

using namespace DAQFormats;
using namespace daqling::utilities;


#define _ms 1000 // used for usleep

TriggerReceiverModule::TriggerReceiverModule() {
  INFO("In TriggerReceiverModule()");
  m_tlb = new FASER::TLBAccess();
  m_decoder = new FASER::TLBDecode();
  m_tlb->SetDebug(0); //Set to 0 for no debug, to 1 for debug. Changes the m_DEBUG variable
}

TriggerReceiverModule::~TriggerReceiverModule() { 
  INFO("In ~TriggerReceiverModule"); 
 
  delete m_tlb;
  delete m_decoder;
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
  
  auto cfg = m_config.getSettings();
  auto cfg_LUTconfig = cfg.value("LUTConfig", "");

  // fixed configs
  cfg["Reset"] = true;
  cfg["ECR"] = true;
  cfg["TriggerEnable"] = false;
  cfg["SoftwareTrigger"] = false;

  INFO("Configuring TLB");
  if (m_tlb->ConfigureAndVerifyTLB(cfg)){INFO("TLB Configuration OK");}
  else{
    ERROR("TLB Configuration failed");
    m_status=STATUS_ERROR;
  }
  INFO("Done.");
  
  if ( cfg_LUTconfig.empty() ) {
    ERROR("No LUT configuration provided. TLB Configuration failed.");
    m_status=STATUS_ERROR;
  }
  else {
    INFO("Configuring LUT");
    m_tlb->ConfigureLUT(cfg_LUTconfig); // TODO: catch when ConfigureLUT fails (requires modification in gpiodrivers)
    INFO("Done.");  
  }
}

void TriggerReceiverModule::enableTrigger(const std::string &arg) {
  INFO("Got enableTrigger command with argument "<<arg);
  //auto myjson = m_config.getSettings(); //Temporary while using USB.
  //int WhatToRead=0x0; //Temp
  //WhatToRead=(WhatToRead|(myjson["EnableTriggerData"].get<bool>()<<13)); //Temp
  //WhatToRead=(WhatToRead|(myjson["EnableMonitoringData"].get<bool>()<<14)); //Temp
  //WhatToRead=(WhatToRead|(myjson["ReadoutFIFOReset"].get<bool>()<<15)); //Temp
  //m_tlb->StartReadout(WhatToRead); //Temp
  m_tlb->EnableTrigger(false,false); //Only enables trigger. Doesn't send ECR nor Reset
}

void TriggerReceiverModule::disableTrigger(const std::string &arg) { //run with "command disableTrigger"
  INFO("Got disableTrigger command with argument "<<arg);
  m_tlb->DisableTrigger();
  //m_tlb->StopReadout(); //Temporary while using USB. Should empty USB buffer.
  usleep(100); //Once ethernet is implemented you should either check if data is pushed or if timeout (100musec).  
}

void TriggerReceiverModule::sendECR() { //run with "command ECR"
  INFO("Got ECR command");
  m_tlb->SendECR();
}


void TriggerReceiverModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  auto myjson = m_config.getSettings();
  int WhatToRead=0x0;
  WhatToRead=(WhatToRead|(myjson["EnableTriggerData"].get<bool>()<<13));
  WhatToRead=(WhatToRead|(myjson["EnableMonitoringData"].get<bool>()<<14));
  WhatToRead=(WhatToRead|(myjson["ReadoutFIFOReset"].get<bool>()<<15));
  m_tlb->StartReadout(WhatToRead);
  usleep(100*_ms);//temporary - wait for all modules
  m_tlb->EnableTrigger(true,true); //sends ECR and Reset
}

void TriggerReceiverModule::stop() {  
  INFO("Stopping readout.");
  m_tlb->DisableTrigger();
  m_tlb->StopReadout();
  usleep(100*_ms); //value to be tweaked. Should be large enough to empty the buffer.
  FaserProcess::stop(); //this turns m_run to false
}

void TriggerReceiverModule::runner() {
  INFO("Running...");
  
  std::vector<std::vector<uint32_t>> vector_of_raw_events;
  std::vector<uint32_t> event;
  uint8_t  local_fragment_tag = EventTags::PhysicsTag;
  uint32_t local_source_id    = SourceIDs::TriggerSourceID;
  uint64_t local_event_id;
  uint16_t local_bc_id;


  while (m_run) {
    vector_of_raw_events = m_tlb->GetTLBEventData();
      
    if (vector_of_raw_events.size()==0){
      usleep(100); //this is to make sure we don't occupy CPU resources if no data is on output
    }
    else {
      for(std::vector<std::vector<uint32_t>>::size_type i=0; i<vector_of_raw_events.size(); i++){
        event = vector_of_raw_events[i];
        size_t total_size = event.size() * sizeof(uint32_t); //Event size in byte

        if (!total_size) continue;

        if (m_decoder->IsTriggerHeader(event[0])){
          local_fragment_tag=EventTags::PhysicsTag;
          TLBDataFragment tlb_fragment = TLBDataFragment(event.data(), total_size);
          local_event_id = tlb_fragment.event_id();
          local_bc_id = tlb_fragment.bc_id();
          if (!tlb_fragment.valid()) m_fragment_status = EventTags::CorruptedTag;
          DEBUG("Data fragment:\n"<<tlb_fragment<<"fragment status: "<<m_fragment_status<<", ECRcount: "<<m_ECRcount);
          m_physicsEventCount+=1;
          m_trigger_payload_size = total_size;
        }
        else if (m_decoder->IsMonitoringHeader(event[0])){
          local_fragment_tag=EventTags::TLBMonitoringTag;
          TLBMonitoringFragment tlb_fragment = TLBMonitoringFragment(event.data(), total_size);
          local_event_id = tlb_fragment.event_id();
          local_bc_id = tlb_fragment.bc_id();
          if (!tlb_fragment.valid()) m_fragment_status = EventTags::CorruptedTag;
          DEBUG("Monitoring fragment:\n"<<tlb_fragment<<"fragment status: "<<m_fragment_status<<", ECRcount: "<<m_ECRcount);
          m_monitoringEventCount+=1;
          m_monitoring_payload_size = total_size;
        }

        local_event_id = (m_ECRcount<<24) + (local_event_id);

        std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, 
                                              local_event_id, local_bc_id, event.data(), total_size));
        fragment->set_status(m_fragment_status);

        if (!m_fragment_status){
          m_badFragmentsCount+=1;
        }

        std::unique_ptr<const byteVector> bytestream(fragment->raw());

        daqling::utilities::Binary binData(bytestream->data(),bytestream->size());
        m_connections.put(0,binData); // place the raw binary event fragment on the output port
      }
    } 
  }
  INFO("Runner stopped");
}
