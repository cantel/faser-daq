/** \file thscan.cxx
 * Simple standalone program to perform a trigger burst test.
 */

#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/ITest.h"
#include "TrackerCalibration/TriggerBurst.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Chip.h"
#include "TrackerCalibration/Utils.h"

#include "TrackerReadout/TRBAccess.h"
#include "TrackerReadout/TRB_ConfigRegisters.h"
#include "TrackerReadout/ConfigurationHandling.h"

#include <iostream>
#include <stdlib.h>
#include <iomanip>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

using namespace std;
using namespace TrackerCalib;

// trigger burst type: 0=L1A, 1=(CalPulse+delay+L1A)
static int burstType(0);

// threshold  
static float threshold(50); /// 50 mV

// total number of triggers to be sent
static int ntrig(1000);

// input calibration charge 
static float charge(1.0); /// 1 fC

// readout mode as int (see help)
static int readoutMode(1); /// LEVEL

// input json configuration file
static std::string ifile(""); 

// output log file
static std::string logfile("log.txt");

 // L1delay 
static unsigned int l1delay(130);

// load Trim values in chip
static bool loadTrim(true); 

// use TRB connection via USB (ethernet otherwise)
bool usb(false); 

// SCIP / DAQIP address for ethernet (assumed to be the same for the moment)
static std::string ipAddress("10.11.65.6");

// base output directory
static std::string outBaseDir="~/run"; 

// verbosity level
static int printLevel(0);

//------------------------------------------------------
void help(){
  std::cout << std::endl;
  std::cout << "Usage: ./tburst -i <JSON_FILE> [OPTION(s)] " << std::endl;
  std::cout << std::endl;
  std::cout << "where : " << std::endl;
  std::cout << "  <JSON_FILE>  Input json configuration file (full plane or single module)" << std::endl;
  std::cout << std::endl;
  std::cout << "Optional arguments: " << std::endl;
  std::cout << "   -b, --burstType <TYPE>          Burst type: 0=L1A, 1=(CalPulse+delay+L1A) (default=" << burstType << ")" << std::endl;
  std::cout << "       --threshold <THRESHOLD>     Threshold in mV (default=" << threshold << ")" << std::endl;
  std::cout << "   -t, --trig <NTRIGGERS>          Number of triggers at each step (default=" << ntrig << ")" << std::endl;
  std::cout << "   -q, --charge <CHARGE>           Input calibration charge in fC (default: " << charge << " fC). Used only when burstType=1." << std::endl;
  std::cout << "   -r, --readout-mode <MODE>       Readout mode for ABCD: 0=HIT (1XX, X1X or XX1), 1=LEVEL (X1X), 2=EDGE (01X), 3=Test (XXX) (default: " << readoutMode << ")" << std::endl;
  std::cout << "   -n, --noTrim                    Do not load in chips the single-channel trimDac values (default=false)." << std::endl;

  std::cout << std::endl;
  std::cout << "   -o, --outBaseDir <OUTDIR>       Base output results directory (default: " << outBaseDir << ")." << std::endl;
  std::cout << "   -v, --verbose <VERBOSE LEVEL>   Sets the verbosity level from 0 to 3 included (default: 0)." << std::endl;
  std::cout << "   -d, --l1delay <L1DELAY>         delay between calibration-pulse and L1A (default: " << l1delay << ")" << std::endl;
  std::cout << "   -l, --log <LOG_FILE>            Output logfile, relative to outBaseDir (default: " << logfile << ")." << std::endl;
  std::cout << "   -u, --usb                       Use TRB connected via USB (default=false)." << std::endl;
  std::cout << "       --ip <IPADDRESS>            SCIP / DAQIP address for ethernet communication (default: " << ipAddress << ")." << std::endl;
  std::cout << "   -h, --help                      Displays this information and exit." << std::endl;
  std::cout << std::endl;
  exit(0);
}

//------------------------------------------------------
int getParams(int argc, char **argv) {
  int ip=1;
  int res=1;
  
  if(argc == 1){
    help();
  }
  else{
    while( ip < argc ){
      //std::cout << "Analyzing argv[" << ip << "] = " << argv[ip] << std::endl;
      
      if(std::string(argv[ip]).substr(0,2) == "--" || std::string(argv[ip]).substr(0,1) == "-"){
	// help
	if(std::string(argv[ip])=="--help" || std::string(argv[ip])=="-h") {
	  help();
	  break;
	}
	// burst type
	else if(std::string(argv[ip]) == "--burstType" || std::string(argv[ip])=="-b")
	  {
	    if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	      burstType = atoi(argv[ip+1]); ip+=2; }
	    else{ std::cout << std::endl << "[tburst] Missing number of triggers." << std::endl << std::endl; res=0; break; }
	  }
	// threshold
	else if(std::string(argv[ip]) == "--threshold")
	  {
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    threshold=atof(argv[ip+1]); ip+=2; }
	  else{ std::cout << std::endl << "[tburst] Missing calibration charge." << std::endl << std::endl; res=0; break; }
	  }
	// number of triggers
	else if(std::string(argv[ip]) == "--trig" || std::string(argv[ip])=="-t")
	  {
	    if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	      ntrig = atoi(argv[ip+1]); ip+=2; }
	    else{ std::cout << std::endl << "[tburst] Missing number of triggers." << std::endl << std::endl; res=0; break; }
	  }
	// charge
	else if(std::string(argv[ip]) == "--charge" || std::string(argv[ip])=="-q")
	  {
	    if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	      charge = atof(argv[ip+1]); ip+=2; }
	    else{ std::cout << std::endl << "[tburst] Missing calibration charge." << std::endl << std::endl; res=0; break; }
	  }
	// readout-mode
	else if(std::string(argv[ip]) == "--readout-mode" || std::string(argv[ip])=="-r")
	  {
	    if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	      readoutMode = atoi(argv[ip+1]);
	      if(readoutMode < 0 || readoutMode > 3){
		std::string message = "\n\nUnknown readoutMode : "+int2str(readoutMode)+".";
		throw std::runtime_error(message);
	      }
	      ip+=2; 
	    } else{ std::cout << std::endl << "[tburst] Missing readout-mode." << std::endl << std::endl; res=0; break; }
	  }	
	// L1delay
	else if(std::string(argv[ip]) == "--l1delay" || std::string(argv[ip])=="-d")
	  {
	    if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	      l1delay = atoi(argv[ip+1]); ip+=2; }
	    else{ std::cout << std::endl << "[tburst] Missing argument." << std::endl << std::endl; res=0; break; }
	  }
	// noTrim
	else if(std::string(argv[ip])=="--noTrim" || std::string(argv[ip])=="-n") {
	  loadTrim=false;
	  ip+=1; 
	}
	// usb
	else if(std::string(argv[ip])=="--usb" || std::string(argv[ip])=="-u") {
	  usb=true;
	  ip+=1; 
	}
	// output base directory
	else if(std::string(argv[ip]) == "--odir" || std::string(argv[ip])=="-o")
	  {
	    if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	      outBaseDir = argv[ip+1]; ip+=2; }
	    else{ std::cout << std::endl << "[tburst] Missing output directory." << std::endl << std::endl; res=0; break; }
	  }
	// input file
	else if(std::string(argv[ip]) == "--ifile" || std::string(argv[ip])=="-i")
	  {
	    if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	      ifile = argv[ip+1]; ip+=2; }
	    else{ std::cout << std::endl << "[tburst] Missing input file." << std::endl << std::endl; res=0; break; }
	  }
	// log file
	else if(std::string(argv[ip]) == "--logfile" || std::string(argv[ip])=="-l")
	  {
	    if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	      logfile = argv[ip+1]; ip+=2; }
	    else{ std::cout << std::endl << "[tburst] Missing logfile." << std::endl << std::endl; res=0; break; }
	  }
	// ip address
	else if(std::string(argv[ip]) == "--ip"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--") {
	    ipAddress = argv[ip+1]; ip+=2; }
	  else{ std::cout << std::endl << "[tcalib] Missing IP address parameter." << std::endl << std::endl; res=0; break;}
	}
	// verbose level
	else if(std::string(argv[ip]) == "--verbose" || std::string(argv[ip])=="-v"){
	  if(ip+1<argc && std::string(argv[ip+1]).substr(0,2) != "--" && std::string(argv[ip+1]).substr(0,1) != "-") {
	    printLevel = atoi(argv[ip+1]); 
	    ip+=2; 
	  } else{
	    printLevel=1;
	    ip+=1;
	  }
	}
	else{
	  std::cout << std::endl << "Invalid argument: " << argv[1] << std::endl << std::endl;
	  res=0;
	  break;
	}
      }
      else{
	std::cout << std::endl << "Invalid argument: " << argv[1] << std::endl << std::endl;
	res=0;
	break;
      }
    }
  }
  return res;
}

//------------------------------------------------------
int main(int argc, char *argv[]){
  
  if( !getParams(argc,argv) )
    return 0;
  
  //-----------------------------
  // 1.- sanity checks
  //-----------------------------
  if( ifile.empty() ){
    std::cout << "[tburst] Missing input cfg file. Exit..." << std::endl;
    return 0;
  }
  
  struct stat buffer;
  if( stat(ifile.c_str(),&buffer) == -1 ){
    std::cout << "[tburst] Input cfg file does not exist. Exit..." << std::endl;
    return 0;
  }
  
  //-----------------------------
  // 2.- Logger
  //-----------------------------
  auto &log = TrackerCalib::Logger::instance();
  
  // create buffer file
  char ctmp[] = "/tmp/log_XXXXXXXX";
  if(mkstemp(ctmp) == -1) {
    std::cout << "Failed to create temporary file" << std::endl;
    return 0;    
  }
  std::string tmplog(ctmp);
  std::ofstream ofile(tmplog);
  log.init(std::cout,ofile);
  
  //-----------------------------
  // 3. Module 
  //-----------------------------
  std::vector<Module*> modList;
  Module *mod = new Module(ifile,printLevel);
  modList.push_back(mod);
  
  // compute globalMask
  uint8_t globalMask(0); 
  for(auto mod : modList){
    int trbchan = mod->trbChannel();    
    globalMask |= (0x1 << trbchan);
  }
  
  //-----------------------------
  // 4.- TRB
  //-----------------------------
  int boardId = modList.at(0)->planeId();
  log << "TRB BoardID (PlaneID in config file): " << boardId << std::endl;

  FASER::TRBAccess *trb = usb ? 
    new FASER::TRBAccess(boardId, false, boardId) :
    new FASER::TRBAccess(ipAddress, ipAddress, boardId, false, boardId);
  trb->ShowTransfers(false);

  FASER::ConfigReg *cfgReg = trb->GetConfig();
  cfgReg->Set_Module_L1En(0x0); // disable hardware L1A
  cfgReg->Set_Module_ClkCmdSelect(0x0); // select CLK/CMD 0
  cfgReg->Set_Module_LedRXEn(0x0); // disable led lines
  cfgReg->Set_Module_LedxRXEn(0x0); // disable ledx lines
  trb->WriteConfigReg();

  FASER::PhaseReg *phaseReg = trb->GetPhaseConfig();
  phaseReg->SetFinePhase_Clk0(0);
  phaseReg->SetFinePhase_Led0(37);
  phaseReg->SetFinePhase_Clk1(0);
  phaseReg->SetFinePhase_Led1(37);
  trb->WritePhaseConfigReg();
  trb->ApplyPhaseConfig();

  bool dataLED(false);
  bool dataLEDx(false);
  for(auto mod : modList){
    for(auto chip : mod->Chips()){
      if( chip->address() < 40 ) 
	dataLED=true;
      else
	dataLEDx=true;
    }
  }
  if(dataLED)  cfgReg->Set_Module_LedRXEn(globalMask);
  if(dataLEDx) cfgReg->Set_Module_LedxRXEn(globalMask);        
  cfgReg->Set_Global_L2SoftL1AEn(true);      // enable software L1A
  cfgReg->Set_Global_RxTimeoutDisable(true); // disable RxTimeout
  cfgReg->Set_Global_L1TimeoutDisable(false);// enable L1Timeout
  cfgReg->Set_Global_Overflow(4095);
  cfgReg->Set_Global_CalLoopNb(100); // number of L1A
  cfgReg->Set_Global_CalLoopDelay(1000); // delay (in 100 ns unit)
  trb->WriteConfigReg();
   
  //-----------------------------
  // 5.- TriggerBurst
  //-----------------------------
  TriggerBurst *tburst = new TriggerBurst();
  tburst->setBurstType(burstType);
  tburst->setNtrig(ntrig);
  tburst->setThreshold(threshold);
  tburst->setCharge(charge);
  ABCD_ReadoutMode rmode(ABCD_ReadoutMode::LEVEL);
  switch(readoutMode){
  case 0: rmode = ABCD_ReadoutMode::HIT  ; break;
  case 1: rmode = ABCD_ReadoutMode::LEVEL; break;
  case 2: rmode = ABCD_ReadoutMode::EDGE ; break;
  case 3: rmode = ABCD_ReadoutMode::TEST ; break;
  default: break;    
  }
  tburst->setReadoutMode(rmode);
  tburst->setOutputDir(outBaseDir+"/TriggerBurst_"+dateStr()+"_"+timeStr());
  tburst->setL1delay(l1delay);
  tburst->setGlobalMask(globalMask);
  tburst->setLoadTrim(loadTrim);
  tburst->setPrintLevel(printLevel);  

  // show basic information from command-line options
  log << std::endl;
  log << " # Selected options" << std::endl;
  log << " - burstType   : " << burstType << std::endl;
  log << " - ntrig       : " << ntrig << std::endl;
  log << " - threshold   : " << threshold << std::endl;
  log << " - charge      : " << charge << std::endl;
  log << " - readoutMode : " << readoutMode << std::endl;
  log << " - outBaseDir  : " << outBaseDir << std::endl;
  log << " - logFile     : " << outBaseDir+"/"+logfile << std::endl;
  log << " - l1delay     : " << std::dec << l1delay  << std::endl;
  log << " - loadTrim    : " << loadTrim << std::endl;
  log << " - printLevel  : " << std::dec << printLevel << std::endl;
  log << " - globalMask  : 0x" << std::hex << std::setfill('0') << unsigned(globalMask) << std::endl;
  log << std::dec;
  int cnt=0;
  log << " - MODULES    : " << modList.size() << std::endl;
  std::vector<Module*>::const_iterator mit;
  for(mit=modList.begin(); mit!=modList.end(); ++mit){
    log << "   * mod [" << cnt << "] : " << (*mit)->print() << std::endl;
    cnt++;
  } 
  
  log << std::endl << " - TRIGGER-BURST params : " << std::endl;
  log << tburst->print() << std::endl;
  
  // run 
  tburst->run(trb,modList);
  
  //-----------------------------
  // 6. cleanup
  //-----------------------------  
  delete tburst;
  delete trb;

  for(auto m : modList)
    delete m;
  modList.clear();

  return 0;
}
