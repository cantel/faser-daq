/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
#include <chrono>
using namespace std::chrono;
/// \endcond

#include "EmulatorMonitorModule.hpp"

#define PI 3.14
#define NBINS 10


EmulatorMonitorModule::EmulatorMonitorModule(const std::string& n):MonitorBaseModule(n) { 
   INFO("");
 }

EmulatorMonitorModule::~EmulatorMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void EmulatorMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_event->event_tag() != m_eventTag ) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    WARNING("Unpacking fragment failed. Skipping event.");
    return;
  }
  // m_rawFragment or m_monitoringFragment should now be filled, depending on tag.

  uint16_t payloadSize = m_fragment->payload_size(); 

  // 1D histogram fill
  m_histogrammanager->fill("payloadsize", payloadSize);
  m_metric_payload = payloadSize;

  m_histogrammanager->fill("sizefrag", m_monitoringFragment->size_fragments_sent/1000.);

  m_histogrammanager->reset("pulse");
  short amp = rand()%10+1;
  float phase = (PI/2.)*(rand()%10)/NBINS;
  std::vector<short> adcs;
  //std::vector<unsigned short> positions;
  for ( unsigned short i = 1; i <= NBINS; i++) {
    adcs.push_back(amp*sin( i*(PI/5.) + phase));
    //positions.push_back(i);
  }
  //m_histogrammanager->fill("pulse", positions, adcs); // fill by providing both vectors for x values and weights. Around 15% slower compared to filling method one line down
  m_histogrammanager->fill("pulse", 1, 1, adcs); 

  // 2D hist fill
  DEBUG("m_monitoringFragment->num_fragments_sent = "<<m_monitoringFragment->num_fragments_sent);
  DEBUG("m_monitoringFragment->size_fragments_sent/1000. = "<<m_monitoringFragment->size_fragments_sent/1000.);
  m_histogrammanager->fill2D("numfrag_vs_sizefrag", m_monitoringFragment->num_fragments_sent, m_monitoringFragment->size_fragments_sent/1000.);

}

void EmulatorMonitorModule::register_hists() {

  INFO(" ... registering histograms in EmulatorMonitor ... " );

  // example of 1D histogram: default is ylabel="counts" non-extendable axes (Axis::Range::NONEXTENDABLE & 60 second publishing interval.
  //m_histogrammanager->registerHistogram("h_tracker_payloadsize", "payload size [bytes]", -0.5, 545.5, 275);
  // example of 1D histogram with extendable x-axis, publishing interval of every 30 seconds.
  m_histogrammanager->registerHistogram("payloadsize", "payload size [bytes]", "event count/2kB", -0.5, 349.5, 175, Axis::Range::EXTENDABLE, m_PUBINT);

  // example 1D histogram with non-extendable axis and resetting after each publish
  m_histogrammanager->registerHistogram("sizefrag", "size of sent fragments [kB]","count/2kB", -0.5, 349.5, 175, Axis::Range::NONEXTENDABLE, m_PUBINT);
  m_histogrammanager->resetOnPublish("sizefrag", true);

  // example pulse reset
   m_histogrammanager->registerHistogram("pulse", "pulse in magic adc", 1, NBINS+1, NBINS, 5);

  // example 2D hist
  m_histogrammanager->register2DHistogram("numfrag_vs_sizefrag", "no. of sent fragments", -0.5, 100.5, 101, "size of sent fragments [kB]", -0.5, 9.5, 20, m_PUBINT);

  INFO(" ... done registering histograms ... " );

  return ;

}

void EmulatorMonitorModule::register_metrics() {

  INFO( "... registering metrics in EmulatorMonitorModule ... " );

  return;
}
