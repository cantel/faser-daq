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
#include "TrackerCalibration/ITest.h"
#include "TrackerCalibration/L1DelayScan.h"
#include "TrackerCalibration/MaskScan.h"
#include "TrackerCalibration/ThresholdScan.h"
#include "TrackerCalibration/StrobeDelay.h"
#include "TrackerCalibration/NPointGain.h"
#include "TrackerCalibration/TrimScan.h"
#include "TrackerCalibration/NoiseOccupancy.h"
#include "TrackerCalibration/TriggerBurst.h"


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

int CalibrationModule::readJson(){
  
  // sanity check
  char *rpath = realpath(m_configLocation.c_str(),NULL);
  if(rpath == nullptr){
    ERROR("ERROR: bad resolved path ");
    return 0;
  }
  std::string fullpath = std::string(rpath);
  free(rpath);
  std::ifstream infile(fullpath);
  if ( !infile.is_open() ){
    ERROR("ERROR: could not open input file "
	<< fullpath);
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
    TrackerCalib::Module *mod = new TrackerCalib::Module(m_configLocation);
    mod->setPrintLevel(m_verboseLevel);
    m_modList.push_back(mod);
  }
  return 1;
}

//-------------------------------

int CalibrationModule::writeJson(std::string outDir){

  char *cp(0);
  
  //
  // 1.- create new LAST output filename (relative path)
  //
  size_t pos = m_configLocation.find_last_of("/\\");
  std::string substr(m_configLocation.substr(pos+1,m_configLocation.length()-pos));
  std::string jsonCfgLast(outDir+"/"+substr);
  jsonCfgLast.insert(jsonCfgLast.find(".json"),"_last");
  
  if(m_verboseLevel >= 2)
    INFO("[CalibManager::writeJson] jsonCfgLast=" << jsonCfgLast);

  //
  // 2.- check if we deal with a plane or single module configuration file
  //
  
  // create json object and parse again original file
  std::ifstream infile(m_configLocation);
  json j = json::parse(infile);
  infile.close();
  
  if( j.contains("Modules") ){
    if(m_verboseLevel >= 2)
      INFO("# We deal with a Plane cfg file... ");
    
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
	INFO("Module " << modFullPath);
      
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
      ERROR("ERROR: could not open output file " <<  jsonCfgLast);
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
	INFO("# we deal with a Single module cfg file... ");
      
      TrackerCalib::Module *mod = m_modList.at(0);
      char command[500];  
      sprintf(command,"cp %s %s", (mod->cfgFileLast()).c_str(), jsonCfgLast.c_str());
      std::system(command);

      if(m_verboseLevel >= 2){
	INFO("# cfgLast     = " << mod->cfgFileLast());
	INFO("# jsonCfgLast = " << jsonCfgLast);
      }
      //mod->writeJson(outDir);
    }
  
  //
  // 3.- summary message
  //
  INFO("File '" << jsonCfgLast << "' created ok");
  
  return 1;
}

int CalibrationModule::finalizeSequence(){
  
  if( ! m_testList.empty() ){
   INFO(std::endl << "[ CalibManager::finalize ]");
    
    // show time information for each test in sequence
   INFO(std::endl << "Timing summary: ");
    int cnt(1);
    for(auto t : m_testList){
     INFO("   * [" << cnt << "] : " 
	  << std::setw(15) << std::setfill(' ') << t->testName() 
	  << " => " << t->printElapsed());
      cnt++;
    }
    
    // write final output config file
    writeJson(m_outBaseDir);    
    
    // add run info and stop-time
    if( !m_noRunNumber ){
      json jdata;
      jdata["outputDirectory"]=m_outBaseDir;
      jdata["outputLog"]= m_outBaseDir+"/"+m_log;    
      
      int res = m_rman->addRunInfo(jdata);
      if(res != 200)
	    INFO("[CalibManager::finalize] ERROR in addRunInfo. Returned status-code = " << res);
    }
  }
   
  // This is THE END
 INFO(std::endl << "GAME OVER -- Insert coin"  << std::endl);
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
  
  m_rman = new TrackerCalib::RunManager(m_verboseLevel);

  INFO("Got tcalib parameters");

  ///---------

  //
  // 0.- output base directory
  //
  std::size_t pos = outBaseDir.length()-1;
  m_outBaseDir = outBaseDir.find_last_of("/") == pos ? 
    outBaseDir.substr(0,pos) : outBaseDir;
    

  // if we have more than one test, append TestSequence_ directory
  //if( m_testSequence.size() > 1 ) 
  // should be read from config calibration.json
  m_outBaseDir+="/TestSequence_"+TrackerCalib::dateStr()+"_"+TrackerCalib::timeStr();

  //
  // 1.- populate list of modules
  //
  if( !readJson() )
    throw std::runtime_error(TrackerCalib::bold+TrackerCalib::red+"\n\nCould not find input json file"+TrackerCalib::reset);

  // compute global mask from list of modules
  for(auto mod : m_modList){
    int trbchan = mod->trbChannel();    
    m_globalMask |= (0x1 << trbchan);
  }

  //
  // 2.- populate list of tests
  //
  std::string testdir("");
  for(auto t : m_testSequence){
    TrackerCalib::TestType tt = (TrackerCalib::TestType)t;
    TrackerCalib::ITest *itest(nullptr);
    std::vector<float> charges;
    switch(tt){
    case TrackerCalib::TestType::L1_DELAY_SCAN:
      itest = new TrackerCalib::L1DelayScan();
      testdir="L1DelayScan_";
      break;
    case TrackerCalib::TestType::MASK_SCAN:
      INFO("45");
      itest = new TrackerCalib::MaskScan();
      INFO("46");
      testdir="MaskScan_";
      break;
    case TrackerCalib::TestType::THRESHOLD_SCAN:
      itest = new TrackerCalib::ThresholdScan();
      testdir="ThresholdScan_";
      break;
    case TrackerCalib::TestType::STROBE_DELAY:
      itest = new TrackerCalib::StrobeDelay();
      testdir="StrobeDelay_";
      break;
    case TrackerCalib::TestType::THREE_POINT_GAIN:
      itest = new TrackerCalib::NPointGain(tt);
      testdir="ThreePointGain_";
      break;
    case TrackerCalib::TestType::RESPONSE_CURVE:
      itest = new TrackerCalib::NPointGain(tt);
      testdir="ResponseCurve_";
      break;
    case TrackerCalib::TestType::TRIM_SCAN:
      itest = new TrackerCalib::TrimScan();
      testdir="TrimScan_";
      break;
    case TrackerCalib::TestType::NOISE_OCCUPANCY:
      itest = new TrackerCalib::NoiseOccupancy();
      testdir="NoiseOccupancy_";
      break;
    case TrackerCalib::TestType::TRIGGER_BURST:
      itest = new TrackerCalib::TriggerBurst();
      testdir="TriggerBurst_";
      break;
    default:
      INFO("[CalibManager::CalibManager] TestType " << t
	  << " not yet implemented. Ignoring for the moment...");
      break;
    }
   
    if(itest != nullptr){
      itest->setPrintLevel(m_verboseLevel);
      itest->setL1delay(m_l1delay);
      itest->setGlobalMask(m_globalMask);
      itest->setEmulateTRB(m_emulateTRB);
      itest->setCalLoop(m_calLoop);
      itest->setLoadTrim(!m_noTrim);
      itest->setSaveDaq(m_saveDaq);
      itest->setOutputDir(m_outBaseDir+"/"+testdir+TrackerCalib::dateStr()+"_"+TrackerCalib::timeStr());
      m_testList.push_back(itest);
    }
  } // end loop in tests
    // in case of running just one test, update outBaseDir 
  // to the output dir of the test
  if( m_testList.size() == 1 )
    m_outBaseDir = m_testList[0]->TrackerCalib::ITest::outputDir();

  
  //
  // 4. create TRBAccess object
  //


  // Check that at least one module is specified
  if(m_modList.size() < 1){
    throw std::runtime_error("ERROR: Need to specify at least one module!");
  }
  // Use first module to identify plane ID (TRB ID for USB access)
  int boardId = m_modList.at(0)->planeId();
  INFO("TRB BoardID (PlaneID in config file): " << boardId);

  // sourceId written to event header so no impact on hardware access, can just be always set to board ID
  int sourceId = boardId;

  int fpga(-1);
  json jtrbConfig; 
  if(m_trb == nullptr){
    m_trb = m_usb ? new FASER::TRBAccess(sourceId, m_emulateTRB, boardId) :
      new FASER::TRBAccess(m_ip, m_ip, sourceId, m_emulateTRB, boardId);

    // suppress on-screen info on bytes transfered
    m_trb->ShowTransfers(false);

    // for debugging
    m_trb->ShowDataRate(false);

    // ensure that TRBs aren't synced to TLB
    m_trb->SetDirectParam(FASER::TRBDirectParameter::L1CounterReset|
			  FASER::TRBDirectParameter::FifoReset|
			  FASER::TRBDirectParameter::ErrCntReset);

    fpga = m_trb->m_FPGA_version;
    //unsigned int major = (0xfff0 & fpga) >> 4;
    //unsigned int minor = 0x000f & fpga;
    //uint16_t answ(0);
    //unsigned int encoder(0);  m_trb->GetSingleFirmwareVersion(0x0001,answ); encoder=answ;
    //unsigned int hardware(0); m_trb->GetSingleFirmwareVersion(0x0002,answ); hardware=answ;
    //unsigned int product(0);  m_trb->GetSingleFirmwareVersion(0x0003,answ); product=answ;

  }
  std::stringstream fwstream;
  fwstream << "0x" << std::hex << fpga;
  jtrbConfig["FW"]=fwstream.str();

  // set appropriate verbosity level in TRB
  int idebug = m_verboseLevel > 2 ? 1 : 0;
  INFO("# Setting TRB verbosity to: " << idebug);
  m_trb->SetDebug(idebug);

  //
  // 4.- initialize TRB
  //
  FASER::ConfigReg *cfgReg = m_trb->GetConfig();
  if(cfgReg == nullptr){
    ERROR("[CalibManager::initTRB] could not get TRB config register. Exit.");
    //return 0;
  }

  cfgReg->Set_Module_L1En(0x0); // disable hardware L1A
  cfgReg->Set_Module_ClkCmdSelect(0x0); // select CLK/CMD 0
  cfgReg->Set_Module_LedRXEn(0x0); // disable led lines
  cfgReg->Set_Module_LedxRXEn(0x0); // disable ledx lines
  if(!m_emulateTRB) m_trb->WriteConfigReg();
 
  // configure phase settings so that to reset SM
  FASER::PhaseReg *phaseReg = m_trb->GetPhaseConfig();
  if(phaseReg == nullptr){
    ERROR("[CalibManager::initTRB] could not get TRB phase register. Exit.");
    //return 0;
  }

  unsigned int finePhaseClk0(0);
  unsigned int finePhaseClk1(0);
  unsigned int finePhaseLed0(35);
  unsigned int finePhaseLed1(35);
  
  phaseReg->SetFinePhase_Clk0(finePhaseClk0);
  phaseReg->SetFinePhase_Led0(finePhaseLed0);
  phaseReg->SetFinePhase_Clk1(finePhaseClk1);
  phaseReg->SetFinePhase_Led1(finePhaseLed1);
  if( !m_emulateTRB ) {
    m_trb->WritePhaseConfigReg();
    m_trb->ApplyPhaseConfig();
  }

  jtrbConfig["FinePhaseClk0"]=finePhaseClk0;
  jtrbConfig["FinePhaseLed0"]=finePhaseLed0;
  jtrbConfig["FinePhaseClk1"]=finePhaseClk1;
  jtrbConfig["FinePhaseLed1"]=finePhaseLed1;

  // check if we expect data in LED and/or LEDx
  bool dataLED(false);
  bool dataLEDx(false);
  for(auto mod : m_modList){
    for(auto chip : mod->Chips()){
      if( chip->address() < 40 ) 
	dataLED=true;
      else
	dataLEDx=true;
    }
  }

  // configure TRB enabling required LED/LEDx  
  if(dataLED)  cfgReg->Set_Module_LedRXEn(m_globalMask);
  if(dataLEDx) cfgReg->Set_Module_LedxRXEn(m_globalMask);        
  cfgReg->Set_Global_L2SoftL1AEn(true);      // enable software L1A
  cfgReg->Set_Global_RxTimeoutDisable(true); // disable RxTimeout
  cfgReg->Set_Global_L1TimeoutDisable(false);// enable L1Timeout
  cfgReg->Set_Global_Overflow(4095);

  // CalLoop settings
  uint32_t calLoopNb(100); // number of L1A
  uint16_t calLoopDelay(500); // delay (in 100 ns unit)
  bool dynamicDelay(true); // use dynamic delay

  cfgReg->Set_Global_CalLoopNb(calLoopNb);  
  cfgReg->Set_Global_CalLoopDelay(calLoopDelay);
  cfgReg->Set_Global_DynamicCalLoopDelay(dynamicDelay);

  jtrbConfig["CalLoopNb"]=int(calLoopNb);
  jtrbConfig["CalLoopDelay"]=int(calLoopDelay);
  jtrbConfig["DynamicDelay"]=dynamicDelay;

  INFO(std::endl << "# TRB settings :" << std::dec << std::endl
  << "  - finePhaseClk0 = " << finePhaseClk0 << std::endl
  << "  - finePhaseLed0 = " << finePhaseLed0 << std::endl
  << "  - finePhaseClk1 = " << finePhaseClk1 << std::endl
  << "  - finePhaseLed1 = " << finePhaseLed1 << std::endl
  << "  - calLoopNb     = " << calLoopNb     << std::endl
  << "  - calLoopDelay  = " << calLoopDelay  << std::endl
  << "  - dynamicDelay  = " << dynamicDelay  << std::endl << std::endl);
  std::cout << std::dec << std::setprecision(3);


  if( !m_emulateTRB ) {
    m_trb->WriteConfigReg();
    
    INFO("# Info from trb->GetConfig()...");
    cfgReg = m_trb->GetConfig();
    INFO(TrackerCalib::printTRBConfig(cfgReg));
  }



  //
  // 5.- create new run number
  //  
  if( !m_noRunNumber ){

    // info on scan or test-sequence
    json jscanConfig;
    jscanConfig["nTests"]=m_testList.size();
    int testcnt(0);
    for(auto t : m_testList){
      std::string teststr="test_"+std::to_string(testcnt);
      jscanConfig[teststr] = t->testName();      
      testcnt++;
    }

    int res = m_rman->newRun(m_configLocation,jtrbConfig,jscanConfig);   
    if(res == 0) 
      ERROR("problem parsing input json cfg file");
      //return 0; 
    else if(res!=0 && res!=201){
      ERROR(std::endl << "Could not create new run number...");
      INFO("Returned status_code = " << res);
      //return 0;
    }
  }

  // show info on run-number
  INFO("###############################" << std::endl
      << "# RUN = " << m_rman->runNumber() << std::endl 
      << "###############################" << std::endl);

  //
  // 6.- show basic information from command-line options
  //
  INFO(" # Selected options" << std::endl
  << " - usb         : " << m_usb << std::endl
  << " - IP          : " << m_ip << std::endl
  << " - outBaseDir  : " << m_outBaseDir << std::endl
  << " - logFile     : " << m_outBaseDir+"/"+m_log << std::endl
  << " - l1delay     : " << std::dec << m_l1delay  << std::endl
  << " - emulateTRB  : " << m_emulateTRB << std::endl
  << " - calLoop     : " << m_calLoop << std::endl
  << " - loadTrim    : " << !m_noTrim << std::endl
  << " - doRunNumber : " << !m_noRunNumber << std::endl
  << " - saveDaq     : " << m_saveDaq  << std::endl
  << " - printLevel  : " << std::dec << m_verboseLevel << std::endl
  << " - globalMask  : 0x" << std::hex << std::setfill('0') << unsigned(m_globalMask) << std::endl
  << std::dec);
  
  int cnt=0;
  INFO(" - MODULES    : " << m_modList.size());
  std::vector<TrackerCalib::Module*>::const_iterator mit;
  for(mit=m_modList.begin(); mit!=m_modList.end(); ++mit){
    INFO("   * mod [" << cnt << "] : " << (*mit)->print());
    cnt++;
  }  
  INFO(" - TESTS      : " << m_testList.size());
  std::vector<TrackerCalib::ITest*>::const_iterator cit;
  for(cnt=0, cit=m_testList.begin(); cit!=m_testList.end(); ++cit){
    INFO("   * test [" << cnt << "] ");
    INFO((*cit)->print(3));
    cnt++;
  }

  if( m_testList.empty() ){
    ERROR("==> No tests selected. Nothing to be done...");
    //return 1;
  }

  INFO(std::endl << "[ CalibManager::run ]");

  ERS_INFO("Done configuring");
}

void CalibrationModule::start(unsigned run_num) {
  ERS_INFO("Starting...");

  m_t = m_testList.at(m_testcnt - 1);

  // show current progress
  INFO(std::endl
  << "###############################################" << std::endl
  << "  Running test " << m_testcnt << " / " << m_testList.size() << std::endl
  << "###############################################" << std::endl);
    
  // run test
  m_t->setRunNumber(m_rman->runNumber());
  
  // run prep and initialize

  if( !m_t->initializeRun(m_trb, m_modList) ) ERROR("Failed to initialize test of type " << m_t->print());

  FaserProcess::start(run_num);

  ERS_INFO("Done starting");
}

void CalibrationModule::stop() {
  FaserProcess::stop();

  /** If there are no more tests, finalize the whole sequence.
   *  Similar to CalibManager::finalize **/

  if(m_testcnt == m_testList.size()){
    finalizeSequence();
  }
  
  ERS_INFO("Stopped calibration run");
}

void CalibrationModule::runner() noexcept {
  ERS_INFO("Running...");
  

  if( !m_t->executeNew(m_trb,m_modList) ) ERROR("Failed to execute test of type " << m_t->print());

  if( !m_t->finalizeRun(m_trb,m_modList) ) ERROR("Failed to finalize test of type " << m_t->print());;
  
  // write configuration file after test
  for(auto mod : m_modList)
    mod->writeJsonDaq(m_t->outputDir());


  ERS_INFO("Run stopped");

  if(m_testcnt < m_testList.size()){
    INFO("Test sequence is not finished, starting another test");
    
    m_testcnt++;
    start(m_run_number); // start a new test

    // works!
  }

  ERS_INFO("Did not find any more tests to run...");
 
}
