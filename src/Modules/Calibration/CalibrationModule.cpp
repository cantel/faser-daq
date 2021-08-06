/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#include "CalibrationModule.hpp"
#include "Utils/Ers.hpp"

CalibrationModule::CalibrationModule(const std::string& n):FaserProcess(n) { ERS_INFO(""); }

CalibrationModule::~CalibrationModule() { ERS_INFO(""); }

// optional (configuration can be handled in the constructor)
void CalibrationModule::configure() {
  FaserProcess::configure();
  ERS_INFO("");
}

void CalibrationModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  ERS_INFO("");
}

void CalibrationModule::stop() {
  FaserProcess::stop();
  ERS_INFO("");
}

void CalibrationModule::runner() noexcept {
  ERS_INFO("Running...");
  while (m_run) {
  }
  ERS_INFO("Runner stopped");
}
