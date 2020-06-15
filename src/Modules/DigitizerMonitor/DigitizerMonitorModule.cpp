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

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_event->event_tag() != m_eventTag ) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    fill_error_status_to_histogram( fragmentUnpackStatus, "h_digitizer_errorcount" );
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  fill_error_status_to_histogram( fragmentStatus, "h_digitizer_errorcount" );

  uint16_t payloadSize = m_fragment->payload_size(); 

  m_histogrammanager->fill("h_digitizer_payloadsize", payloadSize);
  m_metric_payload = payloadSize;

  // 2D hist fill
  m_histogrammanager->fill("h_digitizer_numfrag_vs_sizefrag", m_monitoringFragment->num_fragments_sent/1000., m_monitoringFragment->size_fragments_sent/1000.);
}

void DigitizerMonitorModule::register_hists() {

  INFO(" ... registering histograms in DigitizerMonitor ... " );
  
  m_histogrammanager->registerHistogram("h_digitizer_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);
  std::vector<std::string> categories = {"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"};
  m_histogrammanager->registerHistogram("h_digitizer_errorcount", "error type", categories, 5. );
  // example 2D hist
  m_histogrammanager->register2DHistogram("h_Digitizer_numfrag_vs_sizefrag", "no. of sent fragments", -0.5, 30.5, 31, "size of sent fragments [kB]", -0.5, 9.5, 20 );

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
