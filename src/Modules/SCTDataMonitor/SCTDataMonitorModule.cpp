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

#include "SCTDataMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

SCTDataMonitorModule::SCTDataMonitorModule(const std::string& n): MonitorBaseModule(n),m_prefix_hname_hitp("hitpattern_physics_mod"),m_hname_scterrors("sct_data_errors"){ 

   INFO("SCTDataMonitorModule()");
   auto cfg = getModuleSettings();
   auto cfg_trigbits_select = cfg["PhysicsTriggerBits"];
   if (cfg_trigbits_select!="" && cfg_trigbits_select!=nullptr)
      m_physics_trigbits = cfg_trigbits_select;
   else m_physics_trigbits=0xf;

 }

SCTDataMonitorModule::~SCTDataMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void SCTDataMonitorModule::start(unsigned int run_num){
  INFO("Starting "<<getName());
  m_hit_avg = 0;
  m_hit_avg_count = 0;
  m_physics_strip_count = 0;
  m_random_strip_count = 0;
  MonitorBaseModule::start(run_num);
}

void SCTDataMonitorModule::monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if ( m_event->event_tag() != m_eventTag ) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  bool randTrig(false);
  bool physicsTrig(false);
  auto fragmentUnpackStatus = unpack_full_fragment( eventBuilderBinary, SourceIDs::TriggerSourceID );
  if ( fragmentUnpackStatus ) {
     WARNING("Unpacking error for trigger fragment. Can't use trigger information for this event!");
  } else {
    if (m_tlbdataFragment->tbp() & 0x10) {
        randTrig = true;
        m_random_strip_count+=128;
    }
    if ((m_tlbdataFragment->tbp()&0xf) & m_physics_trigbits) {
        physicsTrig=true;
        m_physics_strip_count+=128;
    }
  }

  fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    WARNING("Unpacking error for tracker fragment. Skipping event!");
    return;
  }

  if ( m_trackerdataFragment ){
    if (m_trackerdataFragment->has_trb_error()) m_histogrammanager->fill("track_data_error_types", "TRBError");
    if (m_trackerdataFragment->has_module_error()) m_histogrammanager->fill("track_data_error_types", "ModuleError");
    if (m_trackerdataFragment->missing_event_id()) m_histogrammanager->fill("track_data_error_types", "NoEventID");
    if (m_trackerdataFragment->missing_bcid()) m_histogrammanager->fill("track_data_error_types", "NoBCID");
    if (m_trackerdataFragment->missing_crc()) m_histogrammanager->fill("track_data_error_types", "NoCRC");
    if (m_trackerdataFragment->missing_frames()) m_histogrammanager->fill("track_data_error_types", "MissingFrames");
    if (m_trackerdataFragment->unrecognized_frames()) m_histogrammanager->fill("track_data_error_types", "UnrecognizedFrames");
    if (!m_trackerdataFragment->valid()) {
      WARNING("Errors were found in tracker fragment:\n"<<*m_trackerdataFragment<<"\n Skipping event!");
      m_metric_total_errors++;
      return;
    }
  }
  else {WARNING("Ignoring empty data fragment.");return;}

  size_t payload_size = m_fragment->payload_size();
  if ( payload_size > MAXFRAGSIZE ) {
     WARNING(" VERY large payload size received. Payload size of "<<payload_size<<" bytes exceeds maximum allowable for histogram filling. Resetting to "<<MAXFRAGSIZE);
     payload_size = MAXFRAGSIZE;
  } 
  m_histogrammanager->fill("payloadsize", payload_size);

  m_bcid = m_trackerdataFragment->bc_id();
  m_l1id = m_trackerdataFragment->event_id();
  m_histogrammanager->fill("bcid", m_bcid);

  goodHits = 0;
  for (unsigned i = 0;i < 8; i++){
      goodHitsMod[i]=0;}

  m_print_WARNINGS = m_total_WARNINGS < kMAXWARNINGS;
  int total_hits(0);

  for ( auto it = m_trackerdataFragment->cbegin(); it != m_trackerdataFragment->cend(); ++it ){
      auto sctEvent = *it;
      if (sctEvent == nullptr) { 
        WARNING("Invalid SCT Event for event "<<m_trackerdataFragment->event_id());
        WARNING("tracker data fragment: "<<*m_trackerdataFragment);
        m_status = STATUS_WARN;
        m_histogrammanager->fill("track_data_error_types", "ModuleDecodeError");
        continue;
      }
     // how about SCT data errors
     std::string hname_mod_scterrors = m_hname_scterrors+"_mod"+std::to_string(sctEvent->GetModuleID());
     if ( sctEvent->HasError() ) {
        auto sctErrorList= sctEvent->GetErrors();
        for ( unsigned idx = 0; idx < 12; idx++ ) {
          auto sctErrors = sctErrorList.at(idx);
          for ( uint8_t sctError : sctErrors ) {
            WARNING("SCT Error for module "<<sctEvent->GetModuleID()<<", chip idx "<<idx<<". Error code = "<<static_cast<int>(sctError));
            //m_total_WARNINGS++;
            switch ( sctError ) {
              case 0x1:
               m_histogrammanager->fill(hname_mod_scterrors, "NoData");
               m_histogrammanager->fill(m_hname_scterrors, "NoData");
               break;
              case 0x2:
               m_histogrammanager->fill(hname_mod_scterrors, "BuffOverflow");
               m_histogrammanager->fill(m_hname_scterrors, "BuffOverflow");
               break;
              case 0x4:
               m_histogrammanager->fill(hname_mod_scterrors, "BuffError");
               m_histogrammanager->fill(m_hname_scterrors, "BuffError");
               break;
              case 0xff:
               m_histogrammanager->fill(hname_mod_scterrors, "UnknownChip");
               m_histogrammanager->fill(m_hname_scterrors, "UnknownChip");
               break;
              default:
               m_histogrammanager->fill(hname_mod_scterrors, "Unknown");
               m_histogrammanager->fill(m_hname_scterrors, "Unknown");
            }
          }
        }
      }; // end of SCT ERRORs check

      int diff_bcid = (m_bcid-sctEvent->GetBCID())&0xFF;
      int diff_l1id = (m_l1id-sctEvent->GetL1ID())&0xF;
      m_histogrammanager->fill("diff_trb_sct_bcid", diff_bcid);
      m_histogrammanager->fill("diff_trb_sct_l1id", diff_l1id);
      if ( diff_bcid != kBCIDOFFSET ) { 
        if (m_print_WARNINGS) WARNING("BCID mismatch for module "<<sctEvent->GetModuleID()<<". TRB BCID = "<<m_bcid<<", SCT BCID = "<<sctEvent->GetBCID());
        m_histogrammanager->fill(hname_mod_scterrors, "BCIDMismatch");
        m_histogrammanager->fill(m_hname_scterrors, "BCIDMismatch");
        m_total_WARNINGS++;
      }
      if ( diff_l1id != 0 ) {
        if (m_print_WARNINGS) WARNING("L1ID mismatch for module "<<sctEvent->GetModuleID()<<". TRB BCID = "<<m_l1id<<", SCT L1ID = "<<sctEvent->GetL1ID());
        m_histogrammanager->fill(hname_mod_scterrors, "L1IDMismatch");
        m_histogrammanager->fill(m_hname_scterrors, "L1IDMismatch");
        m_total_WARNINGS++;
      }
      auto allHits = sctEvent->GetHits();
      uint8_t module = sctEvent->GetModuleID();
      unsigned nhits = sctEvent->GetNHits();
      m_histogrammanager->fill("total_hits_multiplicity", nhits);
      total_hits+= nhits;

      if (randTrig) {
        for ( unsigned chipIdx = 0; chipIdx < 12; chipIdx++) {
          auto hitsPerChip = allHits[chipIdx];
          for (auto hit : hitsPerChip){
              std::bitset<3> bitset_hitp(hit.second);
              m_histogrammanager->fill("hitpattern_random", bitset_hitp.to_string());
              if ( hit.second == 7 || hit.second == 0 ) continue;
                  m_histogrammanager->fill2D("hitmap_random", module, chipIdx, 1);
                  mapline = 8*chipIdx + module;
                  if (mapline < kMAP_SIZE) m_histogrammanager->fill2D("chip_occupancy_noise",MAP[mapline][0],MAP[mapline][1],1 );
          }
        }
      }
      if (physicsTrig && m_lhc_physics_mode) { // only selected physics triggered events during LHC collisions
        std::string hname_hitp = m_prefix_hname_hitp+std::to_string(sctEvent->GetModuleID());
        for ( unsigned chipIdx = 0; chipIdx < (unsigned)kCHIPS_PER_MODULE*0.5; chipIdx++) {
          auto hitsPerChip1 = allHits[chipIdx];
          for (auto hit1 : hitsPerChip1){ // hit is an std::pair<uint8 strip, uint8 pattern>
            if ( hit1.second == 7 ) continue;
            auto strip1 = hit1.first;
            unsigned chipIdx2 = kCHIPS_PER_MODULE - 1 - chipIdx;
            auto hitsPerChip2 = allHits[chipIdx2];
            for (auto hit2 : hitsPerChip2){ // hit is an std::pair<uint8 strip, uint8 pattern>
              if ( hit2.second == 7 ) continue;
              auto strip2 = kSTRIPS_PER_CHIP-1-hit2.first; // invert
              if (module<=3){
                 m_histogrammanager->fill("strip_id_difference_physics_mod0to3",strip1-strip2);}
              else {m_histogrammanager->fill("strip_id_difference_physics_mod4to7",strip1-strip2);}
              if ( std::abs(strip1-strip2) > kSTRIPDIFFTOLERANCE ) continue;
              // good physics hits
              goodHits = goodHits + 1;
              m_histogrammanager->fill2D("hitmap_physics", module, chipIdx, 1);
              m_histogrammanager->fill2D("hitmap_physics", module, chipIdx2, 1);
              mapline = 8*chipIdx + module;
              if (mapline < kMAP_SIZE) m_histogrammanager->fill2D("chip_occupancy_physics",MAP[mapline][0],MAP[mapline][1],1 );
              mapline2 = 8*(chipIdx2) + module;
              if (mapline2 < kMAP_SIZE) m_histogrammanager->fill2D("chip_occupancy_physics",MAP[mapline2][0],MAP[mapline2][1],1 );
              goodHitsMod[module] = goodHitsMod[module] + 1;           
              std::bitset<3> bitset_hitp1(hit1.second);
              m_histogrammanager->fill(hname_hitp, bitset_hitp1.to_string());
              std::bitset<3> bitset_hitp2(hit2.second);
              m_histogrammanager->fill(hname_hitp, bitset_hitp2.to_string());
              update_hitavg(hit1.second);
              update_hitavg(hit2.second);
            }
          }
        }
      }
  }
  m_hit_multiplicity = total_hits;
  if (physicsTrig && m_lhc_physics_mode) {
    m_histogrammanager->fill("good_nhits_physics",goodHits);
    m_histogrammanager->fill("bcid_hit_weighted_physics", m_bcid, goodHits);
    if (goodHitsMod[0]) m_histogrammanager->fill("good_nhits_physics_Mod0",goodHitsMod[0]);
    if (goodHitsMod[1]) m_histogrammanager->fill("good_nhits_physics_Mod1",goodHitsMod[1]);
    if (goodHitsMod[2]) m_histogrammanager->fill("good_nhits_physics_Mod2",goodHitsMod[2]);
    if (goodHitsMod[3]) m_histogrammanager->fill("good_nhits_physics_Mod3",goodHitsMod[3]);
    if (goodHitsMod[4]) m_histogrammanager->fill("good_nhits_physics_Mod4",goodHitsMod[4]);
    if (goodHitsMod[5]) m_histogrammanager->fill("good_nhits_physics_Mod5",goodHitsMod[5]);
    if (goodHitsMod[6]) m_histogrammanager->fill("good_nhits_physics_Mod6",goodHitsMod[6]);
    if (goodHitsMod[7]) m_histogrammanager->fill("good_nhits_physics_Mod7",goodHitsMod[7]);
  }
}

void SCTDataMonitorModule::register_hists() {

  INFO(" ... registering histograms in SCTDataMonitor ... " );

  m_histogrammanager->registerHistogram("payloadsize", "payload size [bytes]", 0, MAXFRAGSIZE/50, MAXFRAGSIZE/2000,  Axis::Range::EXTENDABLE, m_PUBINT*10);

  m_histogrammanager->registerHistogram("bcid", "BCID", -0.5, 3564.5, 3565, 1800);
  m_histogrammanager->registerHistogram("bcid_hit_weighted_physics", "BCID", -0.5, 3564.5, 3565, 30);
  m_histogrammanager->registerHistogram("diff_trb_sct_bcid", "TRB BCID - SCT BCID", -5, 5, 10, Axis::Range::EXTENDABLE, 120);
  m_histogrammanager->registerHistogram("diff_trb_sct_l1id", "TRB BCID - SCT L1ID", -5, 5, 10, Axis::Range::EXTENDABLE, 120);
  m_histogrammanager->registerHistogram("total_hits_multiplicity", "total_hits_multiplicity", 0, 30, 30, 30);
  m_histogrammanager->registerHistogram("good_nhits_physics", "good_nhits_physics", 0, 30, 30, Axis::Range::EXTENDABLE, m_PUBINT);
  m_histogrammanager->registerHistogram("good_nhits_physics_Mod0", "good_nhits_physics_Mod0", 1, 30, 29, m_PUBINT);
  m_histogrammanager->registerHistogram("good_nhits_physics_Mod1", "good_nhits_physics_Mod1", 1, 30, 29, m_PUBINT);
  m_histogrammanager->registerHistogram("good_nhits_physics_Mod2", "good_nhits_physics_Mod2", 1, 30, 29, m_PUBINT);
  m_histogrammanager->registerHistogram("good_nhits_physics_Mod3", "good_nhits_physics_Mod3", 1, 30, 29, m_PUBINT);
  m_histogrammanager->registerHistogram("good_nhits_physics_Mod4", "good_nhits_physics_Mod4", 1, 30, 29, m_PUBINT);
  m_histogrammanager->registerHistogram("good_nhits_physics_Mod5", "good_nhits_physics_Mod5", 1, 30, 29, m_PUBINT);
  m_histogrammanager->registerHistogram("good_nhits_physics_Mod6", "good_nhits_physics_Mod6", 1, 30, 29, m_PUBINT);
  m_histogrammanager->registerHistogram("good_nhits_physics_Mod7", "good_nhits_physics_Mod7", 1, 30, 29, m_PUBINT);
  m_histogrammanager->registerHistogram("strip_id_difference_physics_mod4to7", "strip_id_difference_mod4to7", -130, 130, 52, m_PUBINT);
  m_histogrammanager->registerHistogram("strip_id_difference_physics_mod0to3", "strip_id_difference_mod0to3", -130, 130, 52, m_PUBINT);
  m_histogrammanager->register2DHistogram("chip_occupancy_noise", "module_number",  0, 4, 4, "chip_number", 0, 24, 24, m_PUBINT);
  m_histogrammanager->register2DHistogram("chip_occupancy_physics", "module_number",  0, 4,4,"chip_number", 0, 24 , 24, m_PUBINT);
  m_histogrammanager->register2DHistogram("hitmap_physics", "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx",  0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, m_PUBINT);
  m_histogrammanager->register2DHistogram("hitmap_random", "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx",  0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, m_PUBINT);
  // assign metrics used for event normalisation for occupancy instead of counts:
  m_histogrammanager->setNormalisationMetric("chip_occupancy_physics", &m_physics_strip_count);
  m_histogrammanager->setNormalisationMetric("chip_occupancy_noise", &m_random_strip_count);
  m_histogrammanager->setNormalisationMetric("hitmap_physics", &m_physics_strip_count);
  m_histogrammanager->setNormalisationMetric("hitmap_random", &m_random_strip_count);
// per module
  std::vector<std::string> hitp_categories = { "000", "001", "010", "011", "100", "110", "111" };
  for ( unsigned i = 0; i < kTOTAL_MODULES; i++ ){
    std::string hname_hitp = m_prefix_hname_hitp+std::to_string(i);
    m_histogrammanager->registerHistogram(hname_hitp, "hit pattern", hitp_categories, m_PUBINT);
    m_histogrammanager->normaliseOnPublish(hname_hitp);
  }
  m_histogrammanager->registerHistogram("hitpattern_random", "hit pattern", hitp_categories, m_PUBINT);
  m_histogrammanager->normaliseOnPublish("hitpattern_random");

  std::vector<std::string> trb_error_categories = {"TRBError", "ModuleError", "NoEventID", "NoBCID", "NoCRC", "MissingFrames", "UnrecognizedFrames", "ModuleDecodeError"};
  m_histogrammanager->registerHistogram("track_data_error_types", "error type", trb_error_categories, m_PUBINT);

  std::vector<std::string> sct_error_categories = {"L1IDMismatch", "BCIDMismatch", "NoData", "BuffOverflow", "BuffError", "UnknownChip", "Unknown"};
  for ( unsigned i = 0; i < kTOTAL_MODULES; i++ ){
    std::string hname_mod_scterr = m_hname_scterrors+"_mod"+std::to_string(i);
    m_histogrammanager->registerHistogram(hname_mod_scterr, "error type", sct_error_categories, m_PUBINT);
  }
  m_histogrammanager->registerHistogram(m_hname_scterrors, "error type", sct_error_categories, m_PUBINT); 

  INFO(" ... done registering histograms ... " );

  return ;

}

void SCTDataMonitorModule::register_metrics() {

  INFO( "... registering metrics in SCTDataMonitorModule ... " );

  registerVariable(m_hit_multiplicity, "HitMultiplicity");
  registerVariable(m_hit_avg, "AvgHit");

  return;
}

void SCTDataMonitorModule::update_hitavg(unsigned hit){
  if ( m_hit_weight_assignment.find(hit) != m_hit_weight_assignment.end()){
    float hit_weight = m_hit_weight_assignment[hit];
    m_hit_avg = update_avg(m_hit_avg,m_hit_avg_count, hit_weight);
    m_hit_avg_count++; // increase the number of hits considered
  }
}

float SCTDataMonitorModule::update_avg(float avg, size_t size, float value){
  avg = (size*avg+value)/(size+1);
  return avg;
}

