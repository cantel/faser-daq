/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
#include <cmath> // std::copysign
/// \endcond

#include "TriggerMonitorModule.hpp"

#define MAX_TRIG_ITEMS 6
#define MAX_TRIG_LINES 8

#define MAX_L1A_SPACING 1e4

using namespace std::chrono_literals;
using namespace std::chrono;

TriggerMonitorModule::TriggerMonitorModule(): m_prefix_hname_signal_nextBC("signal_nextBC_ch") { 

  INFO("In TriggerMonitorModule contructor");
  m_previous_orbit = 0;
  m_previous_bcid = 0;
 }

TriggerMonitorModule::~TriggerMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void TriggerMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

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
    if ( m_bcid > MAX_BCID ) WARNING("Retrieved BCID number "<<m_bcid<<" exceeds max LHC range");
    m_orbitid = m_tlbdataFragment->orbit_id();
    m_l1A_spacing  = (double)((m_orbitid - m_previous_orbit)*MAX_BCID + std::copysign((m_bcid - m_previous_bcid)%MAX_BCID,  m_bcid - m_previous_bcid))/MAX_BCID;
    m_histogrammanager->fill("bcid", m_fragment->bc_id()); 
    if ( m_l1A_spacing < MAX_L1A_SPACING ) // protection for stretchy histograms
      m_histogrammanager->fill("l1a_spacing", m_l1A_spacing); 
    else
      WARNING("Computed L1A spacing beyond histogram range we've allowed.");
    m_previous_orbit = m_orbitid;
    m_previous_bcid = m_bcid;
    uint8_t inputs = m_tlbdataFragment->input_bits();
    uint8_t inputs_nextBC = m_tlbdataFragment->input_bits_next_clk();
    uint8_t tap = m_tlbdataFragment->tap();
    m_histogrammanager->fill("trigitems_word", tap);
    m_histogrammanager->fill("triglines_word", inputs);
    m_histogrammanager->fill("triglines_word_bothClk", inputs|inputs_nextBC);
    for ( unsigned k = 0; k < MAX_TRIG_ITEMS; k++ ){
      if (tap & (1<<k)) {
        m_histogrammanager->fill("trigitem_idx", k); 
        switch (k){
          case 0: {m_tap0++; break;}
          case 1: {m_tap1++; break;}
          case 2: {m_tap2++; break;}
          case 3: {m_tap3++; break;}
          case 4: {m_tap4++; break;}
          case 5: {m_tap5++; break;}
          default: WARNING("trigger item "<<k<<" out of range.");
        }
      }
    }
    if (inputs) {
      for ( unsigned i = 0; i < MAX_TRIG_LINES; i++ ){
        if (inputs & (1 << i )) {
          //std::string hname_signal_nextBC = m_prefix_hname_signal_nextBC+std::to_string(i);
          for ( unsigned j = 0; j < MAX_TRIG_LINES; j++ ){
            if (inputs_nextBC & (1 << j )) m_histogrammanager->fill2D("signal_current_vs_nextBC", i, j);
            if (j==i) continue;
            if (inputs & (1 << j )) m_histogrammanager->fill2D("signal_currentBC", i, j);
          }
          for ( unsigned k = 0; k < MAX_TRIG_ITEMS; k++ ){
            if (tap & (1<<k)) m_histogrammanager->fill2D("trigline_vs_trigitem", i, k); 
          }
          m_histogrammanager->fill("trigline_idx", i);
          switch (i){
            case 0: {m_input_channel0++; break;}
            case 1: {m_input_channel1++; break;}
            case 2: {m_input_channel2++; break;}
            case 3: {m_input_channel3++; break;}
            case 4: {m_input_channel4++; break;}
            case 5: {m_input_channel5++; break;}
            case 6: {m_input_channel6++; break;}
            case 7: {m_input_channel7++; break;}
            default: WARNING("trigger line "<<i<<" out of range.");
          }
        } // active bit on i
      } 
    }
  }
  else WARNING("Skipping invalid trigger physics fragment:\n"<<std::dec<<*m_tlbdataFragment);
}

void TriggerMonitorModule::register_hists() {

  INFO(" ... registering histograms in TriggerMonitor ... " );
 
  m_histogrammanager->registerHistogram("bcid", "BCID", -0.5, 4095.5, 4096, 60);
  m_histogrammanager->registerHistogram("l1a_spacing", "L1A Spacing [no. of orbits]", -0.5, 99.5, 500, Axis::Range::EXTENDABLE, 60);
  m_histogrammanager->registerHistogram("trigline_idx", "Trigger Line Idx", 0, MAX_TRIG_LINES, MAX_TRIG_LINES);
  m_histogrammanager->registerHistogram("trigitem_idx", "Trigger Item Idx", 0, MAX_TRIG_ITEMS, MAX_TRIG_ITEMS);
  m_histogrammanager->registerHistogram("trigitems_word", "Trigger Item Word", 0, 16, 16);
  m_histogrammanager->registerHistogram("triglines_word", "Trigger Line Word", 0, 256, 256);
  m_histogrammanager->registerHistogram("triglines_word_bothClk", "Trigger Line Word", 0, 256, 256);
  /*for ( unsigned i = 0; i < MAX_TRIG_LINES; i++ ){
    std::string hname_signal_nextBC = m_prefix_hname_signal_nextBC+std::to_string(i);
    m_histogrammanager->registerHistogram(hname_signal_nextBC, "active inputs next BC", 0, MAX_TRIG_LINES, MAX_TRIG_LINES);
  }*/
  m_histogrammanager->register2DHistogram("signal_current_vs_nextBC", "active line current BC", 0, MAX_TRIG_LINES, MAX_TRIG_LINES, "active line next BC", 0, MAX_TRIG_LINES, MAX_TRIG_LINES);
  m_histogrammanager->register2DHistogram("signal_currentBC", "active line current BC", 0, MAX_TRIG_LINES, MAX_TRIG_LINES, "active line current BC", 0, MAX_TRIG_LINES, MAX_TRIG_LINES);
  m_histogrammanager->register2DHistogram("trigline_vs_trigitem", "trigger line", 0, MAX_TRIG_LINES, MAX_TRIG_LINES, "trigger item", 0, MAX_TRIG_LINES, MAX_TRIG_LINES);

  INFO(" ... done registering histograms ... " );
  return;

}

void TriggerMonitorModule::register_metrics() {

  INFO( "... registering metrics in TriggerMonitorModule ... " );

  register_error_metrics();

  registerVariable(m_input_channel0, "Input0Rate", metrics::RATE);
  registerVariable(m_input_channel1, "Input1Rate", metrics::RATE);
  registerVariable(m_input_channel2, "Input2Rate", metrics::RATE);
  registerVariable(m_input_channel3, "Input3Rate", metrics::RATE);
  registerVariable(m_input_channel4, "Input4Rate", metrics::RATE);
  registerVariable(m_input_channel5, "Input5Rate", metrics::RATE);
  registerVariable(m_input_channel6, "Input6Rate", metrics::RATE);
  registerVariable(m_input_channel7, "Input7Rate", metrics::RATE);

  registerVariable(m_input_channel0, "Input0");
  registerVariable(m_input_channel1, "Input1");
  registerVariable(m_input_channel2, "Input2");
  registerVariable(m_input_channel3, "Input3");
  registerVariable(m_input_channel4, "Input4");
  registerVariable(m_input_channel5, "Input5");
  registerVariable(m_input_channel6, "Input6");
  registerVariable(m_input_channel7, "Input7");

  registerVariable(m_tap0, "TAP0");
  registerVariable(m_tap1, "TAP1");
  registerVariable(m_tap2, "TAP2");
  registerVariable(m_tap3, "TAP3");
  registerVariable(m_tap4, "TAP4");
  registerVariable(m_tap5, "TAP5");

  return;
}
