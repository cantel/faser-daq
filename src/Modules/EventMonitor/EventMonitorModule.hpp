#pragma once

#include "../Monitor/MonitorModule.hpp"

class EventMonitorModule : public MonitorModule {
 public:
  EventMonitorModule();
  ~EventMonitorModule();

 protected:
 
  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_metrics();

};
