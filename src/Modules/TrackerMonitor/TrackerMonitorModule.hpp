#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class TrackerMonitorModule : public MonitorBaseModule {
 public:
  TrackerMonitorModule();
  ~TrackerMonitorModule();

 protected:

  void monitor(daqling::utilities::Binary &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

 private:

  uint16_t m_bcid;
  const std::string m_prefix_hname_hitp;
  const uint8_t kSTRIPDIFFTOLERANCE = 25; // FIXME can be tuned
  const uint8_t kSTRIPS_PER_CHIP = 128;
  const uint8_t kCHIPS_PER_MODULE = 12;

};
