#include "TrackerCalibration/ITest.h"

#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <sys/stat.h>

//------------------------------------------------------
TrackerCalib::ITest::ITest(TestType testType) :
  m_testType(testType),
  m_l1delay(130),
  m_globalMask(0xFF),
  m_emulateTRB(false),
  m_calLoop(false),
  m_loadTrim(true),
  m_saveDaq(false),
  m_outDir(""),
  m_printLevel(0),
  m_tree(0)
{
  m_timer = new Timer();
}

//------------------------------------------------------
TrackerCalib::ITest::~ITest(){
  delete m_timer;
  if( m_tree ) delete m_tree;
  m_errors.clear();
}

//------------------------------------------------------
const std::string TrackerCalib::ITest::testName(){
  std::string name;
  switch(m_testType){
  case TestType::L1_DELAY_SCAN    : name = "L1DelayScan";    break;
  case TestType::MASK_SCAN        : name = "MaskScan";       break;
  case TestType::THRESHOLD_SCAN   : name = "ThresholdScan";  break;
  case TestType::STROBE_DELAY     : name = "StrobeDelay";    break;
  case TestType::THREE_POINT_GAIN : name = "ThreePtGain";    break;
  case TestType::TRIM_SCAN        : name = "TrimScan";       break;
  case TestType::RESPONSE_CURVE   : name = "ResponseCurve";  break;
  case TestType::NOISE_OCCUPANCY  : name = "NoiseOccupancy"; break;
  case TestType::TRIGGER_BURST    : name = "TriggerBurst";   break;
  case TestType::UNKNOWN: default : name = "unknown";        break;
  }
  return name;
}

//------------------------------------------------------
void TrackerCalib::ITest::setOutputDir(std::string outDir){
  m_outDir = outDir;

  // check if directory already exists. If not, create it.
  char command[500];
  struct stat buffer;
  if( stat(m_outDir.c_str(),&buffer) == -1){
    sprintf(command,"mkdir -p %s", m_outDir.c_str());
    std::system(command);
  }
}

//------------------------------------------------------
const std::string TrackerCalib::ITest::print(int indent){
  std::string blank="";
  for(int i=0; i<=indent; i++)
    blank += " ";
  
  std::ostringstream out;
  out << blank << "  - TestType     : " << testName() << std::endl;
  out << blank << "  - OutDir       : " << m_outDir   << std::endl;
  out << blank << "  - L1delay      : " << m_l1delay  << std::endl;
  out << blank << "  - GlobalMask   : 0x" << std::hex << std::setfill('0') 
      << unsigned(m_globalMask)  << std::endl;
  out << blank << "  - EmulateTRB   : " << m_emulateTRB << std::endl;
  out << blank << "  - CalLoop      : " << m_calLoop    << std::endl;  
  out << blank << "  - LoadTrim     : " << m_loadTrim   << std::endl;
  out << blank << "  - SaveDaq      : " << m_saveDaq    << std::endl;
  return out.str();
}

//------------------------------------------------------
void TrackerCalib::ITest::initTree(){

  //
  // 1.- create TTree
  //
  m_tree = new TTree("tree","Tree with metadata");
 
  // 
  // 2.- set branches
  //
  m_tree->Branch("l1delay",     &t_l1delay,     "l1delay/i");
  m_tree->Branch("ntrig",       &t_ntrig,       "ntrig/i");
  m_tree->Branch("readoutMode", &t_readoutMode, "readoutMode/I");
  m_tree->Branch("edgeMode",    &t_edgeMode,    "edgeMode/O");
  m_tree->Branch("calAmp_n",    &t_calAmp_n,    "calAmp_n/i");
  m_tree->Branch("calAmp",      &t_calAmp,      "calAmp[calAmp_n]/F");
  m_tree->Branch("threshold_n", &t_threshold_n, "threshold_n/i");
  m_tree->Branch("threshold",   &t_threshold,   "threshold[threshold_n]/F");

  m_tree->Branch("planeID",     &t_planeID,     "planeID/i");
  m_tree->Branch("module_n",    &t_module_n,    "module_n/i");
  m_tree->Branch("sn",          &t_sn,          "sn[module_n]/l");
  m_tree->Branch("trbChannel",  &t_trbChannel,  "trbChannel[module_n]/I");
  
  //
  // 3.- initialize tree variables 
  //
  t_l1delay     =  0;
  t_ntrig       =  0;
  t_readoutMode = -1;
  t_edgeMode    = false;
  t_calAmp_n    =  0;

  t_calAmp_n =  0;
  for(int i=0; i<MAXCHARGES; i++) 
    t_calAmp[i] = 0;
  
  t_threshold_n =  0;
  for(int i=0; i<MAXTHR; i++)
    t_threshold[i] = 0;

  t_planeID=0;
  t_module_n = 0;
  for(int i=0; i<MAXMODS; i++){
    t_sn[i]=0;
    t_trbChannel[i]=0;
  }
  
  
}


