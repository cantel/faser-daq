/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#include "NewModule.hpp"

NewModule::NewModule(const std::string& n):FaserProcess(n) { INFO(""); }

NewModule::~NewModule() { INFO(""); }

// optional (configuration can be handled in the constructor)
void NewModule::configure() {
  FaserProcess::configure();
  INFO("");
}

void NewModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  INFO("");
}

void NewModule::stop() {
  FaserProcess::stop();
  INFO("");
}

void NewModule::runner() noexcept {
  INFO("Running...");
  while (m_run) {
  }
  INFO("Runner stopped");
}
