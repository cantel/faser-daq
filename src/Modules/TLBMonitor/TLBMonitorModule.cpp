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

  m_metric_event_id = m_tlbmonitoringFragment->event_id();

 // veto counters
  m_deadtime_veto_counter += m_tlbmonitoringFragment->deadtime_veto_counter();
  m_busy_veto_counter += m_tlbmonitoringFragment->busy_veto_counter();
  m_rate_limiter_veto_counter += m_tlbmonitoringFragment->rate_limiter_veto_counter();
  m_bcr_veto_counter += m_tlbmonitoringFragment->bcr_veto_counter();

  DEBUG("m_bcr_veto_counter = "<<m_bcr_veto_counter);
  DEBUG("m_rate_limiter_veto_counter = "<<m_rate_limiter_veto_counter);
  DEBUG("m_metric_event_id = "<<m_metric_event_id);
  DEBUG("bc_id = "<<m_tlbmonitoringFragment->bc_id());

  m_histogrammanager->fill("h_tlb_veto_counts", "SimpleDeadtime", float(m_deadtime_veto_counter));
  m_histogrammanager->fill("h_tlb_veto_counts", "Busy", float(m_busy_veto_counter));
  m_histogrammanager->fill("h_tlb_veto_counts", "RateLimiter", float(m_rate_limiter_veto_counter));
  m_histogrammanager->fill("h_tlb_veto_counts", "BCR", float(m_bcr_veto_counter));

  // 2D hist fill
  //m_histogrammanager->fill("h_tlb_numfrag_vs_sizefrag", m_monitoringFragment->num_fragments_sent/1000., m_monitoringFragment->size_fragments_sent/1000.);
}

void TLBMonitorModule::register_hists() {

  INFO(" ... registering histograms in TLBMonitor ... " );
  
  m_histogrammanager->registerHistogram("h_tlb_payloadsize", "payload size [bytes]", -0.5, 545.5, 275, 3.);
  std::vector<std::string> categories = {"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"};
  m_histogrammanager->registerHistogram("h_tlb_errorcount", "error type", categories, 5. );

  std::vector<std::string> veto_categories = {"SimpleDeadtime", "Busy", "RateLimiter", "BCR"};
  m_histogrammanager->registerHistogram("h_tlb_veto_counts", "veto type", veto_categories, 5. );


  // example 2D hist
  //m_histogrammanager->register2DHistogram("h_tlb_numfrag_vs_sizefrag", "no. of sent fragments", -0.5, 30.5, 31, "size of sent fragments [kB]", -0.5, 9.5, 20, 5 );

  INFO(" ... done registering histograms ... " );
  return;

}

void TLBMonitorModule::register_metrics() {

  INFO( "... registering metrics in TLBMonitorModule ... " );

  register_error_metrics();

  m_metric_payload = 0;
  m_statistics->registerMetric(&m_metric_payload, "payload", daqling::core::metrics::LAST_VALUE);

  m_metric_event_id = 0;
  m_statistics->registerMetric(&m_metric_event_id, "eventID", daqling::core::metrics::LAST_VALUE);

  //veto counters
  m_deadtime_veto_counter = 0;
  m_statistics->registerMetric(&m_deadtime_veto_counter, "deadtimeVetoCounter", daqling::core::metrics::ACCUMULATE);
  m_busy_veto_counter = 0;
  m_statistics->registerMetric(&m_busy_veto_counter, "busyVetoCounter", daqling::core::metrics::ACCUMULATE);
  m_rate_limiter_veto_counter = 0;
  m_statistics->registerMetric(&m_rate_limiter_veto_counter, "ratelimiterVetoCounter", daqling::core::metrics::ACCUMULATE);
  m_bcr_veto_counter = 0;
  m_statistics->registerMetric(&m_bcr_veto_counter, "BCRVetoCounter", daqling::core::metrics::ACCUMULATE);


  return;
}
