#include "TrackerCalibration/StrobeDelay.h"
#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/ThresholdScan.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Chip.h"
#include "TrackerCalibration/Utils.h"

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#ifdef _MACOSX_
#include <unistd.h>
#endif

//------------------------------------------------------
TrackerCalib::StrobeDelay::StrobeDelay() :
  ITest(TestType::STROBE_DELAY),
  m_ntrig(100),
  m_postfix("")
{
  ITest::initTree();
  clear();
}

//------------------------------------------------------
TrackerCalib::StrobeDelay::~StrobeDelay() {
  clear();
}

//------------------------------------------------------
void TrackerCalib::StrobeDelay::clear() {
  // initialize arrays
  for(int i=0; i<MAXSD; i++)
    for(int j=0; j<MAXMODS; j++)
      for(int k=0; k<NLINKS; k++)
	for(int l=0; l<NCHIPS; l++)
	  for(int m=0; m<NSTRIPS; m++)      
	    m_hits[i][j][k][l][m]=0;
}

//------------------------------------------------------
const std::string TrackerCalib::StrobeDelay::print(int indent){
  std::string blank="";
  for(int i=0; i<indent; i++)
    blank += " ";
  
  std::ostringstream out;
  out << ITest::print(indent);  
  out << blank << "   - ntrig        : " << m_ntrig << std::endl;
  return out.str();
}

//------------------------------------------------------
int TrackerCalib::StrobeDelay::run(FASER::TRBAccess *trb,
				   std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << green << "# Running "<< testName() << reset << std::endl;
  
  // initialize variables
  clear();

  // start timer
  m_timer->start(); 
  
  // check TRBAccess pointer is valid
  if( trb == nullptr){
    log << red << bold << "[ERROR] TRBAccess=0. Exit" << reset << std::endl;
    return 0;    
  }

  //
  // 1.- run threshold scan (2fC charge)
  // 
  ThresholdScan *thscan = new ThresholdScan(100, // ntrig
					    2.0, // fC
					    true, // autostop
					    ABCD_ReadoutMode::LEVEL, // X1X
					    false);// edge-mode
  thscan->setStart(0);
  thscan->setStop(140);
  thscan->setStep(1);  
  thscan->setGlobalMask(m_globalMask);  
  thscan->setCalLoop(m_calLoop);
  thscan->setLoadTrim(m_loadTrim); 
  thscan->setSaveDaq(m_saveDaq); 
  thscan->setPrintLevel(m_printLevel);  
  thscan->setOutputDir(m_outDir);  
  thscan->setRunNumber(m_runNumber);

  if( !thscan->run(trb,modList) ) return 0;
  delete thscan;

  //
  // 2.- perform Strobe-delay scan (4fC threshold)
  //
  if(!initialize(trb,modList)) return 0;
  if(!execute(trb,modList)) return 0;
  if(!finalize(trb,modList)) return 0;

  // stop timer and show elapsed time
  m_timer->stop(); 
  log << bold << green << "Elapsed time: " << m_timer->printElapsed() << reset << std::endl;

  return 1;
}
  
//------------------------------------------------------
int TrackerCalib::StrobeDelay::initialize(FASER::TRBAccess *trb,
					    std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [StrobeDelay::initialize]" << reset << std::endl;

  m_postfix = dateStr()+"_"+timeStr();

  //
  // 1. set output binary file for TRBAccess
  //
  if( m_saveDaq ){
    std::string outFilename(m_outDir+"/StrobeDelay_"+m_postfix+".daq");
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
      
      /* Set chip properties. Threshold value should have been 
	 already set in chip class instances from the initial threshold-scan. */
      chip->setReadoutMode(ABCD_ReadoutMode::EDGE); 
      chip->setEdge(true); 
      chip->setCalAmp(4.0); 

      if( !m_emulateTRB ){

	// SoftReset
	trb->GenerateSoftReset(mod->moduleMask());
	usleep(200);
	
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
      }	
    } // end loop in chips
    
    if(m_printLevel > 0){
      log << "- Initial module configuration :" << std::endl;
      log << "   Module " << mod->print() << std::endl;
    }    
  } // end loop in modules

  //
  // tree variables
  //
  t_l1delay     = m_l1delay;
  t_ntrig       = m_ntrig;
  Chip *achip = modList[0]->Chips()[0];
  t_readoutMode  = (int)achip->readoutMode();
  t_edgeMode     = achip->edge();
  t_calAmp_n     = 1;
  t_calAmp[0]    = achip->calAmp();
  t_planeID      = modList[0]->planeId();
  t_module_n     = modList.size();
  for(unsigned int i=0; i<t_module_n; i++){
    t_sn[i]         = modList[i]->id();
    t_trbChannel[i] = modList[i]->trbChannel();
  }
 
  return 1;
}

//------------------------------------------------------
int TrackerCalib::StrobeDelay::execute(FASER::TRBAccess *trb,
					 std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [StrobeDelay::execute]" << reset << std::endl;
  
  // check eventual TRB emulation
  if( m_emulateTRB ){
    log << "emulateTRB = " << m_emulateTRB << ". Nothing to do here... " << std::endl;
    return 1;
  }
  
  // set scan configuration step
  FASER::ScanStepConfig *scanConfig = new FASER::ScanStepConfig;    
  Chip *achip = ((modList[0]->Chips()))[0];
  scanConfig->CalCharge   = fC2dac(achip->calAmp());
  //scanConfig->Threshold = ith;  
  scanConfig->TrimRange   = achip->trimRange();
  scanConfig->ExpectedL1A = m_ntrig;	
  
  //
  // 1.- loop in strobe-delay values
  //
  for(unsigned int isd=0; isd<64; isd++){
    log << blue << " - setting SD to " << std::dec << std::setw(2) << isd 
	<< " [" << std::setw(2) << isd << "/64]..." << reset << std::endl;    
    scanConfig->StrobeDelay = isd;
    
    //
    // 2.- loop in calmodes
    //
    for(unsigned int ical=0; ical<4; ical++){ 
      scanConfig->CalMode = ical;
      trb->SaveScanStepConfig(*scanConfig);
      
      // configure chips of modules
      for( auto mod : modList ){ 
	trb->SCT_WriteStripmask(mod->moduleMask(), ical);
	
	for( auto chip : mod->Chips() ){ 	
	  
	  // set calmode
	  chip->setCalMode(ical);
	  trb->GetSCTSlowCommandBuffer()->SetConfigReg(chip->address(), chip->cfgReg());
	  trb->WriteSCTSlowCommandBuffer();
	  trb->SendSCTSlowCommandBuffer(mod->moduleMask());

	  // set strobe-delay
	  chip->setStrobeDelay(isd); 
	  trb->GetSCTSlowCommandBuffer()->SetStrobeDelay(chip->address(), chip->strobeDelayReg());
	  trb->WriteSCTSlowCommandBuffer();
	  trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	} 	
      } // end loop in modules
      
      // FIFO reset + enable data-taking + SR + Start Readout
      trb->FIFOReset();
      trb->SCT_EnableDataTaking(m_globalMask);
      trb->GenerateSoftReset(m_globalMask);	
      trb->StartReadout();
      
      //
      // 3.- issue Ntrig x (calpulse+delay+L1A)
      //
      //for (unsigned i=0; i<m_ntrig; i++)
      //trb->SCT_CalibrationPulse(m_globalMask, m_l1delay);

      if( !m_calLoop ){
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
		m_hits[isd][imodule][link][ichip][strip]++;		
	      }

	      chipcnt++;
	    }	    
	  }
	}// end loop in modules 
      }// end loop in events
      
      if(ed != nullptr)
	delete ed;
      
    } // end loop in calmodes      
    
  } // end loop in thresholds
    
  delete scanConfig;   

  return 1;
}

//------------------------------------------------------
int TrackerCalib::StrobeDelay::finalize(FASER::TRBAccess *trb,
					  std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [StrobeDelay::finalize]" << reset << std::endl;  
  char name[256], title[256];

  //
  // 1.- Create output ROOT file
  //
  std::string outFilename(m_outDir+"/StrobeDelay_"+m_postfix+".root");
  TFile* outfile = new TFile(outFilename.c_str(), "RECREATE");  
  
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
  TH2F *m_sdscan[MAXMODS][NLINKS];  
  for(int j=0; j<MAXMODS; j++){
    for(int k=0; k<NLINKS; k++){
      sprintf(name,"sdscan_m%d_l%d", j, k);
      sprintf(title,"StrobeDelay [module=%d, link=%d]", j, k);
      m_sdscan[j][k] = new TH2F(name, title, 768, -0.5, 767.5, 64, 0, 64); 
      m_sdscan[j][k]->SetXTitle("Channel number");
      m_sdscan[j][k]->SetYTitle("StrobeDelay [DAC units]");
      m_sdscan[j][k]->SetZTitle("Occupancy");
      
      if( isModulePresent(j,m_globalMask) ){
	for(int i=0; i<MAXSD; i++){
	  for(int l=0; l<NCHIPS; l++){
	    for(int m=0; m<NSTRIPS; m++){
	      float occ = (float)m_hits[i][j][k][l][m] / (float)m_ntrig;
	      m_sdscan[j][k]->SetBinContent(l*NSTRIPS+m+1, i+1, occ);      
	    }
	  }
	}
      }
      m_sdscan[j][k]->Write();  
    }
  }

  //
  // 4.- determine SD-value per chip, update chip registers
  //
  for(auto mod : modList){    
    int modnum = mod->trbChannel();

    for(auto chip : mod->Chips() ){    
      int ilink = chip->address() < 40 ? 0 : 1;      
      int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;

      sprintf(name,"hsd_m%d_l%d_c%d", modnum, ilink, ichip);
      sprintf(title,"SD distribution [mod=%d, link=%d, chip=%d]", modnum, ilink, ichip);
      TH1F *hsd = new TH1F(name, title, 64, 0, 64); 
      hsd->SetXTitle("Strobe-Delay [DAC]");

      // loop in strips of chip and project occupancies
      for(int istrip=0; istrip<NSTRIPS; istrip++){	  

	int bin = ichip*NSTRIPS + istrip + 1;
	//log << "istrip "<< istrip << " bin=" << bin << std::endl;

	sprintf(name,"Projection_m%d_l%d_c%d_s%d", modnum, ilink, ichip, istrip);
	TH1D *proj = m_sdscan[modnum][ilink]->ProjectionY(name, bin, bin);      
	if(proj != nullptr) {	  

	  int plateau_start(0);
	  for(int bx=1; bx<=proj->GetNbinsX(); ++bx){
	    if( proj->GetBinContent(bx) > 0.95 ){
	      plateau_start = bx;
	      break;
	    }
	  }
	  
	  int plateau_end(0);
	  for(int bx=proj->GetNbinsX(); bx>=1; --bx){
	    if( proj->GetBinContent(bx) > 0.95 ) {
	      plateau_end = bx;
	      break;
	    }
	  }
	  
	  float sd = plateau_start + (plateau_end - plateau_start)*0.25;
	  
	  if(m_printLevel > 1)
	    log << " - fitres [ " 
		<< modnum << " , " 
		<< ilink << ","
		<< std::setw(2) << ichip << " , "
		<< std::setw(3) << istrip 
		<< " ] = " 
		<< bin << " , "
		<< plateau_start << " , "
		<< plateau_end << " , "
		<< sd << std::endl;
	  
	  hsd->Fill(sd);
	}

	delete proj;

      } // end loop in strips     

      hsd->Write();

      int mean = (int)hsd->GetMean();

      // set strobe-delay
      chip->setStrobeDelay(mean);
      trb->GetSCTSlowCommandBuffer()->SetStrobeDelay(chip->address(), chip->strobeDelayReg());
      trb->WriteSCTSlowCommandBuffer();
      trb->SendSCTSlowCommandBuffer(mod->moduleMask());

      delete hsd;

    } // end loop in chips
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
  log << "File '" << outFilename 
      << "' created OK" << std::endl << std::endl;  
  return 1;
}
