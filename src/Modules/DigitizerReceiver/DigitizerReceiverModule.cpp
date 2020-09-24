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
  m_digitizer->TestCommInterface();  
  
  // test digitizer board interface
  m_digitizer->TestCommDigitizer();
  
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
  //float vme_interface_rate = m_digitizer->PerformInterfaceVMESpeedTest(4); // NOTE : if there is no data in this buffer this messes things up for good

  // show the words/s
  DEBUG("Speed(interface)     [words/s]: "<<interface_rate    );
  //DEBUG("Speed(interface+vme) [words/s]: "<<vme_interface_rate);
  
  // show the MB/s
  INFO("Speed(interface)     [MB/s]: "<<(interface_rate*4)/1000000.     );
  //INFO("Speed(interface+vme) [MB/s]: "<<(vme_interface_rate*4)/1000000. );

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
      
      // request to get a batch of events 
      // - m_n_events_requested : number of events you should read back
      // - raw_payload : the data allocation of the software buffer
      // - software_buffer : the length of that software buffer
      sendEventBatchSpaceSaver(m_raw_payload, m_software_buffer, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_n_events_requested);
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
      
      // clear the monitoring map which will retrieve the info to pass to monitoring metrics
      m_monitoring.clear();
      
      // request to get a batch of events 
      // - m_n_events_requested : number of events you should read back
      // - raw_payload : the data allocation of the software buffer
      // - software_buffer : the length of that software buffer
      sendEventBatchSpaceSaver(m_raw_payload, m_software_buffer, m_monitoring, n_events_present, m_nchannels_enabled, m_buffer_size, m_readout_method, m_n_events_requested, (bool)cfg["print_event"]);

      m_lock.unlock();
      DEBUG("Total nevents sent in running : "<<m_triggers);
    }
    else{
      // sleep for 1.5 milliseconds. This time is chosen to ensure that the polling 
      // happens at a rate just above the expected trigger rate in the case of no events
      DEBUG("No events - sleeping for a short while");
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



void DigitizerReceiverModule::sendEventSingle() {

  int print_rate=10;

  if(m_triggers%print_rate==0){
    INFO("Sending Digitizer Event ... N(trigger)="<<m_triggers);
  }  
  
  // get the event
  uint32_t raw_payload[MAXFRAGSIZE];
  m_digitizer->ReadRawEvent( raw_payload, true );

  // creating the payload
  int payload_size = Payload_GetEventSize( raw_payload );
  const int total_size = sizeof(uint32_t) * payload_size;  // size of my payload in bytes

  if(m_triggers%print_rate==0){
    INFO("PayloadSize : nwords="<<payload_size<<"  total_size="<<total_size);
  }  
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

void DigitizerReceiverModule::sendEventBatchSpaceSaver(uint32_t raw_payload[], int software_buffer, std::map<std::string, float>& monitoring, int nevents, int nchannels_enabled, int buffer_size, std::string readout_method, int events_to_readout, bool debug) {
  //INFO("Sending full buffer ...");
  
// system time before
  auto start_header_time = chrono::high_resolution_clock::now(); 
  
  std::memset(raw_payload, 0, software_buffer);
  
  // can predict the size of the buffer that will be read out
  // [1] number of events in buffer
  // [2] known event size (nchannels enabled, buffer size)
    
  DEBUG("nevents              : "<<std::dec<<nevents);
  DEBUG("nchannels_enabled    : "<<std::dec<<nchannels_enabled);
  DEBUG("buffer_size          : "<<std::dec<<buffer_size);
  DEBUG("readout_method       : "<<std::dec<<readout_method);
  DEBUG("events_to_readout          : "<<std::dec<<events_to_readout);

  int event_size = (nchannels_enabled*(buffer_size/2.0) + 4);  
  
  // get the full event buffer from the digitizer board
  //uint32_t raw_payload[MAXFRAGSIZE];
  int nwords=-1;
  DEBUG("Prepping to read buffer");
  
  int nevents_to_transfer = events_to_readout;
  
  if(nevents_to_transfer>nevents)
    nevents_to_transfer = nevents;
    
    
// system time before
  auto end_header_time = chrono::high_resolution_clock::now(); 
  float time_header_time = chrono::duration_cast<chrono::nanoseconds>(end_header_time - start_header_time).count() * 1e-9; 
  DEBUG("Time taken by header is : " << fixed << time_header_time << setprecision(5) << " sec ");
    

// system time before
  auto start_read_time = chrono::high_resolution_clock::now(); 

  nwords = m_digitizer->ReadFullBufferSpaceSaver(raw_payload, software_buffer, monitoring, nevents, nchannels_enabled, buffer_size, readout_method, nevents_to_transfer);

// system time before
  auto end_read_time = chrono::high_resolution_clock::now(); 
  float time_read_time = chrono::duration_cast<chrono::nanoseconds>(end_read_time - start_read_time).count() * 1e-9; 
  DEBUG("Time taken by buffer_readout is : " << fixed << time_read_time << setprecision(5) << " sec ");
  
  
  // system time before
  auto start_filler_time = chrono::high_resolution_clock::now(); 

  DEBUG("Buffer has been read");
  
  DEBUG("Monitoring size : "<<monitoring.size());
  std::map<std::string, float>::iterator it;
  for (it = monitoring.begin(); it != monitoring.end(); it++) {
    DEBUG(" - "<<it->first << "   :   " << it->second);
  }
  
  int nevents_after     = m_digitizer->DumpEventCount();
  
  DEBUG(">>>>>>> NEv Before      : "<<std::dec<<nevents);
  DEBUG(">>>>>>> NEv After       : "<<std::dec<<nevents_after);
  DEBUG(">>>>>>> NEv Requested   : "<<std::dec<<nevents_to_transfer);
  
  int nwords_expected = nevents_to_transfer *  event_size;
  
  // these don't necessarily have to line up 
  // perhaps an event appeared just before reading
  // trust the words you obtain and divide by evnet size to know the number
  // of events you will have to parse
  DEBUG("Words expected : "<<nwords_expected);
  DEBUG("Words obtained : "<<nwords);
  
  int nevents_obtained = nwords/event_size;
  
  // ToDo : implement check to make sure it divides
   
  uint32_t single_event_raw_payload[10000];
  int eventLocation=0;
  
  // system time before
  auto end_filler_time = chrono::high_resolution_clock::now(); 
  float time_filler_time = chrono::duration_cast<chrono::nanoseconds>(end_filler_time - start_filler_time).count() * 1e-9; 
  DEBUG("Time taken by buffer_fillerout is : " << fixed << time_filler_time << setprecision(5) << " sec ");
  
  // system time before
  auto start_parse_time = chrono::high_resolution_clock::now(); 
  
  DEBUG("Sending NEvents : "<<nevents_obtained);
  
  // initialize variables only once
  unsigned int Header_TriggerTimeTag = 0;
  
  uint8_t  local_fragment_tag = 0;
  uint32_t local_source_id    = 0;
  uint64_t local_event_id     = 0;
  uint16_t local_bc_id        = 0;
  uint16_t status             = 0;

  int payload_size = 0;
  
  unsigned int Header_EventCounter = 0;
  
  int total_size = 0;

  // perform transfer for every event that was read out in the buffer
  for(int iev=0; iev<nevents_obtained; iev++){
  
    //DEBUG("Moving event : "<<iev<<"  "<<eventLocation<<"  "<<event_size);
  
    // saves one event starting in eventLocation to single_event_raw_payload
    //DEBUG("Retrieve event : location="<<eventLocation<<"  size="<<event_size);
    eventLocation = getSingleEvent(raw_payload, single_event_raw_payload, eventLocation, event_size);
  
  
    if(debug){
      // to check the eventCounter to be sure
      DEBUG("header[0] : "<<ConvertIntToWord(single_event_raw_payload[0]));
      DEBUG("header[1] : "<<ConvertIntToWord(single_event_raw_payload[1]));
      DEBUG("header[2] : "<<ConvertIntToWord(single_event_raw_payload[2]));
      DEBUG("header[3] : "<<ConvertIntToWord(single_event_raw_payload[3]));
    
      int nsampwords = ((event_size-4)/nchannels_enabled);
      int print_chan = 6;
    
      //DEBUG("NData : "<<nsampwords);
    
      //xint idat = 50;
      //DEBUG("data["<<idat  <<"]   : "<<std::dec<< ((single_event_raw_payload[4+(nsampwords*print_chan)+idat]>>16) & 0x0000FFFF) );
      //DEBUG("data["<<idat+1<<"]   : "<<std::dec<< ((single_event_raw_payload[4+(nsampwords*print_chan)+idat] & 0xFFFF0000)>>16) );


    
      for(int idat=0; idat<nsampwords; idat++){
        DEBUG("data["<<(idat*2)  <<"]   : "<<std::dec<< ((single_event_raw_payload[4+(nsampwords*print_chan)+idat])& 0x0000FFFF) );
        DEBUG("data["<<(idat*2)+1<<"]   : "<<std::dec<< ((single_event_raw_payload[4+(nsampwords*print_chan)+idat] & 0xFFFF0000)>>16) );
      }
    }
    
    // do the following for each of the events that has been parsed
    // creating the payload
    //int payload_size = Payload_GetEventSize( single_event_raw_payload );
    payload_size = raw_payload[0] & 0x0FFFFFF;
    total_size = sizeof(uint32_t) * payload_size;  // size of my payload in bytes

    //DEBUG("PayloadSize : nwords="<<payload_size<<"  total_size="<<total_size);

    // the event ID is what should be used, in conjunction with the ECR to give a unique event tag
    // word[2] bits[23:0]
    // need to blank out the top bits because these are the channel masks
    Header_EventCounter   = (single_event_raw_payload[2] & 0xFF000000);
    //SetBit(Header_EventCounter, 31, 0); 
    //SetBit(Header_EventCounter, 30, 0);
    //SetBit(Header_EventCounter, 29, 0);
    //SetBit(Header_EventCounter, 28, 0);
    //SetBit(Header_EventCounter, 27, 0);
    //SetBit(Header_EventCounter, 26, 0);
    //SetBit(Header_EventCounter, 25, 0);
    //SetBit(Header_EventCounter, 24, 0);
  
    // the trigger time tag is used to give a "verification" of the event ID if that fails
    // it is nominally the LHC BCID but we need to do a conversion from our clock to the LHC clock
    // word[3] bits[31:0]
    Header_TriggerTimeTag = single_event_raw_payload[3];
  
    //DEBUG("Header_EventCounter   : "+to_string(Header_EventCounter));
    //DEBUG("Header_TriggerTimeTag : "+to_string(Header_TriggerTimeTag));
  
    // store the faser header information
    local_fragment_tag = EventTags::PhysicsTag;
    local_source_id    = SourceIDs::PMTSourceID;
    local_event_id     = (m_ECRcount<<24) + (Header_EventCounter+1); // from the header and the ECR from sendECR() counting m_ECRcount [ECR]+[EID]
    local_bc_id        = Header_TriggerTimeTag*(m_ttt_converter/125);      // trigger time tag corrected by LHCClock/TrigClock = m_ttt_converter/125, where m_ttt_converter is configurable but by default is 40.08

 
    // create the event fragment
    std::unique_ptr<EventFragment> fragment(new EventFragment(local_fragment_tag, local_source_id, local_event_id, local_bc_id, single_event_raw_payload, total_size ));

    // ToDo : What is the status supposed to be?
    status=0;
    fragment->set_status( status );

    // place the raw binary event fragment on the output port
    std::unique_ptr<const byteVector> bytestream(fragment->raw());
    daqling::utilities::Binary binData(bytestream->data(),bytestream->size());

    // only necessary for the real module
    m_connections.put(0, binData); 
  
  
  } 
  
  // system time after
  auto end_parse_time = chrono::high_resolution_clock::now(); 
  
  float time_parse_time = chrono::duration_cast<chrono::nanoseconds>(end_parse_time - start_parse_time).count() * 1e-9; 
  DEBUG("Time taken by event parsing is : " << fixed << time_parse_time << setprecision(5) << " sec ");
    
  monitoring["time_header_time"]   = time_header_time;  
  monitoring["time_filler_time"]   = time_filler_time;  
  monitoring["time_read_time"]     = time_read_time;
  monitoring["time_parse_time"]    = time_parse_time;

}



int DigitizerReceiverModule::getSingleEvent( uint32_t raw_payload[], uint32_t single_event_raw_payload[], int eventLocation, int eventSize){
  //DEBUG("getSingleEvent : "<<eventLocation<<"  "<<eventSize);
  std::copy(raw_payload+eventLocation, raw_payload+eventLocation+eventSize, single_event_raw_payload);
  return eventLocation+eventSize;
}
