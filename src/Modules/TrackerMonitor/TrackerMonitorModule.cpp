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

   auto cfg = m_config.getConfig()["settings"];
   m_sourceID = cfg["fragmentID"];

 }

TrackerMonitorModule::~TrackerMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TrackerMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary);
  if ( (fragmentUnpackStatus & CorruptedFragment) | (fragmentUnpackStatus & MissingFragment)){
    fill_error_status_to_metric( fragmentUnpackStatus );
    fill_error_status_to_histogram( fragmentUnpackStatus, "h_tracker_errorcount" );
    return;
  }

  // only accept physics events
  if ( m_eventHeader->event_tag != PhysicsTag ) return;

  uint32_t fragmentStatus = m_fragmentHeader->status;
  fill_error_status_to_metric( fragmentStatus );
  
  uint16_t payloadSize = m_fragmentHeader->payload_size; 

  m_histogrammanager->fill("h_tracker_payloadsize", payloadSize);
  std::cout<<"payload size is "<<payloadSize<<std::endl;
  m_metric_payload = payloadSize;

}

void TrackerMonitorModule::register_hists() {

  INFO(" ... registering histograms in TrackerMonitor ... " );

  m_histogrammanager->registerHistogram("h_tracker_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);

  INFO(" ... done registering histograms ... " );

  return ;

}

void TrackerMonitorModule::register_metrics() {

 INFO( "... registering metrics in TrackerMonitorModule ... " );

 if ( m_stats_on ) {
   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_payload, "tracker_payload", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_ok, "tracker_error_ok", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unclassified, "tracker_error_unclassified", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_bcidmismatch, "tracker_error_bcidmismatch", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_tagmismatch, "tracker_error_tagmismatch", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_timeout, "tracker_error_timeout", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_overflow, "tracker_error_overflow", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_corrupted, "tracker_error_corrupted", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_dummy, "tracker_error_dummy", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_missing, "tracker_error_missing", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_empty, "tracker_error_empty", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_duplicate, "tracker_error_duplicate", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

   m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unpack, "tracker_error_unpack", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);
 }

 return;
}
