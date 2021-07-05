/*
  Copyright (C) 2019-2021 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>
/// \endcond

#include "IFTMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

IFTMonitorModule::IFTMonitorModule()
    : m_prefix_hname_hitp("hitpattern_mod"), m_prefix_hname_scterr("sct_data_error_types_mod") {}

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

  for (int TRBBoardID = 0; TRBBoardID < 3; ++TRBBoardID) {
    auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary, SourceIDs::TrackerSourceID + TRBBoardID);
    if (fragmentUnpackStatus) {
      fill_error_status_to_metric(fragmentUnpackStatus);
      return;
    }

    fill_error_status_to_metric(m_fragment->status());

    const TrackerDataFragment* trackerFragment = m_trackerdataFragment;
    if (trackerFragment->valid()) {
      WARNING("Invalid tracker data fragment for TRB " << TRBBoardID);
    }

    for ( auto it = m_trackerdataFragment->cbegin(); it != trackerFragment->cend(); ++it ) {
      auto sctEvent = *it;
      if (sctEvent == nullptr) { 
          WARNING("Invalid SCT Event for event " << m_trackerdataFragment->event_id());
          continue;
      }
      unsigned short onlineModuleID = sctEvent->GetModuleID(); 
      // check for errors
      if (onlineModuleID >= TrackerDataFragment::MODULES_PER_FRAGMENT) {
        ERROR("Invalid module ID (" << onlineModuleID << ") from trb " << TRBBoardID);
        continue;
      }
      if (sctEvent->BCIDMismatch()) {
        ERROR("Module data BCID mismatch between sides for online module " << onlineModuleID);
        continue;
      }
      else if (sctEvent->HasError()) {
        ERROR("Online module " << onlineModuleID << " reports one or more errors.");
        continue;
      }
      else if (sctEvent->MissingData()) {
        ERROR("Online module " << onlineModuleID << " reports missing data.");
        continue;
      }
      else if (!sctEvent->IsComplete()) {
        ERROR("Online module " << onlineModuleID << " reports not complete.");
        continue;
      }

      DEBUG("Processing online module #" << onlineModuleID);
      std::vector<uint32_t> m_moduleMap {};
      uint32_t module = m_moduleMap[onlineModuleID];
      size_t chipIndex{0};
      for (auto hitVector : sctEvent->GetHits()) {
        if (hitVector.size() > 0) {
          int side = chipIndex / TrackerDataFragment::CHIPS_PER_SIDE;
          // uint32_t chipOnSide = chipIndex % TrackerDataFragment::CHIPS_PER_SIDE;
          for (auto hit : hitVector) {
            goodHitsLayer[TRBBoardID]++;
            uint32_t stripOnChip = hit.first;
            if (stripOnChip >= TrackerDataFragment::STRIPS_PER_CHIP) {
              ERROR("Invalid strip number on chip: " << stripOnChip );
              continue;
            }
            int phiModule = module % 4; // 0 to 3 from bottom to top
            int etaModule = -1 + 2*((module%2 + module/4) % 2); // -1 or +1
            DEBUG("layer:" << TRBBoardID << ", side:" << side 
                  << ", row: " << etaModule << ", column: " << phiModule);
          }
        }
        chipIndex++;
      }
    }
  }

  for (auto hits : goodHitsLayer) 
    goodHits += hits;
  m_histogrammanager->fill("good_hits_multiplicity", goodHits);
  if (goodHitsLayer[0]) m_histogrammanager->fill("good_hits_multiplicity_Layer0",goodHitsLayer[0]);
  if (goodHitsLayer[1]) m_histogrammanager->fill("good_hits_multiplicity_Layer1",goodHitsLayer[1]);
  if (goodHitsLayer[2]) m_histogrammanager->fill("good_hits_multiplicity_Layer2",goodHitsLayer[2]);
}

void IFTMonitorModule::register_hists() {

  INFO(" ... registering histograms in IFTMonitor ... " );

  const unsigned kPUBINT = 30; // publishing interval in seconds

  m_histogrammanager->registerHistogram("good_hits_multiplicity", "good_hits_multiplicity", 0, 30, 30, Axis::Range::EXTENDABLE, kPUBINT);
  m_histogrammanager->registerHistogram("good_hits_multiplicity_Layer0", "good_hits_multiplicity_Layer0", 1, 30, 29, kPUBINT);
  m_histogrammanager->registerHistogram("good_hits_multiplicity_Layer1", "good_hits_multiplicity_Layer1", 1, 30, 29, kPUBINT);
  m_histogrammanager->registerHistogram("good_hits_multiplicity_Layer2", "good_hits_multiplicity_Layer2", 1, 30, 29, kPUBINT);

  INFO(" ... done registering histograms ... " );

  return ;

}

void IFTMonitorModule::register_metrics() {

  INFO( "... registering metrics in IFTMonitorModule ... " );

  return;
}
