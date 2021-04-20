/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

#include <cmath>
#define NCHANNELS 16

class DigitizerMonitorModule : public MonitorBaseModule {
 public:
  DigitizerMonitorModule();
  ~DigitizerMonitorModule();

  float GetPedestalMean(std::vector<uint16_t> input, int start, int end);
  float GetPedestalRMS(std::vector<uint16_t> input, int start, int end);
  void CheckBounds(std::vector<uint16_t> input, int& start, int& end);
  
  void FillChannelPulse(std::string histogram_name, int channel);
  float m_display_thresh;
 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists();
  void register_metrics();
  std::atomic<float> m_avg[NCHANNELS];
  std::atomic<float> m_rms[NCHANNELS];


};
