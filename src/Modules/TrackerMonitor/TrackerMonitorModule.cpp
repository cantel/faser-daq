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

void TrackerMonitorModule::runner() {
  INFO("Running...");

  bool noData(true);
  daqling::utilities::Binary eventBuilderBinary;
  EventHeader * eventHeader = new EventHeader;
  EventFragmentHeader * fragmentHeader = new EventFragmentHeader;

  while (m_run) {

      if ( !m_connections.get(1, eventBuilderBinary)){
          if ( !noData ) std::this_thread::sleep_for(10ms);
          noData=true;
          continue;
      }
      noData=false;

      eventHeader = static_cast<EventHeader *>(eventBuilderBinary.data());	

      // only accept physics events
      if ( eventHeader->event_tag != PhysicsTag ) continue;

      uint16_t dataStatus = unpack_data(eventBuilderBinary, eventHeader, fragmentHeader);

      uint32_t fragmentStatus = fragmentHeader->status;
      fragmentStatus |= dataStatus;
      fill_error_status_to_metric( fragmentStatus );
      
      if (fragmentStatus & MissingFragment ) continue; // go no further

      uint16_t payloadSize = fragmentHeader->payload_size; 

      m_histogrammanager->fill("tracker_payloadsize", payloadSize);
      m_metric_payload = payloadSize;
  }

  delete eventHeader;
  delete fragmentHeader;

  INFO("Runner stopped");

}

void TrackerMonitorModule::register_hists() {

  INFO(" ... registering histograms in TrackerMonitor ... " );

  m_histogrammanager->registerHistogram("tracker_payloadsize", "payload size [bytes]", -0.5, 545.5, 275, 5.);

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
