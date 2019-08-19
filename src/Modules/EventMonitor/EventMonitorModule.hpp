#pragma once

#include "../Monitor/MonitorModule.hpp"

class EventMonitorModule : public MonitorModule {
 public:
  EventMonitorModule();
  ~EventMonitorModule();

  void runner();


 protected:

  void register_metrics();

};
