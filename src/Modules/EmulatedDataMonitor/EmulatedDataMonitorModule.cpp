/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "EmulatedDataMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

EmulatedDataMonitorModule::EmulatedDataMonitorModule() { 

   INFO("");
 }

EmulatedDataMonitorModule::~EmulatedDataMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
 }

void EmulatedDataMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_event->event_tag() != m_eventTag ) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    WARNING("Unpacking fragment failed. Skipping event.");
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  
  uint16_t payloadSize = m_fragment->payload_size(); 

  // 1D histogram fill
  m_histogrammanager->fill("payloadsize", payloadSize);
  m_metric_payload = payloadSize;

  // 2D hist fill
  DEBUG("m_monitoringFragment->num_fragments_sent/1000. = "<<m_monitoringFragment->num_fragments_sent/1000.);
  DEBUG("m_monitoringFragment->size_fragments_sent/1000. = "<<m_monitoringFragment->size_fragments_sent/1000.);
  m_histogrammanager->fill2D("numfrag_vs_sizefrag", m_monitoringFragment->num_fragments_sent/1000., m_monitoringFragment->size_fragments_sent/1000.);

}

void EmulatedDataMonitorModule::register_hists() {

  INFO(" ... registering histograms in EmulatedDataMonitor ... " );

  // example of 1D histogram: default is ylabel="counts" non-extendable axes (Axis::Range::NONEXTENDABLE & 60 second publishing interval.
  //m_histogrammanager->registerHistogram("h_tracker_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);
  // example of 1D histogram with extendable x-axis, publishing interval of every 30 seconds.
  m_histogrammanager->registerHistogram("payloadsize", "payload size [bytes]", "event count/2kB", -0.5, 349.5, 175, Axis::Range::EXTENDABLE, 30);

  // example 2D hist
  m_histogrammanager->register2DHistogram("numfrag_vs_sizefrag", "no. of sent fragments", -0.5, 30.5, 31, "size of sent fragments [kB]", -0.5, 9.5, 20, 30 );

  INFO(" ... done registering histograms ... " );

  return ;

}

void EmulatedDataMonitorModule::register_metrics() {

  INFO( "... registering metrics in EmulatedDataMonitorModule ... " );

  return;
}
