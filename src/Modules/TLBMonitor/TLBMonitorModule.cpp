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

   auto cfg = m_config.getConfig()["settings"];
   m_sourceID = cfg["fragmentID"];

 }

TLBMonitorModule::~TLBMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TLBMonitorModule::runner() {
  INFO("Running...");

  bool noData(true);
  daqling::utilities::Binary eventBuilderBinary;

  while (m_run) {

      if ( !m_connections.get(1, eventBuilderBinary)){
          if ( !noData ) std::this_thread::sleep_for(10ms);
          noData=true;
          continue;
      }
      noData=false;

      //auto eventUnpackStatus = unpack_event_header(eventBuilderBinary);

      // only accept physics events
      auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary);
      //auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary); // unpacks to m_fragmentHeader and m_rawFragment.

      if ( m_eventHeader->event_tag != PhysicsTag ) continue;

      uint32_t fragmentStatus = m_fragmentHeader->status;
      fragmentStatus |= fragmentUnpackStatus;
      fill_error_status_to_metric( fragmentStatus );
      fill_error_status_to_histogram( fragmentStatus, "h_tlb_errorcount" );

      if (fragmentStatus & MissingFragment ) continue; // go no further

      uint16_t payloadSize = m_fragmentHeader->payload_size; 

      m_histogrammanager->fill("h_tlb_payloadsize", payloadSize);
      std::cout<<"payload size is "<<payloadSize<<std::endl;
      m_metric_payload = payloadSize;

      m_event_header_unpacked = false;
      m_fragment_header_unpacked = false;
      m_raw_fragment_unpacked = false;
  }

  INFO("Runner stopped");

}

void TLBMonitorModule::register_hists() {

  INFO(" ... registering histograms in TLBMonitor ... " );
  
  m_histogrammanager->registerHistogram("h_tlb_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);
  std::vector<std::string> categories = {"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"};
  m_histogrammanager->registerHistogram("h_tlb_errorcount", "error type", categories, 5. );

  INFO(" ... done registering histograms ... " );
  return;

}

void TLBMonitorModule::register_metrics() {

 INFO( "... registering metrics in TLBMonitorModule ... " );

 if ( m_stats_on ) {
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

 }

 return;
}
