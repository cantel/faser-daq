/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/// \cond
#include <chrono>
#include <map>
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream
/// \endcond

#include "TriggerRateMonitorModule.hpp"

#define MAX_TRIG_LINES 5
#define MASK_ECR 0xFFFFFFFFFF000000
#define _PERCENT 100

using namespace std::chrono_literals;
using namespace std::chrono;

TriggerRateMonitorModule::TriggerRateMonitorModule() { 

   INFO("");
   m_ECR_cnt = 0;
   m_triggered_events = 0;
 }

TriggerRateMonitorModule::~TriggerRateMonitorModule() { 
  INFO("With config: " << m_config.dump());
 }

void TriggerRateMonitorModule::monitor(daqling::utilities::Binary &eventBuilderBinary) {

  auto evtHeaderUnpackStatus = unpack_event_header(eventBuilderBinary);
  if (evtHeaderUnpackStatus) return;

  //auto fragmentUnpackStatus = unpack_fragment_header(eventBuilderBinary); // if only monitoring information in header.
  auto fragmentUnpackStatus = unpack_full_fragment(eventBuilderBinary);
  if ( fragmentUnpackStatus ) {
    fill_error_status_to_metric( fragmentUnpackStatus );
    return;
  }
  // m_tlbmonitoringFragment should now be filled

  uint32_t fragmentStatus = m_fragment->status();
  fill_error_status_to_metric( fragmentStatus );
  
  if (!m_tlbmonitoringFragment->valid()) {
    WARNING("Skipping invalid tlb monitoring fragment\n"<<std::dec<<*m_tlbmonitoringFragment);
    fill_error_status_to_metric(fragmentUnpackStatus);
    return;
  }

  // get veto cnts for later use
  uint32_t deadtime_veto_cnt = m_tlbmonitoringFragment->deadtime_veto_counter();
  uint32_t busy_veto_cnt = m_tlbmonitoringFragment->busy_veto_counter();
  uint32_t rate_limiter_veto_cnt = m_tlbmonitoringFragment->rate_limiter_veto_counter();
  uint32_t bcr_veto_cnt = m_tlbmonitoringFragment->bcr_veto_counter();
  uint32_t digi_busy_cnt = m_tlbmonitoringFragment->digitizer_busy_counter();

  unsigned ECR_cnt = (m_fragment->event_id()&MASK_ECR)>>24;
  if ( ECR_cnt > m_ECR_cnt){
    m_previous_evt_cnt = 0;
    m_ECR_cnt = ECR_cnt;
  }
  m_triggered_events = (m_tlbmonitoringFragment->event_id() - m_previous_evt_cnt);
  m_triggered_events += deadtime_veto_cnt + busy_veto_cnt + rate_limiter_veto_cnt + bcr_veto_cnt + digi_busy_cnt;
  m_previous_evt_cnt = m_tlbmonitoringFragment->event_id();

  // trigger counts
  // --- 
  m_tbp0 += m_tlbmonitoringFragment->tbp(0);
  m_tbp1 += m_tlbmonitoringFragment->tbp(1);
  m_tbp2 += m_tlbmonitoringFragment->tbp(2);
  m_tbp3 += m_tlbmonitoringFragment->tbp(3);
  m_tbp4 += m_tlbmonitoringFragment->tbp(4);
  // --- 
  m_tap0 += m_tlbmonitoringFragment->tap(0);
  m_tap1 += m_tlbmonitoringFragment->tap(1);
  m_tap2 += m_tlbmonitoringFragment->tap(2);
  m_tap3 += m_tlbmonitoringFragment->tap(3);
  m_tap4 += m_tlbmonitoringFragment->tap(4);
  // -- 
  m_tav0 += m_tlbmonitoringFragment->tav(0);
  m_tav1 += m_tlbmonitoringFragment->tav(1);
  m_tav2 += m_tlbmonitoringFragment->tav(2);
  m_tav3 += m_tlbmonitoringFragment->tav(3);
  m_tav4 += m_tlbmonitoringFragment->tav(4);

  for ( uint8_t i = 0; i < MAX_TRIG_LINES; i++ ){
    m_histogrammanager->fill("tlb_tbp_counts", i, float(m_tlbmonitoringFragment->tbp(i)));
    m_histogrammanager->fill("tlb_tap_counts", i, float(m_tlbmonitoringFragment->tap(i)));
    m_histogrammanager->fill("tlb_tav_counts", i, float(m_tlbmonitoringFragment->tav(i)));
  };

 // veto counters
  m_deadtime_veto += deadtime_veto_cnt;
  m_busy_veto += busy_veto_cnt;
  m_rate_limiter_veto += rate_limiter_veto_cnt;
  m_bcr_veto += bcr_veto_cnt;
  m_digi_busy_veto += digi_busy_cnt;
 // veto fractions
  if (m_triggered_events){
    m_deadtime_fraction = (float)_PERCENT*deadtime_veto_cnt/m_triggered_events;
    m_busy_fraction = (float)_PERCENT*busy_veto_cnt/m_triggered_events;
    m_rate_limiter_fraction = (float)_PERCENT*rate_limiter_veto_cnt/m_triggered_events;
    m_bcr_fraction = (float)_PERCENT*bcr_veto_cnt/m_triggered_events;
    m_digi_busy_fraction = (float)_PERCENT*digi_busy_cnt/m_triggered_events;
  }
  m_histogrammanager->fill("tlb_veto_counts", "SimpleDeadtime", deadtime_veto_cnt);
  m_histogrammanager->fill("tlb_veto_counts", "TrackerBusy", busy_veto_cnt);
  m_histogrammanager->fill("tlb_veto_counts", "RateLimiter", rate_limiter_veto_cnt);
  m_histogrammanager->fill("tlb_veto_counts", "BCR", bcr_veto_cnt);
  m_histogrammanager->fill("tlb_veto_counts", "DigiBusy", digi_busy_cnt);

}

void TriggerRateMonitorModule::register_hists() {

  INFO(" ... registering histograms in TriggerRateMonitor ... " );
 
  // trigger counts 
  m_histogrammanager->registerHistogram("tlb_tbp_counts", "TBP idx", 0, 4, 4 );
  m_histogrammanager->registerHistogram("tlb_tap_counts", "TAP idx", 0, 4, 4 );
  m_histogrammanager->registerHistogram("tlb_tav_counts", "TAV idx", 0, 4, 4 );

  //veto counts
  std::vector<std::string> veto_categories = {"SimpleDeadtime", "TrackerBusy", "DigiBusy", "RateLimiter", "BCR"};
  m_histogrammanager->registerHistogram("tlb_veto_counts", "veto type", veto_categories, 5. );

  INFO(" ... done registering histograms ... " );
  return;

}

void TriggerRateMonitorModule::register_metrics() {

  INFO( "... registering metrics in TriggerRateMonitorModule ... " );

  register_error_metrics();

  //TBP rates
  registerVariable(m_tbp0, "TBP0", daqling::core::metrics::RATE);
  registerVariable(m_tbp1, "TBP1", daqling::core::metrics::RATE);
  registerVariable(m_tbp2, "TBP2", daqling::core::metrics::RATE);
  registerVariable(m_tbp3, "TBP3", daqling::core::metrics::RATE);
  registerVariable(m_tbp4, "TBP4", daqling::core::metrics::RATE);
  //TAP rates
  registerVariable(m_tap0, "TAP0", daqling::core::metrics::RATE);
  registerVariable(m_tap1, "TAP1", daqling::core::metrics::RATE);
  registerVariable(m_tap2, "TAP2", daqling::core::metrics::RATE);
  registerVariable(m_tap3, "TAP3", daqling::core::metrics::RATE);
  registerVariable(m_tap4, "TAP4", daqling::core::metrics::RATE);
  //TAV rates
  registerVariable(m_tav0, "TAV0", daqling::core::metrics::RATE);
  registerVariable(m_tav1, "TAV1", daqling::core::metrics::RATE);
  registerVariable(m_tav2, "TAV2", daqling::core::metrics::RATE);
  registerVariable(m_tav3, "TAV3", daqling::core::metrics::RATE);
  registerVariable(m_tav4, "TAV4", daqling::core::metrics::RATE);

  //veto counters
  registerVariable(m_deadtime_veto, "deadtimeVetoRate", daqling::core::metrics::RATE);
  registerVariable(m_busy_veto, "busyVetoRate", daqling::core::metrics::RATE);
  registerVariable(m_rate_limiter_veto, "ratelimiterVetoRate", daqling::core::metrics::RATE);
  registerVariable(m_bcr_veto, "BCRVetoRate", daqling::core::metrics::RATE);
  registerVariable(m_digi_busy_veto, "digiBusyVetoRate", daqling::core::metrics::RATE);
  registerVariable(m_deadtime_veto, "deadtimeVetoCounter");
  registerVariable(m_busy_veto, "busyVetoCounter");
  registerVariable(m_rate_limiter_veto, "ratelimiterVetoCounter");
  registerVariable(m_bcr_veto, "BCRVetoCounter");
  registerVariable(m_digi_busy_veto, "digiBusyVetoCounter");

  registerVariable(m_deadtime_fraction, "deadtimeVetoPercentage");
  registerVariable(m_busy_fraction, "busyVetoPercentage");
  registerVariable(m_rate_limiter_fraction, "ratelimiterVetoPercentage");
  registerVariable(m_bcr_fraction, "BCRVetoPercentage");
  registerVariable(m_digi_busy_fraction, "digiBusyVetoPercentage");

  return;
}
