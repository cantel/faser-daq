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
#include <string>
#include <iostream>

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
      //(static_cast<FASER::dummyInterface*>(m_trb->m_interface))->SetInputFile((m_config.getConfig()["settings"]["emulationFile"])); //file to read events from
      (static_cast<FASER::dummyInterface*>(m_trb->m_interface))->SetInputFile("/home/otheiner/FASER_DAQ/DataTaking_4modules_10kL1A.daq"); //file to read events from
      m_trb->SetupStorageStream("TestEmulatorOutFile.daq");
    }
}    

TrackerReceiverModule::~TrackerReceiverModule() { 
    INFO(""); 
    delete m_trb;
}

// optional (configuration can be handled in the constructor)
void TrackerReceiverModule::configure() {
  FaserProcess::configure();
  INFO("TRB --> configuration");

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

  while (m_run) {
    vector_of_raw_events = m_trb->GetTRBEventData();

    int counter = 0;
    for(auto event : vector_of_raw_events){
      counter += 1;
      std::cout << "printing event " << counter << std::endl;

      *raw_payload = event.data();
      /*//TODO Decode following two numbers from raw data by event decoder
      local_event_id = 
      local_bc_id = 
      std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, 
                                              local_event_id, local_bc_id, Binary(raw_payload, total_size)));
      // TODO : What is the status supposed to be?
        uint16_t status=0;
          fragment->set_status( status );
      
            // place the raw binary event fragment on the output port
              m_connections.put(0, const_cast<Binary&>(fragment->raw()));*/

      //Following for-cycle is only for debugging
      for(auto word : event){
        std::cout << word << std::endl;
      }
    }   
  }
  INFO("Runner stopped");
}
