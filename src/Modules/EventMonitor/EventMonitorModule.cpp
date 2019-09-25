/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream> // std::ofstream
/// \endcond

#include "EventMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

EventMonitorModule::EventMonitorModule() { 

   INFO("");

   auto cfg = m_config.getConfig()["settings"];

 }

EventMonitorModule::~EventMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
 }

void EventMonitorModule::runner() {
  INFO("Running...");

  bool noData(true);
  daqling::utilities::Binary eventBuilderBinary;

  EventHeader * eventHeader = new EventHeader;

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

      uint32_t eventStatus = eventHeader->status;
      fill_error_status_to_metric( eventStatus );

      uint16_t payloadSize = eventHeader->payload_size; 
      m_metric_payload = payloadSize;

  }

  delete eventHeader;

  INFO("Runner stopped");

}

void EventMonitorModule::register_metrics() {

 INFO("... registering metrics in EventMonitorModule ... " );

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_payload, "event_payload", daqling::core::metrics::AVERAGE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_ok, "event_error_ok", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unclassified, "event_error_unclassified", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_bcidmismatch, "event_error_bcidmismatch", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_tagmismatch, "event_error_tagmismatch", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_timeout, "event_error_timeout", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_overflow, "event_error_overflow", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_corrupted, "event_error_corrupted", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_dummy, "event_error_dummy", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_missing, "event_error_missing", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_empty, "event_error_empty", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_duplicate, "event_error_duplicate", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unpack, "event_error_unpack", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 return;
}
