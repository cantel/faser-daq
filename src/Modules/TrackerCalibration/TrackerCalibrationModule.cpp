/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
/*
// TrackerCalibration includes*/
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

// Faser-daq modules includes
#include "TrackerCalibrationModule.hpp"
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
TrackerCalibrationModule::TrackerCalibrationModule(const std::string& n): 
FaserProcess(n) 
{ 
  INFO("constructor");

  ERS_INFO(""); 
}

//-----------------------------------------
TrackerCalibrationModule::~TrackerCalibrationModule() { ERS_INFO(""); }


//------------------ Main functions ---------------

// optional (configuration can be handled in the constructor)
void TrackerCalibrationModule::configure() {
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
  m_usb = cfg["usb"];
  m_ip = cfg["ip"];
  

  INFO("Got tcalib parameters");
    

  //
  // 1.- populate list of modules
  //

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
      m_testList.push_back(itest);
    }
  } // end loop in tests

  
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


  }

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

void TrackerCalibrationModule::start(unsigned run_num) {
  ERS_INFO("Starting...");

  m_t = m_testList.at(m_testcnt - 1);

  // show current progress
  INFO(std::endl
  << "###############################################" << std::endl
  << "  Running test " << m_testcnt << " / " << m_testList.size() << std::endl
  << "###############################################" << std::endl);
  
  // run prep and initialize

  if( !m_t->initializeRun(m_trb, m_modList) ) ERROR("Failed to initialize test of type " << m_t->print());

  FaserProcess::start(run_num);

  ERS_INFO("Done starting");
}

void TrackerCalibrationModule::stop() {
  FaserProcess::stop();

  /** If there are no more tests, finalize the whole sequence.
   *  Similar to CalibManager::finalize **/

  ERS_INFO("Stopped calibration run");
}

void TrackerCalibrationModule::runner() noexcept {
  ERS_INFO("Running...");
  
  if( !m_t->execute(m_trb,m_modList) ) ERROR("Failed to execute test of type " << m_t->print());

  if( !m_t->finalizeRun(m_trb,m_modList) ) ERROR("Failed to finalize test of type " << m_t->print());;
  
  ERS_INFO("Run stopped");

 
}
