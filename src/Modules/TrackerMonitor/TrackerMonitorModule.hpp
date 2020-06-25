#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class TrackerMonitorModule : public MonitorBaseModule {
 public:
  TrackerMonitorModule();
  ~TrackerMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
