/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class EventMonitorModule : public MonitorBaseModule {
 public:
  EventMonitorModule(const std::string&);
  ~EventMonitorModule();

 protected:
 
  // flags that will be used to enable various histogram sets
  bool m_enable_digitizer;
  bool m_enable_tlb;
  bool m_enable_trb[9];

  // BCIDs for the various settings
  unsigned int m_tlb_bcid;
  unsigned int m_digi_bcid;
  unsigned int m_trb_bcid;

  // standard methods for monitoring module
  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists();
  void register_metrics();

  private:
  
};
