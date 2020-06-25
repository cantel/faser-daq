#pragma once

#include "../MonitorBase/MonitorBaseModule.hpp"

class EventMonitorModule : public MonitorBaseModule {
 public:
  EventMonitorModule();
  ~EventMonitorModule();

 protected:
 
  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_metrics();

};
