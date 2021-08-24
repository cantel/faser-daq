/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/*
// TrackerCalibration includes*/
#include "TrackerCalibration/CalibManager.h"
#include "TrackerCalibration/Logger.h"
/*
#include "TrackerCalibration/RunManager.h"
#include "TrackerCalibration/Utils.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Chip.h"
//#include "TrackerCalibration/ITest.h"
//#include "TrackerCalibration/L1DelayScan.h"
#include "TrackerCalibration/MaskScan.h"
//#include "TrackerCalibration/ThresholdScan.h"
//#include "TrackerCalibration/StrobeDelay.h"
//#include "TrackerCalibration/NPointGain.h"
//#include "TrackerCalibration/TrimScan.h"
//#include "TrackerCalibration/NoiseOccupancy.h"
//#include "TrackerCalibration/TriggerBurst.h"


// TrackerReadout includes*/
#include "TrackerReadout/TRBAccess.h"
#include "TrackerReadout/TRB_ConfigRegisters.h"

// Faser-daq modules includes
#include "CalibrationModule.hpp"
#include "Utils/Ers.hpp"

CalibrationModule::CalibrationModule(const std::string& n):FaserProcess(n) { ERS_INFO(""); }

CalibrationModule::~CalibrationModule() { ERS_INFO(""); }

// optional (configuration can be handled in the constructor)
void CalibrationModule::configure() {
  FaserProcess::configure();
  
  //
  // 1. Retrieve the tcalib parameters from config
  // 

  INFO("Getting tcalib parameters");

  auto cfg = getModuleSettings();

  m_configLocation = cfg["configFile"];

  for( auto& testID : cfg["testList"]) 
    m_testList.push_back(testID.get<int>());

  m_outBaseDir = cfg["outBaseDir"];
  m_verboseLevel = cfg["verbose"];
  m_l1delay = cfg["l1delay"];
  m_log = cfg["log"];
  m_emulateTRB = cfg["emulateTRB"];
  m_calLoop = cfg["calLoop"];
  m_saveDaq = cfg["saveDaq"];
  m_noTrim = cfg["noTrim"];
  m_noRunNumber = cfg["noRunNumber"];
  m_usb = cfg["usb"];
  m_ip = cfg["ip"];


  /*TrackerCalib::CalibManager cman(m_outBaseDir,
				  m_configLocation,
				  m_log,
				  m_testList,
				  m_l1delay,
				  m_emulateTRB,
				  m_calLoop,
				  !m_noTrim,
				  m_noRunNumber,
				  m_usb,
				  m_ip,
				  m_saveDaq,
				  m_verboseLevel);*/

  //
  // 2. Get logger instance 
  //

  auto &log = TrackerCalib::Logger::instance();

  //
  // 3. start properly :-)
  //

  log << "[ CalibManager::init ]" << std::endl;

  //
  // 4. create TRBAccess object
  //


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
