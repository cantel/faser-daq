/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

#include "Core/DAQProcess.hpp"
#include "Commons/FaserProcess.hpp"
#include "Modules/MonitorBase/MonitorBaseModule.hpp"

class TCalibMonitorModule : public FaserProcess {
 public:
  TCalibMonitorModule(const std::string& n);
  ~TCalibMonitorModule() override;

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned);
  void stop();

  void runner() noexcept;
};



//--------------------

//class HistFiller : public daqling::core::DAQProcess {
