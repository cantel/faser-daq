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

#include "DigitizerModule.hpp"

DigitizerModule::DigitizerModule() { INFO(""); 

  INFO("");

  auto cfg = m_config.getConfig()["settings"];
  
  m_var = cfg["myVar"];
  
  INFO("SAMSAMSAMSAM myVar printout "<<m_var);
  

  // ip address
  char  ip_addr_string[32];
  strcpy(ip_addr_string, std::string(cfg["ip"]).c_str() ) ; // SIS3153 IP address
  INFO("IP Address : "<<ip_addr_string);

  // vme base address
  std::string vme_base_address_str = std::string(cfg["vme_base_address"]);
  UINT vme_base_address = std::stoi(vme_base_address_str,0,16);
  INFO("Base VME Address = 0x"<<std::setfill('0')<<std::setw(8)<<std::hex<<vme_base_address);

  // make a new digitizer instance
  m_digitizer = new vx1730(ip_addr_string, vme_base_address);

  // test digitizer board interface
  m_digitizer->TestComm();


}

DigitizerModule::~DigitizerModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void DigitizerModule::configure() {
  FaserProcess::configure();
  INFO("Digitizer --> Configuring");
  m_digitizer->Configure(m_config.getConfig()["settings"]);
}

void DigitizerModule::start(int run_num) {
  FaserProcess::start(run_num);
  INFO("Digitizer --> Starting");
  m_digitizer->StartAcquisition();
}

void DigitizerModule::stop() {
  FaserProcess::stop();
  INFO("Digitizer --> Stopping");
  m_digitizer->StopAcquisition();
}

void DigitizerModule::runner() {
  INFO("Running...");
  
  int count=0;  
  
  while (m_run) {
  
    // ask digitizer for events in buffer
    
    count+=1;    
    INFO("Count"<<count);
    if(count%5==0){
      m_digitizer->SendSWTrigger();
    }
    
    INFO("EventCount : "<<m_digitizer->DumpEventCount());
    
    if(m_digitizer->DumpEventCount()){


      /////////////////////////////////
      // get the event
      /////////////////////////////////
      INFO("Get the Event");
      uint32_t raw_payload[MAXFRAGSIZE];
      m_digitizer->ReadRawEvent( raw_payload, true );


      int payload_size = Payload_GetEventSize( raw_payload );
      const int total_size = sizeof(uint32_t) * payload_size;  // size of my payload in bytes

      const EventFragment* fragment;
      
      // FIXME : these are variables that should be controlled by DAQ
      uint8_t  local_fragment_tag = 0;
      uint32_t local_source_id    = 0;
      uint64_t local_event_id     = 0;
      uint16_t local_bc_id        = 0;

      fragment = new EventFragment(local_fragment_tag, local_source_id, local_event_id, local_bc_id, Binary(raw_payload, total_size) );
      
      fragment->payload();
      
      


    
      Wait(1.0);
    
    }
  }
  INFO("Runner stopped");
}
