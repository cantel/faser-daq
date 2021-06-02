/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class TriggerMonitorModule : public MonitorBaseModule {
 public:
  TriggerMonitorModule();
  ~TriggerMonitorModule();

 protected:

  uint16_t m_bcid;
  uint32_t m_orbitid;
  double m_l1A_spacing;
  uint32_t m_previous_orbit;
  uint16_t m_previous_bcid;

  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

  private:
  
   const uint16_t MAX_BCID = 3564;
   const std::string m_prefix_hname_signal_nextBC;

   std::atomic<int> m_input_channel0;
   std::atomic<int> m_input_channel1;
   std::atomic<int> m_input_channel2;
   std::atomic<int> m_input_channel3;
   std::atomic<int> m_input_channel4;
   std::atomic<int> m_input_channel5;
   std::atomic<int> m_input_channel6;
   std::atomic<int> m_input_channel7;
   std::atomic<int> m_tap0;
   std::atomic<int> m_tap1;
   std::atomic<int> m_tap2;
   std::atomic<int> m_tap3;
   std::atomic<int> m_tap4;
   std::atomic<int> m_tap5;

};
