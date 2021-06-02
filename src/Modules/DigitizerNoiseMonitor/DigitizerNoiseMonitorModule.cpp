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

#include "DigitizerNoiseMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

// only monitoring the first two channels
#define NCHANNELS 15

DigitizerNoiseMonitorModule::DigitizerNoiseMonitorModule() { 
  INFO("Instantiating ...");
}

DigitizerNoiseMonitorModule::~DigitizerNoiseMonitorModule() { 
}

void DigitizerNoiseMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {
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
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  DEBUG("EventSize : "<<m_pmtdataFragment->event_size());

  // number of errors into histogram
  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );

  // size of fragment payload
  uint16_t payloadSize = m_fragment->payload_size(); 
  m_metric_payload = payloadSize;

  // anything worth doing to all channels
  for(int iChan=0; iChan<NCHANNELS; iChan++){
  
    float avg,rms;
    GetPedestalMeanRMS(m_pmtdataFragment->channel_adc_counts(iChan),avg,rms);
    m_metric_pedestal[iChan]=avg;
    m_metric_rms[iChan]=rms;
    for(int ii=0;ii<4;ii++) {
      double counts=m_metric_peaks[ii][iChan]+CountPeaks(m_pmtdataFragment->channel_adc_counts(iChan),avg,8+8*ii);
      m_metric_peaks[ii][iChan]=counts;
    }
  }
  
}

void DigitizerNoiseMonitorModule::register_hists() {
  INFO(" ... registering histograms in DigitizerNoise ... " );
  
  INFO(" ... done registering histograms ... " );
  return;
}

void DigitizerNoiseMonitorModule::register_metrics() {
  INFO( "... registering metrics in DigitizerNoiseMonitorModule ... " );

  register_error_metrics();

  m_metric_payload = 0;
  m_statistics->registerMetric(&m_metric_payload, "payload", daqling::core::metrics::LAST_VALUE);
  for(int iChan=0; iChan<NCHANNELS; iChan++){
    m_metric_pedestal[iChan]=0;
    m_metric_rms[iChan]=0;
    m_metric_peaks[0][iChan]=0;
    m_metric_peaks[1][iChan]=0;
    m_metric_peaks[2][iChan]=0;
    m_metric_peaks[3][iChan]=0;
    m_statistics->registerMetric(&m_metric_pedestal[iChan], "pedestal"+std::to_string(iChan), daqling::core::metrics::LAST_VALUE);
    m_statistics->registerMetric(&m_metric_rms[iChan], "rms"+std::to_string(iChan), daqling::core::metrics::LAST_VALUE);
    m_statistics->registerMetric(&m_metric_peaks[0][iChan], "peaks1ch"+std::to_string(iChan), daqling::core::metrics::RATE);
    m_statistics->registerMetric(&m_metric_peaks[1][iChan], "peaks2ch"+std::to_string(iChan), daqling::core::metrics::RATE);
    m_statistics->registerMetric(&m_metric_peaks[2][iChan], "peaks3ch"+std::to_string(iChan), daqling::core::metrics::RATE);
    m_statistics->registerMetric(&m_metric_peaks[3][iChan], "peaks4ch"+std::to_string(iChan), daqling::core::metrics::RATE);
    m_statistics->registerMetric(&m_metric_peaks[0][iChan], "countpeaks1ch"+std::to_string(iChan), daqling::core::metrics::LAST_VALUE);
    m_statistics->registerMetric(&m_metric_peaks[1][iChan], "countpeaks2ch"+std::to_string(iChan), daqling::core::metrics::LAST_VALUE);
    m_statistics->registerMetric(&m_metric_peaks[2][iChan], "countpeaks3ch"+std::to_string(iChan), daqling::core::metrics::LAST_VALUE);
    m_statistics->registerMetric(&m_metric_peaks[3][iChan], "countpeaks4ch"+std::to_string(iChan), daqling::core::metrics::LAST_VALUE);
  }
  return;
}

void DigitizerNoiseMonitorModule::GetPedestalMeanRMS(std::vector<uint16_t> input, float &mean, float &rms){
  int start=0;
  int end=input.size();
  float sum=0.0;
  float sumsq=0.0;
  float count=0;

  for(int i=start; i<end; i++){
    int adc = input.at(i);
    sum += adc;
    sumsq +=adc*adc;
    count++;
  }

  mean = sum/count;
  rms = sqrt(sumsq/count-mean*mean);
}

int DigitizerNoiseMonitorModule::CountPeaks(std::vector<uint16_t> input, float mean, float threshold){
  int start=0;
  int end=input.size();
  bool above=false;
  int count=0;

  for(int i=start; i<end; i++){
    int adc = input.at(i);
    if (adc>mean+threshold) {
      if (!above) count++;
      above=true;
    } else {
      above=false;
    }
  }

  return count;
  
}

