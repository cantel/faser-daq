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

  m_tcalibLocation = cfg["tcalibLocation"];
  m_configLocation = cfg["configFile"];

  for( auto& testID : cfg["testList"]) 
    m_testList.push_back(testID.get<int>());

  m_outBaseDir = cfg["outBaseDir"];
  m_verboseLevel = cfg["verbose"];
  m_l1delay = cfg["l1delay"];
  m_noRunNumber = cfg["noRunNumber"];
  m_usb = cfg["usb"];


  std::string testListString = "";

  for(unsigned int i = 0; i < m_testList.size(); ++i){
    testListString = testListString + std::to_string(m_testList.at(i)) + " ";
    
  }

  sprintf(m_tcalibCommand,"%s -t %s -i %s -o %s -v %s -d %s", m_tcalibLocation.c_str(), testListString.c_str(), m_configLocation.c_str(), m_outBaseDir.c_str(), std::to_string(m_verboseLevel).c_str(), std::to_string(m_l1delay).c_str());

  if(m_noRunNumber){
    sprintf(m_tcalibCommand, "%s -nr", m_tcalibCommand);
  }
  if(m_usb){
    sprintf(m_tcalibCommand, "%s --usb", m_tcalibCommand);
  }


  INFO("command : " << m_tcalibCommand);

  ERS_INFO("Done configuring");
}

void CalibrationModule::start(unsigned run_num) {
  FaserProcess::start(run_num);

  std::system(m_tcalibCommand);

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
