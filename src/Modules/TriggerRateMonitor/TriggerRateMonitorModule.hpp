#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class TriggerRateMonitorModule : public MonitorBaseModule {
 public:
  TriggerRateMonitorModule();
  ~TriggerRateMonitorModule();

 protected:

  uint32_t m_triggered_events;
  uint32_t m_previous_evt_cnt;
  uint32_t m_ECR_cnt;

  //trigger counts
  std::atomic<int> m_tbp0, m_tbp1, m_tbp2, m_tbp3, m_tbp4;
  std::atomic<int> m_tap0, m_tap1, m_tap2, m_tap3, m_tap4;
  std::atomic<int> m_tav0, m_tav1, m_tav2, m_tav3, m_tav4;

  //veto counts
  std::atomic<int> m_metric_event_id;
  std::atomic<int> m_deadtime_veto;
  std::atomic<int> m_busy_veto;
  std::atomic<int> m_rate_limiter_veto;
  std::atomic<int> m_bcr_veto;
  std::atomic<int> m_digi_busy_veto;
  std::atomic<float> m_deadtime_fraction;
  std::atomic<float> m_busy_fraction;
  std::atomic<float> m_rate_limiter_fraction;
  std::atomic<float> m_bcr_fraction;
  std::atomic<float> m_digi_busy_fraction;

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
