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

  std::atomic<int> m_tav0;
  std::atomic<int> m_tav1;
  std::atomic<int> m_tav2;
  std::atomic<int> m_tav3;
  std::atomic<int> m_tav4;
  std::atomic<int> m_tav5;

  std::atomic<int> m_tap0;
  std::atomic<int> m_tap1;
  std::atomic<int> m_tap2;
  std::atomic<int> m_tap3;
  std::atomic<int> m_tap4;
  std::atomic<int> m_tap5;

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
