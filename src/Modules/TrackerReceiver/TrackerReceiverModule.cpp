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
     
    m_trb = std::make_unique<FASER::TRBAccess>(0, m_config.getConfig()["settings"]["emulation"]);
    m_ed = std::make_unique<FASER::TRBEventDecoder>();

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
}


/***************************************
 *        Configure module
 * ************************************/
void TrackerReceiverModule::configure() {
  FaserProcess::configure();
  INFO("TRB --> configuration");

  registerVariable(event_id, "event_id");
  registerVariable(event_size_bytes, "event_size_bytes");
  registerVariable(bc_id, "bc_id");

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
  
  if (m_config.getConfig()["settings"]["L1Atype"] == "internal"){
    m_trb->GetConfig()->Set_Module_L1En(0x0);
    m_trb->GetConfig()->Set_Global_L2SoftL1AEn(true);
  }
  else {
    m_trb->GetConfig()->Set_Module_L1En(m_moduleMask); 
    m_trb->GetConfig()->Set_Global_L2SoftL1AEn(false);
  }
  m_trb->GetConfig()->Set_Module_ClkCmdSelect(m_moduleClkCmdMask);
  m_trb->GetConfig()->Set_Module_LedRXEn(m_moduleMask);
  m_trb->GetConfig()->Set_Module_LedxRXEn(m_moduleMask);

  m_trb->GetConfig()->Set_Global_RxTimeoutDisable(true);
  m_trb->GetConfig()->Set_Global_L1TimeoutDisable(false);
  m_trb->GetConfig()->Set_Global_Overflow(4095);

  m_trb->WriteConfigReg();
  
  //Modules configuration
  //This stage causes Timeout exception in DAQling, probably because of the transfer capacity. Maybe it will disapear with USB3 or ethernet
  for (int l_moduleNo=0; l_moduleNo < 8; l_moduleNo++){ //we have 8 modules per TRB
  
    if ((0x1 << l_moduleNo ) & m_moduleMask){ //checks if the module is active according to the module mask
    INFO("Starting configuration of module number " << l_moduleNo);
        
      std::unique_ptr<FASER::ConfigHandlerSCT> l_cfg(new FASER::ConfigHandlerSCT());
      if (m_config.getConfig()["settings"]["moduleConfigFiles"].contains(std::to_string(l_moduleNo))){ //module numbers in cfg file from 0 to 7!!
        std::string l_moduleConfigFile = m_config.getConfig()["settings"]["moduleConfigFiles"][std::to_string(l_moduleNo)];
        if (l_moduleConfigFile != ""){
            l_cfg->ReadFromFile(l_moduleConfigFile);
        }
        m_trb->ConfigureSCTModule(l_cfg.get(), (0x1 << l_moduleNo)); //sending configuration to corresponding module
        INFO("Configuration of module " << l_moduleNo << " finished.");
      }
      else{
        ERROR("Module " << l_moduleNo << " enabled by mask but no configuration file provided!");
      }
    }
  }
}


void TrackerReceiverModule::sendECR()
{
    m_trb->L1CounterReset();
}


/***************************************
 *        Start module
 * ************************************/
void TrackerReceiverModule::start(unsigned run_num) {
  m_trb->L1CounterReset();
  m_trb->StartReadout();
  INFO("TRB --> readout started.");
  FaserProcess::start(run_num);
}


/***************************************
 *        Stop module
 * ************************************/
void TrackerReceiverModule::stop() {
  m_trb->StopReadout();
  usleep(100);
  FaserProcess::stop();
  INFO("TRB --> readout stopped.");
}


/***************************************
 *        Runner of the module
 * ************************************/
void TrackerReceiverModule::runner() {
  INFO("Running...");
  std::vector<std::vector<uint32_t>> vector_of_raw_events;
  uint8_t  local_fragment_tag = EventTags::PhysicsTag;
  uint32_t local_source_id    = SourceIDs::TrackerSourceID;
  uint64_t local_event_id;
  uint64_t local_bc_id;

  while (m_run) { 
    //m_trb->GenerateL1A(m_moduleMask); //Generate L1A on the board - for testing purposes
    
    vector_of_raw_events = m_trb->GetTRBEventData();

    if (vector_of_raw_events.size() == 0){
      usleep(100); //this is to make sure we don't occupy CPU resources if no data is on output
    }
      else{
        for(std::vector<uint32_t> event : vector_of_raw_events){
          int total_size = event.size() * sizeof(uint32_t); //Event size in bytes      
          event_size_bytes = total_size; //Monitoring data

          if (event.size() == 0){ continue;}

          //TODO should we also send the EndOfDAQ trailer? Currently, we are not sending it
          if (!(m_ed->IsEndOfDAQ(event[0]))){
            m_ed->LoadTRBEventData(event);
            auto decoded_event = m_ed->GetEvents(); 

            if (decoded_event.size() != 0){
                local_event_id = decoded_event[0]->GetL1ID(); //we can always ask for element 0 - we are feeding only one event at the time to m_ed
                event_id = local_event_id | (m_ECRcount << 24); //Monitoring data
                local_bc_id = decoded_event[0]->GetBCID();
                bc_id = local_bc_id; // Monitoring data
            }
            else{
                local_event_id = 0xFFFFFFFF;
                local_bc_id = 0xFFFFFFFF;
            }


            std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, 
                                                local_event_id, local_bc_id, event.data(), total_size));
            
            uint16_t error;
            fragment->set_status(0);
            for (uint32_t frame : event){
              if(m_ed->HasError(frame, error)){
                //TODO If possible specify error
                fragment->set_status(EventStatus::UnclassifiedError);
              }
            }
            
            //This cout can be removed later but it useful for debugging
            /*std::cout << "-------------- printing event " << std::endl;
            std::cout << "Data received from TRB: " << std::endl;
            for(auto word : event){
              std::bitset<32> y(word);
              std::cout << "               " << y << " ";
              if(m_ed->HasError(word, error)){std::cout << "error word";} 
              std:: cout << std::endl;
            }
            std::cout << "event id: 0x"<< fragment->event_id() << std::endl;
            std::cout << "fragment tag: 0x"<< fragment->fragment_tag() << std::endl;
            std::cout << "source id: 0x"<< fragment->source_id() << std::endl;
            std::cout << "bc id: 0x"<< fragment->bc_id() << std::endl;
            std::cout << "status: 0x"<< fragment->status() << std::endl;
            std::cout << "trigger bits: 0x"<< fragment->trigger_bits() << std::endl;
            std::cout << "size: 0x"<< fragment->size() << std::endl;
            std::cout << "payload size: 0x"<< fragment->payload_size() << std::endl;
            std::cout << "timestamp: 0x"<< fragment->timestamp() << std::endl;
            std::cout << "Raw data sent further: " << std::endl;
            auto data = fragment->raw();
            for (int i = 0; i <  data->size(); i++){
                std::bitset<8> y(data->at(i));
                std::cout << "               " << y << std::endl;
            }*/

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
