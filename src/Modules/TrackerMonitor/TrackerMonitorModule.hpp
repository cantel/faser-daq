/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class TrackerMonitorModule : public MonitorBaseModule {
 public:
  TrackerMonitorModule();
  ~TrackerMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

 private:

  uint16_t m_bcid;
  uint16_t number;
  uint16_t goodHits;
  uint16_t goodHitsMod[8];
  uint16_t module;
  uint16_t mapline;
  uint16_t mapline2;
  const std::string m_prefix_hname_hitp;
  const uint8_t kSTRIPDIFFTOLERANCE = 25; // FIXME can be tuned
  const uint8_t kSTRIPS_PER_CHIP = 128;
  const uint8_t kCHIPS_PER_MODULE = 12;
  const uint8_t kTOTAL_MODULES = 8;
  const uint16_t MAP[96][2] = {{3,18},{2,17},{3,6},{2,5},{0,18},{1,17},{0,6},{1,5},{3,19},{2,16},{3,7},{2,4},{0,19},{1,16},{0,7},{1,4},{3,20},{2,15},{3,8},{2,3},{0,20},{1,15},{0,8},{1,3},{3,21},{2,14},{3,9},{2,2},{0,21},{1,14},{0,9},{1,2},{3,22},{2,13},{3,10},{2,1},{0,22},{1,13},{0,10},{1,1},{3,23},{2,12},{3,11},{2,0},{0,23},{1,12},{0,11},{1,0},{2,23},{3,12},{2,11},{3,0},{1,23},{0,12},{1,11},{0,0},{2,22},{3,13},{2,10},{3,1},{1,22},{0,13},{1,10},{0,1},{2,21},{3,14},{2,9},{3,2},{1,21},{0,14},{1,9},{0,2},{2,20},{3,15},{2,8},{3,3},{1,20},{0,15},{1,8},{0,3},{2,19},{3,16},{2,7},{3,4},{1,19},{0,16},{1,7},{0,4},{2,18},{3,17},{2,6},{3,5},{1,18},{0,17},{1,6},{0,5}};
};
