/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#include "CalibrationModule.hpp"
#include "Utils/Ers.hpp"

NewModule::NewModule(const std::string& n):FaserProcess(n) { ERS_INFO(""); }

NewModule::~NewModule() { ERS_INFO(""); }

// optional (configuration can be handled in the constructor)
void NewModule::configure() {
  FaserProcess::configure();
  ERS_INFO("");
}

void NewModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  ERS_INFO("");
}

void NewModule::stop() {
  FaserProcess::stop();
  ERS_INFO("");
}

void NewModule::runner() noexcept {
  ERS_INFO("Running...");
  while (m_run) {
  }
  ERS_INFO("Runner stopped");
}
