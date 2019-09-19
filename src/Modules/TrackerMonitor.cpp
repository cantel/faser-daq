/// \cond
#include <chrono>
/// \endcond
using namespace std::chrono_literals;
using namespace std::chrono;

#include <map>
#include <boost/format.hpp>
#include <boost/histogram/ostream.hpp> // write histogram straight to ostream
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream

using namespace boost::histogram;

#include "Modules/TrackerMonitor.hpp"

#define __MODULEMETHOD_NAME__ daqling::utilities::methodName(__PRETTY_FUNCTION__)
#define __MODULECLASS_NAME__ daqling::utilities::className(__PRETTY_FUNCTION__)

extern "C" TrackerMonitor *create_object() { return new TrackerMonitor; }

extern "C" void destroy_object(TrackerMonitor *object) { delete object; }

TrackerMonitor::TrackerMonitor() { 

   INFO("TrackerMonitor::TrackerMonitor");

   // make this configurable ...?
   m_json_file_name = "tracker_histogram_output.json";

   auto cfg = m_config.getConfig()["settings"];
   m_sourceID = cfg["fragmentID"];
   m_outputdir = cfg["outputDir"];

 }

TrackerMonitor::~TrackerMonitor() { 
  INFO(__MODULEMETHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TrackerMonitor::runner() {
  INFO(__MODULEMETHOD_NAME__ << " Running...");

  bool noData(true);
  daqling::utilities::Binary* eventBuilderBinary = new daqling::utilities::Binary;

  while (m_run) {

      if ( !m_connections.get(1, *eventBuilderBinary)){
          if ( !noData ) std::this_thread::sleep_for(10ms);
          noData=true;
          continue;
      }
      noData=false;

      const EventHeader * eventHeader((EventHeader *)malloc(m_eventHeaderSize));
      EventFragmentHeader * fragmentHeader((EventFragmentHeader *)malloc(m_fragmentHeaderSize));
      eventHeader = static_cast<const EventHeader *>(eventBuilderBinary->data());	

      // only accept physics events
      if ( eventHeader->event_tag != PhysicsTag ) continue;

      uint16_t dataStatus = unpack_data( *eventBuilderBinary, eventHeader, fragmentHeader );

      uint32_t fragmentStatus = fragmentHeader->status;
      fragmentStatus |= dataStatus;
      fill_error_status_to_metric( fragmentStatus );
      
      if (fragmentStatus & MissingFragment ) continue; // go no further

      uint16_t payloadSize = fragmentHeader->payload_size; 

      m_histogrammanager->fill("tracker_payloadsize", payloadSize);
      m_metric_payload = payloadSize;
  }

  INFO(__MODULEMETHOD_NAME__ << " Runner stopped");

}

void TrackerMonitor::register_hists() {

  INFO( __MODULEMETHOD_NAME__ << " ... registering histograms in TrackerMonitor ... " );

  // TRACKER histograms
  m_histogrammanager->registerHistogram("tracker_payloadsize", "payload size [bytes]", -0.5, 545.5, 275, 5.);

  INFO( __MODULEMETHOD_NAME__ << " ... done registering histograms ... " );
  return ;

}

void TrackerMonitor::register_metrics() {

 INFO( __MODULEMETHOD_NAME__ << " ... registering metrics in TrackerMonitor ... " );

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

 return;
}
