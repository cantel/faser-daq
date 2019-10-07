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

void EventMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto eventUnpackStatus = unpack_event_header(eventBuilderBinary);
  if ( eventUnpackStatus & CorruptedFragment){
    fill_error_status_to_metric( eventUnpackStatus );
    return;
  }

  // only accept physics events
  if ( m_eventHeader->event_tag != PhysicsTag ) return;

  uint32_t eventStatus = m_eventHeader->status;
  eventStatus |= eventUnpackStatus;
  fill_error_status_to_metric( eventStatus );

  uint16_t payloadSize = m_eventHeader->payload_size; 
  m_metric_payload = payloadSize;
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
