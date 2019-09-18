#ifndef TLBMONITOR_H_
#define TLBMONITOR_H_

#include <boost/histogram.hpp>
#include <tuple>
#include "Modules/Monitor.hpp"
#include <nlohmann/json.hpp>
#include <list>

#include "Modules/EventFormat.hpp"

class TLBMonitor : public Monitor {
 public:
  TLBMonitor();
  ~TLBMonitor();

  void runner();



 protected:

  void register_hists( );
  void register_metrics();

};

#endif /* TLBMONITOR_H_ */
