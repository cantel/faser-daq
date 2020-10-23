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

#include "TrackerReceiverModule.hpp"
#include "TrackerReadout/TRBAccess.h"
#include "Commons/FaserProcess.hpp"
#include "EventFormats/DAQFormats.hpp"
#include <Utils/Binary.hpp>
#include "TrackerReadout/ConfigurationHandling.h"
#include "TrackerReadout/TRBEventDecoder.h"
#include <string>
#include <iostream>

using namespace DAQFormats;
using namespace daqling::utilities;

TrackerReceiverModule::TrackerReceiverModule() { 
    INFO("");

    auto cfg = m_config.getSettings();

    bool runEmulation = m_config.getConfig()["settings"]["emulation"]; 

    if ( cfg.contains("boardID")) {
      m_userBoardID = cfg["boardID"];
    }
    else if (!runEmulation) {
      ERROR("No board ID specified.");
      throw std::runtime_error("No board ID specified.");
    }

    bool usingUSB(false);
    if ( cfg.contains("SCIP")) {
      m_SCIP = cfg["SCIP"];
      if ( cfg.contains("DAQIP")) {
        m_DAQIP = cfg["DAQIP"];
      }
      else {
        ERROR("No DAQ IP specified.");
        throw std::runtime_error("No DAQ IP specified.");
      }
      INFO("SC and DAQ IPs have been specified. Assuming we're communicating through the ether!");
    }
    else {
       INFO("**NO** SC and DAQ IPs have been specified. Assuming we're communicating via USB!");
       usingUSB = true;
    }

    m_extClkSelect = true;
    if ( cfg.contains("L1Atype")) {
      if (cfg["L1Atype"] == "internal" ) m_extClkSelect = false;
    }

    if ( cfg.contains("ReadoutMode")) {
      m_ABCD_ReadoutMode = cfg["ReadoutMode"];
    }
    else m_ABCD_ReadoutMode = FASER::TRBAccess::ABCD_ReadoutMode::LEVEL;

    if ( cfg.contains("ConfigureModules")) {
      m_configureModules = cfg["ConfigureModules"];
    }
    else m_configureModules = true;

    if ( cfg.contains("FinePhaseClk")) {
       m_finePhaseDelay_Clk = cfg["FinePhaseClk"];
    }
    else {WARNING("No clock fine phase delay setting provided. Setting to 0."); m_finePhaseDelay_Clk = 0;}

    if ( cfg.contains("FinePhaseLed")) {
       m_finePhaseDelay_Led = cfg["FinePhaseLed"];
    }
    else {WARNING("No Led fine phase delay setting provided. Setting to 0."); m_finePhaseDelay_Led = 0;}

    if ( cfg.contains("RxTimeoutDisable")) {
       m_RxTimeoutDisable = cfg["RxTimeoutDisable"];
    }
    else m_RxTimeoutDisable = false;

    INFO("\n*** Configurations for TrackerReceiver module *** \n emulation mode: "<<(runEmulation?"TRUE":"FALSE")<<"\n TRB board ID: "<<m_userBoardID<<"\n using USB: "<<(usingUSB?"TRUE (No SC/DAQ IP required)":"FALSE")<<"\n SC IP: "<<m_SCIP<<"\n DAQ IP: "<<m_DAQIP<<"\n chip readout mode: "<<m_ABCD_ReadoutMode<<" \n CLK fine phase delay: "<<m_finePhaseDelay_Clk<<"\n LED(X) fine phase delay: "<<m_finePhaseDelay_Led<<"\n Configure modules: "<<(m_configureModules?"TRUE":"FALSE")<<"\n RxTimeoutDisable: "<<(m_RxTimeoutDisable?"TRUE":"FALSE")<<"\n debug mode: "<<(m_debug?"TRUE":"FALSE")<<"\n");
    
    if (usingUSB) m_trb = std::make_unique<FASER::TRBAccess>(0, runEmulation, m_userBoardID );
    else m_trb = std::make_unique<FASER::TRBAccess>(m_SCIP, m_DAQIP, 0, runEmulation, m_userBoardID );
    m_ed = std::make_unique<FASER::TRBEventDecoder>();
    
    auto log_level = m_config.getConfig()["loglevel"]["module"];
    m_debug = (log_level=="DEBUG"?1:0);
    if (log_level == "TRACE") {  
      m_trb->SetDebug(true);
    }
    else {
      m_trb->SetDebug(false);    
    }

    //Setting up the emulated interface
    if (runEmulation) {
      std::string l_TRBconfigFile = m_config.getConfig()["settings"]["emulatorFile"];
      static_cast<FASER::dummyInterface*>(m_trb->m_interface)->SetInputFile(l_TRBconfigFile); //file to read events from
    }
}    

TrackerReceiverModule::~TrackerReceiverModule() { 
    INFO("");
}


/***************************************
 *        Configure module
 * ************************************/
void TrackerReceiverModule::configure() {
  FaserProcess::configure();
  INFO("TRB --> configuration");

  registerVariable(m_physicsEventCount, "TriggeredEvents");
  registerVariable(m_physicsEventCount, "PhysicsRate", metrics::RATE);
  registerVariable(m_event_size_bytes, "event_size_bytes");
  registerVariable(m_corrupted_fragments, "BadFragments");
  registerVariable(m_corrupted_fragments, "BadFragmentsRate", metrics::RATE);
  registerVariable(m_checksum_mismatches, "checksum_mismatches");
  registerVariable(m_checksum_mismatches_rate, "checksum_mismatches_rate");
  registerVariable(m_number_of_decoded_events, "number_of_decoded_events");

  //TRB configuration 
  m_moduleMask = 0;
  m_moduleClkCmdMask = 0;
  for (int i = 7; i >= 0; i--){ //there are 8 modules
    if (m_config.getConfig()["settings"]["moduleMask"][i]){
      m_moduleMask |= (1 << i);
    }
    if (m_config.getConfig()["settings"]["moduleClkCmdMask"][i]){
      m_moduleClkCmdMask |= (1 << i);
    }
  }

  if (m_moduleMask > 0xff) {
    ERROR("We have at most 8 modules per TRB, but you specified 0x" <<std::hex<<m_moduleMask<< " as moduleMask!");
    return;
  }
  if (m_moduleClkCmdMask > 0xff) {
    ERROR("We have at most 8 modules per TRB, but you specified 0x" <<std::hex<<m_moduleMask<< " as moduleClkCmdMask!");
    return;
  }

  // disable all before configuring 
  m_trb->SetDirectParam(FASER::TRBDirectParameter::FifoReset | FASER::TRBDirectParameter::ErrCntReset); // disable L1A, BCR and Trigger Clock, else modules can't be configured next time.
  m_trb->GetConfig()->Set_Module_L1En(0); 
  m_trb->GetConfig()->Set_Module_BCREn(0); 
  m_trb->GetConfig()->Set_Module_ClkCmdSelect(m_moduleClkCmdMask);
  m_trb->GetConfig()->Set_Module_LedRXEn(0);
  m_trb->GetConfig()->Set_Module_LedxRXEn(0);
  m_trb->WriteConfigReg();

  m_trb->GetPhaseConfig()->SetFinePhase_Clk0(m_finePhaseDelay_Clk);
  m_trb->GetPhaseConfig()->SetFinePhase_Led0(m_finePhaseDelay_Led);
  m_trb->GetPhaseConfig()->SetFinePhase_Clk1(m_finePhaseDelay_Clk);
  m_trb->GetPhaseConfig()->SetFinePhase_Led1(m_finePhaseDelay_Led);
  m_trb->WritePhaseConfigReg();
  m_trb->ApplyPhaseConfig();
 

  if ( m_configureModules ) {
  //Modules configuration
  for (int l_moduleNo=0; l_moduleNo < 8; l_moduleNo++){ //we have 8 modules per TRB
  
    if ((0x1 << l_moduleNo ) & m_moduleMask){ //checks if the module is active according to the module mask
    INFO("Starting configuration of module number " << l_moduleNo);
        
      std::unique_ptr<FASER::ConfigHandlerSCT> l_cfg(new FASER::ConfigHandlerSCT());
      if (m_config.getConfig()["settings"]["moduleConfigFiles"].contains(std::to_string(l_moduleNo))){ //module numbers in cfg file from 0 to 7!!
        std::string l_moduleConfigFile = m_config.getConfig()["settings"]["moduleConfigFiles"][std::to_string(l_moduleNo)];
        if (l_moduleConfigFile != ""){
            l_cfg->ReadFromFile(l_moduleConfigFile);
            l_cfg->SetReadoutMode(m_ABCD_ReadoutMode);
        }
        m_trb->ConfigureSCTModule(l_cfg.get(), (0x1 << l_moduleNo)); //sending configuration to corresponding module
        INFO("Configuration of module " << l_moduleNo << " finished.");
      }
      else{
        m_status=STATUS_ERROR;
        ERROR("Module " << l_moduleNo << " enabled by mask but no configuration file provided!");
      }
    }
    }
  }

  if ( m_extClkSelect ) {
    bool retry(true);
    int8_t nRetries(3);
    while (retry && nRetries--) {
      m_trb->SetDirectParam(FASER::TRBDirectParameter::TLBClockSelect);
      // give it a moment to sync with clock?
      for(unsigned int i = 5; i >0; i--){
        uint16_t status;
        m_trb->ReadStatus(status);
        if ((status & 0x4)){ // check if TLBClkSel is TRUE
          INFO( "   TLB CLK synchronized");
          break;
        }
        INFO( "    TLB clock NOT OK ...");
        usleep(100*1000);
      }
      try {
        m_trb->GenerateSoftReset(m_moduleMask);
        m_trb->GetConfig()->Set_Global_RxTimeoutDisable(m_RxTimeoutDisable);
        m_trb->GetConfig()->Set_Global_L1TimeoutDisable(false);
        m_trb->GetConfig()->Set_Global_L2SoftL1AEn(false);
        m_trb->GetConfig()->Set_Global_Overflow(4095);
        m_trb->GetConfig()->Set_Module_L1En(m_moduleMask); 
        m_trb->GetConfig()->Set_Module_BCREn(m_moduleMask); 
        m_trb->GetConfig()->Set_Global_TLBClockSel(m_extClkSelect);
        m_trb->GetConfig()->Set_Module_ClkCmdSelect(m_moduleClkCmdMask);
        m_trb->GetConfig()->Set_Module_LedRXEn(m_moduleMask);
        m_trb->GetConfig()->Set_Module_LedxRXEn(m_moduleMask);
        m_trb->WriteConfigReg();
        m_trb->SCT_EnableDataTaking(m_moduleMask);
        m_trb->SetDirectParam(FASER::TRBDirectParameter::L1AEn | FASER::TRBDirectParameter::BCREn | FASER::TRBDirectParameter::TLBClockSelect);
      } catch ( Exceptions::BaseException &e ){ // FIXME figure out why it won't catch TRBAccessException
         if (!nRetries) { m_status=STATUS_ERROR; sleep(1); throw e; };
         ERROR("Sending configuration commands failed. Will try resyncing to clock. "<<(int)nRetries<<" retries remaining.");
         m_trb->SetDirectParam(0);
         for(unsigned int i = 5; i >0; i--){
           uint16_t status;
           m_trb->ReadStatus(status);
           if (!(status & 0x4)){ // check if TLBClkSel is FALSE
             INFO( "   Fell back to internal CLK");
             break;
            }
            INFO( "    Still falling back to internal CLK ...");
            usleep(100*1000);
          }
          continue;
      }
      retry = false;
    }
  }
  else { //running on internal clock
    m_trb->GenerateSoftReset(m_moduleMask);
    m_trb->GetConfig()->Set_Global_L1TimeoutDisable(false);
    m_trb->GetConfig()->Set_Global_L2SoftL1AEn(true);
    m_trb->GetConfig()->Set_Global_Overflow(4095);
    m_trb->GetConfig()->Set_Global_TLBClockSel(m_extClkSelect);
    m_trb->GetConfig()->Set_Module_ClkCmdSelect(m_moduleClkCmdMask);
    m_trb->GetConfig()->Set_Module_LedRXEn(m_moduleMask);
    m_trb->GetConfig()->Set_Module_LedxRXEn(m_moduleMask);
    m_trb->WriteConfigReg();
    m_trb->SCT_EnableDataTaking(m_moduleMask);
    m_trb->SetDirectParam(FASER::TRBDirectParameter::SoftCounterMuxEn);
  }

  m_trb->PrintStatus();


}


void TrackerReceiverModule::sendECR()
{
  INFO("TRB --> ECR." << " ECRcount: " << m_ECRcount);
  m_trb->L1CounterReset();
}


/***************************************
 *        Start module
 * ************************************/
void TrackerReceiverModule::start(unsigned run_num) {
  m_triggerEnabled = true;
  m_trb->StartReadout(FASER::TRBReadoutParameters::READOUT_L1COUNTER_RESET | FASER::TRBReadoutParameters::READOUT_ERRCOUNTER_RESET | FASER::TRBReadoutParameters::READOUT_FIFO_RESET); //doing ErrCnTReset, FifoReset,L1ACounterReset
  INFO("TRB --> readout started." );
  FaserProcess::start(run_num);
}


/***************************************
 *        Stop module
 * ************************************/
void TrackerReceiverModule::stop() {
  m_trb->StopReadout();
  usleep(100);
  m_trb->SetDirectParam(0); // disable L1A, BCR and Trigger Clock, else modules can't be configured next time.
  FaserProcess::stop();
  INFO("TRB --> readout stopped.");
}


/****************************************  
 *        Disable Trigger
 ****************************************/
void TrackerReceiverModule::disableTrigger(const std::string &) {
  //m_trb->StopReadout();
  m_triggerEnabled = false;
  INFO("TRB --> disable trigger.");
  usleep(100);
}

/****************************************  
 *        Enable Trigger
 ****************************************/
void TrackerReceiverModule::enableTrigger(const std::string &) {
  //m_trb->StartReadout();
  m_triggerEnabled = true; 
  INFO("TRB --> enable trigger.");
  usleep(100);
}

/***************************************
 *        Runner of the module
 * ************************************/
void TrackerReceiverModule::runner() {
  INFO("Running...");
  std::vector<std::vector<uint32_t>> vector_of_raw_events;
  uint8_t  local_fragment_tag = EventTags::PhysicsTag;
  uint32_t local_source_id    = SourceIDs::TrackerSourceID + m_trb->GetBoardID();
  uint64_t local_event_id;
  uint64_t local_bc_id;

  while (m_run || vector_of_raw_events.size()) { 
    if (!m_extClkSelect && m_triggerEnabled == true){
      m_trb->GenerateL1A(m_moduleMask); //Generate L1A on the board
    }

    vector_of_raw_events = m_trb->GetTRBEventData();

    if (vector_of_raw_events.size() == 0){
      usleep(100); //this is to make sure we don't occupy CPU resources if no data is on output
    }
      else{
        for(std::vector<uint32_t> event : vector_of_raw_events){
          DEBUG("-------------- printing new event ---------------- ");
          int total_size = event.size() * sizeof(uint32_t); //Event size in bytes      
          m_event_size_bytes = total_size; //Monitoring data
          m_physicsEventCount += 1; //Monitoring data

          if (event.size() == 0){ continue;}

          //TODO should we also send the EndOfDAQ trailer? Currently, we are not sending it
          if (!(m_ed->IsEndOfDAQ(event[0]))){
            m_ed->LoadTRBEventData(event);
            auto decoded_event = m_ed->GetEvents(); 

            if (decoded_event.size() != 0){
                local_event_id = decoded_event[0]->GetL1ID(); //we can always ask for element 0 - we are feeding only one event at the time to m_ed
                local_event_id = local_event_id | (m_ECRcount << 24);
                local_bc_id = (decoded_event[0]->GetBCID()-6)%3564; // software correction to agree with TLB.
                if (m_debug){
	  	  DEBUG("N modules present = "<<decoded_event[0]->GetNModulesPresent());
	  	  if(decoded_event[0]->GetNModulesPresent()>0){
	  	     auto ourSCTevent = decoded_event[0]->GetModule(0);
	  	     DEBUG("BCID = "<<ourSCTevent->GetBCID());
	  	     DEBUG("L1ID = "<<ourSCTevent->GetL1ID());
	  	  }
                }
            }
            else{
                local_event_id = 0xFFFFFFFF;
                local_bc_id = 0xFFFFFFFF;
                m_status=STATUS_ERROR;
            }

            std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, 
                                                local_event_id, local_bc_id, event.data(), total_size));
            
            uint16_t error;
            fragment->set_status(0);
   
            if (decoded_event.size() != 0){
              m_number_of_decoded_events += 1; //Monitoring data
              if (decoded_event[0]->GetIsChecksumValid() == true){ 
                DEBUG("Checksums match for this event.");
                m_checksum_mismatches_rate = m_checksum_mismatches/m_number_of_decoded_events; //Monitoring data
              }
              else { 
                WARNING("Checksum mismatch.");
                fragment->set_status(EventStatus::CorruptedFragment);
                m_corrupted_fragments += 1; //Monitoring data
                m_checksum_mismatches += 1; //Monitoring data
                m_checksum_mismatches_rate = m_checksum_mismatches/m_number_of_decoded_events; //Monitoring data
              }         
            }

            for (uint32_t frame : event){
              if(m_ed->HasError(frame, error)){
                //TODO If possible specify error
                fragment->set_status(EventStatus::UnclassifiedError);
                m_corrupted_fragments += 1; //Monitoring data
              }
            }

            if (m_debug){
              DEBUG("event id: 0x"<< std::hex << fragment->event_id());
              DEBUG("fragment tag: 0x"<< std::hex << fragment->fragment_tag());
              DEBUG("source id: 0x"<< std::hex << fragment->source_id());
              DEBUG("bc id: 0x"<< std::hex << fragment->bc_id());
              DEBUG("status: 0x"<< std::hex << fragment->status());
              DEBUG("trigger bits: 0x"<< std::hex << fragment->trigger_bits());
              DEBUG("size: 0x"<< std::hex << fragment->size());
              DEBUG("payload size: 0x"<< std::hex << fragment->payload_size());
              DEBUG("timestamp: 0x"<< std::hex << fragment->timestamp());
             
              DEBUG("Data received from TRB: ");
              for(auto word : event){
                std::bitset<32> y(word);
                if(m_ed->HasError(word, error)){DEBUG("               " << y << " error word");}
                else{DEBUG("               " << y);}
              }

              DEBUG("Raw data sent further: ");
              const DAQFormats::byteVector *data = fragment->raw();
              for (unsigned i = 0; i <  (unsigned)data->size(); i++){
                  std::bitset<8> y(data->at(i));
                  DEBUG("               " << y);
              }
              delete data;
            }
            else{
              INFO("event id: 0x"<< fragment->event_id());
            }

            // place the raw binary event fragment on the output port
            std::unique_ptr<const byteVector> bytestream(fragment->raw());
            daqling::utilities::Binary binData(bytestream->data(),bytestream->size());
            m_connections.put(0, binData);
          }
       }
    }   
  } 
  INFO("Runner stopped");
}
