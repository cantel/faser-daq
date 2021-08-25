#include "TrackerCalibration/ThresholdScan.h"
#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Utils.h"

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>
#include <TGraph.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#ifdef _MACOSX_
#include <unistd.h>
#endif

using namespace std;

//------------------------------------------------------
TrackerCalib::ThresholdScan::ThresholdScan() :
  ITest(TestType::THRESHOLD_SCAN),
  m_ntrig(100),
  m_charge(1.0), // fC
  m_autoStop(true),
  m_rmode(ABCD_ReadoutMode::LEVEL),
  m_edge(false),
  m_start(0), // DAC cnts
  m_stop(255),// DAC cnts
  m_step(1),  // DAC cnts
  m_nullOccCnt(0),
  m_outFilename(""),
  m_postfix("")
{
  ITest::initTree();
  setStartThreshold();
  clear();
}

//------------------------------------------------------
TrackerCalib::ThresholdScan::ThresholdScan(int ntrig,
					   float charge,
					   bool autoStop,
					   ABCD_ReadoutMode rmode,
					   bool edge) :
  ITest(TestType::THRESHOLD_SCAN),
  m_ntrig(ntrig),
  m_charge(charge),
  m_autoStop(autoStop),
  m_rmode(rmode),
  m_edge(edge),  
  m_start(0),
  m_stop(255),
  m_step(1),
  m_nullOccCnt(0),
  m_outFilename(""),
  m_postfix("")
{
  ITest::initTree();
  setStartThreshold();
  clear();
}

//------------------------------------------------------
TrackerCalib::ThresholdScan::~ThresholdScan() 
{}

//------------------------------------------------------
void TrackerCalib::ThresholdScan::clear(){
  for(int j=0; j<MAXMODS; j++){
    for(int k=0; k<NLINKS; k++){
      for(int l=0; l<NCHIPS; l++){
	for(int m=0; m<NSTRIPS; m++){
	  m_vt50[j][k][l][m]=0;
	  m_evt50[j][k][l][m]=0;
	  m_sigma[j][k][l][m]=0;	  
	  for(int i=0; i<MAXTHR; i++){
	    m_hits[i][j][k][l][m]=0;
	  }	  
	}
      }
    }
  }  
}

//------------------------------------------------------
void TrackerCalib::ThresholdScan::setCharge(float charge) { 
  m_charge = charge; 
  setStartThreshold();
}

//------------------------------------------------------
void TrackerCalib::ThresholdScan::setStartThreshold(){
  float fac(30);
  if( m_charge >= 6 ){
    if( m_charge >= 8 ) fac = 45;
    else fac = 40;
  }
  float tmv = fac*(m_charge-1);
  m_start = tmv < 0 ? 0 : mV2dac(tmv);
}

//------------------------------------------------------
const std::string TrackerCalib::ThresholdScan::print(int indent){
  std::string blank="";
  for(int i=0; i<indent; i++)
    blank += " ";
  
  std::ostringstream out;
  out << ITest::print(indent);  
  out << blank << "   - ntrig        : " << m_ntrig << std::endl;
  out << blank << "   - charge       : " << m_charge << std::endl;
  out << blank << "   - autoStop     : " << m_autoStop << std::endl;
  std::string rmode;
  if(m_rmode == ABCD_ReadoutMode::HIT)        rmode = "HIT";
  else if(m_rmode == ABCD_ReadoutMode::LEVEL) rmode = "LEVEL";
  else if(m_rmode == ABCD_ReadoutMode::EDGE)  rmode = "EDGE";
  else if(m_rmode == ABCD_ReadoutMode::TEST)  rmode = "TEST";  
  out << blank << "   - rmode        : " << rmode     << std::endl;
  out << blank << "   - edge         : " << m_edge    << std::endl;
  out << blank << "   - calLoop      : " << m_calLoop << std::endl;
  out << blank << "   - start        : " << m_start   << std::endl;
  out << blank << "   - stop         : " << m_stop    << std::endl;
  out << blank << "   - step         : " << m_step    << std::endl;  
  return out.str();
}

//------------------------------------------------------
int TrackerCalib::ThresholdScan::run(FASER::TRBAccess *trb,
				     std::vector<Module*> &modList){
  //get logger instance
  auto &log = TrackerCalib::Logger::instance();
  
  // initialize variables
  m_nullOccCnt = 0;
  clear();

  // start timer
  m_timer->start(); 

  // print basic information about scan
  log << bold << green << "# Running "<< testName() 
      << std::dec << std::fixed
      << " L1delay=" << m_l1delay
      << " globalMask=0x" << std::hex << std::setfill('0') << unsigned(m_globalMask)
      << " Ntrig="  << std::dec << m_ntrig 
      << " q[fC]=" << std::setprecision(2) << m_charge 
      << " start="  << m_start
      << " stop="  << m_stop
      << " step="  << m_step
      << " autostop="  << m_autoStop
      << " calLoop="  << m_calLoop
      << " printLevel="  << m_printLevel
      << reset << std::endl;
  
  // check TRBAccess pointer is valid
  if( trb == nullptr){
    log << red << bold << "[ERROR] TRBAccess=0. Exit" << reset << std::endl;
    return 0;    
  }

  if( !initialize(trb,modList) ) return 0;
  if( !execute(trb,modList) ) return 0;
  if( !finalize(trb,modList) ) return 0;

  // stop timer and show elapsed time
  m_timer->stop(); 
  log << bold << green << "Elapsed time: " << m_timer->printElapsed() << reset << std::endl;
  
  return 1;
}

//------------------------------------------------------
int TrackerCalib::ThresholdScan::initialize(FASER::TRBAccess *trb,
					    std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [ThresholdScan::initialize]" << reset << std::endl;
  
  m_postfix = dateStr()+"_"+timeStr();

  //
  // 1. set output binary file for TRBAccess
  //
  if( m_saveDaq ){
    std::string outFilename(m_outDir+"/ThresholdScan_"+m_postfix+".daq");
    trb->SetupStorageStream(outFilename);
    log << "- Binary output file set to : " << outFilename << std::endl;  
  }

  //
  // 2.- configure TRB CalLoopNb
  // 
  FASER::ConfigReg *cfgReg = trb->GetConfig();
  if(cfgReg == nullptr){
    log << red << "Could not get TRB config register. Exit." << reset << std::endl;
    return 0;
  }

  uint32_t calLoopNb(m_ntrig);
  cfgReg->Set_Global_CalLoopNb(calLoopNb);  
  trb->WriteConfigReg();

  //
  // 3. configure SCT modules
  //
  for(auto mod : modList){    
    for(auto chip : mod->Chips()){

      // set chip properties
      chip->setReadoutMode(m_rmode); // readout mode
      chip->setEdge(m_edge); // edge-detect mode
      chip->setCalAmp(m_charge); // input charge

      if( !m_emulateTRB ){

	// SoftReset
	trb->GenerateSoftReset(mod->moduleMask());
	usleep(200);
	
	// write mask register
	//std::vector<uint16_t> mask;
	//or(int i=0; i<8; i++) mask.push_back(0xFFFF);
	//trb->GetSCTSlowCommandBuffer()->SetStripMask(chip->address(), mask);
	//trb->WriteSCTSlowCommandBuffer();
	//trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	//mask.clear();
	
	// write configuration register
	trb->GetSCTSlowCommandBuffer()->SetConfigReg(chip->address(), chip->cfgReg());
	trb->WriteSCTSlowCommandBuffer();
	trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	
	// write bias register
	trb->GetSCTSlowCommandBuffer()->SetBiasDac(chip->address(), chip->biasReg());
	trb->WriteSCTSlowCommandBuffer();
	trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	
	// write threshold / calibration register
	trb->GetSCTSlowCommandBuffer()->SetThreshold(chip->address(), chip->threshcalReg());
	trb->WriteSCTSlowCommandBuffer();
	trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	
	// write delay register
	trb->GetSCTSlowCommandBuffer()->SetStrobeDelay(chip->address(), chip->strobeDelayReg());
	trb->WriteSCTSlowCommandBuffer();
	trb->SendSCTSlowCommandBuffer(mod->moduleMask());

	// loop in strips and set trimDacs
	if( m_loadTrim ){
	  log << " - Configuring chip " << chip->address() 
		    << ". Loading trimDac values..." << std::endl;
	  for(unsigned int istrip=0; istrip<NSTRIPS; istrip++){
	    trb->GetSCTSlowCommandBuffer()->SetTrimDac(chip->address(), chip->trimWord(istrip));
	    trb->WriteSCTSlowCommandBuffer();
	    trb->SendSCTSlowCommandBuffer(mod->moduleMask());	  
	  }
	}
	
      }	
    } // end loop in chips
    
    if(m_printLevel > 0){
      log << "- Initial module configuration :" << std::endl;
      log << "   Module " << mod->print() << std::endl;
    }        
  } // end loop in modules
  
  //
  // 3. tree variables
  //
  t_l1delay     = m_l1delay;
  t_ntrig       = m_ntrig;
  Chip *achip = modList[0]->Chips()[0];
  t_readoutMode = (int)achip->readoutMode();
  t_edgeMode    = achip->edge();
  t_calAmp_n    = 1;
  t_calAmp[0]   = achip->calAmp();
  t_planeID     = modList[0]->planeId();
  t_module_n    = modList.size();
  for(unsigned int i=0; i<t_module_n; i++){
    t_sn[i] = modList[i]->id();
    t_trbChannel[i] = modList[i]->trbChannel();
  }
  
  return 1;
}

//------------------------------------------------------
int TrackerCalib::ThresholdScan::execute(FASER::TRBAccess *trb,
					 std::vector<Module*> &modList){

  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [ThresholdScan::execute]" << reset << std::endl;
  
  // check eventual TRB emulation
  if( m_emulateTRB ){
    log << "emulateTRB = " << m_emulateTRB << ". Nothing to do here... " << std::endl;
    return 1;
  }
  
  // set scan configuration step
  FASER::ScanStepConfig *scanConfig = new FASER::ScanStepConfig;    
  Chip *achip = ((modList[0]->Chips()))[0];
  scanConfig->CalCharge   = fC2dac(achip->calAmp());
  scanConfig->StrobeDelay = achip->strobeDelay();
  scanConfig->TrimRange   = achip->trimRange();
  scanConfig->ExpectedL1A = m_ntrig;	
  
  //
  // 1.- loop in thresholds
  //
  int thrcnt(1);
  const int nThresholds = (int)((float)(m_stop-m_start)/(float)m_step) + 1;
  for(unsigned int ith=m_start; ith<=m_stop; ith+=m_step){ // DAC
    log << blue << " - setting threshold to " 
	      << std::setw(3) << std::dec << ith 
	      << " (" << 2.5*ith << " mV) [" << thrcnt
	      << "/" << nThresholds << "] ..." << reset << std::endl;    
    scanConfig->Threshold = ith;  
    
    //
    // 2.- loop in calmodes
    //
    for(unsigned int ical=0; ical<4; ical++){ 
      if(m_printLevel > 2) 
	log << blue << bold << std::dec << "   - ical = " << ical << " ... " << reset << std::endl;          
      scanConfig->CalMode = ical;
      trb->SaveScanStepConfig(*scanConfig);
      
      // configure chips of modules
      for( auto mod : modList ){ 
	
	//trb->SCT_WriteStripmask(mod->moduleMask(), ical);	
	//uint16_t maskword = 0x1111; 	
	///	maskword <<= ical;
	
	for( auto chip : mod->Chips() ){ 	
	  /*
	  //	  log << chip->print() << std::endl;
	  std::vector<uint16_t> vmask;
	  for(int i=7; i>=0; i--){
	    uint16_t block = 0;	   
	    for(int j=15; j>=0; j--){
	    unsigned int istrip = 16*i+j;
	    block <<= 1;
	    if( chip->isChannelMasked(istrip) == 0 ){ // channel not to be masked
	    block |= 1;
	    }
	    }
	    //log << 16*i << " " << block << std::endl;
	    maskword &= block;
	    vmask.push_back(maskword);
	    }
	  */
	  //	  trb->GetSCTSlowCommandBuffer()->SetStripMask(chip->address(), vmask);
	  trb->GetSCTSlowCommandBuffer()->SetStripMask(chip->address(), chip->getMaskWords(ical));
	  trb->WriteSCTSlowCommandBuffer();
	  trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	  //	  vmask.clear();

	  chip->setCalMode(ical);
	  trb->GetSCTSlowCommandBuffer()->SetConfigReg(chip->address(), chip->cfgReg());
	  trb->WriteSCTSlowCommandBuffer();
	  trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	  
	  chip->setThreshold(2.5*ith); // in mV
	  trb->GetSCTSlowCommandBuffer()->SetThreshold(chip->address(), chip->threshcalReg());
	  trb->WriteSCTSlowCommandBuffer();
	  trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	} 	
	//if(m_printLevel > 0) log << "   Module " << mod->print() << std::endl;	
      } // end loop in modules
      
      // FIFO reset + enable data-taking + SR + Start Readout
      trb->FIFOReset();
      trb->SCT_EnableDataTaking(m_globalMask);
      trb->GenerateSoftReset(m_globalMask);	
      trb->StartReadout();
      
      //
      // 3.- Send triggers 
      //
      if( !m_calLoop ){
	// issue Ntrig x (calpulse+delay+L1A)
	for (unsigned i=0; i<m_ntrig; i++)
	  trb->SCT_CalibrationPulse(m_globalMask, m_l1delay, m_calLoop);
      }
      else{
	trb->SCT_CalibrationPulse(m_globalMask, m_l1delay, m_calLoop);
	usleep(50);
	bool isLoopRunning=false;
	do{ isLoopRunning = trb->IsCalibrationLoopRunning(); } while( isLoopRunning );
      }
      
      // stop readout
      trb->StopReadout();
      
      //
      // 4.- event decoding
      //
      FASER::TRBEventDecoder *ed = new FASER::TRBEventDecoder();
      ed->LoadTRBEventData(trb->GetTRBEventData());
      
      auto evnts = ed->GetEvents();
      std::vector<int> hitsPerModule;
      hitsPerModule.resize(8);	
      for (auto evnt : evnts){ 	
	for (unsigned int imodule=0; imodule<8; imodule++){
	  // imodule effectively corresponds to trbChannel
	  auto sctEvent = evnt->GetModule(imodule); 
	  if (sctEvent != nullptr){
	    hitsPerModule[imodule] += sctEvent->GetNHits();	      
	    auto hits = sctEvent->GetHits(); 
	    int chipcnt(0);
	    for(auto hit : hits){ 
	      for(auto h : hit){ 
		int strip = (int)h.first;		  
		int link = (int)(chipcnt/6);		  
		int ichip = link > 0 ? chipcnt - 6 : chipcnt;
		m_hits[ith][imodule][link][ichip][strip]++;		
	      }
	      chipcnt++;
	    }	    
	  }
	}// end loop in modules 
      }// end loop in events
      
      //
      // 5.- auto-stop scan
      // [SGS] TBD need to do it in a clever way, e.g problem in case of non-masked noisy strips ?
      //
      int tot=0;
      for(unsigned int i=0; i<8; i++)
	tot += hitsPerModule[i];
      
      if(tot == 0)
	m_nullOccCnt++;

      if(m_autoStop && m_nullOccCnt >=12){
	if(ed != nullptr) delete ed;
	delete scanConfig;	  
	log << "Auto-stopping scan...." << std::endl;
	return 1;
      }
      
      if(ed != nullptr)
	delete ed;
      
    } // end loop in calmodes      

    t_threshold[t_threshold_n] = 2.5*ith; // mV
    t_threshold_n++;
    thrcnt++;    

  } // end loop in thresholds

  delete scanConfig;  
  return 1;
}

//------------------------------------------------------
int TrackerCalib::ThresholdScan::finalize(FASER::TRBAccess *trb,
					  std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();  
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [ThresholdScan::finalize]" << reset << std::endl;  
  char name[256], title[256];

  //
  // 1.- Create output ROOT file
  //
  m_outFilename = (m_outDir+"/ThresholdScan_"+m_postfix+".root");
  TFile* outfile = new TFile(m_outFilename.c_str(), "RECREATE"); 
  
  //
  // 2.- TTree with metadata
  //
  if(m_tree){
    m_tree->Fill();
    m_tree->Write();
  }
  
  //
  // 3.- fill and write occupancy histograms
  //
  TH2F *thscan[MAXMODS][NLINKS];  
  for(int j=0; j<MAXMODS; j++){
    for(int k=0; k<NLINKS; k++){
     
      // declare histogram
      sprintf(name,"thscan_m%d_l%d", j, k);
      sprintf(title,"ThresholdScan [module=%d, link=%d]", j, k);
      thscan[j][k] = new TH2F(name, title, 768, -0.5, 767.5, 256, -0.5, 255.5); 
      thscan[j][k]->SetXTitle("Channel number");
      thscan[j][k]->SetYTitle("Threshold [DAC]"); 
      thscan[j][k]->SetZTitle("Occupancy");
      
      // check if link is present
      bool isLinkPresent(false);
      for(auto mod : modList){
	if( mod->trbChannel() == j ){
	  for(auto chip : mod->Chips()){
	    if( (k==0 && chip->address()<40 ) || (k==1 && chip->address()>=40 ) ) 
	      isLinkPresent=true;
	  }
	}
      }

      // only fill occupancy histograms for modules and links present in daq.
      // This is helpful to avoid crashes when attempting a projection on an effectively 
      // empty histogram (i.e. filled with zeroes) by simply checking its number of entries.
      if( isModulePresent(j,m_globalMask) && isLinkPresent ){
	for(int i=0; i<MAXTHR; i++){
	  for(int l=0; l<NCHIPS; l++){
	    for(int m=0; m<NSTRIPS; m++){
	      float occ = (float)m_hits[i][j][k][l][m] / (float)m_ntrig;
	      thscan[j][k]->SetBinContent(l*128+m+1, i+1, occ);      
	    }
	  }
	}
      }
      thscan[j][k]->Write();  
    }
  }

  //
  // 4.- fit scurves, fill vt50 and sigma histograms
  //
  TH1F *hvt50Link[NLINKS];
  TH1F *hsigmaLink[NLINKS];

  log << "# [ThresholdScan] Fitting scurves...." << std::endl;

  for(auto mod : modList){  // loop in modules
    // we take the convention of using trbChannel instead of mod->id() in case we assign a 
    // different TRB channnel to a given module (e.g. module X connected to TRB channel Y)
    // In a standard configuration, mod->id() and mod->trbChannel() should be the same.
    int modnum = mod->trbChannel();
    
    for(int k=0; k<NLINKS; k++){ 
      sprintf(name,"hvt50_m%d_l%d", modnum, k);
      hvt50Link[k] = new TH1F(name, NULL, 100, 0, m_charge*100); 
      hvt50Link[k]->SetXTitle("vt50 [mV]");
      
      sprintf(name,"hsigma_m%d_l%d", modnum, k);
      hsigmaLink[k] = new TH1F(name, NULL, 100, 0, 25); 
      hsigmaLink[k]->SetXTitle("sigma [mV]");
    }
    
    for(auto chip : mod->Chips() ){ // loop in chips          
      int ilink = chip->address() < 40 ? 0 : 1;      
      int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;

      sprintf(name,"hvt50_m%d_l%d_c%d", modnum, ilink, ichip);
      TH1F *hvt50 = new TH1F(name, NULL, 100, 0, m_charge*100); 
      hvt50->SetXTitle("vt50 [mV]");
      
      sprintf(name,"hsigma_m%d_l%d_c%d", modnum, ilink, ichip);
      TH1F *hsigma = new TH1F(name, NULL, 100, 0, 25); 
      hsigma->SetXTitle("sigma [mV]");
      
      // loop in strips of chip
      for(int istrip=0; istrip<NSTRIPS; istrip++){	  
	
	// mask dead channels
	bool channelHasData(false);	
	for(int i=m_start; i<(int)(m_start+t_threshold_n); i++){
	  if( m_hits[i][modnum][ilink][ichip][istrip] != 0 ){
	    channelHasData=true;
	    break;
	  }
	}

	if( !channelHasData ){
	  if( !chip->isChannelMasked(istrip) ){
	    chip->setChannelMask(istrip,1);
	    log << " - Channel " << std::setw(3) << NSTRIPS*ichip + istrip
		<< " [mod " << modnum << ", link " << ilink 
		<< ", chip " << ichip << ", " << std::setw(3) << istrip 
		<< "] is dead => masking..."<< std::endl;
	  }
	  continue;
	}
	
	fitScurves(modnum, ilink, ichip, istrip);	
	double xvt = m_vt50[modnum][ilink][ichip][istrip];  // mV
	double xsg = m_sigma[modnum][ilink][ichip][istrip]; // mV

	hvt50->Fill(xvt); 
	hsigma->Fill(xsg);
	hvt50Link[ilink]->Fill(xvt); 
	hsigmaLink[ilink]->Fill(xsg); 
      }
      
      /******************************************************************************** 
	 Update threshold value in chip object. Note that the true threshold update 
	 (i.e. sending command from the TRB to the chip to update the register) 
	 is to be done somewhere else. 
      **********************************************************************************/
      chip->setThreshold(hvt50->GetMean()); // mV
      
      hvt50->Write();      
      hsigma->Write();      

      delete hvt50;
      delete hsigma;
      
    } // end loop in chips

    for(int k=0; k<NLINKS; k++){
      hvt50Link[k]->Write();
      hsigmaLink[k]->Write();
      delete hvt50Link[k];
      delete hsigmaLink[k];
    }
    log << "# [ThresholdScan] done" << std::endl;    
  } // end loop in modules

  int cnt=0;  
  log << "Modules config after test '" << testName() << "' :"<< std::endl;
  log << " - MODULES    : " << modList.size() << std::endl;
  std::vector<Module*>::const_iterator mit;
  for(mit=modList.begin(); mit!=modList.end(); ++mit){
    log << "   * mod [" << cnt << "] : " << (*mit)->print() << std::endl;
    cnt++;
  }  

  //
  // 5.- close ROOT file
  //
  outfile->Close();       
  log << std::endl << bold << "File '" << m_outFilename 
      << "' created OK" << reset << std::endl << std::endl;  
  return 1;
}

//------------------------------------------------------
void TrackerCalib::ThresholdScan::fitScurves(int imodule, 
					     int ilink, 
					     int ichip, 
					     int istrip){

  //
  // 0. get logger instance
  //
  auto &log = TrackerCalib::Logger::instance();

  /*  log << "####### fitScurves [ " << imodule << " , "   
      << ilink << " , "  << std::setw(2) << ichip << " , "
      << std::setw(3) << istrip << std::endl;  */
  
  //
  // 1. find initial value for vt50 
  //
  /*  float midthr(0);
  for(int i=m_start; i<(int)(m_start+t_threshold_n); i++){
    int nhits = m_hits[i][imodule][ilink][ichip][istrip];
    double occ = nhits / (float)m_ntrig;    
    //log << i << " " << occ << std::endl;
    // occupancy decreases with increasing thresholds. 
    // we stop when we pass the mid-point.
    if(occ < 0.5){
      midthr = (float)i;
      break;
    }
  }

  //
  // 2. Histogram for scurve
  //
  char name[200];
  sprintf(name,"hocc_m%d_l%d_c%d_s%d",imodule,ilink,ichip,istrip);
  TH1F *hocc = new TH1F(name, name, t_threshold_n, (float)m_start, (float)(m_start+t_threshold_n));
  hocc->SetYTitle("Occupancy");
  hocc->SetXTitle("Threshold [DAC]");
  for(int i=m_start; i<(int)(m_start+t_threshold_n); i++){
    int nhits = m_hits[i][imodule][ilink][ichip][istrip];
    double occ = nhits / (float)m_ntrig;
    hocc->SetBinContent(i-m_start+1,occ);
  }

  //
  // 3. fit function
  //
  TF1 *func = new TF1("scurve","0.5*TMath::Erfc((x-[0])/([1]*TMath::Sqrt(2)))", 
		      m_start, m_start+t_threshold_n);
  func->SetParameters(midthr,5); // DAC

  hocc->Fit("scurve", "0RMQ");
	  
  Double_t par[2];
  func->GetParameters(&par[0]);
  float vt50  = par[0] * 2.5; // (mV) 
  float evt50 = func->GetParError(0) * 2.5; // (mV)
  float sigma = par[1] * 2.5; // (mV) / 0.0001602; //(e)
  */

 
  int midthridx(0);
  for(int i=0; i<(int)(t_threshold_n); i++){
    double occ = m_hits[m_start+i][imodule][ilink][ichip][istrip] / (float)m_ntrig;    
    if(occ <= 0.5){
      midthridx = i;
      break;
    }
  }

  //  cout  << "thrmin=" << thrmin << " thrmax=" << thrmax << " midthridx=" << midthridx << endl;

  float thrmin = t_threshold[0];  
  float thrmax = t_threshold[t_threshold_n-1];
  float midthr = t_threshold[midthridx];
  TF1 *func = new TF1("scurve","0.5*TMath::Erfc((x-[0])/([1]*TMath::Sqrt(2)))", thrmin-0.6*thrmin, thrmax+0.1*thrmax);
  func->SetParameters(midthr,10);


  /*  char name[200];
  sprintf(name,"hocc_m%d_l%d_c%d_s%d",imodule,ilink,ichip,istrip);
  TH1F *hocc = new TH1F(name, name, t_threshold_n, thrmin, thrmax);
  hocc->SetYTitle("Occupancy");
  hocc->SetXTitle("Threshold [mV]");
  for(int i=0; i<(int)(t_threshold_n); i++){
    double occ = m_hits[m_start+i][imodule][ilink][ichip][istrip] / (float)m_ntrig;    
    hocc->SetBinContent(i+1,occ);
    }*/

  float xv[MAXTHR], yv[MAXTHR];  
  for(int i=0; i<(int)(t_threshold_n); i++){
    xv[i] = t_threshold[i];
    double occ = m_hits[m_start+i][imodule][ilink][ichip][istrip] / (float)m_ntrig;  
    yv[i] = occ;
  }

  //hocc->Fit("scurve", "0RMQ");
  //double vt50  = hocc->GetFunction("scurve")->GetParameter(0);
  //double evt50 = hocc->GetFunction("scurve")->GetParError(0);
  //ddouble sigma = hocc->GetFunction("scurve")->GetParameter(1);

  TGraph *gr = new TGraph(t_threshold_n, xv, yv);  
  gr->Fit("scurve", "0RMQ");
  double vt50  = gr->GetFunction("scurve")->GetParameter(0);
  double evt50 = gr->GetFunction("scurve")->GetParError(0);
  double sigma = gr->GetFunction("scurve")->GetParameter(1);
	  
  //----------------------------------------------
  m_vt50[imodule][ilink][ichip][istrip]  = vt50;  // mV
  m_evt50[imodule][ilink][ichip][istrip] = evt50; // mV
  m_sigma[imodule][ilink][ichip][istrip] = sigma; // mV

  if( m_printLevel > 1 )
    log << " - fitres [ " 
	<< imodule << " , "   
	<< ilink << " , "
	<< std::setw(2) << ichip << " , "
	<< std::setw(3) << istrip 
	<< " ] " << "midthr(DAC)=" << (int)midthr << " :"
	<< " vt50=" << vt50 << " [mV], " 
	<< " evt50=" << evt50 << " [mV], " 
	<< " sigma=" << sigma << " [mV]" 
      //	<< " p[0]=" << par[0] << " p[1]="  << par[1] 
	<< std::endl;
  
  //  hocc->Write();
  //  delete hocc;

  gr->Write();
  delete gr;

  delete func;
}

//------------------------------------------------------
float TrackerCalib::ThresholdScan::vt50(int imodule,
					int ilink, 
					int ichip,		
					int istrip){
  // check ranges !!!
  // include map
  return m_vt50[imodule][ilink][ichip][istrip];

}

//------------------------------------------------------
float TrackerCalib::ThresholdScan::evt50(int imodule,
					 int ilink, 
					 int ichip,
					 int istrip){
  // check ranges !!!
  // include map
  return m_evt50[imodule][ilink][ichip][istrip];
}

//------------------------------------------------------
float TrackerCalib::ThresholdScan::sigma(int imodule,
					 int ilink, 
					 int ichip,
					 int istrip){
  return m_sigma[imodule][ilink][ichip][istrip];
}

