#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"
#include "TrackerReadout/TRBEventDecoder.h"

class TrackerMonitorModule : public MonitorBaseModule {
 public:
  TrackerMonitorModule();
  ~TrackerMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();
 
 private:
  FASER::TRBEventDecoder * m_decoder;

};
