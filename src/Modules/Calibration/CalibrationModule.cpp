/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/*
// TrackerCalibration includes*/
#include "TrackerCalibration/CalibManager.h"
#include "TrackerCalibration/Logger.h"

#include "TrackerCalibration/RunManager.h"
#include "TrackerCalibration/Utils.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Chip.h"
//#include "TrackerCalibration/ITest.h"
//#include "TrackerCalibration/L1DelayScan.h"
//#include "TrackerCalibration/MaskScan.h"
//#include "TrackerCalibration/ThresholdScan.h"
//#include "TrackerCalibration/StrobeDelay.h"
//#include "TrackerCalibration/NPointGain.h"
//#include "TrackerCalibration/TrimScan.h"
//#include "TrackerCalibration/NoiseOccupancy.h"
//#include "TrackerCalibration/TriggerBurst.h"


// TrackerReadout includes
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

  //
  // read json file
  //


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


  //if( !cman.init() ) return 1;

  
  // sanity check
  char *rpath = realpath(m_configLocation.c_str(),NULL);
  if(rpath == nullptr){
    std::cout << "ERROR: bad resolved path " << std::endl;
    //return 0;
  }
  std::string fullpath = std::string(rpath);
  free(rpath);
  std::ifstream infile(fullpath);
  if ( !infile.is_open() ){
    std::cout << "ERROR: could not open input file "
	<< fullpath << std::endl;
    //return 0;
  } 
  
  // parse input file 
  json j = json::parse(infile);
  infile.close();
  
  // check if we deal with a plane configuration file
  if(j.contains("Modules")){
    std::string modCfgFullpath;
	  
    for( const auto &m : j["Modules"] ){
      std::string modCfg = m["cfg"];

      /* check if it is relative or absolute path. Look for any of the two types
	 of directory separators ('/' for unix and '\' for Windows, just in case...) */
    if( modCfg.find_last_of("/\\") == std::string::npos ){ // relative path
	
	// construct full-path taking as basedir the one from the initial function argument 
	bool isUnix = fullpath.find_last_of("/") != std::string::npos;
	std::size_t pos = isUnix ? fullpath.find_last_of("/") : fullpath.find_last_of("\\");	
	modCfgFullpath = fullpath.substr(0,pos+1)+modCfg;
      }
      else{
	modCfgFullpath = modCfg;
      }
      m_modList.push_back(new TrackerCalib::Module(modCfgFullpath, m_verboseLevel));
    }
  }
  else{ // single module configuration file
    m_modList.push_back(new TrackerCalib::Module(m_configLocation, m_verboseLevel));

  }


  //
  // 4. create TRBAccess object
  //


  // Check that at least one module is specified
  if(m_modList.size() < 1){
    //throw std::runtime_error("ERROR: Need to specify at least one module!");
  }

  // Use first module to identify plane ID (TRB ID for USB access)
  int boardId = m_modList.at(0)->planeId();
  std::cout << "TRB BoardID (PlaneID in config file): " << boardId << std::endl;

  // sourceId written to event header so no impact on hardware access, can just be always set to board ID
  int sourceId = boardId;




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
