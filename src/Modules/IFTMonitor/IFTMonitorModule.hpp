/*
  Copyright (C) 2019-2021 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"
#include <vector>

class IFTMonitorModule : public MonitorBaseModule {
 public:
  IFTMonitorModule();
  ~IFTMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

 private:
  enum class HitMode { HIT = 0, LEVEL, EDGE };
  HitMode m_hitMode = HitMode::HIT;

  const uint8_t kSTRIPDIFFTOLERANCE = 25; // FIXME can be tuned
  const uint8_t kSTRIPS_PER_CHIP = 128;
  const uint8_t kCHIPS_PER_MODULE = 12;
  const uint8_t kTOTAL_MODULES = 8;

  std::string hitmaps[3] {"hitmap_layer0", "hitmap_layer1", "hitmap_layer2"};
};
