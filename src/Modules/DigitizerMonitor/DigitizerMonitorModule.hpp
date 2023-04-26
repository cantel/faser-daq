/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

#include <cmath>
#include <cstdint>
#define NCHANNELS 16
#define THRESHOLDS 4

class DigitizerMonitorModule : public MonitorBaseModule {
 public:
  DigitizerMonitorModule(const std::string&);
  ~DigitizerMonitorModule();
  void start(unsigned int);

  float GetPedestalMean(std::vector<uint16_t> input, int start, int end);
  float GetPedestalRMS(std::vector<uint16_t> input, int start, int end);
  void CheckBounds(std::vector<uint16_t> input, int& start, int& end);

  float FFTPhase(const std::vector<float>& data);

  void FillChannelPulse(std::string histogram_name, std::vector<float>& values);
  float m_display_thresh;
 protected:

  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists();
  void register_metrics();
  float m_thresholds[NCHANNELS][THRESHOLDS];
  int m_intime[8];
  int m_late[8];
  int m_early[8];
  std::atomic<float> m_avg[NCHANNELS];
  std::atomic<float> m_rms[NCHANNELS];
  std::atomic<float> m_t0[NCHANNELS];
  std::atomic<float> m_lateTrig[8];
  std::atomic<float> m_earlyTrig[8];
  std::atomic<int> m_thresh_counts[NCHANNELS][THRESHOLDS];
  std::atomic<int> m_collisionLike;
  std::atomic<int> m_saturatedCollisions;

  nlohmann::json m_cfg_min_collisions;
  nlohmann::json m_cfg_nominal_t0;
};
