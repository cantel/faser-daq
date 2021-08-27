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

/*----- The inclusion below is problematic------*/

#include "TrackerCalibration/ITest.h"

/*----------------------------------------------*/

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

#include "TrackerReadout/ConfigurationHandling.h"

#include "nlohmann/json.hpp"



// std includes
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <time.h>
#ifdef _MACOSX_
#include <unistd.h>
#endif

using json = nlohmann::json;
using Chip = TrackerCalib::Chip;

/*
,
m_configLocation{m_cfg["configFile"]},
m_verboseLevel{m_cfg["verbose"]},
m_l1delay{m_cfg["l1delay"]},
m_log{m_cfg["log"]},
m_emulateTRB{m_cfg["emulateTRB"]},
m_calLoop{m_cfg["calLoop"]},
m_saveDaq{m_cfg["saveDaq"]},
m_noTrim{m_cfg["noTrim"]},
m_noRunNumber{m_cfg["noRunNumber"]},
m_usb{m_cfg["usb"]},
m_ip{m_cfg["ip"]} 
 */

//-----------------------------------------
CalibrationModule::CalibrationModule(const std::string& n): 
FaserProcess(n) 
{ 
  INFO("constructor");

  ERS_INFO(""); 
}

//-----------------------------------------
CalibrationModule::~CalibrationModule() { ERS_INFO(""); }

//---------- Functions ---------------

int CalibrationModule::readJson(std::string jsonCfg){
  auto &log = TrackerCalib::Logger::instance();
  
  // sanity check
  char *rpath = realpath(jsonCfg.c_str(),NULL);
  if(rpath == nullptr){
    log << "ERROR: bad resolved path " << std::endl;
    return 0;
  }
  std::string fullpath = std::string(rpath);
  free(rpath);
  std::ifstream infile(fullpath);
  if ( !infile.is_open() ){
    log << "ERROR: could not open input file "
	<< fullpath << std::endl;
    return 0;
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
      TrackerCalib::Module *mod = new TrackerCalib::Module(modCfgFullpath);
      mod->setPrintLevel(m_verboseLevel);
      m_modList.push_back(mod);
    }
  }
  else{ // single module configuration file
    TrackerCalib::Module *mod = new TrackerCalib::Module(jsonCfg);
    mod->setPrintLevel(m_verboseLevel);
    m_modList.push_back(mod);
  }
  return 1;
}

//-------------------------------

int CalibrationModule::writeJson(std::string outDir){
  auto &log = TrackerCalib::Logger::instance();
  char *cp(0);
  
  //
  // 1.- create new LAST output filename (relative path)
  //
  size_t pos = m_configLocation.find_last_of("/\\");
  std::string substr(m_configLocation.substr(pos+1,m_configLocation.length()-pos));
  std::string jsonCfgLast(outDir+"/"+substr);
  jsonCfgLast.insert(jsonCfgLast.find(".json"),"_last");
  
  if(m_verboseLevel >= 2)
    log << "[CalibManager::writeJson] jsonCfgLast=" << jsonCfgLast << std::endl;

  //
  // 2.- check if we deal with a plane or single module configuration file
  //
  
  // create json object and parse again original file
  std::ifstream infile(m_configLocation);
  json j = json::parse(infile);
  infile.close();
  
  if( j.contains("Modules") ){
    if(m_verboseLevel >= 2)
      log << "# We deal with a Plane cfg file... " << std::endl;
    
    std::vector<json> jModules;
    json jCfg; // global object
    
    for( const auto &m : j["Modules"] ){
      std::string modCfg = m["cfg"];
      
      std::string modFullPath;
      if( modCfg.find_last_of("/\\") == std::string::npos ){ // relative path
	
	// absolute path of input config file
	cp = realpath(m_configLocation.c_str(),NULL);
	std::string fullInputPath(cp);
	
	// extract base directory from parent file and build full path
	std::string basedir = fullInputPath.substr(0,fullInputPath.find_last_of("/\\")+1);	
	modFullPath = basedir+modCfg;
      }
      else{ // already absolute path: do nothing
	modFullPath = modCfg;      
      }

      if(m_verboseLevel >= 2)
	log << "Module " << modFullPath << std::endl;
      
      // loop in modList and find corresponding module based on initial configuration file
      for(auto mod : m_modList){

	if(mod->cfgFile() == modFullPath){
	  cp = realpath(mod->cfgFileLast().c_str(),NULL);
	  std::string fullout(cp);	  
	  jCfg["cfg"] = fullout;
	  jModules.push_back(jCfg);
	  break;
	}
      }
    } // end loop in j["Modules']

    // write updated json file
    std::ofstream ofile;
    ofile.open(jsonCfgLast);
    if ( !ofile.is_open() ){
      log << "ERROR: could not open output file " <<  jsonCfgLast << std::endl;
      return 0;
    }
    json cfg;
    cfg["Modules"] = jModules;
    ofile << std::setw(2) << cfg;
    ofile.close();            
  }  
  else // single module configuration file: just copy latest cfg file
    {
      if(m_verboseLevel >= 2)
	log << "# we deal with a Single module cfg file... " << std::endl;
      
      TrackerCalib::Module *mod = m_modList.at(0);
      char command[500];  
      sprintf(command,"cp %s %s", (mod->cfgFileLast()).c_str(), jsonCfgLast.c_str());
      std::system(command);

      if(m_verboseLevel >= 2){
	log << "# cfgLast     = " << mod->cfgFileLast() << std::endl;
	log << "# jsonCfgLast = " << jsonCfgLast << std::endl;
      }
      //mod->writeJson(outDir);
    }
  
  //
  // 3.- summary message
  //
  log << std::endl << TrackerCalib::green << TrackerCalib::bold;
  for(int i=0; i<90; i++) log << "=";
  log  << std::endl << std::endl
       << "File '" << jsonCfgLast << "' created ok" 
       << std::endl << std::endl;
  for(int i=0; i<90; i++) log << "=";
  log << TrackerCalib::reset << std::endl << std::endl;
  
  return 1;
}

//------------------ Main functions ---------------

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
    m_testSequence.push_back(testID.get<int>());

  std::string outBaseDir = cfg["outBaseDir"];
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
  

  INFO("Got tcalib parameters");

  ///---------


  auto &log = TrackerCalib::Logger::instance();

  //
  // 0.- output base directory
  //
  std::size_t pos = outBaseDir.length()-1;
  m_outBaseDir = outBaseDir.find_last_of("/") == pos ? 
    outBaseDir.substr(0,pos) : outBaseDir;
    
  INFO("1");


  // if we have more than one test, append TestSequence_ directory
  if( m_testSequence.size() > 1 ) 
    m_outBaseDir+="/TestSequence_"+TrackerCalib::dateStr()+"_"+TrackerCalib::timeStr();


  INFO("2");
  //
  // 1.- populate list of modules
  //
  if( !readJson(m_configLocation) )
    throw std::runtime_error(TrackerCalib::bold+TrackerCalib::red+"\n\nCould not find input json file"+TrackerCalib::reset);

  INFO("3");

  // compute global mask from list of modules
  for(auto mod : m_modList){
    int trbchan = mod->trbChannel();    
    m_globalMask |= (0x1 << trbchan);
  }

  INFO("4");
    // in case of running just one test, update outBaseDir 
  // to the output dir of the test
  if( m_testList.size() == 1 )
    m_outBaseDir = m_testList[0]->TrackerCalib::ITest::outputDir();

  INFO("5");

/*
  TrackerCalib::CalibManager cman(outBaseDir,
				  m_configLocation,
				  m_log,
				  m_testSequence,
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

  
  
  //
  // 4. create TRBAccess object
  //


  // Check that at least one module is specified
  if(m_modList.size() < 1){
    throw std::runtime_error("ERROR: Need to specify at least one module!");
  }

  INFO("6");

  // Use first module to identify plane ID (TRB ID for USB access)
  int boardId = m_modList.at(0)->planeId();
  std::cout << "TRB BoardID (PlaneID in config file): " << boardId << std::endl;

  INFO("7");
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
