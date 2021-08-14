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

  auto cfg = getModuleSettings();
  
  // retrieve the tcalib command parameters
  INFO("Getting tcalib parameters");

  m_configLocation = cfg["configFile"];

  for( auto& testID : cfg["testList"]) 
    m_testList.push_back(testID.get<int>());

/*
  INFO("config location " << m_configLocation);
  for(unsigned int i = 0; i < m_testList.size(); ++i){
    INFO("test " << m_testList.at(i));
    */
  }

  ERS_INFO("Done configuring");
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
