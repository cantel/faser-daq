#pragma once

#include "Modules/Monitor/MonitorModule.hpp"

class TLBMonitorModule : public MonitorModule {
 public:
  TLBMonitorModule();
  ~TLBMonitorModule();

  void runner();



 protected:

  void register_hists( );
  void register_metrics();

};
