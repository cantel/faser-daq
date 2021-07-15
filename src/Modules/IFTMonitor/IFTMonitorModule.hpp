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
  struct SpacePoint {
    int event;
    int layer;
    double x;
    double y;
  };

  bool adjacent(unsigned int strip1, unsigned int strip2);
  int average(std::vector<int> strips);
  double intersection(double y1, double y2);

  std::vector<SpacePoint> m_spacePoints = {};

  const std::string m_hit_maps[3] = {"hitmap_l0", "hitmap_l1", "hitmap_l2"};
  double kMODULEPOS[4] = {65.02388382, 1.34389615, -62.55613708, -126.21612549}; // values in mm
  const double kSTRIP_PITCH = 0.08; // in mm
  const double kXMIN = -63.04; // in mm
  const double kXMAX = 63.04; // in mm
  const double kSTRIP_LENGTH = 126.08; // in mm
  const double kSTRIP_ANGLE = 0.04; // in radian
  int m_eventId = 0;
  uint16_t m_bcid;
  uint32_t m_l1id;
  uint16_t number;
  uint16_t module;
  unsigned m_total_WARNINGS;
  bool m_print_WARNINGS;
  const uint8_t kTRB_BOARDS = 3;
  const uint8_t kSTRIPDIFFTOLERANCE = 25;
  const uint8_t kSTRIPS_PER_CHIP = 128;
  const uint8_t kCHIPS_PER_MODULE = 12;
  const uint8_t kTOTAL_MODULES = 8;
  const uint8_t kBCIDOFFSET = 4;
  const uint8_t kMAXWARNINGS = 50;
};
