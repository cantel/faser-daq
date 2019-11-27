/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "TLBMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

TLBMonitorModule::TLBMonitorModule() { 

   INFO("");
 }

TLBMonitorModule::~TLBMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TLBMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;
 
  if ( m_event->event_tag() != MonitoringTag ) return; //redundant check if have configured pub/sub filter (daqling v0.6+)
                                                       //Forcing here to guard against incorrect configuration
                                                       //as calling MonitoringFragment values for 2D hist.

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    fill_error_status_to_histogram( fragmentUnpackStatus, "h_tlb_errorcount" );
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  fill_error_status_to_histogram( fragmentStatus, "h_tlb_errorcount" );

  uint16_t payloadSize = m_fragment->payload_size(); 

  m_histogrammanager->fill("h_tlb_payloadsize", payloadSize);
  m_metric_payload = payloadSize;

  // 2D hist fill
  m_histogrammanager->fill("h_tlb_numfrag_vs_sizefrag", m_monitoringFragment->num_fragments_sent/1000., m_monitoringFragment->size_fragments_sent/1000.);
}

void TLBMonitorModule::register_hists() {

  INFO(" ... registering histograms in TLBMonitor ... " );
  
  m_histogrammanager->registerHistogram("h_tlb_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);
  std::vector<std::string> categories = {"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"};
  m_histogrammanager->registerHistogram("h_tlb_errorcount", "error type", categories, 5. );
  // example 2D hist
  m_histogrammanager->register2DHistogram("h_tlb_numfrag_vs_sizefrag", "no. of sent fragments", -0.5, 30.5, 31, "size of sent fragments [kB]", -0.5, 9.5, 20 );

  INFO(" ... done registering histograms ... " );
  return;

}

void TLBMonitorModule::register_metrics() {

  INFO( "... registering metrics in TLBMonitorModule ... " );

  std::string module_short_name = "tlb";
 
  register_error_metrics(module_short_name);

  m_metric_payload = 0;
  m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_payload, module_short_name+"_payload", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

  return;
}
