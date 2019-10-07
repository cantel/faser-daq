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
  
  std::string module_short_name = "event";
 
  register_error_metrics(module_short_name);
  m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_payload, module_short_name+"_payload", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

  return;
}
