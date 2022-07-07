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

#include "EventMonitorModule.hpp"

EventMonitorModule::EventMonitorModule(const std::string& n):MonitorBaseModule(n) { 
  INFO("In EventMonitorModule contructor");
  m_tlb_bcid = 0;
  m_digi_bcid = 0;
  m_trb_bcid = 0;
}

void EventMonitorModule::configure(){
  INFO("Configuring EventMonitorModule");
  
  auto cfg = getModuleSettings();
  
  // determine which systems are configured for monitoring
  m_enable_digitizer = (bool)cfg["enable_digitizer"];
  m_enable_tlb       = (bool)cfg["enable_tlb"];
  
  m_n_trb = (int)cfg["enable_trb"].size();
  if (m_n_trb > kMAXTRBs) throw MonitorBase::ConfigurationIssue(ERS_HERE, "Number of configured TRBs is larger than what exists.");
  
  m_enabled_trbs = cfg["enable_trb"].get<std::vector<int>>();

  MonitorBaseModule::configure();

}

EventMonitorModule::~EventMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void EventMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  // TLB BCID
  if (m_enable_tlb){
    auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary, SourceIDs::TriggerSourceID);
    if ( fragmentUnpackStatus ) {
      ERROR("Error retrieving TLB data fragment.");
      if (fragmentUnpackStatus & MissingFragment) m_missing_tlb++;
      return;
    }
    m_tlb_bcid = m_fragment->bc_id();
  }

  // Digi BCID
  if (m_enable_digitizer){
    auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary, SourceIDs::PMTSourceID);
    if ( fragmentUnpackStatus ) {
      ERROR("Error retrieving digitizer data fragment.");
      if (fragmentUnpackStatus & MissingFragment) m_missing_digi++;
    } else {
    if (m_enable_tlb){
      m_digi_bcid = m_fragment->bc_id();
      //std::cout<<"  tlb bcid: "<<m_tlb_bcid<<std::endl;
      //std::cout<<"  digi bcid: "<<m_digi_bcid<<std::endl;
      int diff_tlb_digi_bcid = m_tlb_bcid - m_digi_bcid;
      if ( std::abs(diff_tlb_digi_bcid) < kMAXBCID ) m_histogrammanager->fill("h_diff_tlb_digi_bcid", diff_tlb_digi_bcid );
      else WARNING("Difference between TLB BCID = "<<m_tlb_bcid<<" and digi BCID = "<<m_digi_bcid<<" too big to fill.");
      }
    }
  }
  
  //// TRB BCID
  for (int8_t i=0;i<m_n_trb;i++){
    auto trb_id = m_enabled_trbs[i];
    auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary, SourceIDs::TrackerSourceID+trb_id);
    if ( fragmentUnpackStatus ) {
      ERROR("Error retrieving tracker data fragment.");
      if (fragmentUnpackStatus & MissingFragment) m_missing_trb++;
    } else {
      if (m_enable_tlb) {
        m_trb_bcid = m_fragment->bc_id();
        //std::cout<<"tlb bcid = "<<m_tlb_bcid<<"  trb bcid = "<<m_trb_bcid<<std::endl;
        //std::cout<<"tlb bcid - trb bcid = "<<m_tlb_bcid - m_trb_bcid<<std::endl;
        int diff_tlb_trb_bcid = m_tlb_bcid - m_trb_bcid;
        if ( std::abs(diff_tlb_trb_bcid) < kMAXBCID ) m_histogrammanager->fill(m_tlb_trb_hist_names.at(i), diff_tlb_trb_bcid );
        else WARNING("Difference between TLB BCID = "<<m_tlb_bcid<<" and TRB BCID = "<<m_trb_bcid<<" too big to fill.");
      }
    }
  }

}

void EventMonitorModule::register_hists() {

  INFO(" ... registering histograms in EventMonitor ... " );
 
  // trigger counts 
  if ( m_enable_tlb && m_enable_digitizer){
    m_histogrammanager->registerHistogram("h_diff_tlb_digi_bcid", "TLB BCID - Digi BCID", -5, 5, 10, Axis::Range::EXTENDABLE, m_PUBINT);
  }
  if ( m_enable_tlb){
    for ( auto i: m_enabled_trbs){
      std::stringstream histo_title;
      histo_title<<"h_diff_tlb_trb"<<i<<"_bcid";
      m_tlb_trb_hist_names.push_back(histo_title.str());
      std::stringstream histo_label;
      histo_label<<"TLB BCID - TRB"<<i<<" BCID";
      m_histogrammanager->registerHistogram(histo_title.str(), histo_label.str(), -5, 5, 10, Axis::Range::EXTENDABLE, m_PUBINT);
    }
  }

  INFO(" ... done registering histograms ... " );
  return;

}

void EventMonitorModule::register_metrics() {

  INFO( "... registering metrics in EventMonitorModule ... " );

  registerVariable(m_missing_tlb, "MissingTLBCnt");
  registerVariable(m_missing_digi, "MissingDigiCnt");
  registerVariable(m_missing_trb, "MissingTRBCnt");

  return;
}
