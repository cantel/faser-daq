/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
#include <cmath> // std::copysign
/// \endcond

#include "TriggerMonitorModule.hpp"

#define MAX_TRIG_LINES 5

#define MAX_L1A_SPACING 10000000

using namespace std::chrono_literals;
using namespace std::chrono;

TriggerMonitorModule::TriggerMonitorModule() { 

  INFO("In TriggerMonitorModule contructor");
  m_previous_orbit = 0;
  m_previous_bcid = 0;
 }

TriggerMonitorModule::~TriggerMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void TriggerMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
 
  if ( m_tlbdataFragment->valid() ) {
    m_bcid = m_tlbdataFragment->bc_id();
    m_orbitid = m_tlbdataFragment->orbit_id();
    m_l1A_spacing  = (m_orbitid - m_previous_orbit)*MAX_BCID + std::copysign((m_bcid - m_previous_bcid)%MAX_BCID,  m_bcid - m_previous_bcid);
    m_histogrammanager->fill("bcid", m_fragment->bc_id()); 
    if ( m_l1A_spacing < MAX_L1A_SPACING ) // protection for stretchy histograms
      m_histogrammanager->fill("l1a_spacing", m_l1A_spacing); 
    else
      WARNING("Computed L1A spacing beyond histogram range we've allowed.");
    m_previous_orbit = m_orbitid;
    m_previous_bcid = m_bcid;
  }
  else WARNING("Ignoring corrupted data fragment.");

}

void TriggerMonitorModule::register_hists() {

  INFO(" ... registering histograms in TriggerMonitor ... " );
 
  // trigger counts 
  m_histogrammanager->registerHistogram("bcid", "BCID", -0.5, 3564.5, 3565, 3600);
  m_histogrammanager->registerHistogram("l1a_spacing", "L1A Spacing [no. of BCID]", 0.5, 8000.5, 16, Axis::Range::EXTENDABLE, 3600);

  INFO(" ... done registering histograms ... " );
  return;

}

void TriggerMonitorModule::register_metrics() {

  INFO( "... registering metrics in TriggerMonitorModule ... " );

  register_error_metrics();

  return;
}
