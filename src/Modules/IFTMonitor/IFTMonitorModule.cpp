/*
  Copyright (C) 2019-2021 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
#include <math.h>
#include <numeric>
/// \endcond

#include "IFTMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;


bool IFTMonitorModule::adjacent(unsigned int strip1, unsigned int strip2) {
  return ((strip2 - strip1 == 1) or (strip1 - strip2 == 1));
}

int IFTMonitorModule::average(std::vector<int> strips) {
  size_t n = strips.size();
  return n != 0 ? (int)std::accumulate(strips.begin(), strips.end(), 0.0) / n : 0;
}

double IFTMonitorModule::intersection(double y1, double y2) {
  // calculate line line intersection given two points on each line (top and bottom of each strip)
  // https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
  double det = -kSTRIP_LENGTH * kSTRIP_LENGTH * tan(kSTRIP_ANGLE);
  double ix = kSTRIP_LENGTH * kSTRIP_LENGTH * y1 + kSTRIP_LENGTH * kXMIN * (2*y2 + kSTRIP_LENGTH * tan(kSTRIP_ANGLE));
  return det != 0 ? ix/det : 0;
}



IFTMonitorModule::IFTMonitorModule() {}

IFTMonitorModule::~IFTMonitorModule() { 
  INFO("With config: " << m_config.dump());
}

void IFTMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  if (m_event->event_tag() != m_eventTag) {
    ERROR("Event tag does not match filter tag. Are the module's filter settings correct?");
    return;
  }

  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary, SourceIDs::TriggerSourceID);
  if (fragmentUnpackStatus) {
    fill_error_status_to_metric(fragmentUnpackStatus);
    return;
  }

  if (m_tlbdataFragment->tbp() & 0x10) {
    WARNING("Skipping random trigger data");
    return;
  }


  for (int TRBBoardId=0; TRBBoardId < kTRB_BOARDS; TRBBoardId++) {
    fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary, SourceIDs::TrackerSourceID + TRBBoardId);
    if (fragmentUnpackStatus) {
      fill_error_status_to_metric(fragmentUnpackStatus);
      return;
    }

    fill_error_status_to_metric(m_fragment->status());
  
    const TrackerDataFragment* trackerdataFragment = m_trackerdataFragment;

    if (TRBBoardId == 0)
      DEBUG("event " << m_trackerdataFragment->event_id());
    
    size_t payload_size = m_fragment->payload_size();
    if (payload_size > MAXFRAGSIZE) {
       WARNING(" VERY large payload size received. Payload size of " << payload_size<<" bytes exceeds maximum allowable for histogram filling. Resetting to "<<MAXFRAGSIZE);
       payload_size = MAXFRAGSIZE;
    } 

    if (trackerdataFragment->valid()){
      m_bcid = trackerdataFragment->bc_id();
      m_l1id = trackerdataFragment->event_id();
    }
    else {
      WARNING("Ignoring corrupted data fragment.");
      return;
    }

    m_print_WARNINGS = m_total_WARNINGS < kMAXWARNINGS;

    for (auto it = trackerdataFragment->cbegin(); it != trackerdataFragment->cend(); ++it) {
      auto sctEvent = *it;
      if (sctEvent == nullptr) {
        WARNING("Invalid SCT Event for event " << trackerdataFragment->event_id());
        WARNING("tracker data fragment: " << *trackerdataFragment);
        m_status = STATUS_WARN;
        continue;
      }

      if (sctEvent->HasError()) {
        auto sctErrorList= sctEvent->GetErrors();
        for (unsigned idx = 0; idx < 12; idx++) {
          auto sctErrors = sctErrorList.at(idx);
          for (uint8_t sctError : sctErrors) {
            WARNING("SCT Error for module "<<sctEvent->GetModuleID()<<", chip idx "<<idx<<". Error code = "<<static_cast<int>(sctError));
          }
        }
      };

      int diff_bcid = (m_bcid-sctEvent->GetBCID())&0xFF;
      int diff_l1id = (m_l1id-sctEvent->GetL1ID())&0xF;
      if (diff_bcid != kBCIDOFFSET) {
        if (m_print_WARNINGS) WARNING("BCID mismatch for module "<<sctEvent->GetModuleID()<<". TRB BCID = "<<m_bcid<<", SCT BCID = "<<sctEvent->GetBCID());
        m_total_WARNINGS++;
      }
      if (diff_l1id != 0) {
        if (m_print_WARNINGS) WARNING("L1ID mismatch for module "<<sctEvent->GetModuleID()<<". TRB BCID = "<<m_l1id<<", SCT L1ID = "<<sctEvent->GetL1ID());
        m_total_WARNINGS++;
      }



      int module = sctEvent->GetModuleID();

      // combine adjacent strips to a cluster
      std::vector<int> clustersPerChip = {};
      std::vector<int> currentCluster = {};
      int previousStrip = 0;
      int currentStrip;
      auto allHits = sctEvent->GetHits();
      std::map<unsigned, std::vector<int>> allClusters;
      for (unsigned chipIdx = 0; chipIdx < kCHIPS_PER_MODULE; chipIdx++) {
        auto hitsPerChip = allHits[chipIdx];
        previousStrip = 0;
        for (auto hit : hitsPerChip) {
          if (hit.second == 7) continue;
          currentStrip = hit.first;
          if ((not adjacent(previousStrip, currentStrip)) and (not currentCluster.empty())) {
            clustersPerChip.push_back(average(currentCluster));
            currentCluster.clear();
          }
          currentCluster.push_back(currentStrip);
          previousStrip = currentStrip;
        }
        if (not currentCluster.empty()) clustersPerChip.push_back(average(currentCluster));
        if (not clustersPerChip.empty()) allClusters[chipIdx] = clustersPerChip;
        currentCluster.clear();
        clustersPerChip.clear();
      }


      for (unsigned chipIdx = 0; chipIdx < kCHIPS_PER_MODULE; chipIdx++) {
        if (allClusters[chipIdx].empty()) continue;
        auto clustersPerChip = allClusters[chipIdx];
        for (auto cluster : clustersPerChip) {
          DEBUG("m " << module << ", c " << chipIdx << ", cluster" << cluster);
        }
      }

//      module = sctEvent->GetModuleID();
//      std::vector<int> cluster1 {};
//      std::vector<int> cluster2 {};
//      for ( unsigned chipIdx = 0; chipIdx < (unsigned)kCHIPS_PER_MODULE*0.5; chipIdx++) {
//        auto hitsPerChip1 = allHits[chipIdx];
//        for (auto hit1 : hitsPerChip1) { // hit is an std::pair<uint8 strip, uint8 pattern>
//          if (hit1.second == 7) continue;
//          int strip1 = hit1.first;
//          unsigned int chipIdx2 = kCHIPS_PER_MODULE - 1 - chipIdx;
//          auto hitsPerChip2 = allHits[chipIdx2];
//          for (auto hit2 : hitsPerChip2) { // hit is an std::pair<uint8 strip, uint8 pattern>
//            if (hit2.second == 7) continue;
//            int strip2 = kSTRIPS_PER_CHIP-1-hit2.first; // invert
//
//            // good physics hits
//            if (std::abs(strip1-strip2) > kSTRIPDIFFTOLERANCE) continue;
//            m_histogrammanager->fill2D(m_hit_maps[TRBBoardId], module, chipIdx, 1);
//            m_histogrammanager->fill2D(m_hit_maps[TRBBoardId], module, chipIdx2, 1);
//
//            chipIdx2 = chipIdx2 % 6;
//
//            // every second moudle is flipped
//            if (module % 2 == 0) {
//              chipIdx2 = 5 - chipIdx2;
//            } else {
//              chipIdx = 5 - chipIdx;
//            }
//
//            double y1 = kMODULEPOS[module % 4] + (chipIdx * kSTRIPS_PER_CHIP + strip1) * kSTRIP_PITCH;
//            double y2 = kMODULEPOS[module % 4] + (chipIdx2 * kSTRIPS_PER_CHIP + strip2) * kSTRIP_PITCH;
//            double py = 0.5 * (y1 + y2);
//            double px = intersection(y1, y2);
//
//            px = module % 2 == 0 ? 2 * kXMIN + px : 2 * kXMAX - px;
//            DEBUG("x=" << px << ", y=" << py);
//          }
//        }
//      }
    }
  }
}

void IFTMonitorModule::register_hists() {
  INFO(" ... registering histograms in TrackerMonitor ... " );
  const unsigned kPUBINT = 30; // publishing interval in seconds
  for ( const auto& hit_map : m_hit_maps)
    m_histogrammanager->register2DHistogram(hit_map, "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx",  0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, kPUBINT);
  INFO(" ... done registering histograms ... " );
}

void IFTMonitorModule::register_metrics() {
  INFO( "... registering metrics in TrackerMonitorModule ... " );
  register_error_metrics();
}
