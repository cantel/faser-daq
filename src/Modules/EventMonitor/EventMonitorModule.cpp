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
 }

EventMonitorModule::~EventMonitorModule() { 
  INFO("With config: " << m_config.dump() << " getState: " << this->getState());
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
  if ( std::abs(diff_tlb_digi_bcid) < 5000 ) m_histogrammanager->fill("diff_tlb_digi_bcid", diff_tlb_digi_bcid );
  else WARNING("difference between tlb bcid = "<<m_tlb_bcid<<" and trb bcid = "<<m_digi_bcid<<" too big.");
  if ( (m_tlb_bcid < 5000) && (m_digi_bcid < 5000) ) m_histogrammanager->fill2D("digibcid_vs_tlbbcid", m_digi_bcid, m_tlb_bcid );

  // TRB BCID
  fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary, SourceIDs::TrackerSourceID+1);
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }
  m_trb_bcid = m_fragment->bc_id();
  //std::cout<<"tlb bcid = "<<m_tlb_bcid<<"  trb bcid = "<<m_trb_bcid<<std::endl;
  //std::cout<<"tlb bcid - trb bcid = "<<m_tlb_bcid - m_trb_bcid<<std::endl;
  int diff_tlb_trb_bcid = m_tlb_bcid - m_trb_bcid;
  if ( std::abs(diff_tlb_trb_bcid) < 5000 ) m_histogrammanager->fill("diff_tlb_trb_bcid", diff_tlb_trb_bcid );
  else WARNING("difference between tlb bcid = "<<m_tlb_bcid<<" and trb bcid = "<<m_trb_bcid<<" too big.");
  if ( (m_tlb_bcid < 5000) && (m_trb_bcid < 5000) ) m_histogrammanager->fill2D("trbbcid_vs_tlbbcid", m_trb_bcid, m_tlb_bcid );

  short random = rand()%100+1;
  m_histogrammanager->fill2D("test_2d", random, random);


}

void EventMonitorModule::register_hists() {

  INFO(" ... registering histograms in EventMonitor ... " );
 
  // trigger counts 
  m_histogrammanager->registerHistogram("diff_tlb_trb_bcid", "TLB BCID - TRB0 BCID", -20, 20, 40, Axis::Range::EXTENDABLE, 7200);
  m_histogrammanager->registerHistogram("diff_tlb_digi_bcid", "TLB BCID - Digi BCID", -20, 20, 40, Axis::Range::EXTENDABLE, 7200);
  
  m_histogrammanager->register2DHistogram("trbbcid_vs_tlbbcid", "TRB BCID", 0, 3570, 3570, "TLB BCID", 0, 3570, 3570, 7200);
  m_histogrammanager->register2DHistogram("digibcid_vs_tlbbcid", "DIGI BCID", 0, 3570, 3570, "TLB BCID", 0, 3570, 3570, 7200);

  // dummy
  m_histogrammanager->register2DHistogram("test_2d", "x", 0, 100, 100, "y", 0, 100, 100, 600);

  INFO(" ... done registering histograms ... " );
  return;

}

void EventMonitorModule::register_metrics() {

  INFO( "... registering metrics in EventMonitorModule ... " );

  register_error_metrics();

  return;
}
