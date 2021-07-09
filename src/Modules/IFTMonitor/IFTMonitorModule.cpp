/*
  Copyright (C) 2019-2021 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <iostream>
#include <sstream>
#include <fstream>
/// \endcond

#include "IFTMonitorModule.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;

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

    for ( auto it = m_trackerdataFragment->cbegin(); it != m_trackerdataFragment->cend(); ++it ) {
      auto sctEvent = *it;
      if (sctEvent == nullptr) { 
          WARNING("Invalid SCT Event for event " << m_trackerdataFragment->event_id());
          continue;
      }
      unsigned short moduleId = sctEvent->GetModuleID(); 
      if (moduleId >= TrackerDataFragment::MODULES_PER_FRAGMENT) {
        ERROR("Invalid module ID (" << moduleId << ") from trb " << TRBBoardID);
        continue;
      }
      if (sctEvent->BCIDMismatch()) {
        ERROR("Module data BCID mismatch between sides for online module " << moduleId);
        continue;
      }
      else if (sctEvent->HasError()) {
        ERROR("Online module " << moduleId << " reports one or more errors.");
        continue;
      }
      else if (sctEvent->MissingData()) {
        ERROR("Online module " << moduleId << " reports missing data.");
        continue;
      }
      else if (!sctEvent->IsComplete()) {
        ERROR("Online module " << moduleId << " reports not complete.");
        continue;
      }

      DEBUG("Processing online module #" << moduleId);
      auto allHits = sctEvent->GetHits();
      for (unsigned int chipIdx = 0; chipIdx < (unsigned int)kCHIPS_PER_MODULE*0.5; chipIdx++) {
        auto hitsPerChip1 = allHits[chipIdx];
        for (auto hit1 : hitsPerChip1) { // hit is an std::pair<uint8 strip, uint8 pattern>
          uint32_t hitPattern1  = hit1.second;
          if (((hitPattern1 & 0x1) == 0) && ((hitPattern1 & 0x2) == 0) && ((hitPattern1 & 0x4) == 0)) continue;
          if (m_hitMode == HitMode::EDGE && (((hitPattern1 & 0x2) == 0 ) || ((hitPattern1 & 0x4) != 0) ) ) continue; // 01X
          if (m_hitMode == HitMode::LEVEL && ((hitPattern1 & 0x2) == 0)) continue; // X1X
          auto strip1 = hit1.first;
          unsigned int chipIdx2 = kCHIPS_PER_MODULE - 1 - chipIdx;
          auto hitsPerChip2 = allHits[chipIdx2];
          for (auto hit2 : hitsPerChip2) {
            uint32_t hitPattern2  = hit2.second;
            if (((hitPattern2 & 0x1) == 0) && ((hitPattern2 & 0x2) == 0) && ((hitPattern2 & 0x4) == 0)) continue;
            if (m_hitMode == HitMode::EDGE && (((hitPattern2 & 0x2) == 0 ) || ((hitPattern2 & 0x4) != 0) ) ) continue; // 01X
            if (m_hitMode == HitMode::LEVEL && ((hitPattern2 & 0x2) == 0)) continue; // X1X
            auto strip2 = kSTRIPS_PER_CHIP-1-hit2.first; // invert
            if (std::abs(strip1-strip2) > kSTRIPDIFFTOLERANCE) continue;
            // good physics hits
            m_histogrammanager->fill2D(hitmaps[TRBBoardID], moduleId, chipIdx, 1);
            m_histogrammanager->fill2D(hitmaps[TRBBoardID], moduleId, chipIdx2, 1);
          }
        }
      }
    }
  }
}

void IFTMonitorModule::register_hists() {

  INFO(" ... registering histograms in IFTMonitor ... " );

  const unsigned kPUBINT = 30; // publishing interval in seconds

  m_histogrammanager->register2DHistogram("hitmap_layer0", "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx", 0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, kPUBINT);
  m_histogrammanager->register2DHistogram("hitmap_layer1", "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx", 0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, kPUBINT);
  m_histogrammanager->register2DHistogram("hitmap_layer2", "module idx", 0, kTOTAL_MODULES, kTOTAL_MODULES, "chip idx", 0, kCHIPS_PER_MODULE, kCHIPS_PER_MODULE, kPUBINT);

  INFO(" ... done registering histograms ... " );

  return ;

}

void IFTMonitorModule::register_metrics() {

  INFO( "... registering metrics in IFTMonitorModule ... " );
  INFO(" ... done registering metrics ... " );

  return;
}
