#pragma once

#include "Modules/Monitor/MonitorModule.hpp"

class DigitizerMonitorModule : public MonitorModule {
 public:
  DigitizerMonitorModule();
  ~DigitizerMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
