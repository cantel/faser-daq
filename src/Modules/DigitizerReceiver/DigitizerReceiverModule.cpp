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
    // check to make sure you can copy over the config
    int length = strlen(std::string(cfg_ip).c_str());
    if(length>HOSTNAME_MAX_LENGTH){
      ERROR("This is too long of a name : "<<std::string(cfg_ip));
      ERROR("Max length of IP hostname : "<<HOSTNAME_MAX_LENGTH);
      THROW(DigitizerHardwareException, "IP address or hostname is too long");
    }

    // lookup the IP address dynamically
    // determines if its an IP address or a hostname
    std::string ip_str = std::string(GetIPAddress(std::string(cfg["host_pc"]).c_str()));
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
  bool set_interface_jumbo = (bool)cfg["readout"]["interface_jumbo"];
  m_digitizer->SetInterfaceEthernetJumboFrames(set_interface_jumbo);
  
  unsigned int set_interface_packet_gap = (unsigned int)cfg["readout"]["interface_packet_gap"];
  m_digitizer->SetInterfaceEthernetGap(set_interface_packet_gap);

  unsigned int set_interface_max_packets = (unsigned int)cfg["readout"]["interface_max_packets"];
  m_digitizer->SetInterfaceEthernetMaxPackets(set_interface_max_packets);

  // ethernet speed test
  // these can be removed if the bootup is too slow
  // gives nwords/second --> need to translate to MB/s
  float interface_rate     = m_digitizer->PerformInterfaceSpeedTest(4);

  DEBUG("Speed(interface)     [words/s]: "<<interface_rate    );
  INFO("Speed(interface)     [MB/s]: "<<(interface_rate*4)/1000000.     );

  // store local run settings
  auto cfg_software_trigger_rate = cfg["trigger_software"]["rate"];
  if(cfg_software_trigger_rate==nullptr){
    INFO("You did not specify a SW trigger rate - we will set it to 0 to be safe.");
    m_software_trigger_rate = 0;
  }
  else{
    m_software_trigger_rate = cfg_software_trigger_rate;
  }
  INFO("Trigger rate for SW triggers at : "<<m_software_trigger_rate);
  
  // check for consistency of SW triggers and acquisition
  if(m_software_trigger_rate!=0){
    auto trig_acquisition_sw = cfg["trigger_acquisition"]["software"];
    if(trig_acquisition_sw==0){
      INFO("Inconsistent settings - you have enabled SW triggers to be sent but not acquiring on SW triggers.");
      INFO("Are you sure you want this settings?");
    }
  }


  // for the TLB conversion factor on the trigger time tag
  auto cfg_ttt_converter = cfg["parsing"]["ttt_converter"];
  if(cfg_ttt_converter==nullptr){
    INFO("You did not specify the TTT converter, setting it to the LHC 40.08 MHz");
    m_ttt_converter = 40.08;
  }
  else{
    m_ttt_converter = (float)cfg_ttt_converter;  // perhaps there is a better way to do this
  }
  INFO("Setting TLB-Digitizer TTT clock to : "<<m_ttt_converter);
  
  // BCID matching parameter performs a software "delay" on the calculated BCID
  // by delaying the TriggerTimeTag by some fixed amount.  This "delay" can be positive
  // or negative and is tuned to match the BCID from the TLB
  auto cfg_bcid_ttt_fix = cfg["parsing"]["bcid_ttt_fix"];
  if(cfg_ttt_converter==nullptr){
    INFO("You did not specify the BCID fix, setting it to 0.");
    m_bcid_ttt_fix = 0;
  }
  else{
    m_bcid_ttt_fix = (float)cfg_bcid_ttt_fix;  // perhaps there is a better way to do this
  }
  INFO("Setting Digitizer BCID fix to : "<<m_bcid_ttt_fix);

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
  
  registerVariable(m_hw_buffer_space, "BufferSpace");
  
  registerVariable(m_hw_buffer_occupancy, "HWBufferOccupancy");
  
  registerVariable(m_temp_ch00, "temp_ch00");
  registerVariable(m_temp_ch01, "temp_ch01");
  registerVariable(m_temp_ch02, "temp_ch02");
  registerVariable(m_temp_ch03, "temp_ch03");
  registerVariable(m_temp_ch04, "temp_ch04");
  registerVariable(m_temp_ch05, "temp_ch05");
  registerVariable(m_temp_ch06, "temp_ch06");
  registerVariable(m_temp_ch07, "temp_ch07");
  registerVariable(m_temp_ch08, "temp_ch08");
  registerVariable(m_temp_ch09, "temp_ch09");
  registerVariable(m_temp_ch10, "temp_ch10");
  registerVariable(m_temp_ch11, "temp_ch11");
  registerVariable(m_temp_ch12, "temp_ch12");
  registerVariable(m_temp_ch13, "temp_ch13");
  registerVariable(m_temp_ch14, "temp_ch14");
  registerVariable(m_temp_ch15, "temp_ch15");
  
  registerVariable(m_time_read,     "time_RetrieveEvents");
  registerVariable(m_time_parse,    "time_ParseEvents");
  registerVariable(m_time_overhead, "time_Overhead");

  
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
  m_readout_method = (std::string)cfg["readout"]["readout_method"];
  m_readout_blt    = (int)cfg["readout"]["readout_blt"];

  // dynamically allocate the array for the raw payload
  // guidance : http://www.fredosaurus.com/notes-cpp/newdelete/50dynamalloc.html#:~:text=Allocate%20an%20array%20with%20code,and%20allocates%20that%20size%20array.
  DEBUG("Allocating software buffer ...");
  unsigned int* m_raw_payload = NULL;   
  m_software_buffer = (int)cfg["readout"]["software_buffer"];           
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
    DEBUG("[sendECR] - Sending batch of events : totalEvents = "<<std::dec<<n_events_present<<"  eventsRequested = "<<std::dec<<m_n_events_requested);

      // clear the monitoring map which will retrieve the info to pass to monitoring metrics
      m_monitoring.clear();
              
      // get the data from the board into the software buffer        
      int nevents_obtained = 0;
      nevents_obtained = m_digitizer->RetrieveEventBatch(m_raw_payload, m_software_buffer, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_n_events_requested, m_ECRcount, m_ttt_converter);

      // count triggers sent
      m_triggers += nevents_obtained;
      DEBUG("Total nevents sent in running : "<<m_triggers);

      // parse the events and decorate them with a FASER header
      std::vector<EventFragment> fragments;
      fragments = m_digitizer->ParseEventBatch(m_raw_payload, m_software_buffer, nevents_obtained, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_n_events_requested, m_ECRcount, m_ttt_converter, m_bcid_ttt_fix);
      
      DEBUG("NEventsParsed : "<<fragments.size());
      
      // ToDo : write a method to print one full event
      //if((bool)myConfig["readout"]["print_event"]){
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
      
      m_lock.unlock();
      
      DEBUG("Time read    : "<<m_monitoring["time_read_time"]);
      DEBUG("Time parse   : "<<m_monitoring["time_parse_time"]);
      DEBUG("Time header  : "<<m_monitoring["time_header_time"]);
      DEBUG("Time filler  : "<<m_monitoring["time_filler_time"]);
      
      float total_batch = m_monitoring["time_read_time"]+m_monitoring["time_parse_time"]+m_monitoring["time_header_time"]+m_monitoring["time_filler_time"];
      
      DEBUG("Parse/Read   : "<<m_monitoring["time_parse_time"]/m_monitoring["time_read_time"]);
      
      DEBUG("Total batch  : "<<total_batch);
      DEBUG("Time looping : "<<m_monitoring["time_looping_time"]<<"   ("<<total_batch/m_monitoring["time_looping_time"]<<")");
  }
  //release the lock - not after the conditional due to the ECR method
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
  m_readout_method = (std::string)cfg["readout"]["readout_method"];
  m_readout_blt    = (int)cfg["readout"]["readout_blt"];

  // dynamically allocate the array for the raw payload
  // guidance : http://www.fredosaurus.com/notes-cpp/newdelete/50dynamalloc.html#:~:text=Allocate%20an%20array%20with%20code,and%20allocates%20that%20size%20array.
  DEBUG("Allocating software buffer ...");
  unsigned int* m_raw_payload = NULL;   
  m_software_buffer = (int)cfg["readout"]["software_buffer"];           
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
  
  // start time
  auto time_start = chrono::high_resolution_clock::now();
  int last_check_count = 0;

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
    
    // how much space is left in the buffer
    m_hw_buffer_space = 1024-n_events_present;
    m_hw_buffer_occupancy = n_events_present;
    
    if(n_events_present){
      DEBUG("[Running] - Sending batch of events : totalEvents = "<<std::dec<<n_events_present<<"  eventsRequested = "<<std::dec<<m_n_events_requested);
      DEBUG("With m_ECRcount : "<<m_ECRcount);
      
      // clear the monitoring map which will retrieve the info to pass to monitoring metrics
      m_monitoring.clear();
              
      // get the data from the board into the software buffer        
      int nevents_obtained = 0;
      nevents_obtained = m_digitizer->RetrieveEventBatch(m_raw_payload, m_software_buffer, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_n_events_requested, m_ECRcount, m_ttt_converter);

      // count triggers sent
      m_triggers += nevents_obtained;
      DEBUG("Total nevents sent in running : "<<m_triggers);

      // parse the events and decorate them with a FASER header
      DEBUG("tttConverter : "<<m_ttt_converter);
      std::vector<EventFragment> fragments;
      fragments = m_digitizer->ParseEventBatch(m_raw_payload, m_software_buffer, nevents_obtained, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_n_events_requested, m_ECRcount, m_ttt_converter, m_bcid_ttt_fix);
      
      DEBUG("NEventsParsed : "<<fragments.size());
      
      // ToDo : write a method to print one full event
      //if((bool)myConfig["readout"]["print_event"]){
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
      
      DEBUG("Time read    : "<<m_monitoring["time_read_time"]);
      DEBUG("Time parse   : "<<m_monitoring["time_parse_time"]);
      DEBUG("Time header  : "<<m_monitoring["time_header_time"]);
      DEBUG("Time filler  : "<<m_monitoring["time_filler_time"]);
      
      m_time_read      = m_monitoring["time_read_time"]; 
      m_time_parse     = m_monitoring["time_parse_time"];  
      m_time_overhead  = m_monitoring["time_header_time"] + m_monitoring["time_filler_time"];   

      
      float total_batch = m_monitoring["time_read_time"]+m_monitoring["time_parse_time"]+m_monitoring["time_header_time"]+m_monitoring["time_filler_time"];
      
      DEBUG("Parse/Read   : "<<m_monitoring["time_parse_time"]/m_monitoring["time_read_time"]);
      
      DEBUG("Total batch  : "<<total_batch);
      DEBUG("Time looping : "<<m_monitoring["time_looping_time"]<<"   ("<<total_batch/m_monitoring["time_looping_time"]<<")");
    
    }
    else{
      // sleep for 1 milliseconds. This time is chosen to ensure that the polling 
      // happens at a rate just above the expected trigger rate in the case of no events
      //DEBUG("No events - sleeping for a short while");
      //release the lock - not after the conditional due to the ECR method
      m_lock.unlock();
      usleep(1000); 
    }
           

    // only get the temperatures once per second to cut down on the VME data rate
    // amount of time in loop in milliseconds
    int time_now   = (chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now() - time_start).count() * 1e-6)/1000;
    if(time_now > last_check_count){
      // update where you are in time counting
      last_check_count = time_now;
        
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
    daqling::utilities::Binary binData(bytestream->data(),bytestream->size());
    m_connections.put(0, binData); 
  }

  DEBUG("Finished passing fragments");
}
