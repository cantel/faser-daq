/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once

#include "Commons/FaserProcess.hpp"

class CalibrationModule : public FaserProcess {
 public:
  CalibrationModule(const std::string& n);
  ~CalibrationModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void start(unsigned);
  void stop();

  void runner() noexcept;
};
