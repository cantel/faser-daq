/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
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

DigitizerMonitorModule::DigitizerMonitorModule(const std::string& n):MonitorBaseModule(n) { 
  INFO("Instantiating ...");
}

DigitizerMonitorModule::~DigitizerMonitorModule() { 
  INFO("With config: " << m_config.dump());
}

void DigitizerMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {
  DEBUG("Digitizer monitoring");

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
    if (!m_pmtdataFragment->channel_has_data(iChan)) continue;
    std::string chStr = std::to_string(iChan);
    if (iChan<10) chStr = "0"+chStr;

    // mean and rms for monitoring a channel that goes out of wack
    float avg = GetPedestalMean(m_pmtdataFragment->channel_adc_counts(iChan), 0, 100);
    float rms = GetPedestalRMS(m_pmtdataFragment->channel_adc_counts(iChan), 0, 100);
    
    m_avg[iChan]=avg;
    if (m_rms[iChan]==0) m_rms[iChan]=rms; //initialize on first event
    m_rms[iChan]=0.02*rms+0.98*m_rms[iChan]; //exponential moving average

    // example pulse
    auto v = m_pmtdataFragment->channel_adc_counts(iChan);
    float min_value = *std::min_element(v.begin(),v.end());
    float max_value = *std::max_element(v.begin(),v.end());
    if ((avg-min_value)>m_display_thresh||(max_value-avg)>m_display_thresh) {
      FillChannelPulse("h_pulse_ch"+chStr, iChan);
    }
    float peak=avg-min_value;
    if ((max_value-avg)>peak) peak=avg-max_value;
    m_histogrammanager->fill("h_peak_ch"+chStr, peak/8.192, 1.0); //FIXME: assume 2V range
    for(int ii=0;ii<THRESHOLDS;ii++) {
      if (m_thresholds[iChan][ii] && peak/8.192>m_thresholds[iChan][ii])  //FIXME: assume 2V range
	m_thresh_counts[iChan][ii]++;
    }
  }
  
}

void DigitizerMonitorModule::register_hists() {
  INFO(" ... registering histograms in DigitizerMonitor ... " );
  
  m_display_thresh=(float)getModuleSettings()["display_thresh"];
  // payload size
  m_histogrammanager->registerHistogram("h_digitizer_payloadsize", "payload size [bytes]", -0.5, 545.5, 275, m_PUBINT);

  // payload status
  std::vector<std::string> categories = {"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"};
  m_histogrammanager->registerHistogram("h_digitizer_errorcount", "error type", categories, m_PUBINT );

  // synthesis common for all channels
  int buffer_length = (int)getModuleSettings()["buffer_length"];
  for(int iChan=0; iChan<NCHANNELS; iChan++){
    std::string chStr = std::to_string(iChan);
    if (iChan<10) chStr = "0"+chStr;
    // example pulse
    m_histogrammanager->registerHistogram("h_pulse_ch"+chStr, "ADC Pulse ch"+std::to_string(iChan)+" Sample Number", "ADC Counts", -0.5, buffer_length-0.5, buffer_length, m_PUBINT);
    m_histogrammanager->registerHistogram("h_peak_ch"+chStr, "Peak signal [mV]", -200, 2000, 110, m_PUBINT);
  
  }
  
  
  INFO(" ... done registering histograms ... " );
  return;
}

void DigitizerMonitorModule::register_metrics() {
  INFO( "... registering metrics in DigitizerMonitorModule ... " );

  register_error_metrics();

  m_metric_payload = 0;
  registerVariable(m_metric_payload,"payload");

  json thresholds = getModuleSettings()["rate_thresholds"];
  for(int iChan=0; iChan<NCHANNELS; iChan++){
    std::string chStr = std::to_string(iChan);
    if (iChan<10) chStr = "0"+chStr;

    json ch_thresh=thresholds[iChan];
    if (ch_thresh.size()>THRESHOLDS) {
      ERROR("Too many thresholds specified for channel "+chStr);
    }
    for(unsigned int ii=0;ii<THRESHOLDS;ii++) {
      m_thresholds[iChan][ii]=0;
      if (ii<ch_thresh.size()) {
	  float thresh=ch_thresh[ii];
	  if (thresh==0) continue;
	  registerVariable(m_thresh_counts[iChan][ii], "rate_ch"+chStr+"_"+std::to_string(thresh)+"mV",daqling::core::metrics::RATE);
	  m_thresholds[iChan][ii]=thresh;
	}
    }
    registerVariable(m_avg[iChan], "pedestal_mean_ch"+chStr);
    registerVariable(m_rms[iChan], "pedestal_rms_ch"+chStr);
  }

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

void DigitizerMonitorModule::CheckBounds(std::vector<uint16_t> input, int& start, int& end){
  // if the number of samples is less than the desired pedestal sampling length, then take the full duration
  if(start>=end){
    WARNING("Pedestal calculation has a start after and end : ["<<start<<","<<end<<"]");
    end = start;
  }
  
  // if the number of samples is less than the desired pedestal sampling length, then take the full duration
  if(end>(int)input.size()){
    WARNING("The pulse length is not long enough for a pedestal calculation of : ["<<start<<","<<end<<"]");
    end = input.size();
  }
}

void DigitizerMonitorModule::FillChannelPulse(std::string histogram_name, int channel){
  DEBUG("Filling pulse for : "<<histogram_name<<"  "<<channel);

  m_histogrammanager->reset(histogram_name);
  m_histogrammanager->fill(histogram_name,0,1,m_pmtdataFragment->channel_adc_counts(channel));
}
