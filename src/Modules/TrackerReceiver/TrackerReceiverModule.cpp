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
#include "GPIOBase/DummyInterface.h"
#include "Commons/FaserProcess.hpp"
#include "Commons/EventFormat.hpp"
#include "Commons/RawExampleFormat.hpp"
#include "TrackerReadout/ConfigurationHandling.h"
#include "TrackerReadout/TRBEventDecoder.h"
#include <string>
#include <iostream>
#include <bitset>

TrackerReceiverModule::TrackerReceiverModule() { 
    INFO("");
     
    m_trb = new FASER::TRBAccess(0, m_config.getConfig()["settings"]["emulation"]);

    if (m_config.getConfig()["loglevel"]["module"] == "DEBUG") {  
      m_trb->SetDebug(true);
    }
    else {
      m_trb->SetDebug(false);    
    }
    
    //Setting up the emulated interface
    if (m_config.getConfig()["settings"]["emulation"] == true) {
      std::string l_TRBconfigFile = m_config.getConfig()["settings"]["emulatorFile"];
      static_cast<FASER::dummyInterface*>(m_trb->m_interface)->SetInputFile(l_TRBconfigFile); //file to read events from
      m_trb->SetupStorageStream("TestEmulatorOutFile.daq");
    }
}    

TrackerReceiverModule::~TrackerReceiverModule() { 
    INFO(""); 
    delete m_trb;
}

void TrackerReceiverModule::configure() {
  FaserProcess::configure();
  INFO("TRB --> configuration");

  //TRB configuration
  unsigned int m_moduleMask = m_config.getConfig()["settings"]["moduleMask"].get<int>();

  if (m_moduleMask > 0xff) {
    ERROR("We have at most 8 modules per TRB, but you specified 0x" <<std::hex<<m_moduleMask<< " as moduleMask!");
    return;
  }
  
  m_trb->GetConfig()->Set_Global_L1TimeoutDisable(true);
  for (unsigned int i = 0; i < 8; i++){
    if ( (0x1 << i) & m_moduleMask) { // module enabled
      m_trb->GetConfig()->Set_Module_L1En(i, false);
      m_trb->GetConfig()->Set_Module_ClkCmdSelect(i, false); // switch between CMD_0 and CMD_1 lines
      m_trb->GetConfig()->Set_Module_LedxRXEn(i, true, true); // switch between led and ledx lines
    }
  }
  m_trb->GetConfig()->Set_Global_RxTimeoutDisable(true);
  m_trb->GetConfig()->Set_Global_L1TimeoutDisable(false);
  m_trb->GetConfig()->Set_Global_L2SoftL1AEn(true);
 
  m_trb->WriteConfigReg();
  
  //Modules configuration
  //TODO Why this couses runtime error (in log messades of daqlink)
  /*for (int l_moduleNo=0; l_moduleNo < 8; l_moduleNo++){ //we have 8 modules per TRB
  
    if ((0x1 << l_moduleNo ) & m_moduleMask){ //checks if the module is active according to the module mask
    INFO("Starting configuration of module number " << l_moduleNo);
        
      FASER::ConfigHandlerSCT *l_cfg = new FASER::ConfigHandlerSCT();
      if (m_config.getConfig()["settings"]["moduleConfigFiles"].contains(std::to_string(l_moduleNo))){ //module numbers in cfg file from 0 to 7!!
        std::string l_moduleConfigFile = m_config.getConfig()["settings"]["moduleConfigFiles"][std::to_string(l_moduleNo)];
        //std::cout << "-----------------------------------------" << l_moduleConfigFile <<", " << std::dec << (0x1 << l_moduleNo)  << std::endl;
        if (l_moduleConfigFile != ""){
            l_cfg->ReadFromFile(l_moduleConfigFile);
        }
        m_trb->ConfigureSCTModule(l_cfg, (0x1 << l_moduleNo)); //sending configuration to corresponding module
        INFO("Configuration of module " << l_moduleNo << " finished.");
      }
      else{
        ERROR("Module " << l_moduleNo << " enabled by mask but no configuration file provided!");
      }
    }
  }*/
}

void TrackerReceiverModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  m_trb->StartReadout();
  INFO("TRB --> readout started.");
}

void TrackerReceiverModule::stop() {
  FaserProcess::stop();
  m_trb->StopReadout();
  INFO("TRB --> readout stopped.");
}

void TrackerReceiverModule::runner() {
  INFO("Running...");
  std::vector<std::vector<uint32_t>> vector_of_raw_events;
  uint32_t* raw_payload[MAXFRAGSIZE];
  uint8_t  local_fragment_tag = EventTags::PhysicsTag;
  uint32_t local_source_id    = SourceIDs::TrackerSourceID;
  uint64_t local_event_id;
  uint16_t local_bc_id;
  FASER::TRBEventDecoder *ed = new FASER::TRBEventDecoder();

  int counter = 0;
  while (m_run) {
    vector_of_raw_events = m_trb->GetTRBEventData();
   
    for(auto event : vector_of_raw_events){
      //TODO Decode following two numbers from raw data by event decoder
      *raw_payload = event.data();
      int total_size = (*raw_payload[0] & 0xFFFFFFF) * sizeof(uint32_t); //Event size in bytes     

      ed->LoadTRBEventData(event);
      auto decoded_event = ed->GetEvents(); 

      //TODO Here I get runtime error: null pointer exception
 /*     local_event_id = decoded_event[0]->GetL1ID();
      local_bc_id = decoded_event[0]->GetBCID();
      std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, 
                                              local_event_id, local_bc_id, Binary(raw_payload, total_size)));
      // TODO : What is the status supposed to be?
        uint16_t status=0;
          fragment->set_status( status );
      
            // place the raw binary event fragment on the output port
              m_connections.put(0, const_cast<Binary&>(fragment->raw()));

      //Following for-cycle is only for debugging
      for(auto word : event){
        std::bitset<32> y(word);
        std::cout << y << std::endl;
      }*/
     }   
  }
  INFO("Runner stopped");
    }
