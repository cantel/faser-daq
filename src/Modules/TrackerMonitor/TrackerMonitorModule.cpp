/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "TrackerMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

TrackerMonitorModule::TrackerMonitorModule() { 

   INFO("");
 }

TrackerMonitorModule::~TrackerMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void TrackerMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

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
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  
  uint16_t payloadSize = m_fragment->payload_size(); 

  m_histogrammanager->fill("h_tracker_payloadsize", payloadSize);
  m_metric_payload = payloadSize;

}

void TrackerMonitorModule::register_hists() {

  INFO(" ... registering histograms in TrackerMonitor ... " );

  // example of 1D histogram: default is ylabel="counts" non-extendable axes (Axis::Range::NONEXTENDABLE & 60 second publishing interval.
  //m_histogrammanager->registerHistogram("h_tracker_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);
  // example of 1D histogram with extendable x-axis, publishing interval of every 30 seconds.
  m_histogrammanager->registerHistogram("h_tracker_payloadsize", "payload size [bytes]", "event count/2kB", -0.5, 349.5, 175, Axis::Range::EXTENDABLE, 30);

  INFO(" ... done registering histograms ... " );

  return ;

}

void TrackerMonitorModule::register_metrics() {

  INFO( "... registering metrics in TrackerMonitorModule ... " );

  register_error_metrics();

  m_metric_payload = 0;
  m_statistics->registerMetric(&m_metric_payload, "payload", daqling::core::metrics::LAST_VALUE);

  return;
}
