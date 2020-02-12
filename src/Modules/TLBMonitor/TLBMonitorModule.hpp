#pragma once

#include "Modules/Monitor/MonitorModule.hpp"

class TLBMonitorModule : public MonitorModule {
 public:
  TLBMonitorModule();
  ~TLBMonitorModule();

 protected:

  std::atomic<int> m_metric_event_id;
  std::atomic<int> m_deadtime_veto_counter;
  std::atomic<int> m_busy_veto_counter;
  std::atomic<int> m_rate_limiter_veto_counter;
  std::atomic<int> m_bcr_veto_counter;


  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
