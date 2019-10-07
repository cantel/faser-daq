/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "TrackerMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

TrackerMonitorModule::TrackerMonitorModule() { 

   INFO("");

   auto cfg = m_config.getConfig()["settings"];
   m_sourceID = cfg["fragmentID"];

 }

TrackerMonitorModule::~TrackerMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TrackerMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_eventHeader->event_tag != PhysicsTag ) return;

  auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary);
  if ( (fragmentUnpackStatus & CorruptedFragment) | (fragmentUnpackStatus & MissingFragment)){
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }

  uint32_t fragmentStatus = m_fragmentHeader->status;
  fill_error_status_to_metric( fragmentStatus );
  
  uint16_t payloadSize = m_fragmentHeader->payload_size; 

  m_histogrammanager->fill("h_tracker_payloadsize", payloadSize);
  std::cout<<"payload size is "<<payloadSize<<std::endl;
  m_metric_payload = payloadSize;

}

void TrackerMonitorModule::register_hists() {

  INFO(" ... registering histograms in TrackerMonitor ... " );

  m_histogrammanager->registerHistogram("h_tracker_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);

  INFO(" ... done registering histograms ... " );

  return ;

}

void TrackerMonitorModule::register_metrics() {

  INFO( "... registering metrics in TrackerMonitorModule ... " );

  std::string module_short_name = "tracker";
 
  register_error_metrics(module_short_name);
  m_statistics->registerVariable<std::atomic<int>, int>(&m_metric_payload, module_short_name+"_payload", daqling::core::metrics::LAST_VALUE, daqling::core::metrics::INT);

  return;
}
