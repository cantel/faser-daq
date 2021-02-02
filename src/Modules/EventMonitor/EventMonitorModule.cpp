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

#define MAX_TRIG_LINES 5

#define MAX_L1A_SPACING 10000000

using namespace std::chrono_literals;
using namespace std::chrono;

EventMonitorModule::EventMonitorModule() { 
  INFO("In EventMonitorModule contructor");
  
  auto cfg = m_config.getConfig()["settings"];
  
  // determine which systems are configured for monitoring
  m_enable_digitizer = (bool)cfg["enable_digitizer"];
  m_enable_tlb       = (bool)cfg["enable_tlb"];
  
  int n_trb = (int)cfg["enable_trb"].size();
  
  if(n_trb<9){
    WARNING("It appears you have fewer TRB planes than expected : "<<(int)cfg["enable_trb"].size());
  }
  
  for(int itrb=0; itrb<n_trb; itrb++){
    m_enable_trb[itrb] = (bool)cfg["enable_trb"].at(itrb);
  }
  
}

EventMonitorModule::~EventMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void EventMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  // TLB BCID
  auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary, SourceIDs::TriggerSourceID);
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }
  m_tlb_bcid = m_fragment->bc_id();

  // Digi BCID
  fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary, SourceIDs::PMTSourceID);
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }
  m_digi_bcid = m_fragment->bc_id();
  //std::cout<<"  tlb bcid: "<<m_tlb_bcid<<std::endl;
  //std::cout<<"  digi bcid: "<<m_digi_bcid<<std::endl;
  int diff_tlb_digi_bcid = m_tlb_bcid - m_digi_bcid;
  if ( std::abs(diff_tlb_digi_bcid) < 5000 ) m_histogrammanager->fill("h_diff_tlb_digi_bcid", diff_tlb_digi_bcid );
  else WARNING("difference between tlb bcid = "<<m_tlb_bcid<<" and trb bcid = "<<m_digi_bcid<<" too big.");
  
  // if ( (m_tlb_bcid < 5000) && (m_digi_bcid < 5000) ) m_histogrammanager->fill2D("h_digibcid_vs_tlbbcid", m_digi_bcid, m_tlb_bcid );

  //// TRB BCID
  //fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary, SourceIDs::TrackerSourceID+1);
  //if ( fragmentUnpackStatus ) {
  //  fill_error_status_to_metric( fragmentUnpackStatus );
  //  return;
  //}
  //m_trb_bcid = m_fragment->bc_id();
  ////std::cout<<"tlb bcid = "<<m_tlb_bcid<<"  trb bcid = "<<m_trb_bcid<<std::endl;
  ////std::cout<<"tlb bcid - trb bcid = "<<m_tlb_bcid - m_trb_bcid<<std::endl;
  //int diff_tlb_trb_bcid = m_tlb_bcid - m_trb_bcid;
  //if ( std::abs(diff_tlb_trb_bcid) < 5000 ) m_histogrammanager->fill("h_diff_tlb_trb_bcid", diff_tlb_trb_bcid );
  //else WARNING("difference between tlb bcid = "<<m_tlb_bcid<<" and trb bcid = "<<m_trb_bcid<<" too big.");
  //if ( (m_tlb_bcid < 5000) && (m_trb_bcid < 5000) ) m_histogrammanager->fill2D("h_trbbcid_vs_tlbbcid", m_trb_bcid, m_tlb_bcid );


}

void EventMonitorModule::register_hists() {

  INFO(" ... registering histograms in EventMonitor ... " );
 
  // trigger counts 
  m_histogrammanager->registerHistogram("h_diff_tlb_trb_bcid", "TLB BCID - TRB0 BCID", -20, 20, 40, Axis::Range::EXTENDABLE, 7200);
  m_histogrammanager->registerHistogram("h_diff_tlb_digi_bcid", "TLB BCID - Digi BCID", -20, 20, 40, Axis::Range::EXTENDABLE, 7200);
  
  // ToDo : Figure out a way to display 2D info to be able to examine the correlation of the BCID matches
  // m_histogrammanager->register2DHistogram("h_trbbcid_vs_tlbbcid", "TRB BCID", 0, 3570, 3570, "TLB BCID", 0, 3570, 3570, 7200);
  // m_histogrammanager->register2DHistogram("h_digibcid_vs_tlbbcid", "DIGI BCID", 0, 3570, 3570, "TLB BCID", 0, 3570, 3570, 7200);

  INFO(" ... done registering histograms ... " );
  return;

}

void EventMonitorModule::register_metrics() {

  INFO( "... registering metrics in EventMonitorModule ... " );

  register_error_metrics();

  return;
}
