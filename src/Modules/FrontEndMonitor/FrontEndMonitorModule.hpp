/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#pragma once

#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class FrontEndMonitorModule : public MonitorBaseModule {
 public:
  FrontEndMonitorModule(const std::string&);
  ~FrontEndMonitorModule();

 protected:

  void monitor(DataFragment<daqling::utilities::Binary> &eventBuilderBinary);
  void register_hists( );
  void register_metrics();

};
