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
      fill_error_status_to_histogram( fragmentStatus, "tlb_errorcount" );
      
      if (fragmentStatus & MissingFragment ) continue; // go no further

      uint16_t payloadSize = fragmentHeader->payload_size; 

      m_histogrammanager->fill("tlb_payloadsize", payloadSize);
      m_metric_payload = payloadSize;
  }

  delete eventHeader;
  delete fragmentHeader;

  INFO("Runner stopped");

}

void TLBMonitorModule::register_hists() {

  INFO(" ... registering histograms in TLBMonitor ... " );
  
  m_histogrammanager->registerHistogram("tlb_payloadsize", "payload size [bytes]", -0.5, 545.5, 275, 5.);
  std::vector<std::string> categories = {"Ok", "Unclassified", "BCIDMistmatch", "TagMismatch", "Timeout", "Overflow","Corrupted", "Dummy", "Missing", "Empty", "Duplicate", "DataUnpack"};
  m_histogrammanager->registerHistogram("tlb_errorcount", "error type", categories, 5. );

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
