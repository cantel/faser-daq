/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

#include <cmath>

class DigitizerNoiseMonitorModule : public MonitorBaseModule {
 public:
  DigitizerNoiseMonitorModule(const std::string&);
  ~DigitizerNoiseMonitorModule();

  void GetPedestalMeanRMS(std::vector<uint16_t> input, float &mean, float &rms);
  int CountPeaks(std::vector<uint16_t> input, float mean, float threshold);
  
  std::atomic<float> m_metric_pedestal[16];
  std::atomic<float> m_metric_rms[16];
  std::atomic<double> m_metric_peaks[4][16];

 protected:

  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists();
  void register_metrics();

};
