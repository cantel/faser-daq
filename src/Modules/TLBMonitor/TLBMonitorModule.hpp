#pragma once

#include <boost/histogram.hpp>
#include <tuple>
#include <list>

#include "Modules/Monitor/MonitorModule.hpp"
#include "Commons/EventFormat.hpp"

class TLBMonitorModule : public MonitorModule {
 public:
  TLBMonitorModule();
  ~TLBMonitorModule();

  void runner();



 protected:

  void initialize_hists( );
  void register_metrics();

};
