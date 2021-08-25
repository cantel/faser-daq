#include "TrackerCalibration/L1DelayScan.h"
#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/Chip.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/Utils.h"

#include <algorithm>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>

#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>

//------------------------------------------------------
TrackerCalib::L1DelayScan::L1DelayScan() :
  ITest(TestType::L1_DELAY_SCAN),
  m_ntrig(100), 
  m_startDelay(128), 
  m_endDelay(132),
  m_postfix("")
{
  ITest::initTree();
  clear();
}

//------------------------------------------------------
TrackerCalib::L1DelayScan::~L1DelayScan()
{}

//------------------------------------------------------
void TrackerCalib::L1DelayScan::clear(){
  // initialize arrays
  for(int i=0; i<MAXDELAYS; i++)
    for(int j=0; j<MAXMODS; j++)
      for(int k=0; k<NLINKS; k++)
	for(int l=0; l<NCHIPS; l++)
	  m_hits[i][j][k][l]=0;  
}

//------------------------------------------------------
const std::string TrackerCalib::L1DelayScan::print(int indent){
  std::string blank="";
  for(int i=0; i<indent; i++)
    blank += " ";
  
  std::ostringstream out;
  out << ITest::print(indent);  
  out << blank << "   - ntrig      : " << m_ntrig << std::endl;
  out << blank << "   - startDelay : " << m_startDelay << std::endl;
  out << blank << "   - endDelay   : " << m_endDelay << std::endl;
  return out.str();
}

//------------------------------------------------------
int TrackerCalib::L1DelayScan::run(FASER::TRBAccess *trb,
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
  
  if( !initialize(trb,modList) ) return 0;
  if( !execute(trb,modList) ) return 0;
  if( !finalize(trb,modList) ) return 0;  

  // stop timer and show elapsed time
  m_timer->stop(); 
  log << bold << green << "Elapsed time: " << m_timer->printElapsed() << reset << std::endl;
  
  return 1;
}

//------------------------------------------------------
int TrackerCalib::L1DelayScan::initialize(FASER::TRBAccess *trb,
					  std::vector<Module*> &modList){

  auto &log = TrackerCalib::Logger::instance();
  log << red << bold << "** [L1DelayScan::initialize]" << reset << std::endl;

  m_postfix = dateStr()+"_"+timeStr();

  //
  // 1. set output binary file for TRBAccess
  //
  if( m_saveDaq ){
    std::string outFilename(m_outDir+"/L1DelayScan_"+m_postfix+".daq");
    trb->SetupStorageStream(outFilename);
    log << "- Binary output file set to : " << outFilename << std::endl;  
  }
    
  //
  // 2. configure SCT modules
  //
  for( auto mod : modList ){        
    for( auto chip : mod->Chips() ){

      // set chip properties
      chip->setReadoutMode(ABCD_ReadoutMode::LEVEL); 
      chip->setEdge(false);
      chip->setMask(false); 
      chip->setStrobeDelay(10); 
      chip->setCalAmp(3.0); 
      chip->setThreshold(150); 
      
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
  // 3. tree variables
  //
  t_ntrig        =  m_ntrig;
  Chip *achip = modList[0]->Chips()[0];
  t_readoutMode  = (int)achip->readoutMode();
  t_edgeMode     = achip->edge();
  t_calAmp_n     = 1;
  t_calAmp[0]    = achip->calAmp();
  t_threshold_n  = 1;
  t_threshold[0] = achip->threshold();
  t_planeID      = modList[0]->planeId();
  t_module_n     = modList.size();
  for(unsigned int i=0; i<t_module_n; i++){
    t_sn[i]         = modList[i]->id();
    t_trbChannel[i] = modList[i]->trbChannel();
  }
  
  return 1;  
}

//------------------------------------------------------
int TrackerCalib::L1DelayScan::execute(FASER::TRBAccess *trb,
				       std::vector<Module*> &modList){
  
  auto &log = TrackerCalib::Logger::instance();
  log << red << bold << "** [L1DelayScan::execute]" << reset << std::endl;
  
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
  scanConfig->Threshold   = mV2dac(achip->threshold());
  scanConfig->ExpectedL1A = m_ntrig;	

  int delays[MAXDELAYS];
  for(int i=0; i<MAXDELAYS; i++)
    delays[i]=0;

  //
  // 1.- loop in L1delays
  //
  for(unsigned int idelay=m_startDelay; idelay<=m_endDelay; idelay++){ // loop in delays
    log << blue << " - setting delay to " << std::dec << idelay << " ... " << reset << std::endl;    
    delays[idelay]=1;
    
    //
    // 2.- loop in calmodes
    //
    for(int ical=0; ical<4; ical++){ 
      if(m_printLevel > 1) 
	log << blue << bold << std::dec << "   - ical = " << ical << " ... " << reset << std::endl;    
      scanConfig->CalMode = ical;
      trb->SaveScanStepConfig(*scanConfig);
      
      // configure chips of modules
      for( auto mod : modList ){ 
	for( auto chip : mod->Chips() ){ 
	  chip->setCalMode(ical);	
	  
	  trb->GetSCTSlowCommandBuffer()->SetStripMask(chip->address(), chip->getMaskWords(ical));
	  trb->WriteSCTSlowCommandBuffer();
	  trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	  
	  trb->GetSCTSlowCommandBuffer()->SetConfigReg(chip->address(), chip->cfgReg());
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
      for (unsigned i=0; i<m_ntrig; i++)
	trb->SCT_CalibrationPulse(m_globalMask, idelay);
            
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
	      
	      int link = (int)(chipcnt/6);		  
	      int ichip = link > 0 ? chipcnt - 6 : chipcnt;
	      m_hits[idelay][imodule][link][ichip] += (int)(hit.size());		

	      /*for(auto h : hit){ 
		//int strip = (int)h.first;		  
		int link = (int)(chipcnt/6);		  
		int ichip = link > 0 ? chipcnt - 6 : chipcnt;
		m_hits[idelay][imodule][link][ichip]++;		
		}*/

	      chipcnt++;
	    }	    
	  }
	}// end loop in modules 
      }// end loop in events

      if(ed != nullptr)
	delete ed;

    } // end loop in calmodes      
  } // end loop in delays
    
  delete scanConfig;   
  
  log << "Summary" << std::endl;
  for(int i=0; i<MAXDELAYS; i++){	
    if(delays[i])
      {
	log << "Delay " << i << std::endl;
	for(int j=0; j<MAXMODS; j++){
	  int tot(0);
	  for(int k=0; k<NLINKS; k++){
	    for(int l=0; l<NCHIPS; l++){
	      tot += m_hits[i][j][k][l];
	    }
	  }	
	  log << "  - module " << j << " : " << tot << std::endl;
	} // end loop in modules
      }
  }

  return 1;
}
  
//------------------------------------------------------
int TrackerCalib::L1DelayScan::finalize(FASER::TRBAccess *trb,
					std::vector<Module*> &modList){

  auto &log = TrackerCalib::Logger::instance();
  log << red << bold << "** [L1DelayScan::finalize]" << reset << std::endl;  
  char name[256], title[256];

  //
  // 1.- Create output ROOT file
  //
  std::string outFilename = (m_outDir+"/L1DelayScan_"+m_postfix+".root");
  TFile* outfile = new TFile(outFilename.c_str(), "RECREATE");  

  //
  // 2.- TTree with metadata
  //
  if(m_tree){
    m_tree->Fill();
    m_tree->Write();
  }
  
  //
  // 3.- fill and write histograms
  //
  TH1F *hNhits[MAXMODS];
  TH1F *hNhitsLink[MAXMODS][NLINKS];
  for(int j=0; j<MAXMODS; j++){
    // total number of hits per module
    sprintf(name,"l1delay_m%d", j);
    sprintf(title,"L1Delay Scan [module=%d]", j);   
    hNhits[j] = new TH1F(name, title, m_endDelay-m_startDelay+1, m_startDelay-0.5, m_endDelay+0.5);
    hNhits[j]->SetMarkerStyle(20);
    hNhits[j]->SetMarkerSize(1.0);
    hNhits[j]->SetLineStyle(1);

    for(int k=0; k<NLINKS; k++){
      // total number of hits per module and link
      sprintf(name,"l1delay_m%d_l%d", j, k);
      sprintf(title,"L1Delay Scan [module=%d, link=%d]", j, k);   
      hNhitsLink[j][k] = new TH1F(name, title, m_endDelay-m_startDelay+1, m_startDelay-0.5, m_endDelay+0.5);
      hNhitsLink[j][k]->SetMarkerStyle(20);
      hNhitsLink[j][k]->SetMarkerSize(1.0);
      hNhitsLink[j][k]->SetLineStyle(1);

      for(int l=0; l<NCHIPS; l++){
	for(int i=0; i<MAXDELAYS; i++){	
	  int n = m_hits[i][j][k][l];		
	  hNhits[j]->Fill(i,n);
	  hNhitsLink[j][k]->Fill(i,n);	    
	} // end loop in delays
      } // end loop in chips
  
      hNhitsLink[j][k]->Write();

    } // end loop in link

    hNhits[j]->Write();
    
  } // end loop in modules

  //
  // 4.- Close ROOT file
  //
  outfile->Close();       
  log << std::endl << bold << "File '" << outFilename << "' created OK" << std::endl;  
  return 1;
}

