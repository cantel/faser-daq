#ifndef TRACKERMONITOR_H_
#define TRACKERMONITOR_H_

#include "Modules/Monitor.hpp"

class TrackerMonitor : public Monitor {
 public:
  TrackerMonitor();
  ~TrackerMonitor();

  void runner();



 protected:

  void register_hists( );
  void register_metrics();

};

#endif /* TRACKERMONITOR_H_ */
