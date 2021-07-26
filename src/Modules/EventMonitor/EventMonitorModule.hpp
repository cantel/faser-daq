/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class EventMonitorModule : public MonitorBaseModule {
 public:
  EventMonitorModule(const std::string&);
  ~EventMonitorModule();
  void configure();

 protected:
 
  // flags that will be used to enable various histogram sets
  bool m_enable_digitizer;
  bool m_enable_tlb;
  std::vector<int> m_enabled_trbs;

  int m_tlb_bcid;
  int m_trb_bcid;
  int m_digi_bcid;
  int m_n_trb;
  std::vector<std::string> m_tlb_trb_hist_names;

  // standard methods for monitoring module
  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists();
  void register_metrics();

  private:

  // constants
  const uint8_t kMAXTRBs = 9;
  const uint16_t kMAXBCID = 5000;

  // metrics
  std::atomic<int> m_missing_tlb;
  std::atomic<int> m_missing_trb;
  std::atomic<int> m_missing_digi;
  
};
