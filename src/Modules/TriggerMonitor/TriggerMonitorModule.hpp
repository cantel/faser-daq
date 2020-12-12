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

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

  private:
  
   const uint16_t MAX_BCID = 3564;

};
