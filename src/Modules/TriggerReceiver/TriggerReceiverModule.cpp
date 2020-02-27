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
#include "Commons/EventFormat.hpp"

TriggerReceiverModule::TriggerReceiverModule() {
  INFO("");
  m_tlb = new FASER::TLBAccess();
  m_decode = new FASER::TLBDecode();
  m_tlb->SetDebug(0); //Set to 0 for no debug, to 1 for debug. Changes the m_DEBUG variable
}

TriggerReceiverModule::~TriggerReceiverModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void TriggerReceiverModule::configure() {
  FaserProcess::configure();

  registerVariable(m_physicsEventCount, "PhysicsEvents");
  registerVariable(m_physicsEventCount, "PhysicsRate", metrics::RATE);
  registerVariable(m_monitoringEventCount, "MonitoringEvents");
  registerVariable(m_monitoringEventCount, "MonitoringRate", metrics::RATE);
  registerVariable(m_badFragmentsCount, "BadFragments");
  registerVariable(m_badFragmentsCount, "BadFragmentsRate", metrics::RATE);
  registerVariable(m_status, "Status");
  registerVariable(m_trigger_payload_size, "TriggerPayloadSize");
  registerVariable(m_monitoring_payload_size, "MonitoringPayloadSize");
  
  auto cfg = m_config.getSettings();
  
  INFO("Configuring TLB");
  if (m_tlb->ConfigureAndVerifyTLB(cfg)){INFO("TLB Configuration OK");}
  else{ERROR("TLB Configuration failed");}
  INFO("Done.");
  
  INFO("Configuring LUT");
  m_tlb->ConfigureLUT("/home/eljohnso/daq/gpiodrivers/TLBAccess/config/LUT1.txt"); //Path has to be absolut
  INFO("Done.");  
}

void TriggerReceiverModule::enableTrigger(const std::string &arg) {
  INFO("Got enableTrigger command with argument "<<arg);
  // everything but the TLB process should ignore this
  m_tlb->EnableTrigger();
}

void TriggerReceiverModule::disableTrigger(const std::string &arg) { //run with "command disableTrigger"
  INFO("Got disableTrigger command with argument "<<arg);
  // everything but the TLB proces should ignore this
  m_tlb->DisableTrigger();
  usleep(100); //value to be tweaked. Should be large enough to empty the on-board buffer.
}

void TriggerReceiverModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  INFO("");
  
  auto myjson = m_config.getSettings();
  int WhatToRead=0x0;
  WhatToRead=(WhatToRead|(myjson["EnableTriggerData"].get<bool>()<<13));
  WhatToRead=(WhatToRead|(myjson["EnableMonitoringData"].get<bool>()<<14));
  WhatToRead=(WhatToRead|(myjson["ReadoutFIFOReset"].get<bool>()<<15));
  m_tlb->StartReadout(WhatToRead);
  
}

void TriggerReceiverModule::stop() {  
  std::cout<<"Stopping readout."<<std::endl;
  m_tlb->DisableTrigger();
  m_tlb->StopReadout();
  usleep(100); //value to be tweaked. Should be large enough to empty the on-board buffer.
  FaserProcess::stop(); //this turns m_run to false
  INFO("");
}

void TriggerReceiverModule::runner() {
  INFO("Running...");
  
  std::vector<std::vector<uint32_t>> vector_of_raw_events;
  uint32_t raw_payload[64000/4];
  uint32_t* raw_payload_ptr = raw_payload;
  uint16_t status=0;
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
        raw_payload_ptr = vector_of_raw_events[i].data(); //converts each vector event to an array
        int total_size = vector_of_raw_events[i].size() * sizeof(uint32_t); //Event size in byte
        if (!total_size) continue;
        //std::cout<<"Header: "<<std::hex<<vector_of_raw_events[i][0]<<std::dec<<std::endl;
        if (m_decode->IsTriggerHeader(vector_of_raw_events[i][0])){
          local_fragment_tag=EventTags::PhysicsTag;
          m_physicsEventCount+=1;
          m_trigger_payload_size = total_size;
        }
        if (m_decode->IsMonitoringHeader(vector_of_raw_events[i][0])){
          local_fragment_tag=EventTags::TLBMonitoringTag;
          m_monitoringEventCount+=1;
          m_monitoring_payload_size = total_size;
        }
        status=m_decode->GetL1IDandBCID(vector_of_raw_events[i], local_event_id, local_bc_id);
        m_status=status;
        if (status!=0){m_badFragmentsCount+=1;}
        std::cout<<std::dec<<"L1ID: "<<local_event_id<<" BCID: "<<local_bc_id;
        if (local_fragment_tag==EventTags::PhysicsTag){std::cout<<" Trigger";}
        if (local_fragment_tag==EventTags::TLBMonitoringTag){std::cout<<" Monitoring";}
        std::cout<<" Status: "<<status<<" ECRcount: "<<m_ECRcount<<std::endl;
        local_event_id = (m_ECRcount<<24) + (local_event_id);
        std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, 
                                              local_event_id, local_bc_id, Binary(raw_payload_ptr, total_size)));
        fragment->set_status(status);
        m_connections.put(0, const_cast<Binary&>(fragment->raw())); // place the raw binary event fragment on the output port
      }
    } 
  }
  INFO("Runner stopped");
}
