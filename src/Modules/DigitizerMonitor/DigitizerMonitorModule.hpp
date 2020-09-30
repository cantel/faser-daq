#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

#include <cmath>

class DigitizerMonitorModule : public MonitorBaseModule {
 public:
  DigitizerMonitorModule();
  ~DigitizerMonitorModule();

  float GetPedestalMean(std::vector<uint16_t> input, int start, int end);
  float GetPedestalRMS(std::vector<uint16_t> input, int start, int end);
  void CheckBounds(std::vector<uint16_t> input, int& start, int& end);
  
  void FillChannelPulse(std::string histogram_name, int channel);
  int m_pulse_sample_space;

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists();
  void register_metrics();

};
