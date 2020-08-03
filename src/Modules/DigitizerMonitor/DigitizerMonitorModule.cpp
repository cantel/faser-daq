/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "DigitizerMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

#define NCHANNELS 16

DigitizerMonitorModule::DigitizerMonitorModule() { 
  INFO("Instantiating ...");
}

DigitizerMonitorModule::~DigitizerMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
}

void DigitizerMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {
  INFO("Digitizer monitoring");

  // the m_event object is populated with the event binary here
  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  // consistency check that the event received is the type of data we are configured to take
  if ( m_event->event_tag() != m_eventTag ) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    ERROR("Error in unpacking");
    fill_error_status_to_metric( fragmentUnpackStatus );
    fill_error_status_to_histogram( fragmentUnpackStatus, "h_digitizer_errorcount" );
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  DEBUG("EventSize : "<<m_pmtdataFragment->event_size());

  // number of errors into histogram
  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  fill_error_status_to_histogram( fragmentStatus, "h_digitizer_errorcount" );

  // size of fragment payload
  uint16_t payloadSize = m_fragment->payload_size(); 
  m_histogrammanager->fill("h_digitizer_payloadsize", payloadSize);
  m_metric_payload = payloadSize;

  // anything worth doing to all channels
  for(int iChan=0; iChan<NCHANNELS; iChan++){
  
    // example pulse
    FillChannelPulse("h_pulse_ch"+std::to_string(iChan), iChan);
    
    // mean and rms for monitoring a channel that goes out of wack
    float avg = GetPedestalMean(m_pmtdataFragment->channel_adc_counts(iChan), 0, 100);
    float rms = GetPedestalRMS(m_pmtdataFragment->channel_adc_counts(iChan), 0, 100);

    m_histogrammanager->fill("h_avg_ch"+std::to_string(iChan), avg);
    m_histogrammanager->fill("h_rms_ch"+std::to_string(iChan), rms);    
  }
}

void DigitizerMonitorModule::register_hists() {
  INFO(" ... registering histograms in DigitizerMonitor ... " );
  
  // payload size
  m_histogrammanager->registerHistogram("h_digitizer_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);

  // payload status
  std::vector<std::string> categories = {"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"};
  m_histogrammanager->registerHistogram("h_digitizer_errorcount", "error type", categories, 5. );

  // synthesis common for all channels
  int buffer_length = (int)m_config.getConfig()["settings"]["buffer_length"];
  for(int iChan=0; iChan<NCHANNELS; iChan++){
    m_histogrammanager->registerHistogram("h_pulse_ch"+std::to_string(iChan), "ADC Pulse ch"+std::to_string(iChan), 0, buffer_length, buffer_length);
    m_histogrammanager->registerHistogram("h_avg_ch"+std::to_string(iChan),"Average of Baseline Start : ch"+std::to_string(iChan),0,20000,200);
    m_histogrammanager->registerHistogram("h_rms_ch"+std::to_string(iChan),"RMS of Baseline Start : ch"+std::to_string(iChan),0,1000,100);
  }

  INFO(" ... done registering histograms ... " );
  return;
}

void DigitizerMonitorModule::register_metrics() {
  INFO( "... registering metrics in DigitizerMonitorModule ... " );

  register_error_metrics();

  m_metric_payload = 0;
  m_statistics->registerMetric(&m_metric_payload, "payload", daqling::core::metrics::LAST_VALUE);

  return;
}

float DigitizerMonitorModule::GetPedestalMean(std::vector<uint16_t> input, int start, int end){

  CheckBounds(input, start, end);

  float sum=0.0;
  float count=0;

  for(int i=start; i<end; i++){
    sum += input.at(i);
    count++;
  }

  if(count<=0)
    return -1;

  return sum/count;
}

float DigitizerMonitorModule::GetPedestalRMS(std::vector<uint16_t> input, int start, int end){

  CheckBounds(input, start, end);

  float mean = GetPedestalMean(input, start, end);

  float sum_rms = 0;
  int count=0;

  for(int i=start; i<end; i++){
    sum_rms += pow(input.at(i)-mean , 2.0);
    count++;
  }

  if(count<=0)
    return -1;

  return pow(sum_rms/count, 0.5);
}

void CheckBounds(std::vector<uint16_t> input, int& start, int& end){
  // if the number of samples is less than the desired pedestal sampling length, then take the full duration
  if(start>=end){
    WARNING("Pedestal calculation has a start after and end : ["<<start<<","<<end<<"]");
    end = start;
  }
  
  // if the number of samples is less than the desired pedestal sampling length, then take the full duration
  if(end>input.size()){
    WARNING("The pulse length is not long enough for a pedestal calculation of : ["<<start<<","<<end<<"]");
    end = input.size();
  }
}

void DigitizerMonitorModule::FillChannelPulse(std::string histogram_name, int channel){
  DEBUG("Filling pulse for : "<<histogram_name<<"  "<<channel);

  m_histogrammanager->reset(histogram_name);
  for(int isamp=0; isamp<m_pmtdataFragment->channel_adc_counts(channel).size(); isamp++){
    m_histogrammanager->fill(histogram_name,isamp,m_pmtdataFragment->channel_adc_counts(channel).at(isamp));
  }
}
