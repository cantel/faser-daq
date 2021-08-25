#include "TrackerCalibration/MaskScan.h"
#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Utils.h"

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TF1.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#ifdef _MACOSX_
#include <unistd.h>
#endif

//------------------------------------------------------
TrackerCalib::MaskScan::MaskScan() :
  ITest(TestType::MASK_SCAN),
  m_ntrig(1000),
  m_outFilename(""),
  m_postfix("")
{
  ITest::initTree();
  clear();
}

//------------------------------------------------------
TrackerCalib::MaskScan::MaskScan(int ntrig) :
  ITest(TestType::MASK_SCAN),
  m_ntrig(ntrig),
  m_outFilename(""),
  m_postfix("")
{
  ITest::initTree();
  clear();
}

//------------------------------------------------------
TrackerCalib::MaskScan::~MaskScan() 
{}

//------------------------------------------------------
void TrackerCalib::MaskScan::clear(){

  // initialize arrays
  for(int i=0; i<2; i++){
    for(int j=0; j<MAXMODS; j++){
      for(int k=0; k<NLINKS; k++){
	for(int l=0; l<NCHIPS; l++){
	  for(int m=0; m<NSTRIPS; m++){
	    m_hits[i][j][k][l][m]=0;
	  }	  
	}
      }
    }
  }
}

//------------------------------------------------------
const std::string TrackerCalib::MaskScan::print(int indent){
  std::string blank="";
  for(int i=0; i<indent; i++)
    blank += " ";
  
  std::ostringstream out;
  out << ITest::print(indent);  
  out << blank << "   - ntrig        : " << m_ntrig << std::endl;
  return out.str();
}

//------------------------------------------------------
int TrackerCalib::MaskScan::run(FASER::TRBAccess *trb,
				std::vector<Module*> &modList){
  // get logger instance
  auto &log = TrackerCalib::Logger::instance();
  
  // initialize variables
  clear();

  // start timer
  m_timer->start(); 

  // print basic information about scan
  log << bold << green << "# Running "<< testName() 
      << std::dec << std::fixed
      << " L1delay=" << m_l1delay
      << " globalMask=0x" << std::hex << std::setfill('0') << unsigned(m_globalMask)
      << " Ntrig="  << std::dec << m_ntrig 
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
int TrackerCalib::MaskScan::initialize(FASER::TRBAccess *trb,
				       std::vector<Module*> &modList){

  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [MaskScan::initialize]" << reset << std::endl;

  m_postfix = dateStr()+"_"+timeStr();

  //
  // 1. set output binary file for TRBAccess
  //
  if( m_saveDaq ){
    std::string outFilename(m_outDir+"/MaskScan_"+m_postfix+".daq");
    trb->SetupStorageStream(outFilename);
    log << "- Binary output file set to : " << outFilename << std::endl;  
  }
  
  //
  // 2. configure SCT modules
  //
  for(auto mod : modList){    
    log << "- Configuring module " << mod->trbChannel()  << std::endl;
    for(auto chip : mod->Chips()){

      // set chip properties
      chip->setReadoutMode(ABCD_ReadoutMode::LEVEL); // readout mode
      chip->setEdge(false); // edge-detect mode
      chip->setCalAmp(0); // input charge
      chip->setTrimRange(0); // trim range

      if( !m_emulateTRB ){

	// SoftReset
	trb->GenerateSoftReset(mod->moduleMask());
	usleep(200);
	
	// force enable all channels
	for(unsigned int istrip=0; istrip<NSTRIPS; istrip++){ 
	  chip->setChannelMask(istrip,0);
	}
	chip->prepareMaskWords();	
	
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
	log << "  - Configuring chip " << chip->address() 
		  << ": setting trimDac values to 0 for all channels..." 
		  << std::endl;	
	for(unsigned int istrip=0; istrip<NSTRIPS; istrip++){
	  chip->setTrimDac(istrip,0);
	  trb->GetSCTSlowCommandBuffer()->SetTrimDac(chip->address(), chip->trimWord(istrip));
	  trb->WriteSCTSlowCommandBuffer();
	  trb->SendSCTSlowCommandBuffer(mod->moduleMask());	  
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
  t_ntrig        =  m_ntrig;
  Chip *achip = modList[0]->Chips()[0];
  t_readoutMode  = (int)achip->readoutMode();
  t_edgeMode     = achip->edge();
  t_planeID      = modList[0]->planeId();
  t_module_n     = modList.size();
  for(unsigned int i=0; i<t_module_n; i++){
    t_sn[i]         = modList[i]->id();
    t_trbChannel[i] = modList[i]->trbChannel();
  }

  return 1;
}

//------------------------------------------------------
int TrackerCalib::MaskScan::execute(FASER::TRBAccess *trb,
				    std::vector<Module*> &modList){

  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [MaskScan::execute]" << reset << std::endl;
  
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

  /* array of threshold points. Just two thresholds defined:
     i)    0 mV -> check dead  strips
     ii) 250 mV -> check noisy strips
  */
  float thresholds[2]={0,250}; // mV

  for(unsigned int ith=0; ith<2; ith++){
    log << blue << " - setting threshold to " 
	<< std::setw(3) << std::dec << (int)thresholds[ith] << " mV ["
	<< ith+1 << "/2]..." << reset << std::endl;    
    scanConfig->Threshold = ith;  
    
    //
    // 2.- loop in calmodes
    //
    for(unsigned int ical=0; ical<4; ical++){ 
      scanConfig->CalMode = ical;
      trb->SaveScanStepConfig(*scanConfig);
      
      // configure chips of modules
      for( auto mod : modList ){ 

	// using this function is ok since we do not want to use
	// any custom mask pattern, but just the default one needed
	// to read one out of four channels.
	trb->SCT_WriteStripmask(mod->moduleMask(), ical);
	
	for( auto chip : mod->Chips() ){ 	
	  chip->setThreshold(thresholds[ith]); // in mV
	  trb->GetSCTSlowCommandBuffer()->SetThreshold(chip->address(), chip->threshcalReg());
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
      // 3.- issue Ntrig x L1A
      //
      for (unsigned int i=0; i<m_ntrig; i++)
	trb->GenerateL1A(m_globalMask);
   
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
		m_hits[ith][imodule][link][ichip][strip]++;		
	      }
	      chipcnt++;
	    }	    
	  }
	}// end loop in modules 
      }// end loop in events
      
      if(ed != nullptr)
	delete ed;
      
    } // end loop in calmodes      

    //
    // 5. root TTree
    //
    t_threshold[ith] = thresholds[ith];
    t_threshold_n++;

  } // end loop in thresholds
    
  delete scanConfig;   

  return 1;
}

//------------------------------------------------------
int TrackerCalib::MaskScan::finalize(FASER::TRBAccess *trb,
				     std::vector<Module*> &modList){

  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
       << red << bold << "** [MaskScan::finalize]" << reset << std::endl;  
  char name[256], title[256];

  //
  // 1.- Create output ROOT file
  //
  m_outFilename = (m_outDir+"/MaskScan_"+m_postfix+".root");
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
  TH2F *thocc[MAXMODS][NLINKS];  
  TH1F *hmask[MAXMODS][NLINKS];  
  for(int j=0; j<MAXMODS; j++){
    for(int k=0; k<NLINKS; k++){
      sprintf(name,"thocc_m%d_l%d", j, k);
      sprintf(title,"MaskScan [module=%d, link=%d]", j, k);
      thocc[j][k] = new TH2F(name, title, 768, -0.5, 767.5, 2, -0.5, 1.5); 
      thocc[j][k]->SetXTitle("Channel number");
      thocc[j][k]->SetYTitle("Threshold index [au]");
      thocc[j][k]->SetZTitle("Occupancy");

      sprintf(name,"hmask_m%d_l%d", j, k);
      sprintf(title,"Mask [module=%d, link=%d] (1=dead ; 2=noisy)", j, k);
      hmask[j][k] = new TH1F(name, title, 768, -0.5, 767.5); 
      hmask[j][k]->SetXTitle("Channel number");
      hmask[j][k]->SetYTitle("Channel category");
      hmask[j][k]->SetLineWidth(2);

      // fill histograms only if module has been enabled
      if( isModulePresent(j,m_globalMask) ){
	for(int i=0; i<2; i++){
	  for(int l=0; l<NCHIPS; l++){
	    for(int m=0; m<NSTRIPS; m++){
	      float occ = (float)m_hits[i][j][k][l][m] / (float)m_ntrig;
	      thocc[j][k]->SetBinContent(l*128+m+1, i+1, occ);      
	    }
	  }
	}
      }
      thocc[j][k]->Write();  
    }
  } 

  //
  // 4.- mask channels
  //
  for(auto mod : modList){    
    int modnum = mod->trbChannel();

    int totdead(0), totnoisy(0);
    for(auto chip : mod->Chips() ){          
      int ilink = chip->address() < 40 ? 0 : 1;      
      int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;
      
      for(int istrip=0; istrip<NSTRIPS; istrip++){	  
	
	// check for dead strips
	if( thocc[modnum][ilink]->GetBinContent(ichip*NSTRIPS+istrip+1,1) < 0.05){
	  log << " - Channel " << std::setw(3) << NSTRIPS*ichip + istrip
		    << " [mod " << modnum << ", link " << ilink 
		    << ", chip " << ichip << ", " << std::setw(3) << istrip << "] with occ = "
		    << thocc[modnum][ilink]->GetBinContent(ichip*NSTRIPS+istrip+1,1) << " -> masking (DEAD)" << std::endl;
	  chip->setChannelMask(istrip,1);
	  hmask[modnum][ilink]->SetBinContent(ichip*NSTRIPS+istrip+1,1);
	  totdead++;
	}
	
	// check for noisy strips
	if( thocc[modnum][ilink]->GetBinContent(ichip*NSTRIPS+istrip+1,2) > 0.1){
	  log << " - Channel " << std::setw(3) << NSTRIPS*ichip + istrip
		    << " [mod " << modnum << ", link " << ilink 
		    << ", chip " << ichip << ", " << std::setw(3) << istrip << "] with occ = "
		    << thocc[modnum][ilink]->GetBinContent(ichip*NSTRIPS+istrip+1,1) << " -> masking (NOISY)" << std::endl;
	  chip->setChannelMask(istrip,1);
	  hmask[modnum][ilink]->SetBinContent(ichip*NSTRIPS+istrip+1,2);
	  totnoisy++;
	}
      }  
      // update 16b words for mask
      chip->prepareMaskWords();    
    }

    log << "Results for module " << std::dec << modnum << " : " 
	<< "totdead=" << totdead << " ; totnoisy=" << totnoisy << std::endl; 
  }

  for(int j=0; j<MAXMODS; j++)
    for(int k=0; k<NLINKS; k++)
      hmask[j][k]->Write();

  int cnt=0;  
  log << "Modules config after test '" << testName() << "' :"<< std::endl;
  log << " - MODULES    : " << modList.size() << std::endl;
  std::vector<Module*>::const_iterator mit;
  for(mit=modList.begin(); mit!=modList.end(); ++mit){
    (*mit)->setPrintLevel(2);
    log << "   * mod [" << cnt << "] : " << (*mit)->print() << std::endl;
    cnt++;
  }  
  
  //
  // 5.- close ROOT file
  //
  outfile->Close();       
  log << std::endl << bold << "File '" << m_outFilename 
      << "' created OK" << reset << std::endl;  

  return 1;
}

