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
  IFTMonitorModule(const std::string&);
  ~IFTMonitorModule();

 protected:

  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists( );
  void register_metrics();
  std::atomic<float> m_x;
  std::atomic<float> m_y;
  std::atomic<float> mean_x;
  std::atomic<float> mean_y;
  std::atomic<float> rms_x;
  std::atomic<float> rms_y;
  std::atomic<int> m_number_good_events = 0;

 private:
  bool adjacent(int strip1, int strip2);
  int average(std::vector<int> strips);
  double intersection(double y1, double y2);
  std::pair<Vector3, Vector3> linear_fit(const std::vector<Vector3>& spacepoints);
  double mse_fit(std::vector<Vector3> track, std::pair<Vector3, Vector3> fit);
  double mean(double* x, int n);
  double rms(double* x, int n);

  std::map<int, std::vector<Vector3>> m_spacepoints = {};
  uint32_t m_stationID = 0;
  const std::string m_hit_maps[3] = {"hitmap_l0", "hitmap_l1", "hitmap_l2"};

  double kLAYERPOS[3] = {16.2075, 47.7075, 79.2075}; // values in mm
  double kMODULEPOS[4] = {64.92386246, 1.20386696, -62.55613708, -126.25613403}; // values in mm
  int m_vec_idx = 0;
  static const int kAVGSIZE = 10000;
  double m_x_vec[kAVGSIZE] = {0};
  double m_y_vec[kAVGSIZE] = {0};
  const double kLAYER_OFFSET[3] = {0, -5, 5}; // in mm
  const double kSTRIP_PITCH = 0.08; // in mm
  const double kXMIN = -63.96; // in mm
  const double kXMAX = 63.96; // in mm
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
  const uint8_t kSTRIPDIFFTOLERANCE = 32; // arctan(0.04) * 63.96mm / 0.08mm = 32
  const uint8_t kSTRIPS_PER_CHIP = 128;
  const uint8_t kCHIPS_PER_MODULE = 12;
  const uint8_t kTOTAL_MODULES = 8;
  const uint8_t kBCIDOFFSET = 4;
  const uint8_t kMAXWARNINGS = 50;
};
