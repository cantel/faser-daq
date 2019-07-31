/// \cond
#include <chrono>
/// \endcond
using namespace std::chrono_literals;
using namespace std::chrono;

#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream

#include "Modules/EventMonitor.hpp"

#define __MODULEMETHOD_NAME__ daqling::utilities::methodName(__PRETTY_FUNCTION__)
#define __MODULECLASS_NAME__ daqling::utilities::className(__PRETTY_FUNCTION__)

extern "C" EventMonitor *create_object() { return new EventMonitor; }

extern "C" void destroy_object(EventMonitor *object) { delete object; }

EventMonitor::EventMonitor() { 

   INFO("EventMonitor::EventMonitor");

   auto cfg = m_config.getConfig()["settings"];
   m_outputdir = cfg["outputDir"];

 }

EventMonitor::~EventMonitor() { 
  INFO(__MODULEMETHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
 }

void EventMonitor::runner() {
  INFO(__MODULEMETHOD_NAME__ << " Running...");

  bool isData(true);

  while (m_run) {

    daqling::utilities::Binary eventBuilderBinary;

    isData = m_connections.get(1, eventBuilderBinary);
    if ( !isData ) std::this_thread::sleep_for(10ms);
    else {
        const EventHeader * eventHeader((EventHeader *)malloc(m_eventHeaderSize));

        eventHeader = static_cast<const EventHeader *>(eventBuilderBinary.data());	
	// check integrity - not the correct check yet. should be checked within data.
	if ( eventHeader->marker != EventMarker ) {
	    ERROR(__MODULEMETHOD_NAME__ <<  " something went wrong in unpacking event header. Data NOT ok.");
            m_metric_error_unpack += 1;
	    continue;
	}
        if ( m_eventHeaderSize != eventHeader->header_size ) ERROR("event header gives wrong size!");

        // only accept physics events
        if ( eventHeader->event_tag != PhysicsTag ) continue;

	uint32_t eventStatus = eventHeader->status;
	fill_error_status( eventStatus );

        uint16_t payloadSize = eventHeader->payload_size; 
        m_metric_payload = payloadSize;

    }

  }

  INFO(__MODULEMETHOD_NAME__ << " Runner stopped");

}

void EventMonitor::register_metrics() {

 INFO( __MODULEMETHOD_NAME__ << " ... registering metrics in EventMonitor ... " );

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
