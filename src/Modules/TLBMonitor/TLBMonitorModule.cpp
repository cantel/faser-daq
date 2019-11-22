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

   auto cfg = m_config.getSettings();
   m_sourceID = cfg["fragmentID"];

 }

TLBMonitorModule::~TLBMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TLBMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_event->event_tag() != PhysicsTag ) return;

  auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary);
  if ( (fragmentUnpackStatus & CorruptedFragment) | (fragmentUnpackStatus & MissingFragment)){
    fill_error_status_to_metric( fragmentUnpackStatus );
    fill_error_status_to_histogram( fragmentUnpackStatus, "h_tlb_errorcount" );
    return;
  }

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  fill_error_status_to_histogram( fragmentStatus, "h_tlb_errorcount" );

  uint16_t payloadSize = m_fragment->payload_size(); 

  m_histogrammanager->fill("h_tlb_payloadsize", payloadSize);
  m_metric_payload = payloadSize;
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

  std::string module_short_name = "tlb";
 
  register_error_metrics(module_short_name);

  m_metric_payload = 0;
  m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_payload, module_short_name+"_payload", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

  return;
}
