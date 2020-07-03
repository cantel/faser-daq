#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class EmulatedDataMonitorModule : public MonitorBaseModule {
 public:
  EmulatedDataMonitorModule();
  ~EmulatedDataMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
