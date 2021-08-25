// TrackerCalibration includes
#include "TrackerCalibration/CalibManager.h"
#include "TrackerCalibration/RunManager.h"
#include "TrackerCalibration/Logger.h"
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

// std includes
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <time.h>
#ifdef _MACOSX_
#include <unistd.h>
#endif

#include "TrackerReadout/ConfigurationHandling.h"
using json = nlohmann::json;
using Chip = TrackerCalib::Chip;

//------------------------------------------------------
TrackerCalib::CalibManager::CalibManager() :
  m_trb(nullptr),
  m_rman(new RunManager()),
  m_outBaseDir("."),
  m_jsonInputFile(""),
  m_l1delay(130),
  m_globalMask(0x00),
  m_emulateTRB(false),
  m_calLoop(false),
  m_loadTrim(true),
  m_doRunNumber(true),
  m_usb(false),
  m_ip("10.11.65.8"),
  m_saveDaq(false),
  m_printLevel(0)
{}

//------------------------------------------------------
TrackerCalib::CalibManager::CalibManager(std::string outBaseDir,
					 std::string jsonInputFile,
					 std::string logfile,
					 std::vector<int> testSequence,
					 unsigned int l1delay,
					 bool emulateTRB,
					 bool calLoop,
					 bool loadTrim,
					 bool noRunNumber,
					 bool usb,
					 std::string ip,
					 bool saveDaq,
					 int printLevel) :
  m_trb(nullptr),
  m_rman(new RunManager(printLevel)),
  m_jsonInputFile(jsonInputFile),
  m_logfile(logfile),
  m_l1delay(l1delay),
  m_globalMask(0x00),
  m_emulateTRB(emulateTRB),
  m_calLoop(calLoop),
  m_loadTrim(loadTrim),
  m_doRunNumber(!noRunNumber),
  m_usb(usb),
  m_ip(ip),
  m_saveDaq(saveDaq),
  m_printLevel(printLevel){

  auto &log = TrackerCalib::Logger::instance();

  //
  // 0.- output base directory
  //
  std::size_t pos = outBaseDir.length()-1;
  m_outBaseDir = outBaseDir.find_last_of("/") == pos ? 
    outBaseDir.substr(0,pos) : outBaseDir;

  // if we have more than one test, append TestSequence_ directory
  if( testSequence.size() > 1 ) 
    m_outBaseDir+="/TestSequence_"+dateStr()+"_"+timeStr();

  //
  // 1.- populate list of modules
  //
  if( !readJson(m_jsonInputFile) )
    throw std::runtime_error(bold+red+"\n\nCould not find input json file"+reset);

  // compute global mask from list of modules
  for(auto mod : m_modList){
    int trbchan = mod->trbChannel();    
    m_globalMask |= (0x1 << trbchan);
  }

  //
  // 2.- populate list of tests
  //
  std::string testdir("");
  for(auto t : testSequence){
    TestType tt = (TestType)t;
    ITest *itest(nullptr);
    std::vector<float> charges;
    switch(tt){
    case TestType::L1_DELAY_SCAN:
      itest = new L1DelayScan();
      testdir="L1DelayScan_";
      break;
    case TestType::MASK_SCAN:
      itest = new MaskScan();
      testdir="MaskScan_";
      break;
    case TestType::THRESHOLD_SCAN:
      itest = new ThresholdScan();
      testdir="ThresholdScan_";
      break;
    case TestType::STROBE_DELAY:
      itest = new StrobeDelay();
      testdir="StrobeDelay_";
      break;
    case TestType::THREE_POINT_GAIN:
      itest = new NPointGain(tt);
      testdir="ThreePointGain_";
      break;
    case TestType::RESPONSE_CURVE:
      itest = new NPointGain(tt);
      testdir="ResponseCurve_";
      break;
    case TestType::TRIM_SCAN:
      itest = new TrimScan();
      testdir="TrimScan_";
      break;
    case TestType::NOISE_OCCUPANCY:
      itest = new NoiseOccupancy();
      testdir="NoiseOccupancy_";
      break;
    case TestType::TRIGGER_BURST:
      itest = new TriggerBurst();
      testdir="TriggerBurst_";
      break;
    default:
      log << "[CalibManager::CalibManager] TestType " << t
	  << " not yet implemented. Ignoring for the moment..." << std::endl;
      break;
    }
   
    if(itest != nullptr){
      itest->setPrintLevel(m_printLevel);
      itest->setL1delay(m_l1delay);
      itest->setGlobalMask(m_globalMask);
      itest->setEmulateTRB(m_emulateTRB);
      itest->setCalLoop(m_calLoop);
      itest->setLoadTrim(m_loadTrim);
      itest->setSaveDaq(m_saveDaq);
      itest->setOutputDir(m_outBaseDir+"/"+testdir+dateStr()+"_"+timeStr());
      m_testList.push_back(itest);
    }
  } // end loop in tests

  // in case of running just one test, update outBaseDir 
  // to the output dir of the test
  if( m_testList.size() == 1 )
    m_outBaseDir = m_testList[0]->outputDir();
}

//------------------------------------------------------
TrackerCalib::CalibManager::~CalibManager(){
  if(m_trb != nullptr)
    delete m_trb;
  m_trb=nullptr;

  delete m_rman;
  m_rman=nullptr;

  for(auto m : m_modList)
    delete m;
  m_modList.clear();

  for(auto t : m_testList)
    delete t;
  m_testList.clear();
}

//------------------------------------------------------
int TrackerCalib::CalibManager::readJson(std::string jsonCfg){
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
      m_modList.push_back(new Module(modCfgFullpath, m_printLevel));
    }
  }
  else{ // single module configuration file
    m_modList.push_back(new Module(jsonCfg, m_printLevel));
  }
  return 1;
}

//------------------------------------------------------
int TrackerCalib::CalibManager::writeJson(std::string outDir){
  auto &log = TrackerCalib::Logger::instance();
  char *cp(0);
  
  //
  // 1.- create new LAST output filename (relative path)
  //
  size_t pos = m_jsonInputFile.find_last_of("/\\");
  std::string substr(m_jsonInputFile.substr(pos+1,m_jsonInputFile.length()-pos));
  std::string jsonCfgLast(outDir+"/"+substr);
  jsonCfgLast.insert(jsonCfgLast.find(".json"),"_last");
  
  if(m_printLevel >= 2)
    log << "[CalibManager::writeJson] jsonCfgLast=" << jsonCfgLast << std::endl;

  //
  // 2.- check if we deal with a plane or single module configuration file
  //
  
  // create json object and parse again original file
  std::ifstream infile(m_jsonInputFile);
  json j = json::parse(infile);
  infile.close();
  
  if( j.contains("Modules") ){
    if(m_printLevel >= 2)
      log << "# We deal with a Plane cfg file... " << std::endl;
    
    std::vector<json> jModules;
    json jCfg; // global object
    
    for( const auto &m : j["Modules"] ){
      std::string modCfg = m["cfg"];
      
      std::string modFullPath;
      if( modCfg.find_last_of("/\\") == std::string::npos ){ // relative path
	
	// absolute path of input config file
	cp = realpath(m_jsonInputFile.c_str(),NULL);
	std::string fullInputPath(cp);
	
	// extract base directory from parent file and build full path
	std::string basedir = fullInputPath.substr(0,fullInputPath.find_last_of("/\\")+1);	
	modFullPath = basedir+modCfg;
      }
      else{ // already absolute path: do nothing
	modFullPath = modCfg;      
      }

      if(m_printLevel >= 2)
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
      if(m_printLevel >= 2)
	log << "# we deal with a Single module cfg file... " << std::endl;
      
      Module *mod = m_modList.at(0);
      char command[500];  
      sprintf(command,"cp %s %s", (mod->cfgFileLast()).c_str(), jsonCfgLast.c_str());
      std::system(command);

      if(m_printLevel >= 2){
	log << "# cfgLast     = " << mod->cfgFileLast() << std::endl;
	log << "# jsonCfgLast = " << jsonCfgLast << std::endl;
      }
      //mod->writeJson(outDir);
    }
  
  //
  // 3.- summary message
  //
  log << std::endl << green << bold;
  for(int i=0; i<90; i++) log << "=";
  log  << std::endl << std::endl
       << "File '" << jsonCfgLast << "' created ok" 
       << std::endl << std::endl;
  for(int i=0; i<90; i++) log << "=";
  log << reset << std::endl << std::endl;
  
  return 1;
}

//------------------------------------------------------
const std::string TrackerCalib::CalibManager::Logo(){
  const int n=90;  
  std::ostringstream out;     

  out << std::endl;
  for(int i=0; i<n; i++) out << "=";
  out << std::endl << std::endl;
  
  out << FaserLogo();
  out << CalibLogo();

  for(int i=0; i<n; i++) out << "=";
  
  return out.str();
}

//------------------------------------------------------
const std::string TrackerCalib::CalibManager::FaserLogo(){
  std::time_t currTime;
  time(&currTime);
  struct tm *stm = localtime(&currTime);  
  char dateString[100], timeString[100];
  //char runString[100];  
  strftime(dateString, 50, "%A %d %B, %Y", stm);
  strftime(timeString, 50, "%T", stm);
  //sprintf(runString, "RUN : %d", m_rman->runNumber());
  
  std::ostringstream out;
  out << " ███████╗ █████╗ ███████╗███████╗██████╗     " << std::endl;
  out << " ██╔════╝██╔══██╗██╔════╝██╔════╝██╔══██╗    " << std::endl;
  out << " █████╗  ███████║███████╗█████╗  ██████╔╝    " << dateString << std::endl;
  out << " ██╔══╝  ██╔══██║╚════██║██╔══╝  ██╔══██╗    " << timeString << std::endl;
  out << " ██║     ██║  ██║███████║███████╗██║  ██║    " << std::endl;
  out << " ╚═╝     ╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝    " << std::endl;
  return out.str();
}

//------------------------------------------------------
const std::string TrackerCalib::CalibManager::CalibLogo(){  
  std::ostringstream out;
  out << "  _____               _                ____      _ _ _               _   _         " << std::endl;
  out << " |_   _| __ __ _  ___| | _____ _ __   / ___|__ _| (_) |__  _ __ __ _| |_(_) ___  _ __  " << std::endl;
  out << "   | || '__/ _` |/ __| |/ / _ \\ '__| | |   / _` | | | '_ \\| '__/ _` | __| |/ _ \\| '_ \\ " << std::endl;
  out << "   | || | | (_| | (__|   <  __/ |    | |__| (_| | | | |_) | | | (_| | |_| | (_) | | | |" << std::endl;
  out << "   |_||_|  \\__,_|\\___|_|\\_\\___|_|     \\____\\__,_|_|_|_.__/|_|  \\__,_|\\__|_|\\___/|_| |_|" << std::endl;
  out << "                                                                                       " << std::endl;
  return out.str();
}

//------------------------------------------------------
void TrackerCalib::CalibManager::addModule(const TrackerCalib::Module *module){
  if( module != nullptr){
    TrackerCalib::Module *m = new TrackerCalib::Module(*module);
    m->setPrintLevel(m_printLevel);
    m_modList.push_back(m);
  }
}

//------------------------------------------------------
void TrackerCalib::CalibManager::addTest(const TrackerCalib::ITest *test){
  if( test != nullptr ){
    ITest *t = test->clone();
    t->setPrintLevel(m_printLevel);
    m_testList.push_back(t);
  }
}

//------------------------------------------------------
void TrackerCalib::CalibManager::setPrintLevel(int printLevel){
  m_printLevel=printLevel;
  
  for(auto a : m_modList)
    a->setPrintLevel(printLevel);

  for(auto a : m_testList)
    a->setPrintLevel(printLevel);
}

//------------------------------------------------------
int TrackerCalib::CalibManager::init(){

  //
  // 1.- get Logger instance
  //  
  auto &log = TrackerCalib::Logger::instance();

  //
  // 2.- start properly :-)
  //
  log << Logo() << std::endl;  
  log << blue << bold << "[ CalibManager::init ]" << reset << std::endl;

  //
  // 3.- create TRBAccess object
  //

  // Check that at least one module is specified
  if(m_modList.size() < 1){
    throw std::runtime_error("ERROR: Need to specify at least one module!");
  }

  // Use first module to identify plane ID (TRB ID for USB access)
  int boardId = m_modList.at(0)->planeId();
  log << "TRB BoardID (PlaneID in config file): " << boardId << std::endl;

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

    // output to log file only, since the information below is
    // sent to cout from within the GPIOBaseClass constructor...
    std::ostringstream outID;
    uint16_t bid(m_trb->GetBoardID());
    outID << "GPIO board ID = 0x" << std::hex << bid << std::endl;
    log.extra(outID.str());

    std::ostringstream fw;
    fpga = m_trb->m_FPGA_version;
    unsigned int major = (0xfff0 & fpga) >> 4;
    unsigned int minor = 0x000f & fpga;
    uint16_t answ(0);
    unsigned int encoder(0);  m_trb->GetSingleFirmwareVersion(0x0001,answ); encoder=answ;
    unsigned int hardware(0); m_trb->GetSingleFirmwareVersion(0x0002,answ); hardware=answ;
    unsigned int product(0);  m_trb->GetSingleFirmwareVersion(0x0003,answ); product=answ;

    fw << "Firmware versions: " << std::endl;
    fw << "   FPGA       : " << major << "." << minor << " = 0x" 
       << std::hex << fpga << std::endl;
    fw << "   Encoder    : " << encoder << std::endl;
    fw << "   Hardware   : " << hardware << std::endl;
    fw << "   Product ID : " << product << std::endl;
    fw << std::endl;
    log.extra(fw.str());
  }

  std::stringstream fwstream;
  fwstream << "0x" << std::hex << fpga;
  jtrbConfig["FW"]=fwstream.str();

  // set appropriate verbosity level in TRB
  int idebug = m_printLevel > 2 ? 1 : 0;
  log << "# Setting TRB verbosity to: " << idebug << std::endl;
  m_trb->SetDebug(idebug);

  //
  // 4.- initialize TRB
  //
  FASER::ConfigReg *cfgReg = m_trb->GetConfig();
  if(cfgReg == nullptr){
    log << red << "[CalibManager::initTRB] could not get TRB config register. Exit."
	      << reset << std::endl;
    return 0;
  }

  cfgReg->Set_Module_L1En(0x0); // disable hardware L1A
  cfgReg->Set_Module_ClkCmdSelect(0x0); // select CLK/CMD 0
  cfgReg->Set_Module_LedRXEn(0x0); // disable led lines
  cfgReg->Set_Module_LedxRXEn(0x0); // disable ledx lines
  if(!m_emulateTRB) m_trb->WriteConfigReg();
 
  // configure phase settings so that to reset SM
  FASER::PhaseReg *phaseReg = m_trb->GetPhaseConfig();
  if(phaseReg == nullptr){
    log << red << "[CalibManager::initTRB] could not get TRB phase register. Exit."
	      << reset << std::endl;
    return 0;
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

  log << std::endl;
  log << "# TRB settings :" << std::dec << std::endl;
  log << "  - finePhaseClk0 = " << finePhaseClk0 << std::endl;
  log << "  - finePhaseLed0 = " << finePhaseLed0 << std::endl;
  log << "  - finePhaseClk1 = " << finePhaseClk1 << std::endl;
  log << "  - finePhaseLed1 = " << finePhaseLed1 << std::endl;
  log << "  - calLoopNb     = " << calLoopNb     << std::endl;
  log << "  - calLoopDelay  = " << calLoopDelay  << std::endl;
  log << "  - dynamicDelay  = " << dynamicDelay  << std::endl << std::endl;
  std::cout << std::dec << std::setprecision(3);


  if( !m_emulateTRB ) {
    m_trb->WriteConfigReg();
    //log << "# Info from trb->ReadConfigReg()..." << std::endl;
    //std::vector<uint16_t> configIs = m_trb->ReadConfigReg();
    //FASER::ConfigReg actualCfg;
    //actualCfg.FillFromPayload(configIs);
    //log << printTRBConfig(&actualCfg) << std::endl;
    ////actualCfg.Print();
    
    log << "# Info from trb->GetConfig()..." << std::endl;
    cfgReg = m_trb->GetConfig();
    log << printTRBConfig(cfgReg) << std::endl;
  }

  //
  // 5.- create new run number
  //  
  if( m_doRunNumber ){

    // info on scan or test-sequence
    json jscanConfig;
    jscanConfig["nTests"]=m_testList.size();
    int testcnt(0);
    for(auto t : m_testList){
      std::string teststr="test_"+std::to_string(testcnt);
      jscanConfig[teststr] = t->testName();      
      testcnt++;
    }

    int res = m_rman->newRun(m_jsonInputFile,jtrbConfig,jscanConfig);   
    if(res == 0) // problem parsing input json cfg file
      return 0; 
    else if(res!=0 && res!=201){
      log << std::endl << red << bold << "Could not create new run number..." 
	  << reset << std::endl;
      log << "Returned status_code = " << res << std::endl;
      return 0;
    }
  }

  // show info on run-number
  log << red << bold 
      << "###############################" << std::endl
      << "# RUN = " << m_rman->runNumber() << std::endl 
      << "###############################" << std::endl
      << reset << std::endl;

  //
  // 6.- show basic information from command-line options
  //
  log << " # Selected options" << std::endl;
  log << " - usb         : " << m_usb << std::endl;
  log << " - IP          : " << m_ip << std::endl;
  log << " - outBaseDir  : " << m_outBaseDir << std::endl;
  log << " - logFile     : " << m_outBaseDir+"/"+m_logfile << std::endl;
  log << " - l1delay     : " << std::dec << m_l1delay  << std::endl;
  log << " - emulateTRB  : " << m_emulateTRB << std::endl;
  log << " - calLoop     : " << m_calLoop << std::endl;
  log << " - loadTrim    : " << m_loadTrim << std::endl;
  log << " - doRunNumber : " << m_doRunNumber << std::endl;
  log << " - saveDaq     : " << m_saveDaq  << std::endl;
  log << " - printLevel  : " << std::dec << m_printLevel << std::endl;
  log << " - globalMask  : 0x" << std::hex << std::setfill('0') << unsigned(m_globalMask) << std::endl;
  log << std::dec;
  
  int cnt=0;
  log << " - MODULES    : " << m_modList.size() << std::endl;
  std::vector<Module*>::const_iterator mit;
  for(mit=m_modList.begin(); mit!=m_modList.end(); ++mit){
    log << "   * mod [" << cnt << "] : " << (*mit)->print() << std::endl;
    cnt++;
  }  
  log << " - TESTS      : " << m_testList.size() << std::endl;
  std::vector<ITest*>::const_iterator cit;
  for(cnt=0, cit=m_testList.begin(); cit!=m_testList.end(); ++cit){
    log << "   * test [" << cnt << "] " << std::endl;
    log << (*cit)->print(3) << std::endl;
    cnt++;
  }
  return 1;
}

//------------------------------------------------------
int TrackerCalib::CalibManager::run(){
  auto &log = TrackerCalib::Logger::instance();

  if( m_testList.empty() ){
    log << "==> No tests selected. Nothing to be done..." << std::endl;
    return 1;
  }

  log << std::endl << blue << bold << "[ CalibManager::run ]" << reset << std::endl;

  // loop in tests
  int testcnt(1);
  for(auto t : m_testList){

    // show current progress
    log << std::endl << bold
	<< "###############################################" << std::endl
	<< "  Running test " << testcnt << " / " << m_testList.size() << std::endl
	<< "###############################################" << std::endl
	<< reset << std::endl;
    
    // run test
    t->setRunNumber(m_rman->runNumber());
    if( !t->run(m_trb, m_modList) ) return 0;
    
    // write configuration file after test
    for(auto mod : m_modList)
      mod->writeJson(t->outputDir());

    testcnt++;
  }
  return 1;
}

//------------------------------------------------------
int TrackerCalib::CalibManager::finalize(){
  auto &log = TrackerCalib::Logger::instance();
  
  if( ! m_testList.empty() ){
    log << std::endl << blue << bold << "[ CalibManager::finalize ]" << reset << std::endl;
    
    // show time information for each test in sequence
    log << std::endl << "Timing summary: " << std::endl;
    int cnt(1);
    for(auto t : m_testList){
      log << "   * [" << cnt << "] : " 
	  << std::setw(15) << std::setfill(' ') << t->testName() 
	  << " => " << t->printElapsed() 
	  << std::endl;
      cnt++;
    }
    
    // write final output config file
    writeJson(m_outBaseDir);    
    
    // add run info and stop-time
    if( m_doRunNumber ){
      json jdata;
      jdata["outputDirectory"]=m_outBaseDir;
      jdata["outputLog"]= m_outBaseDir+"/"+m_logfile;    
      
      int res = m_rman->addRunInfo(jdata);
      if(res != 200)
	log << "[CalibManager::finalize] ERROR in addRunInfo. Returned status-code = " << res << std::endl;
    }

    

  }
   
  // This is THE END
  log << std::endl << "GAME OVER -- Insert coin"  << std::endl << std::endl;
  return 1;
}


