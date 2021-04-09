/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
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
  m_software_trigger_enable = false;
  if(m_software_trigger_rate!=0){
    auto trig_acquisition_sw = cfg["trigger_acquisition"]["software"];
    if(trig_acquisition_sw==0){
      INFO("Inconsistent settings - you have enabled SW triggers to be sent but not acquiring on SW triggers.");
      INFO("Are you sure you want this settings?");
    } else {
      m_software_trigger_enable = true;
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

  registerVariable(m_corrupted_events, "CorruptedEvents");
  registerVariable(m_empty_events, "EmptyEvents");

  registerVariable(m_info_udp_receive_timeout_counter,"info_udp_receive_timeout_counter");
  registerVariable(m_info_wrong_cmd_ack_counter,"info_wrong_cmd_ack_counter");
  registerVariable(m_info_wrong_received_nof_bytes_counter,"info_wrong_received_nof_bytes_counter");
  registerVariable(m_info_wrong_received_packet_id_counter,"info_wrong_received_packet_id_counter");

  registerVariable(m_info_clear_UdpReceiveBuffer_counter,"info_clear_UdpReceiveBuffer_counter");
  registerVariable(m_info_read_dma_packet_reorder_counter,"info_read_dma_packet_reorder_counter");

  registerVariable(m_udp_single_read_receive_ack_retry_counter,"udp_single_read_receive_ack_retry_counter");
  registerVariable(m_udp_single_read_req_retry_counter,"udp_single_read_req_retry_counter");

  registerVariable(m_udp_single_write_receive_ack_retry_counter,"udp_single_write_receive_ack_retry_counter");
  registerVariable(m_udp_single_write_req_retry_counter,"udp_single_write_req_retry_counter");

  registerVariable(m_udp_dma_read_receive_ack_retry_counter,"udp_dma_read_receive_ack_retry_counter");
  registerVariable(m_udp_dma_read_req_retry_counter,"udp_dma_read_req_retry_counter");

  registerVariable(m_udp_dma_write_receive_ack_retry_counter,"udp_dma_write_receive_ack_retry_counter");
  registerVariable(m_udp_dma_write_req_retry_counter,"udp_dma_write_req_retry_counter");
  
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
  std::this_thread::sleep_for(std::chrono::microseconds(100000)); //wait for events 
  FaserProcess::stop();
  INFO("Stopping ...");
  m_digitizer->StopAcquisition();
}

void DigitizerReceiverModule::sendECR() {
  // should send ECR to electronics here. In case of failure, set m_status to STATUS_ERROR
  DEBUG("Sending ECR");

  // ensure no data readout going on in parallel
  m_lock.lock();

  // stop acquisition
  m_digitizer->StopAcquisition();
  
  // restart acquisition to reset event counter
  m_digitizer->StartAcquisition();
  m_prev_event_id = m_ECRcount<<24;
  //release the lock
  m_lock.unlock();
}


void DigitizerReceiverModule::runner() noexcept {
  INFO("Running...");  
  
  auto cfg = m_config.getConfig()["settings"];
  m_prev_event_id=0;

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
  int m_event_size =  ((m_buffer_size/2)*m_nchannels_enabled)+4; //should check this from digitizer
  INFO("Expected event size: "<<m_event_size<<" words");
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
      INFO("Software trigger sending trigger number : "<<m_sw_count);
      m_digitizer->SendSWTrigger();
      usleep((1.0/m_software_trigger_rate)*1000000);
    }
    
    // lock to prevent accidental double reading with the sendECR() call
    m_lock.lock();
  
    // polling of the hardware - if any number of events is detected
    // then all events in the buffer are read out
    int n_events_present = 0;
    try {
      n_events_present = m_digitizer->DumpEventCount();
    } catch (DigitizerHardwareException &e) {
      static int numErrors=100;
      ERROR("Failed to read number of events: " << e.what());
      if (numErrors--==0) {
	INFO("Too many read errors - bailing out");
	throw e;
      }
    }
    // how much space is left in the buffer
    m_hw_buffer_space = 1024-n_events_present;
    m_hw_buffer_occupancy = n_events_present;
    bool shouldsleep = (n_events_present==0);
    float read_time=0;
    float parse_time=0;
    while(n_events_present){
      DEBUG("[Running] - Sending batch of events : totalEvents = "<<std::dec<<n_events_present<<"  eventsRequested = "<<std::dec<<m_n_events_requested);
      DEBUG("With m_ECRcount : "<<m_ECRcount);
      
      // clear the monitoring map which will retrieve the info to pass to monitoring metrics
      m_monitoring.clear();
              
      // get the data from the board into the software buffer        
      int nwords_obtained = 0;
      int nerrors = 0;
      nwords_obtained = m_digitizer->ReadSingleEvent(m_raw_payload, m_event_size, m_monitoring, nerrors,
						     m_readout_method, false);
      read_time+=m_monitoring["block_readout_time"];

      if (nwords_obtained==0) { //most likely reqest didn't arrive at VME card
	m_empty_events++;
	continue;
      }
      if ((nwords_obtained!=m_event_size)&&(nerrors==0)) {
	WARNING("Got "<<nwords_obtained<<" words while expecting "<<m_event_size<<" words, but no errors?");
	nerrors=1;
      }
      // count triggers sent
      m_triggers ++;

      // parse the events and decorate them with a FASER header
      auto fragment = m_digitizer->ParseEventSingle(m_raw_payload, m_event_size, m_monitoring, 
						    m_ECRcount, m_ttt_converter, m_bcid_ttt_fix, nerrors);

      parse_time+=m_monitoring["time_parse_time"];

      // send vent to event builder
      if (fragment->event_id()!=m_prev_event_id+1) {
	WARNING("Got fragment "<<fragment->event_id()<<" was expecting: "<<m_prev_event_id+1);
      }
      m_prev_event_id=fragment->event_id();
      if (fragment->status()) m_corrupted_events++;


      // place the raw binary event fragment on the output port
      std::unique_ptr<const byteVector> bytestream(fragment->raw());
      daqling::utilities::Binary binData(bytestream->data(),bytestream->size());
      m_connections.send(0, binData);  
      
      n_events_present--;
    }
    m_lock.unlock();

    // sleep for 1 milliseconds. This time is chosen to ensure that the polling 
    // happens at a rate just above the expected trigger rate in the case of no events
    if (shouldsleep) {
      usleep(1000); 
    } else {
      m_time_read = read_time;
      m_time_parse = parse_time;
    }


    m_info_udp_receive_timeout_counter=m_digitizer->m_crate->info_udp_receive_timeout_counter;
    
    m_info_wrong_cmd_ack_counter=m_digitizer->m_crate->info_wrong_cmd_ack_counter;
    m_info_wrong_received_nof_bytes_counter=m_digitizer->m_crate->info_wrong_received_nof_bytes_counter;
    m_info_wrong_received_packet_id_counter=m_digitizer->m_crate->info_wrong_received_packet_id_counter;

    m_info_clear_UdpReceiveBuffer_counter=m_digitizer->m_crate->info_clear_UdpReceiveBuffer_counter;
    m_info_read_dma_packet_reorder_counter=m_digitizer->m_crate->info_read_dma_packet_reorder_counter;

    m_udp_single_read_receive_ack_retry_counter=m_digitizer->m_crate->udp_single_read_receive_ack_retry_counter;
    m_udp_single_read_req_retry_counter=m_digitizer->m_crate->udp_single_read_req_retry_counter;

    m_udp_single_write_receive_ack_retry_counter=m_digitizer->m_crate->udp_single_write_receive_ack_retry_counter;
    m_udp_single_write_req_retry_counter=m_digitizer->m_crate->udp_single_write_req_retry_counter;

    m_udp_dma_read_receive_ack_retry_counter=m_digitizer->m_crate->udp_dma_read_receive_ack_retry_counter;
    m_udp_dma_read_req_retry_counter=m_digitizer->m_crate->udp_dma_read_req_retry_counter;

    m_udp_dma_write_receive_ack_retry_counter=m_digitizer->m_crate->udp_dma_write_receive_ack_retry_counter;
    m_udp_dma_write_req_retry_counter=m_digitizer->m_crate->udp_dma_write_req_retry_counter;
           
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
