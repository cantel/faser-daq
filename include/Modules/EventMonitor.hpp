#ifndef EVENTMONITOR_H_
#define EVENTMONITOR_H_

#include "Modules/Monitor.hpp"

class EventMonitor : public Monitor {
 public:
  EventMonitor();
  ~EventMonitor();

  void runner();



 protected:

  void register_metrics();

};

#endif /* EVENTMONITOR_H_ */
