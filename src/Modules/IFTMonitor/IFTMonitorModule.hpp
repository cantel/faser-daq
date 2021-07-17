/*
  Copyright (C) 2019-2021 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"
#include <Eigen/Dense>
#include <vector>

typedef Eigen::Matrix<double, 3, 1> Vector3;


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

  struct EventInfo {
    int event;
    double x;
    double y;
    double z;
    double phi1;
    double phi2;
  };

  bool adjacent(int strip1, int strip2);
  int average(std::vector<int> strips);
  double intersection(double y1, double y2);
  std::pair<Vector3, Vector3> linear_fit(const std::vector<Vector3>& spacepoints);
  double mse_fit(std::vector<Vector3> track, std::pair<Vector3, Vector3> fit);

  std::map<int, std::vector<Vector3>> m_spacepoints = {};
  std::vector<EventInfo> m_eventInfo = {};
  std::vector<SpacePoint> m_spacepointsList = {};

  const std::string m_hit_maps[3] = {"hitmap_l0", "hitmap_l1", "hitmap_l2"};
  double kLAYERPOS[3] = {16.2075, 47.7075, 79.2075}; // values in mm
  double kMODULEPOS[4] = {64.92386246, 1.20386696, -62.55613708, -126.25613403}; // values in mm
  const double kLAYER_OFFSET[3] = {0, -5, 5}; // in mm
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
