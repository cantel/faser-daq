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

   initialize_hists();

   // make this configurable ...?
   m_json_file_name = "tracker_histogram_output.json";

   auto cfg = m_config.getConfig()["settings"];
   m_sourceID = cfg["fragmentID"];
   m_timeblock = cfg["lengthTimeBlock"]; // seconds
   m_outputdir = cfg["outputDir"];
   m_timeDelayTolerance = cfg["timeDelayTolerance"]; // microseconds

 }

TrackerMonitor::~TrackerMonitor() { 
  INFO(__MODULEMETHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TrackerMonitor::runner() {
  INFO(__MODULEMETHOD_NAME__ << " Running...");

  bool isData(true);

  m_error_rate_cnt = 0;
  seconds timestamp_in_seconds;
  bool update = true;
  auto starting_time_in_seconds = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
 
  while (m_run) {

    daqling::utilities::Binary eventBuilderBinary;

    isData = m_connections.get(1, eventBuilderBinary);
    if ( !isData ) std::this_thread::sleep_for(10ms);
    else {
        const EventHeader * eventHeader((EventHeader *)malloc(m_eventHeaderSize));
        EventFragmentHeader * fragmentHeader((EventFragmentHeader *)malloc(m_fragmentHeaderSize));
	bool dataOk = unpack_data( eventBuilderBinary, eventHeader, fragmentHeader );

	if (!dataOk) { 
		ERROR(__MODULEMETHOD_NAME__ << " ERROR in unpacking data "); 
		m_hist_map.fillHist("h_fragmenterrors","DataUnpack");
                m_metric_error_unpack + 1;
		m_error_rate_cnt++;
		continue;
	}

	uint32_t fragmentStatus = fragmentHeader->status;
	fill_error_status( "h_fragmenterrors", fragmentStatus );
	fill_error_status( fragmentStatus );

        uint16_t payloadSize = fragmentHeader->payload_size; 

	m_hist_map.fillHist( "h_payloadsize", payloadSize);
        m_metric_payload = payloadSize;

        timestamp_in_seconds = duration_cast<seconds>(system_clock::now().time_since_epoch());
	if ( (timestamp_in_seconds.count() - starting_time_in_seconds)%m_timeblock == 0){
	    if ( update ) { 
	    m_hist_map.fillHist("g_error_rate_per_timeblock", std::make_pair(timestamp_in_seconds.count(), m_error_rate_cnt));
	    m_error_rate_cnt = 0;
	    update = false;
	    }
 	}
	else update = true;
    }

  }

  INFO(__MODULEMETHOD_NAME__ << " Runner stopped");

}

void TrackerMonitor::initialize_hists() {

  INFO( __MODULEMETHOD_NAME__ << " ... initializing ... " );

  // TRACKER histograms

  RegularHist h_payloadsize = {"h_payloadsize","payload size [bytes]"};
  h_payloadsize.object = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
  m_hist_map.addHist(h_payloadsize.name, h_payloadsize);

  CategoryHist h_fragmenterrors = { "h_fragmenterrors", "error type" };
  h_fragmenterrors.object = make_histogram(m_axis_fragmenterrors);
  m_hist_map.addHist(h_fragmenterrors.name, h_fragmenterrors);

  Graph g_error_rate_per_timeblock = {"g_error_rate_per_timeblock", "time [s]", "error rate" };
  m_hist_map.addHist(g_error_rate_per_timeblock.name, g_error_rate_per_timeblock);

  return ;

}

void TrackerMonitor::register_metrics() {

 INFO( __MODULEMETHOD_NAME__ << " ... registering metrics in TrackerMonitor ... " );

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_payload, "tracker_payload", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_ok, "tracker_error_ok", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unclassified, "tracker_error_unclassified", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_bcidmismatch, "tracker_error_bcidmismatch", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_tagmismatch, "tracker_error_tagmismatch", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_timeout, "tracker_error_timeout", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_overflow, "tracker_error_overflow", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_corrupted, "tracker_error_corrupted", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_dummy, "tracker_error_dummy", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_missing, "tracker_error_missing", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_empty, "tracker_error_empty", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_duplicate, "tracker_error_duplicate", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unpack, "tracker_error_unpack", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 return;
}
