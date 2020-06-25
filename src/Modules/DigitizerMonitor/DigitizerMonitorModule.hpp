#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class DigitizerMonitorModule : public MonitorBaseModule {
 public:
  DigitizerMonitorModule();
  ~DigitizerMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists();
  void register_metrics();

};
