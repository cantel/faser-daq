#pragma once

#include "Modules/Monitor/MonitorModule.hpp"

class TrackerMonitorModule : public MonitorModule {
 public:
  TrackerMonitorModule();
  ~TrackerMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
