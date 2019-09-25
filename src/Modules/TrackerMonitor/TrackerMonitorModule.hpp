#pragma once

#include "Modules/Monitor/MonitorModule.hpp"

class TrackerMonitorModule : public MonitorModule {
 public:
  TrackerMonitorModule();
  ~TrackerMonitorModule();

  void runner();



 protected:

  void register_hists( );
  void register_metrics();

};
