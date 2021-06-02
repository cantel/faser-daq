/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "FrontEndMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

#define PI 3.14

FrontEndMonitorModule::FrontEndMonitorModule() { 

   INFO("");
 }

FrontEndMonitorModule::~FrontEndMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void FrontEndMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_eventTag != PhysicsTag ) return; // only analysing physics events

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    WARNING("Unpacking fragment failed. Skipping event.");
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  
  uint16_t payloadSize = m_fragment->payload_size(); 

  // 1D histogram fill
  m_histogrammanager->fill("payloadsize", payloadSize);
  m_metric_payload = payloadSize;

  m_histogrammanager->fill("bcid", m_rawFragment->bc_id);


}

void FrontEndMonitorModule::register_hists() {

  INFO(" ... registering histograms in FrontEndMonitor ... " );

  // example of 1D histogram: default is ylabel="counts" non-extendable axes (Axis::Range::NONEXTENDABLE & 60 second publishing interval.
  //m_histogrammanager->registerHistogram("h_tracker_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);
  // example of 1D histogram with extendable x-axis, publishing interval of every 5 seconds.
  m_histogrammanager->registerHistogram("payloadsize", "payload size [bytes]", "event count/2kB", -0.5, 99.5, 50, Axis::Range::EXTENDABLE, 5);

  // example histograms with lots of bins
  m_histogrammanager->registerHistogram("bcid", "BCID", -0.5, 3564.5, 3565, 10);


  INFO(" ... done registering histograms ... " );

  return ;

}

void FrontEndMonitorModule::register_metrics() {

  INFO( "... registering metrics in FrontEndMonitorModule ... " );

  return;
}
