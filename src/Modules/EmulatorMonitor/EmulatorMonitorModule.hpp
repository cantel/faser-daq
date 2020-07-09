#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class EmulatorMonitorModule : public MonitorBaseModule {
 public:
  EmulatorMonitorModule();
  ~EmulatorMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
