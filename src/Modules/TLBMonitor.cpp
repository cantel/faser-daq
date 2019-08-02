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

#include "Modules/TLBMonitor.hpp"

#define __MODULEMETHOD_NAME__ daqling::utilities::methodName(__PRETTY_FUNCTION__)
#define __MODULECLASS_NAME__ daqling::utilities::className(__PRETTY_FUNCTION__)

extern "C" TLBMonitor *create_object() { return new TLBMonitor; }

extern "C" void destroy_object(TLBMonitor *object) { delete object; }

TLBMonitor::TLBMonitor() { 

   INFO("TLBMonitor::TLBMonitor");

   // make this configurable ...?
   m_json_file_name = "tlb_histogram_output.json";

   auto cfg = m_config.getConfig()["settings"];
   m_sourceID = cfg["fragmentID"];
   m_outputdir = cfg["outputDir"];

 }

TLBMonitor::~TLBMonitor() { 
  INFO(__MODULEMETHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TLBMonitor::runner() {
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
      fill_error_status( "h_fragmenterrors", fragmentStatus );
      fill_error_status( fragmentStatus );
      
      if (fragmentStatus & MissingFragment ) continue; // go no further

      uint16_t payloadSize = fragmentHeader->payload_size; 

      m_hist_map.fillHist( "h_payloadsize", payloadSize);
      m_metric_payload = payloadSize;
  }

  INFO(__MODULEMETHOD_NAME__ << " Runner stopped");

}

void TLBMonitor::initialize_hists() {

  RegularHist h_payloadsize = {"h_payloadsize","payload size [bytes]"};
  h_payloadsize.object = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
  m_hist_map.addHist(h_payloadsize.name, h_payloadsize);

  CategoryHist h_fragmenterrors = { "h_fragmenterrors", "error type" };
  h_fragmenterrors.object = make_histogram(m_axis_fragmenterrors);
  m_hist_map.addHist(h_fragmenterrors.name, h_fragmenterrors);

  return ;
}

void TLBMonitor::register_metrics() {

 INFO( __MODULEMETHOD_NAME__ << " ... registering metrics in TLBMonitor ... " );

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_payload, "tlb_payload", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_ok, "tlb_error_ok", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unclassified, "tlb_error_unclassified", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_bcidmismatch, "tlb_error_bcidmismatch", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_tagmismatch, "tlb_error_tagmismatch", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_timeout, "tlb_error_timeout", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_overflow, "tlb_error_overflow", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_corrupted, "tlb_error_corrupted", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_dummy, "tlb_error_dummy", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_unpack, "tlb_error_unpack", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_missing, "tlb_error_missing", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_empty, "tlb_error_empty", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_error_duplicate, "tlb_error_duplicate", daqling::core::metrics::ACCUMULATE, daqling::core::metrics::INT);

 return;
}
