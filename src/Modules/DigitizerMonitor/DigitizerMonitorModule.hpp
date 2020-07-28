#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class DigitizerMonitorModule : public MonitorBaseModule {
 public:
  DigitizerMonitorModule();
  ~DigitizerMonitorModule();

  float GetMean(std::vector<uint16_t> input, int start, int end);
  float GetRMS(std::vector<uint16_t> input, int start, int end);

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists();
  void register_metrics();

};
