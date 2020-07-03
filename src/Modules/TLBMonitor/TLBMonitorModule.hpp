#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class TLBMonitorModule : public MonitorBaseModule {
 public:
  TLBMonitorModule();
  ~TLBMonitorModule();

 protected:

  //trigger counts
  std::atomic<int> m_tbp0, m_tbp1, m_tbp2, m_tbp3, m_tbp4;
  std::atomic<int> m_tap0, m_tap1, m_tap2, m_tap3, m_tap4;
  std::atomic<int> m_tav0, m_tav1, m_tav2, m_tav3, m_tav4;

  //tav counts
  std::atomic<int> m_metric_event_id;
  std::atomic<int> m_deadtime_veto;
  std::atomic<int> m_busy_veto;
  std::atomic<int> m_rate_limiter_veto;
  std::atomic<int> m_bcr_veto;

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
