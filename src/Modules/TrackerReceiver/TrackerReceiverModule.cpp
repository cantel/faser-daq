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

    bool ethernetComms(true);

    auto cfg = m_config.getSettings();

    if ( cfg.contains("boardID")) {
      m_userBoardID = cfg["boardID"];
    }
    //else m_userBoardID = -1; //don't care
    else {
      ERROR("No board ID specified.");
      throw std::runtime_error("No board ID specified.");
    }

    if ( cfg.contains("SCIP")) {
      m_SCIP = cfg["SCIP"];
      if ( cfg.contains("DAQIP")) {
        m_DAQIP = cfg["DAQIP"];
      }
      else {
        ERROR("No DAQ IP specified.");
        throw std::runtime_error("No DAQ IP specified.");
      }
      INFO("SC and DAQ IPs have been specified. Assuming we're communicating via ethernet!");
    }
    else {
       INFO("**NO** SC and DAQ IPs have been specified. Assuming we're communicating via USB!");
       ethernetComms = false;
    }


    if ( cfg.contains("ReadoutMode")) {
      m_ABCD_ReadoutMode = cfg["ReadoutMode"];
    }
    else m_ABCD_ReadoutMode = FASER::TRBAccess::ABCD_ReadoutMode::LEVEL;

    INFO("Will connect to TRB with board id "<<m_userBoardID);
    INFO("Will set chip read out mode to "<<m_ABCD_ReadoutMode);
     
    if (ethernetComms) m_trb = std::make_unique<FASER::TRBAccess>(m_SCIP, m_DAQIP, 0, m_config.getConfig()["settings"]["emulation"], m_userBoardID );
    else m_trb = std::make_unique<FASER::TRBAccess>(0, m_config.getConfig()["settings"]["emulation"], m_userBoardID );
    m_ed = std::make_unique<FASER::TRBEventDecoder>();

    //m_trb->SetReadoutMode( m_ABCD_ReadoutMode );

    auto log_level = m_config.getConfig()["loglevel"]["module"];
    m_debug = (log_level=="DEBUG"?1:0);
    if (m_config.getConfig()["loglevel"]["module"] == "TRACE") {  
      m_trb->SetDebug(true);
    }
    else {
      m_trb->SetDebug(false);    
    }

    m_trb->SetupStorageStream("rawData.daq"); //FIXME: Outputting for now.
    //m_ed->SetEventInfoVerbosity(5);

    //Setting up the emulated interface
    if (m_config.getConfig()["settings"]["emulation"] == true) {
      std::string l_TRBconfigFile = m_config.getConfig()["settings"]["emulatorFile"];
      static_cast<FASER::dummyInterface*>(m_trb->m_interface)->SetInputFile(l_TRBconfigFile); //file to read events from
      m_trb->SetupStorageStream("TestEmulatorOutFile.daq");
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

  //registerVariable(event_id, "event_id");
  registerVariable(event_size_bytes, "event_size_bytes");
  //registerVariable(bc_id, "bc_id");
  registerVariable(corrupted_fragments, "corrupted_fragments", metrics::RATE);
  registerVariable(m_physicsEventCount, "PhysicsRate", metrics::RATE);

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

  m_trb->SetDirectParam(FASER::TRBDirectParameter::FifoReset | FASER::TRBDirectParameter::ErrCntReset); // disable L1A, BCR and Trigger Clock, else modules can't be configured next time.

  m_trb->GetPhaseConfig()->SetFinePhase_Clk0(0);
  m_trb->GetPhaseConfig()->SetFinePhase_Led0(m_finePhaseDelay);
  m_trb->GetPhaseConfig()->SetFinePhase_Clk1(0);
  m_trb->GetPhaseConfig()->SetFinePhase_Led1(m_finePhaseDelay);
  m_trb->WritePhaseConfigReg();
  m_trb->ApplyPhaseConfig();
  
  if (m_config.getConfig()["settings"]["L1Atype"] == "internal"){
    m_trb->GetConfig()->Set_Module_L1En(0x0);
    m_trb->GetConfig()->Set_Global_L2SoftL1AEn(true);
  }
  else {
    m_trb->GetConfig()->Set_Module_L1En(0); 
    m_trb->GetConfig()->Set_Module_BCREn(0); 
    m_trb->GetConfig()->Set_Global_L2SoftL1AEn(false);
  }
  m_trb->GetConfig()->Set_Module_ClkCmdSelect(m_moduleClkCmdMask);
  m_trb->GetConfig()->Set_Module_LedRXEn(0);
  m_trb->GetConfig()->Set_Module_LedxRXEn(0);

  m_trb->GetConfig()->Set_Global_RxTimeoutDisable(false);
  m_trb->GetConfig()->Set_Global_L1TimeoutDisable(false);
  m_trb->GetConfig()->Set_Global_Overflow(4095);

  m_trb->WriteConfigReg();

  //Modules configuration
  //This stage causes Timeout exception in DAQling, probably because of the transfer capacity. Maybe it will disapear with USB3 or ethernet]
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
   sleep(1);
 }

  m_trb->GetConfig()->Set_Global_RxTimeoutDisable(false);
  m_trb->GetConfig()->Set_Global_L1TimeoutDisable(false);
  m_trb->GetConfig()->Set_Global_L2SoftL1AEn(false);
  m_trb->GetConfig()->Set_Global_Overflow(4095);
  m_trb->GetConfig()->Set_Module_L1En(m_moduleMask); 
  m_trb->GetConfig()->Set_Module_BCREn(m_moduleMask); 
  m_trb->GetConfig()->Set_Global_L2SoftL1AEn(false);
  m_trb->GetConfig()->Set_Module_ClkCmdSelect(m_moduleClkCmdMask);
  m_trb->GetConfig()->Set_Module_LedRXEn(m_moduleMask);
  m_trb->GetConfig()->Set_Module_LedxRXEn(m_moduleMask);
  m_trb->WriteConfigReg();
  m_trb->SCT_EnableDataTaking(m_moduleMask);
  m_trb->GenerateSoftReset(m_moduleMask);

  m_trb->SetDirectParam(FASER::TRBDirectParameter::L1AEn | FASER::TRBDirectParameter::BCREn | FASER::TRBDirectParameter::TLBClockSelect);


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
  corrupted_fragments = 0; //Setting this monitring variable to 0 
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
void TrackerReceiverModule::disableTrigger(const std::string &arg) {
  m_trb->StopReadout();
  m_triggerEnabled = false;
  INFO("TRB --> disable trigger.");
  usleep(100);
}

/****************************************  
 *        Enable Trigger
 ****************************************/
void TrackerReceiverModule::enableTrigger(const std::string &arg) {
  m_trb->StartReadout();
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
  unsigned int errcount(0);

  while (m_run || vector_of_raw_events.size()) { 
    if (m_config.getConfig()["settings"]["L1Atype"] == "internal" && m_triggerEnabled == true){
      m_trb->GenerateL1A(m_moduleMask); //Generate L1A on the board
    }

    //INFO("Checking error count ..." );
    //m_trb->ReadErrorCounter(errcount);
    
    vector_of_raw_events = m_trb->GetTRBEventData();

    if (vector_of_raw_events.size() == 0){
      usleep(100); //this is to make sure we don't occupy CPU resources if no data is on output
    }
      else{
        for(std::vector<uint32_t> event : vector_of_raw_events){
          int total_size = event.size() * sizeof(uint32_t); //Event size in bytes      
          event_size_bytes = total_size; //Monitoring data
          m_physicsEventCount += 1; //Monitoring data

          if (event.size() == 0){ continue;}

          //TODO should we also send the EndOfDAQ trailer? Currently, we are not sending it
          if (!(m_ed->IsEndOfDAQ(event[0]))){
            m_ed->LoadTRBEventData(event);
            auto decoded_event = m_ed->GetEvents(); 

            if (decoded_event.size() != 0){
                local_event_id = decoded_event[0]->GetL1ID(); //we can always ask for element 0 - we are feeding only one event at the time to m_ed
                local_event_id = local_event_id | (m_ECRcount << 24);
                local_bc_id = decoded_event[0]->GetBCID();
                if (m_debug){
	  	  DEBUG("N modules present = "<<decoded_event[0]->GetNModulesPresent());
	  	  if(decoded_event[0]->GetNModulesPresent()>0){
	  	     auto ourSCTevent = decoded_event[0]->GetModule(0);
	  	     DEBUG("BCID = "<<ourSCTevent->GetBCID());
	  	     DEBUG("L1ID = "<<ourSCTevent->GetL1ID());
	  	  }
                }
                // local_bc_id = (local_bc_id-7)%3564
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
            for (uint32_t frame : event){
              if(m_ed->HasError(frame, error)){
                //TODO If possible specify error
                fragment->set_status(EventStatus::UnclassifiedError);
                corrupted_fragments += 1; //Monitoring data
              }
            }
            
            if (m_debug){
              DEBUG("-------------- printing new event ---------------- ");
              DEBUG("Data received from TRB: ");
              for(auto word : event){
                std::bitset<32> y(word);
                if(m_ed->HasError(word, error)){DEBUG("               " << y << " error word");}
                else{DEBUG("               " << y);}
              }
              DEBUG("event id: 0x"<< fragment->event_id());
              DEBUG("fragment tag: 0x"<< fragment->fragment_tag());
              DEBUG("source id: 0x"<< fragment->source_id());
              DEBUG("bc id: 0x"<< fragment->bc_id());
              DEBUG("status: 0x"<< fragment->status());
              DEBUG("trigger bits: 0x"<< fragment->trigger_bits());
              DEBUG("size: 0x"<< fragment->size());
              DEBUG("payload size: 0x"<< fragment->payload_size());
              DEBUG("timestamp: 0x"<< fragment->timestamp());
              DEBUG("Raw data sent further: ");
              auto data = fragment->raw();
              for (int i = 0; i <  data->size(); i++){
                  std::bitset<8> y(data->at(i));
                  DEBUG("               " << y);
              }
            }
            else{
              INFO("event id: 0x"<< fragment->event_id());
            }

            // place the raw binary event fragment on the output porti
            std::unique_ptr<const byteVector> bytestream(fragment->raw());
            daqling::utilities::Binary binData(bytestream->data(),bytestream->size());
            m_connections.put(0, binData);
          }
       }
    }   
  } 
  INFO("Runner stopped");
}
