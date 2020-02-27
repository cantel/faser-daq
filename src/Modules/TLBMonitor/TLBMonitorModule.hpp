#pragma once

#include "Modules/Monitor/MonitorModule.hpp"

class TLBMonitorModule : public MonitorModule {
 public:
  TLBMonitorModule();
  ~TLBMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
