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

#include "DigitizerReceiverModule.hpp"

DigitizerReceiverModule::DigitizerReceiverModule() { INFO(""); 
  INFO("DigitizerReceiverModule Constructor");

  auto cfg = m_config.getConfig()["settings"];
  
  // retrieve the ip address of the sis3153 master board
  // this is configured via the arp and route commands on the network switch
  char  ip_addr_string[32];
  strcpy(ip_addr_string, std::string(cfg["ip"]).c_str() ) ; 
  INFO("IP Address : "<<ip_addr_string);

  // retrieval of the VME base address which is set via the physical rotary
  // switches on the digitizer and forms the base of all register read/writes
  std::string vme_base_address_str = std::string(cfg["vme_base_address"]);
  UINT vme_base_address = std::stoi(vme_base_address_str,0,16);
  INFO("Base VME Address = 0x"<<std::setfill('0')<<std::setw(8)<<std::hex<<vme_base_address);

  // make a new digitizer instance
  m_digitizer = new vx1730(ip_addr_string, vme_base_address);

  // test the communication line of the digitizer
  m_digitizer->TestComm();
}

DigitizerReceiverModule::~DigitizerReceiverModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void DigitizerReceiverModule::configure() {
  FaserProcess::configure();
  INFO("Digitizer --> Configuring");
  m_digitizer->Configure(m_config.getConfig()["settings"]);
}

void DigitizerReceiverModule::start(unsigned int run_num) {
  INFO("Digitizer --> Starting BEFORE");
  FaserProcess::start(run_num);
  INFO("Digitizer --> Starting");
  INFO("CONFIG before startAcquire");
  m_digitizer->DumpConfig();
  m_digitizer->StartAcquisition();
  INFO("CONFIG after startAcquire");
  m_digitizer->DumpConfig();
}

void DigitizerReceiverModule::stop() {
  FaserProcess::stop();
  INFO("Digitizer --> Stopping");
  m_digitizer->StopAcquisition();
}

void DigitizerReceiverModule::sendECR() {
  // should send ECR to electronics here. In case of failure, seet m_status to STATUS_ERROR
  
  // stop acquisition
  m_digitizer->StopAcquisition();
  
  // read out the data from the buffer
  while(m_digitizer->DumpEventCount()){
    sendEvent();
  }

  // start acquisition
  m_digitizer->StartAcquisition();
}


void DigitizerReceiverModule::runner() {
  INFO("Running...");  
  while (m_run) {    
    if(m_digitizer->DumpEventCount()){
      DEBUG("Sending Event");
      sendEvent();
    }
  }
  INFO("Runner stopped");
}



void DigitizerReceiverModule::sendEvent() {
  INFO("Sending Event...");
  
  // get the event
  uint32_t raw_payload[MAXFRAGSIZE];
  m_digitizer->ReadRawEvent( raw_payload, true );

  // creating the payload
  int payload_size = Payload_GetEventSize( raw_payload );
  const int total_size = sizeof(uint32_t) * payload_size;  // size of my payload in bytes

  // the event ID is what should be used, in conjunction with the ECR to give a unique event tag
  // word[2] bits[23:0]
  // need to blank out the top bits because these are the channel masks
  unsigned int Header_EventCounter   = raw_payload[2];
  SetBit(Header_EventCounter, 31, 0); 
  SetBit(Header_EventCounter, 30, 0);
  SetBit(Header_EventCounter, 29, 0);
  SetBit(Header_EventCounter, 28, 0);
  SetBit(Header_EventCounter, 27, 0);
  SetBit(Header_EventCounter, 26, 0);
  SetBit(Header_EventCounter, 25, 0);
  SetBit(Header_EventCounter, 24, 0);
  
  // the trigger time tag is used to give a "verification" of the event ID if that fails
  // it is nominally the LHC BCID but we need to do a conversion from our clock to the LHC clock
  // word[3] bits[31:0]
  unsigned int Header_TriggerTimeTag = raw_payload[3];
  
  DEBUG("Header_EventCounter   : "+to_string(Header_EventCounter));
  DEBUG("Header_TriggerTimeTag : "+to_string(Header_TriggerTimeTag));
  
  // store the faser header information
  uint8_t  local_fragment_tag = EventTags::PhysicsTag;
  uint32_t local_source_id    = SourceIDs::PMTSourceID;
  uint64_t local_event_id     = (m_ECRcount<<24) + (Header_EventCounter+1); // from the header and the ECR from sendECR() counting m_ECRcount [ECR]+[EID]
  uint16_t local_bc_id        = Header_TriggerTimeTag*(40.0/62.5);      // trigger time tag corrected by LHCClock/TrigClock = 40/62.5

  // create the event fragment
  std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, local_event_id, local_bc_id, raw_payload, total_size ));

  // ToDo : What is the status supposed to be?
  uint16_t status=0;
  fragment->set_status( status );

  // place the raw binary event fragment on the output port
  
  std::unique_ptr<const byteVector> bytestream(fragment->raw());
  daqling::utilities::Binary binData(bytestream->data(),bytestream->size());

  m_connections.put(0, binData);  
}