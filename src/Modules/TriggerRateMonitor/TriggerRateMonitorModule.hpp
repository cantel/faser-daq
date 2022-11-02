/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"
#define MAX_TRIG_ITEMS 6

class TriggerRateMonitorModule : public MonitorBaseModule {
 public:
  TriggerRateMonitorModule(const std::string&);
  ~TriggerRateMonitorModule();
  void start(unsigned int);

 protected:

  uint32_t m_total_unvetoed_events;
  uint32_t m_previous_evt_cnt;
  uint32_t m_ECR_cnt;

  //trigger counts
  std::atomic<int> m_tapORed, m_tavORed;
  std::atomic<int> m_tbp0, m_tbp1, m_tbp2, m_tbp3, m_tbp4, m_tbp5;
  std::atomic<int> m_tap0, m_tap1, m_tap2, m_tap3, m_tap4, m_tap5;
  std::atomic<int> m_tav0, m_tav1, m_tav2, m_tav3, m_tav4, m_tav5;
  // per trig item deadtime
  std::atomic<float> m_tav0_deadtime, m_tav1_deadtime, m_tav2_deadtime, m_tav3_deadtime, m_tav4_deadtime, m_tav5_deadtime;

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
  std::atomic<float> m_global_deadtime_fraction;
  std::atomic<int> m_total_orbits_lost;

  // pointer arrays of trigitem metrics
  std::array<std::atomic<float>*,MAX_TRIG_ITEMS> m_trigitem_deadtime_metrics = {&m_tav0_deadtime,&m_tav1_deadtime,&m_tav2_deadtime,&m_tav3_deadtime,&m_tav4_deadtime,&m_tav5_deadtime};
  std::array<std::atomic<int>*,MAX_TRIG_ITEMS> m_tbp_metrics = {&m_tbp0,&m_tbp1,&m_tbp2,&m_tbp3,&m_tbp4,&m_tbp5};
  std::array<std::atomic<int>*,MAX_TRIG_ITEMS> m_tap_metrics = {&m_tap0,&m_tap1,&m_tap2,&m_tap3,&m_tap4,&m_tap5};
  std::array<std::atomic<int>*,MAX_TRIG_ITEMS> m_tav_metrics = {&m_tav0,&m_tav1,&m_tav2,&m_tav3,&m_tav4,&m_tav5};

  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

 private:
     void update_trigitem_mon(uint8_t);

};
