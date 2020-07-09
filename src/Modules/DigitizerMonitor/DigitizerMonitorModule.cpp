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

DigitizerMonitorModule::DigitizerMonitorModule() { 
  INFO("Instantiating ...");
}

DigitizerMonitorModule::~DigitizerMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
}

void DigitizerMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  INFO("InternalMonitor");

  // the m_event object is populated with the event binary here
  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  // consistency check that the event received is the type of data we are configured to take
  if ( m_event->event_tag() != m_eventTag ) {
    DEBUG("Event tag does not match filter tag. Are the module's filter settings correct?");
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    DEBUG("Error in unpacking");
    fill_error_status_to_metric( fragmentUnpackStatus );
    fill_error_status_to_histogram( fragmentUnpackStatus, "h_digitizer_errorcount" );
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  DEBUG("EventSize from Claire : "<<m_pmtdataFragment->event_size());

  // example 1 - number of errors into histogram
  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  fill_error_status_to_histogram( fragmentStatus, "h_digitizer_errorcount" );

  // example 2 - size of fragment payload
  uint16_t payloadSize = m_fragment->payload_size(); 
  m_histogrammanager->fill("h_digitizer_payloadsize", payloadSize);
  m_metric_payload = payloadSize;


  // unpack and get full pulse shape from channel 0 into histogram
  INFO("NSamp(0) : "<<m_pmtdataFragment->channel_adc_counts(0).size());

  /*
  // need to be able to clear a histogram to output the pulse
  for(int isamp=0; isamp<m_pmtdataFragment->channel_adc_counts(0).size(); isamp++){
    m_histogrammanager->fill("h_pulse_chan0",isamp,m_pmtdataFragment->channel_adc_counts(0).at(isamp));
  }
  */

  float mean = GetMean(m_pmtdataFragment->channel_adc_counts(0), 0, 100);
  float rms  = GetRMS(m_pmtdataFragment->channel_adc_counts(0), 0, 100);

  m_histogrammanager->fill("mean_chan0", mean);
  m_histogrammanager->fill("rms_chan0", rms);


  // example 3 - 2D hist fill
  //m_histogrammanager->fill("h_digitizer_numfrag_vs_sizefrag", m_pmtdataFragment->num_fragments_sent/1000., m_monitoringFragment->size_fragments_sent/1000.);
  
}

void DigitizerMonitorModule::register_hists() {

  INFO(" ... registering histograms in DigitizerMonitor ... " );
  
  m_histogrammanager->registerHistogram("h_digitizer_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);

  std::vector<std::string> categories = {"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"};
  m_histogrammanager->registerHistogram("h_digitizer_errorcount", "error type", categories, 5. );


  // print out raw pulse for channel 0
  m_histogrammanager->registerHistogram("h_pulse_chan0","channel pulse [0]",0,1000,1000);

  // mean and rms of first 100 samples
  m_histogrammanager->registerHistogram("mean_chan0","mean_chan0",0,20000,200);
  m_histogrammanager->registerHistogram("rms_chan0","rms_chan0",0,1000,100);

  // example 2D hist
  //m_histogrammanager->register2DHistogram("h_Digitizer_numfrag_vs_sizefrag", "no. of sent fragments", -0.5, 30.5, 31, "size of sent fragments [kB]", -0.5, 9.5, 20 );

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

float GetMean(std::vector<uint16_t> input, int start, int end){

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

float GetRMS(std::vector<uint16_t> input, int start, int end){


  float mean = GetMean(input, start, end);

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
