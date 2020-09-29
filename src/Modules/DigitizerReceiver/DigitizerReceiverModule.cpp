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
#define NCHANNELS 16

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
  
  // test digitizer board interface
  m_digitizer->TestCommInterface(cfg);  
  
  // test digitizer board interface
  m_digitizer->TestCommDigitizer(cfg);
  
  // configuring ethernet transmission settings for interface board  
  // NOTE : you also need to ensure that the utm setting on ifconfig is sufficiently high for jumbo frames to work
  bool set_interface_jumbo = (bool)cfg["interface_jumbo"];
  m_digitizer->SetInterfaceEthernetJumboFrames(set_interface_jumbo);
  
  unsigned int set_interface_packet_gap = (unsigned int)cfg["interface_packet_gap"];
  m_digitizer->SetInterfaceEthernetGap(set_interface_packet_gap);

  unsigned int set_interface_max_packets = (unsigned int)cfg["interface_max_packets"];
  m_digitizer->SetInterfaceEthernetMaxPackets(set_interface_max_packets);

  // ethernet speed test
  // these can be removed if the bootup is too slow
  // gives nwords/second --> need to translate to MB/s
  float interface_rate     = m_digitizer->PerformInterfaceSpeedTest(4);

  DEBUG("Speed(interface)     [words/s]: "<<interface_rate    );
  INFO("Speed(interface)     [MB/s]: "<<(interface_rate*4)/1000000.     );

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

DigitizerReceiverModule::~DigitizerReceiverModule() { 
  INFO("DigitizerReceiverModule::Destructor"); 
}

// optional (configuration can be handled in the constructor)
void DigitizerReceiverModule::configure() {
  FaserProcess::configure();
  
  // register the metrics for monitoring data
  INFO("Configuring monitoring metrics");
  registerVariable(m_triggers, "TriggeredEvents");
  registerVariable(m_triggers, "TriggeredRate", metrics::RATE);
  
  registerVariable(m_temp_ch00, "ADC Temperature CH00");
  registerVariable(m_temp_ch01, "ADC Temperature CH01");
  registerVariable(m_temp_ch02, "ADC Temperature CH02");
  registerVariable(m_temp_ch03, "ADC Temperature CH03");
  registerVariable(m_temp_ch04, "ADC Temperature CH04");
  registerVariable(m_temp_ch05, "ADC Temperature CH05");
  registerVariable(m_temp_ch06, "ADC Temperature CH06");
  registerVariable(m_temp_ch07, "ADC Temperature CH07");
  registerVariable(m_temp_ch08, "ADC Temperature CH08");
  registerVariable(m_temp_ch09, "ADC Temperature CH09");
  registerVariable(m_temp_ch10, "ADC Temperature CH10");
  registerVariable(m_temp_ch11, "ADC Temperature CH11");
  registerVariable(m_temp_ch12, "ADC Temperature CH12");
  registerVariable(m_temp_ch13, "ADC Temperature CH13");
  registerVariable(m_temp_ch14, "ADC Temperature CH14");
  registerVariable(m_temp_ch15, "ADC Temperature CH15");
  
  // configuration of hardware
  INFO("Configuring ...");  
  m_digitizer->Configure(m_config.getConfig()["settings"]);
  
  INFO("Finished configuring - the settings of the digitizer are :");
  m_digitizer->DumpConfig();
  INFO("End of configure()");
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
  DEBUG("Sending ECR");
  // stop acquisition
  m_digitizer->StopAcquisition();
  
  auto cfg = m_config.getConfig()["settings"];
  
  // for reading the buffer
  m_readout_method = (std::string)cfg["readout_method"];
  m_readout_blt    = (int)cfg["readout_blt"];

  // dynamically allocate the array for the raw payload
  // guidance : http://www.fredosaurus.com/notes-cpp/newdelete/50dynamalloc.html#:~:text=Allocate%20an%20array%20with%20code,and%20allocates%20that%20size%20array.
  DEBUG("Allocating software buffer ...");
  unsigned int* m_raw_payload = NULL;   
  m_software_buffer = (int)cfg["software_buffer"];           
  DEBUG("NBuffer ..."<<std::dec<<m_software_buffer);
  m_raw_payload = new unsigned int[m_software_buffer];  
  
  // number of channels enabled
  m_nchannels_enabled = 0;
  for(int iChan=0; iChan<16; iChan++){
    if((int)cfg["channel_readout"].at(iChan)["enable"]==1)
      m_nchannels_enabled++;
  }
  
  // size of the buffer in samples for a single channel
  DEBUG("Getting buffer size");
  m_buffer_size = m_digitizer->RetrieveBufferLength();
  
  // maximum number of events to be requested
  DEBUG("Getting readout request size");
  m_n_events_requested = m_readout_blt;
  
  // read out the data from the buffer
  m_lock.lock();
  int n_events_present = m_digitizer->DumpEventCount();
  if(n_events_present){
    DEBUG("Sending batch of events : totalEvents = "<<std::dec<<n_events_present<<"  eventsRequested = "<<std::dec<<m_n_events_requested);
    
    // clear the monitoring map which will retrieve the info to pass to monitoring metrics
    m_monitoring.clear();
    
    // count triggers sent
    m_triggers += m_n_events_requested;
    
    // request to get a batch of events 
    // - m_n_events_requested : number of events you should read back
    // - raw_payload : the data allocation of the software buffer
    // - software_buffer : the length of that software buffer
    m_digitizer->sendEventBatch(m_raw_payload, m_software_buffer, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_ECRcount, m_ttt_converter, m_n_events_requested);
    DEBUG("Total nevents sent in running : "<<m_triggers);
  }
  m_lock.unlock();
  
  // delete the memory that was allocated
  delete [] m_raw_payload;  
  m_raw_payload = NULL;     

  // start acquisition
  m_digitizer->StartAcquisition();
}


void DigitizerReceiverModule::runner() {
  INFO("Running...");  
  
  auto cfg = m_config.getConfig()["settings"];
  
  // for reading the buffer
  m_readout_method = (std::string)cfg["readout_method"];
  m_readout_blt    = (int)cfg["readout_blt"];

  // dynamically allocate the array for the raw payload
  // guidance : http://www.fredosaurus.com/notes-cpp/newdelete/50dynamalloc.html#:~:text=Allocate%20an%20array%20with%20code,and%20allocates%20that%20size%20array.
  DEBUG("Allocating software buffer ...");
  unsigned int* m_raw_payload = NULL;   
  m_software_buffer = (int)cfg["software_buffer"];           
  DEBUG("NBuffer ..."<<std::dec<<m_software_buffer);
  m_raw_payload = new unsigned int[m_software_buffer];  
  
  // number of channels enabled
  m_nchannels_enabled = 0;
  for(int iChan=0; iChan<16; iChan++){
    if((int)cfg["channel_readout"].at(iChan)["enable"]==1)
      m_nchannels_enabled++;
  }
  
  // size of the buffer in samples for a single channel
  DEBUG("Getting buffer size");
  m_buffer_size = m_digitizer->RetrieveBufferLength();
  
  // maximum number of events to be requested
  DEBUG("Getting readout request size");
  m_n_events_requested = m_readout_blt;
  
  // the polling loop
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
      DEBUG("Sending batch of events : totalEvents = "<<std::dec<<n_events_present<<"  eventsRequested = "<<std::dec<<m_n_events_requested);
      DEBUG("With m_ECRcount : "<<m_ECRcount);
      
      // clear the monitoring map which will retrieve the info to pass to monitoring metrics
      m_monitoring.clear();
              
      // new method        
      // get the data from the board into the software buffer        
      int nevents_obtained = 0;
      nevents_obtained = m_digitizer->retrieveEventBatch(m_raw_payload, m_software_buffer, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_n_events_requested, m_ECRcount, m_ttt_converter);

      // parse the events and decorate them with a FASER header
      std::vector<EventFragment> fragments;
      fragments = m_digitizer->parseEventBatch(m_raw_payload, m_software_buffer, nevents_obtained, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_n_events_requested, m_ECRcount, m_ttt_converter);
      
      DEBUG("NEventsParsed : "<<fragments.size());
      
      // ToDo : write a method to print one full event
      //if((bool)myConfig["print_event"]){
      //  if(fragments.size()>=1){
      //    const EventFragment frag = fragments.at(0);
      //    DigitizerDataFragment digitizer_data_frag = DigitizerDataFragment(frag.payload<const uint32_t*>(), frag.payload_size());
      //    std::cout<<"Digitizer data fragment:"<<std::endl;
      //    std::cout<<digitizer_data_frag<<std::endl;
      //  }
      //}
      
      // pass the group of events on in faser/daq
      // NOTE : this should only exist in the DigitizerReceiver module due to access
      //        of the DAQling methods and such
      this->PassEventBatch(fragments); // defined in the Digitizer
      
      //release the lock - not after the conditional due to the ECR method
      m_lock.unlock();
      DEBUG("Total nevents sent in running : "<<m_triggers);
    }
    else{
      // sleep for 1.5 milliseconds. This time is chosen to ensure that the polling 
      // happens at a rate just above the expected trigger rate in the case of no events
      DEBUG("No events - sleeping for a short while");
      
      //release the lock - not after the conditional due to the ECR method
      m_lock.unlock();
      usleep(100); 
    }
           
    DEBUG("Time read    : "<<m_monitoring["time_read_time"]);
    DEBUG("Time parse   : "<<m_monitoring["time_parse_time"]);
    DEBUG("Time header  : "<<m_monitoring["time_header_time"]);
    DEBUG("Time filler  : "<<m_monitoring["time_filler_time"]);
    
    float total_batch = m_monitoring["time_read_time"]+m_monitoring["time_parse_time"]+m_monitoring["time_header_time"]+m_monitoring["time_filler_time"];
    
    DEBUG("Parse/Read   : "<<m_monitoring["time_parse_time"]/m_monitoring["time_read_time"]);
    
    DEBUG("Total batch  : "<<total_batch);
    DEBUG("Time looping : "<<m_monitoring["time_looping_time"]<<"   ("<<total_batch/m_monitoring["time_looping_time"]<<")");
    
    // monitoring of temperature on ADCs
    std::vector<int> adc_temp = m_digitizer->GetADCTemperature(false);
    if(adc_temp.size()==NCHANNELS){
      m_temp_ch00 = adc_temp.at(0);
      m_temp_ch01 = adc_temp.at(1);
      m_temp_ch02 = adc_temp.at(2);
      m_temp_ch03 = adc_temp.at(3);
      m_temp_ch04 = adc_temp.at(4);
      m_temp_ch05 = adc_temp.at(5);
      m_temp_ch06 = adc_temp.at(6);
      m_temp_ch07 = adc_temp.at(7);
      m_temp_ch08 = adc_temp.at(8);
      m_temp_ch09 = adc_temp.at(9);
      m_temp_ch10 = adc_temp.at(10);
      m_temp_ch11 = adc_temp.at(11);
      m_temp_ch12 = adc_temp.at(12);
      m_temp_ch13 = adc_temp.at(13);
      m_temp_ch14 = adc_temp.at(14);
      m_temp_ch15 = adc_temp.at(15);
    }
    else{
      ERROR("Temperature monitoring picked up incorrect number of channels : "<<NCHANNELS);
    }
  }
  
  // delete the memory that was allocated
  delete [] m_raw_payload;  
  m_raw_payload = NULL;     
  
  INFO("Runner stopped");
}

void DigitizerReceiverModule::PassEventBatch(std::vector<EventFragment> fragments){
  DEBUG("passEventBatch()");
  DEBUG("Passing NFrag : "<<fragments.size());

  for(int ifrag=0; ifrag<(int)fragments.size(); ifrag++){
  
    // create the event fragment
    //std::unique_ptr<EventFragment> fragment;
    //fragment = fragments.at(ifrag);

    // ToDo : What is the status supposed to be?
    int status = 0;
    fragments.at(ifrag).set_status( status );

    // place the raw binary event fragment on the output port
    std::unique_ptr<const byteVector> bytestream(fragments.at(ifrag).raw());
    
    // only necessary for the real module
    daqling::utilities::Binary binData(bytestream->data(),bytestream->size());
    m_connections.put(0, binData); 
  }

  DEBUG("Finished passing fragments");
}