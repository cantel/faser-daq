#include "TrackerCalibration/TriggerBurst.h"
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
TrackerCalib::TriggerBurst::TriggerBurst() :
  ITest(TestType::TRIGGER_BURST),
  m_burstType(0), // L1A
  m_ntrig(1000),
  m_ntrigBurst(100),
  m_threshold(50), // 50 mV
  m_charge(1.0), // fC
  m_rmode(ABCD_ReadoutMode::LEVEL),
  m_edge(false),
  m_outFilename(""),
  m_postfix("")
{
  ITest::initTree();
  clear();
}

//------------------------------------------------------
TrackerCalib::TriggerBurst::TriggerBurst(int burstType,
					 int ntrig,
					 float threshold,
					 float charge,
					 ABCD_ReadoutMode rmode,
					 bool edge) :
  ITest(TestType::TRIGGER_BURST),
  m_burstType(burstType),
  m_ntrig(ntrig),
  m_ntrigBurst(100),
  m_charge(charge),
  m_rmode(rmode),
  m_edge(edge),  
  m_outFilename(""),
  m_postfix("")
{
  setThreshold(threshold);
  ITest::initTree();
  clear();
}

//------------------------------------------------------
TrackerCalib::TriggerBurst::~TriggerBurst() 
{}

//------------------------------------------------------
void TrackerCalib::TriggerBurst::clear(){
  for(int j=0; j<MAXMODS; j++){
    for(int k=0; k<NLINKS; k++){
      for(int l=0; l<NCHIPS; l++){
	for(int m=0; m<NSTRIPS; m++){
	    m_hits[j][k][l][m]=0;
	}
      }
    }
  }  
}

//------------------------------------------------------
void TrackerCalib::TriggerBurst::setThreshold(float threshold){
  m_threshold = 2.5 * mV2dac(threshold);
}

//------------------------------------------------------
const std::string TrackerCalib::TriggerBurst::print(int indent){
  std::string blank="";
  for(int i=0; i<indent; i++)
    blank += " ";
  
  std::ostringstream out;
  out << ITest::print(indent);
  std::string btype = m_burstType ? " (CalPulse+delay+L1A)" : " (L1A)";
  out << blank << "   - burstType  : " << m_burstType  << btype << std::endl;
  out << blank << "   - ntrig      : " << m_ntrig      << std::endl;
  out << blank << "   - ntrigBurst : " << m_ntrigBurst << std::endl;
  out << blank << "   - threshold  : " << m_threshold  << std::endl;
  out << blank << "   - charge     : " << m_charge     << std::endl;
  std::string rmode;
  if(m_rmode == ABCD_ReadoutMode::HIT)        rmode = "HIT";
  else if(m_rmode == ABCD_ReadoutMode::LEVEL) rmode = "LEVEL";
  else if(m_rmode == ABCD_ReadoutMode::EDGE)  rmode = "EDGE";
  else if(m_rmode == ABCD_ReadoutMode::TEST)  rmode = "TEST";  
  out << blank << "   - rmode      : " << rmode     << std::endl;
  out << blank << "   - edge       : " << m_edge    << std::endl;
  return out.str();
}

//------------------------------------------------------
int TrackerCalib::TriggerBurst::run(FASER::TRBAccess *trb,
				     std::vector<Module*> &modList){
  //get logger instance
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
      << " NtrigBurst="  << std::dec << m_ntrigBurst
      << " thresh[mV]="  << std::dec << m_threshold 
      << " q[fC]=" << std::setprecision(2) << m_charge 
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
int TrackerCalib::TriggerBurst::initialize(FASER::TRBAccess *trb,
					    std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [TriggerBurst::initialize]" << reset << std::endl;

  m_postfix = dateStr()+"_"+timeStr();

  //
  // 1. set output binary file for TRBAccess
  //
  if(m_saveDaq){
    std::string outFilename(m_outDir+"/TriggerBurst_"+m_postfix+".daq");
    trb->SetupStorageStream(outFilename);
    log << "- Binary output file set to : " << outFilename << std::endl;  
  }
  
  //
  // 2. configure SCT modules
  //
  for(auto mod : modList){    
    for(auto chip : mod->Chips()){
      
      // set chip properties
      chip->setThreshold(m_threshold); // threshold [mV]
      chip->setReadoutMode(m_rmode); // readout mode
      chip->setEdge(m_edge); // edge-detect mode
      chip->setCalAmp(m_charge); // input charge [fC]

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
  t_l1delay      = m_l1delay;
  t_ntrig        = m_ntrig;
  Chip *achip = modList[0]->Chips()[0];
  t_readoutMode  = (int)achip->readoutMode();
  t_edgeMode     = achip->edge();
  t_calAmp_n     = 1;
  t_calAmp[0]    = achip->calAmp();
  t_threshold_n  = 1;
  t_threshold[0] = achip->threshold();
  t_planeID     = modList[0]->planeId();
  t_module_n    = modList.size();
  for(unsigned int i=0; i<t_module_n; i++){
    t_sn[i] = modList[i]->id();
    t_trbChannel[i] = modList[i]->trbChannel();
  }
  
  return 1;
}

//------------------------------------------------------
int TrackerCalib::TriggerBurst::execute(FASER::TRBAccess *trb,
					 std::vector<Module*> &modList){

  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [TriggerBurst::execute]" << reset << std::endl;
  
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
  scanConfig->Threshold   = m_threshold;

  unsigned int totL1A(0); // total number of L1A

  //
  // 1.- Loop in bursts
  //
  while( 1 ){
    
    // condition to stop scan
    if(totL1A >= m_ntrig) break;
    
    log << blue << "# totL1A="  << totL1A << " / " << m_ntrig << std::endl;
        
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
	for( auto chip : mod->Chips() ){ 	
	  
	  trb->GetSCTSlowCommandBuffer()->SetStripMask(chip->address(), chip->getMaskWords(ical));
	  trb->WriteSCTSlowCommandBuffer();
	  trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	  
	  chip->setCalMode(ical);
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
      // 3.- send command
      //
      if( m_burstType == 0 )
	{
	  for (unsigned int i=0; i<m_ntrigBurst; i++) 
	    trb->GenerateL1A(m_globalMask);
	}
      else if( m_burstType == 1 )
	{
	  trb->SCT_CalibrationPulse(m_globalMask, m_l1delay, true);
	  usleep(50);
	  bool isLoopRunning=false;
	  do{ isLoopRunning = trb->IsCalibrationLoopRunning(); } while( isLoopRunning );
	}

      totL1A += m_ntrigBurst;
      
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
		m_hits[imodule][link][ichip][strip]++;		
	      }
	      chipcnt++;
	    }	    
	  }
	}// end loop in modules 
      }// end loop in events
      
      if(ed != nullptr)
	delete ed;
      
    } // end loop in calmodes
    
  } // end while
  
  delete scanConfig;  
  return 1;
}

//------------------------------------------------------
int TrackerCalib::TriggerBurst::finalize(FASER::TRBAccess *trb,
					 std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();  
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [TriggerBurst::finalize]" << reset << std::endl;  
  char name[256], title[256];

  //
  // 1.- Create output ROOT file
  //
  m_outFilename = (m_outDir+"/TriggerBurst_"+m_postfix+".root");
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
  TH1F *hitmap[MAXMODS][NLINKS];  
  for(int j=0; j<MAXMODS; j++){
    for(int k=0; k<NLINKS; k++){
     
      // declare histogram
      sprintf(name,"hitmap_m%d_l%d", j, k);
      sprintf(title,"Hitmap [module=%d, link=%d]", j, k);
      hitmap[j][k] = new TH1F(name, title, 768, -0.5, 767.5); 
      hitmap[j][k]->SetXTitle("Channel number");
      hitmap[j][k]->SetYTitle("Hits"); 
      
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
	for(int l=0; l<NCHIPS; l++){
	  for(int m=0; m<NSTRIPS; m++){
	    hitmap[j][k]->SetBinContent(l*128+m+1, (float)m_hits[j][k][l][m]);
	  }
	}
      }

      hitmap[j][k]->Write();  
    }
  }
  
  //
  // 4.- close ROOT file
  //
  outfile->Close();       
  log << std::endl << bold << "File '" << m_outFilename 
      << "' created OK" << reset << std::endl << std::endl;  
  return 1;
}
