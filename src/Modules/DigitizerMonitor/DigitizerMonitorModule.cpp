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

  auto tlb = get_tlb_data_fragment(eventBuilderBinary);

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    ERROR("Error in unpacking");
    return;
  }

  if (!m_pmtdataFragment->valid()) {
    ERROR("Invalid PMT fragment. Skipping event!");
    return;
  }

  DEBUG("EventSize : "<<m_pmtdataFragment->event_size());

  // size of fragment payload
  uint16_t payloadSize = m_fragment->payload_size(); 
  m_histogrammanager->fill("h_digitizer_payloadsize", payloadSize);
  m_metric_payload = payloadSize;

  float peaks[NCHANNELS];
  float tzeros[NCHANNELS];
  std::vector<float> signals[NCHANNELS];
  float scale[]={2.,2.,2.,2., //FIXME: should  read from configuration
		 2.,2.,2.,2.,
		 2.,2.,2.,2.,
		 2.,2.,2.,2.};
	       
  // anything worth doing to all channels
  bool saturated=false;
  for(int iChan=0; iChan<NCHANNELS; iChan++){
    if (!m_pmtdataFragment->channel_has_data(iChan)) continue;
    std::string chStr = std::to_string(iChan);
    if (iChan<10) chStr = "0"+chStr;

    auto v = m_pmtdataFragment->channel_adc_counts(iChan);


    // mean and rms for monitoring a channel that goes out of wack
    float avg = GetPedestalMean(v, 0, 50);
    float rms = GetPedestalRMS(v, 0, 50);
    
    m_avg[iChan]=avg;
    if (m_rms[iChan]==0) m_rms[iChan]=rms; //initialize on first event
    m_rms[iChan]=0.02*rms+0.98*m_rms[iChan]; //exponential moving average

    //convert to mV and find peaks
    std::vector<float>& signal=signals[iChan];
    signal.resize(v.size());
    float min_value=0;
    float max_value=0;
    size_t max_idx=0;
    for(size_t ii=0;ii<v.size();ii++) {
      if (v[ii]<10 && iChan!=15) saturated=true;
      float value=(avg-v[ii])*scale[iChan]/16.384;
      if (value>max_value) {
	max_value=value;
	max_idx=ii;
      }
      if (value<min_value) min_value=value;
      signal[ii]=value;
    }
    //find t0 
    const int delta=3;
    const float k=0.3;
    max_idx=std::min(max_idx,v.size()-delta-2); //protect against overflow
    max_idx+=delta;
    while((k*signal[max_idx]-signal[max_idx-delta]<0)&&max_idx!=delta) max_idx--;
    double y1=k*signal[max_idx]-signal[max_idx-delta];
    double y2=k*signal[max_idx+1]-signal[max_idx-delta+1];
    double dt=y1/(y1-y2);
    double t0=2.*(max_idx+dt-v.size()+300);
    tzeros[iChan]=t0;
    // example pulse
    if ((-min_value)>m_display_thresh||(max_value>m_display_thresh)) {
      FillChannelPulse("h_pulse_ch"+chStr, signal);
    }
    float peak=max_value;
    peaks[iChan]=peak;
    if ((-min_value)>peak) peak=min_value;

    m_histogrammanager->fill("h_peak_ch"+chStr, peak, 1.0); 
    for(int ii=0;ii<THRESHOLDS;ii++) {
      if (m_thresholds[iChan][ii] && peak>m_thresholds[iChan][ii])  
	m_thresh_counts[iChan][ii]++;
    }
  }
  //select collision events without saturated channels
  if (peaks[6]>100 && peaks[7]>100&& ((peaks[8]>25&&peaks[9]>25)||(peaks[10]>25&&peaks[11]>25))&&!saturated) {
    if (m_pmtdataFragment->channel_has_data(15)) { //assume that clock data is here
      float phase=FFTPhase(signals[15]);
      if (phase<-5) phase+=25;
      m_histogrammanager->fill("h_clockphase", phase);

      for(int iChan=0; iChan<NCHANNELS; iChan++){
	if (iChan==15) continue;
	if (peaks[iChan]<5) continue; // Need a minimum signal

	std::string chStr = std::to_string(iChan);
	if (iChan<10) chStr = "0"+chStr;

	m_histogrammanager->fill("h_time_ch"+chStr,tzeros[iChan]-phase);
	if (m_t0[iChan]==0) m_t0[iChan]=tzeros[iChan]; //initialize on first event
	m_t0[iChan]=0.02*tzeros[iChan]+0.98*m_t0[iChan]; //exponential moving average
      }
    }
    auto inputBitsNext = tlb.input_bits_next_clk();
    for(int inputBit=0; inputBit<8;inputBit++) {
      if ((inputBitsNext&(1<<inputBit))!=0) {
	//FIXME: hardcoded combinations
	float missedSignal=0;
	if (inputBit<2) missedSignal=std::max(peaks[inputBit*2],peaks[inputBit*2+1]);	
	else if (inputBit>6) missedSignal=peaks[inputBit*2];
	else missedSignal=std::min(peaks[inputBit*2],peaks[inputBit*2+1]);
	m_outtime[inputBit]++;
	m_histogrammanager->fill("h_late_bit"+std::to_string(inputBit),missedSignal);
	std::cout<<inputBit<<" "<<m_outtime[inputBit]<<" "<<m_intime[inputBit]<<std::endl;
      }
    }
    auto inputBits = tlb.input_bits();
    for(int inputBit=0; inputBit<8;inputBit++) {
      if ((inputBits&(1<<inputBit))!=0) {
	m_intime[inputBit]++;
	m_late[inputBit]=1.*m_outtime[inputBit]/(m_outtime[inputBit]+m_intime[inputBit]);
      }
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
    m_histogrammanager->registerHistogram("h_pulse_ch"+chStr, "ADC Pulse ch"+std::to_string(iChan)+" Sample Number", "Inverted signal [mV]", -0.5, buffer_length-0.5, buffer_length, m_PUBINT);
    m_histogrammanager->registerHistogram("h_peak_ch"+chStr, "Peak signal [mV]", -200, 2000, 550, m_PUBINT);
    if (iChan==15) continue;
    m_histogrammanager->registerHistogram("h_time_ch"+chStr, "Peak timing [ns]", 150, 300, 300, m_PUBINT);

  }
  for(int inputBit=0; inputBit<8;inputBit++) {
    m_histogrammanager->registerHistogram("h_late_bit"+std::to_string(inputBit), "Peak signal for late triggers [mV]", 0, 500, 250, m_PUBINT);

  }
  m_histogrammanager->registerHistogram("h_clockphase", "Clock phase [ns]", -50, 50, 500, m_PUBINT);

  
  INFO(" ... done registering histograms ... " );
  return;
}

void DigitizerMonitorModule::register_metrics() {
  INFO( "... registering metrics in DigitizerMonitorModule ... " );

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
    if (iChan!=15) 
      registerVariable(m_t0[iChan], "signal_timing_ch"+chStr);
  }
  for(int inputBit=0;inputBit<8;inputBit++) {
    registerVariable(m_late[inputBit],"Late_trigger_fraction_bit"+std::to_string(inputBit));
    m_intime[inputBit]=0; //FIXME - doesn't get reset on run stop/start
    m_outtime[inputBit]=0; //FIXME - doesn't get reset on run stop/start
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

float DigitizerMonitorModule::FFTPhase(const std::vector<float>& data) {
  int N=data.size();
  float sumRe=0;
  float sumIm=0;
  int k=int(40.08/500.*N); //clock/digitizer frequency
  for(int ii=0;ii<N;ii++) {
    sumRe+=data[ii]*cos(2*M_PI*ii*k/N);
    sumIm+=data[ii]*sin(2*M_PI*ii*k/N);
  }
  return 25*atan2(sumIm,sumRe)/2/M_PI;
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

void DigitizerMonitorModule::FillChannelPulse(std::string histogram_name, std::vector<float>& values){
  m_histogrammanager->reset(histogram_name);
  m_histogrammanager->fill(histogram_name,0,1,values);
}
