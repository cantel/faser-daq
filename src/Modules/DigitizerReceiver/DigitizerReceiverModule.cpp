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

#define HOSTNAME_MAX_LENGTH 100

DigitizerReceiverModule::DigitizerReceiverModule() { INFO(""); 
  INFO("DigitizerReceiverModule Constructor");

  auto cfg = m_config.getConfig()["settings"];
  
  // retrieve the ip address of the sis3153 master board
  // this is configured via the arp and route commands on the network switch
  INFO("Getting IP Address");
  char  ip_addr_string[HOSTNAME_MAX_LENGTH];
  auto cfg_ip = cfg["host_pc"];
  if (cfg_ip!="" && cfg_ip!=nullptr){
    // temporary location for IP name
    char input_ip_location[HOSTNAME_MAX_LENGTH];

    // check to make sure you can copy over the config
    int length = strlen(std::string(cfg_ip).c_str());
    if(length>HOSTNAME_MAX_LENGTH){
      ERROR("This is too long of a name : "<<std::string(cfg_ip));
      ERROR("Max length of IP hostname : "<<HOSTNAME_MAX_LENGTH);
      THROW(DigitizerHardwareException, "IP address or hostname is too long");
    }

    // lookup the IP address dynamically
    // determines if its an IP address or a hostname
    strcpy(input_ip_location, std::string(cfg["host_pc"]).c_str() ) ;
    INFO("Input locale : "<<input_ip_location);
    std::string ip_str = std::string(GetIPAddress(input_ip_location));
    strcpy(ip_addr_string, ip_str.c_str());
    INFO("Input address/host  : "<<cfg["host_pc"]);
    INFO("Returned IP Address : "<<ip_addr_string);

    // check to make sure the thing is an IP address
    if(IsIPAddress(ip_addr_string)==false){
      strcpy(ip_addr_string,"0.0.0.0");
      ERROR("This is not an IP address : "<<ip_addr_string);
      THROW(DigitizerHardwareException, "Invalid IP address");
    }
  }
  else{
    ERROR("No IP address setting in the digitizer configuration.");
    THROW(DigitizerHardwareException, "No valid IP address setting in your config");
  }
    
  INFO("Digitizer IP Address : "<<ip_addr_string);

  // retrieval of the VME base address which is set via the physical rotary
  // switches on the digitizer and forms the base of all register read/writes
  INFO("Getting VME digitizer HW address");
  UINT vme_base_address;
  auto cfg_vme_base_address = cfg["digitizer_hw_address"];
  if(cfg_vme_base_address!="" && cfg_vme_base_address!=nullptr){
    vme_base_address = std::stoi(std::string(cfg_vme_base_address),0,16);
  }
  else{
    ERROR("No VME Base address setting in the digitizer configuration.");
    THROW(DigitizerHardwareException, "No valid VME Base address setting in your config");
  }
  INFO("Base VME Address = 0x"<<std::setfill('0')<<std::setw(8)<<std::hex<<vme_base_address);

  // make a new digitizer instance
  m_digitizer = new vx1730(ip_addr_string, vme_base_address);

  // test the communication line of the digitizer
  m_digitizer->TestComm();

  // store local run settings
  auto cfg_software_trigger_enable = cfg["trigger_software"]["enable"];
  if(cfg_software_trigger_enable==nullptr){
    INFO("You did not specify if you wanted to send SW triggers - assuming FALSE");
    m_software_trigger_enable = false;
  }
  else{
    m_software_trigger_enable = cfg_software_trigger_enable;
    auto global_trig_sw = cfg["global_trigger"]["software"];
    if(global_trig_sw==0){
      INFO("Inconsistent settings - you have enabled SW triggers to be sent but not acquiring on SW triggers.");
      INFO("Are you sure you want this settings?");
    }
  }
  INFO("Are software triggers enabled? : "<<m_software_trigger_enable);

  auto cfg_software_trigger_rate = cfg["trigger_software"]["rate"];
  if(cfg_software_trigger_rate==nullptr){
    INFO("You did not specify a SW trigger rate");
    cfg_software_trigger_rate = 1;
  }
  else{
    m_software_trigger_rate = cfg_software_trigger_rate;
  }

  if(cfg_software_trigger_enable){
    INFO("Trigger rate for SW triggers at : "<<m_software_trigger_rate);
  }
  
  // for the TLB conversion factor on the trigger time tag
  auto cfg_ttt_converter = cfg["ttt_converter"];
  if(cfg_ttt_converter==nullptr){
    INFO("You did not specify the TTT converter, setting it to the LHC 40.08 MHz");
    cfg_ttt_converter = "40.08";
  }
  m_ttt_converter = std::atof(std::string(cfg_ttt_converter).c_str());  // perhaps there is a better way to do this

  INFO("Setting TLB-Digitizer TTT clock to : "<<m_ttt_converter);
  


}

DigitizerReceiverModule::~DigitizerReceiverModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void DigitizerReceiverModule::configure() {
  FaserProcess::configure();
  
  // register the metrics for monitoring data
  INFO("Configuring monitoring metrics");
  registerVariable(m_triggers, "TriggeredEvents");
  registerVariable(m_triggers, "TriggeredRate", metrics::RATE);
  
  // configuration of hardware
  INFO("Configuring ...");  
  m_digitizer->Configure(m_config.getConfig()["settings"]);
  
  INFO("Finished configuring - the settings of the digitizer are :");
  m_digitizer->DumpConfig();
}

void DigitizerReceiverModule::start(unsigned int run_num) {
  INFO("Starting ...");

  // register the metrics for monitoring data
  INFO("Initializing monitoring metrics");
  m_triggers=0;
  m_sw_count=0;

  // starting of acquisition in hardware
  m_digitizer->StartAcquisition();
  
  FaserProcess::start(run_num);
}

void DigitizerReceiverModule::stop() {
  FaserProcess::stop();
  INFO("Stopping ...");
  m_digitizer->StopAcquisition();
}

void DigitizerReceiverModule::sendECR() {
  // should send ECR to electronics here. In case of failure, set m_status to STATUS_ERROR
  
  // stop acquisition
  m_digitizer->StopAcquisition();
  
  // read out the data from the buffer
  m_lock.lock();
  while(m_digitizer->DumpEventCount()){
    sendEvent();
  }
  m_lock.unlock();

  // start acquisition
  m_digitizer->StartAcquisition();
}


void DigitizerReceiverModule::runner() {
  INFO("Running...");  
  while (m_run) {    

    // send software triggers for development if enabled
    // these are sent with a pause after the sending to have a rate limit
    if(m_software_trigger_enable){
      m_sw_count++;
      INFO("Software trigger sending : "<<m_sw_count);
      m_digitizer->SendSWTrigger();
      usleep((1.0/m_software_trigger_rate)*1000000);
    }
    else{
      DEBUG("You are not sending random triggers");
    }
    

    // lock to prevent accidental double reading with the sendECR() call
    m_lock.lock();
    
    // polling of the hardware - if any number of events is detected
    // then all events in the buffer are read out
    int n_events_present = m_digitizer->DumpEventCount();
    if(n_events_present){
      DEBUG("Sending all events - NEvents = "<<n_events_present);
      for(int iev=0; iev<n_events_present; iev++){
        sendEvent();
        m_triggers++;
      }
      m_lock.unlock();
      DEBUG("Total nevents sent in running : "<<m_triggers);
    }
    else{
      // sleep for 1.5 milliseconds. This time is chosen to ensure that the polling 
      // happens at a rate just above the expected trigger rate in the case of no events
      DEBUG("No events - sleeping for a short while");
      m_lock.unlock();
      usleep(1500); 
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

  DEBUG("PayloadSize : nwords="<<payload_size<<"  total_size="<<total_size);

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
  uint16_t local_bc_id        = Header_TriggerTimeTag*(m_ttt_converter/125);      // trigger time tag corrected by LHCClock/TrigClock = m_ttt_converter/125, where m_ttt_converter is configurable but by default is 40.08


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
