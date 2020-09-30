#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class EventMonitorModule : public MonitorBaseModule {
 public:
  EventMonitorModule();
  ~EventMonitorModule();

 protected:

  unsigned int m_tlb_bcid;
  unsigned int m_digi_bcid;
  unsigned int m_trb_bcid;

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

  private:
  
};
