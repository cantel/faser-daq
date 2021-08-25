#include "TrackerCalibration/NoiseOccupancy.h"
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
#include <thread>
#include <mutex> 
#ifdef _MACOSX_
#include <unistd.h>
#endif

std::mutex mutex;

//------------------------------------------------------
TrackerCalib::NoiseOccupancy::NoiseOccupancy() :
  ITest(TestType::NOISE_OCCUPANCY),
  m_minNumHits(20),
  m_maxNumTrig(512000),
  m_targetOcc(5E-6),
  m_absMaxNL1A(1E9),
  m_autoMask(true),
  m_calLoopDelay(3000),
  m_dynamicDelay(true),
  m_outFilename(""),
  m_postfix("")
{
  ITest::initTree();
  clear();
}

//------------------------------------------------------
TrackerCalib::NoiseOccupancy::~NoiseOccupancy() 
{}

//------------------------------------------------------
void TrackerCalib::NoiseOccupancy::clear(){
  for(int i=0; i<MAXTHR; i++){
    m_triggers[i]=0;    
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
const std::string TrackerCalib::NoiseOccupancy::print(int indent){
  std::string blank="";
  for(int i=0; i<indent; i++)
    blank += " ";
  
  std::ostringstream out;
  out << ITest::print(indent);  
  
  out << blank << "   - minNumHits   : " << m_minNumHits << std::endl;
  out << blank << "   - maxNumTrig   : " << (float)m_maxNumTrig << std::endl;
  out << std::setprecision(1) << std::scientific;
  out << blank << "   - targetOcc    : " << m_targetOcc << std::endl;
  out << blank << "   - absMaxNL1A   : " << m_absMaxNL1A << std::endl;
  out << blank << "   - autoMask     : " << m_autoMask << std::endl;
  out << blank << "   - calLoopDelay : " << m_calLoopDelay << std::endl;
  out << blank << "   - dynamicDelay : " << m_dynamicDelay << std::endl;
  return out.str();
}

//------------------------------------------------------
int TrackerCalib::NoiseOccupancy::run(FASER::TRBAccess *trb,
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
      << " minNumHits=" << m_minNumHits
      << std::setprecision(1) << std::scientific
      << " maxNumTrig=" << m_maxNumTrig
      << " targetOcc=" << m_targetOcc
      << " autoMask=" << m_autoMask
      << " absMaxNL1A=" << m_absMaxNL1A
      << " globalMask=0x" << std::hex << std::setfill('0') << unsigned(m_globalMask)
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
int TrackerCalib::NoiseOccupancy::initialize(FASER::TRBAccess *trb,
					    std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [NoiseOccupancy::initialize]" << reset << std::endl;

  m_postfix = dateStr()+"_"+timeStr();

  //
  // set output binary file for TRBAccess
  //
  if( m_saveDaq ){
    std::string outFilename(m_outDir+"/NoiseOccupancy_"+m_postfix+".daq");
    trb->SetupStorageStream(outFilename);
    log << "- Binary output file set to : " << outFilename << std::endl;  
  }

  //
  // configure TRB calLoop
  //
  FASER::ConfigReg *cfgReg = trb->GetConfig();
  if(cfgReg == nullptr){
    log << red << "[NoiseOccupancy::initialize] could not get TRB config register. Exit."
	<< reset << std::endl;
    return 0;
  }

  uint32_t calLoopNb(1000);  
  // uint32_t calLoopNb(128000); 
  // uint32_t calLoopNb(1000000);   
  
  cfgReg = trb->GetConfig();
  cfgReg->Set_Global_CalLoopNb(calLoopNb);
  cfgReg->Set_Global_CalLoopDelay(m_calLoopDelay);
  cfgReg->Set_Global_DynamicCalLoopDelay(m_dynamicDelay);
  trb->WriteConfigReg(); // this sets std::hex to std::cout without resetting it !!!  
  
  log << std::endl;
  log << "### TRB calLoop settings ###" << std::dec << std::endl;
  log << "   - calLoopNb    = " << calLoopNb << std::endl;
  log << "   - calLoopDelay = " << m_calLoopDelay << std::endl;
  log << "   - dynamicDelay = " << m_dynamicDelay << std::endl << std::endl;
  std::cout << std::dec << std::setprecision(3);
  
  //
  // configure SCT modules
  //
  for(auto mod : modList){    
    log << "- Configuring module " << mod->trbChannel()  << std::endl;
    for(auto chip : mod->Chips()){
      
      // set chip properties
      chip->setReadoutMode(ABCD_ReadoutMode::LEVEL); // readout mode
      chip->setEdge(false); // edge-detect mode
      chip->setCalAmp(0); // input charge

      if( !m_emulateTRB ){

	// SoftReset
	trb->GenerateSoftReset(mod->moduleMask());
	usleep(200);

	// prepare mask
	chip->prepareMaskWords();
	//chip->enableAllChannelsMaskWords();	

	/*	int ical=0;
	std::vector<uint16_t> vec = chip->getMaskWords(ical);	  
	std::cout << "Mask pattern for ical = "  << ical << std::endl;	  
	for(auto v : vec ){
	  std::bitset<16> bits(v);
	  std::cout << split(bits,4) << std::endl;
	  }*/

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
	log << " - Configuring chip " << std::dec << chip->address() 
	    << ". Loading trimDac values..." << std::endl;
	for(unsigned int istrip=0; istrip<NSTRIPS; istrip++){
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
  t_l1delay     = m_l1delay;
  Chip *achip = modList[0]->Chips()[0];
  t_readoutMode = (int)achip->readoutMode();
  t_edgeMode    = achip->edge();
  t_calAmp_n    = 0;
  t_planeID     = modList[0]->planeId();
  t_module_n    = modList.size();
  for(unsigned int i=0; i<t_module_n; i++){
    t_sn[i] = modList[i]->id();
    t_trbChannel[i] = modList[i]->trbChannel();
  }
  
  return 1;
}

//------------------------------------------------------
void TrackerCalib::NoiseOccupancy::decode(std::vector< std::vector<uint32_t> > &trbEventData, 
					  int &ithresholdIdx,
					  int &totEvents,
					  bool &stop){
  
 
  //  while( !stop ){
    
  //std::this_thread::sleep_for(std::chrono::milliseconds(50));

    FASER::TRBEventDecoder *ed = new FASER::TRBEventDecoder();

      
    // mutex.lock();
    //std::cout << "thread " << trbEventData.size() << std::endl;
    ed->LoadTRBEventData(trbEventData);
    // mutex.unlock();

    auto evnts = ed->GetEvents();
    totEvents += evnts.size();
    
    for(auto evnt : evnts){ 	
      for(unsigned int imodule=0; imodule<8; imodule++){
	auto sctEvent = evnt->GetModule(imodule);	  
	if (sctEvent != nullptr){
	  m_modocc[imodule] += sctEvent->GetNHits(); 
	  
	  //std::cout << "[" << std::dec << std::setw(6) << totEvents << "]" 
	  //	    << " mod=" << imodule << " nhits=" << sctEvent->GetNHits() 
	  //	    << " ; cumulated nhits = " << m_modocc[imodule] << std::endl;

	  auto hits = sctEvent->GetHits(); 
	  int chipcnt(0);
	  for(auto hit : hits){ 
	    int link = (int)(chipcnt/6);		  
	    int ichip = link > 0 ? chipcnt - 6 : chipcnt;		
	    for(auto h : hit){ 
	      int strip = (int)h.first;		  
	      m_hits[ithresholdIdx][imodule][link][ichip][strip]++;		
	    }
	    chipcnt++;
	  }
	}
      }// end loop in modules 
    }// end loop in events    
    delete ed;
    // } 
  //delete ed;
}

//------------------------------------------------------
int TrackerCalib::NoiseOccupancy::execute(FASER::TRBAccess *trb,
					  std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << std::endl << red << bold << "** [NoiseOccupancy::execute]" << reset << std::endl << std::endl;
  
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

  //-------------------------------------------
  // initialize variables
  //-------------------------------------------
  int ith(0); // threshold counter
  bool fstop(false); // global flag to stop scan
  //uint32_t nL1A(1000000);
  uint32_t nL1A(1000);
  uint32_t lastCalLoopNb(nL1A);

  //const double maxThreshold=110;  // update 02.10.20 
  //const double maxThreshold=100;  // 17.11.20
  const double maxThreshold=130;
  
  bool allModulesOccTarget(false); // have all modules reached the target occupancy ? 
  log << "(-) Conditions for end-of-scan (ored with allModulesOccTarget):" << std::endl;
  log << "   - absMaxNL1A   = " << std::scientific << m_absMaxNL1A << std::endl;
  log << "   - maxThreshold = " << std::fixed << std::dec << (int)maxThreshold << " [mV]" << std::endl << std::endl;
  
  // cache number of active channels per module
  int nactiveChannels[8];
  for(int i=0; i<8; i++) nactiveChannels[i]=0;
  for(auto mod : modList)
    nactiveChannels[mod->trbChannel()] = mod->nActiveChannels();

  // TRBEvent raw data
  std::vector< std::vector<uint32_t> > trbEventData;  
  
  //-------------------------------------------
  // 1.- FIRST while loop: threshold control
  //-------------------------------------------
  while( 1 ){
    
    // stop scan ? 
    if(fstop == true) break;
    
    double thrmv = 2.5*ith;
    if(m_printLevel > 0) 
      log << blue << bold << std::fixed << std::dec << "   - threshold[" << ith << "] = " << thrmv 
	  << " [mV] ; nL1A=" << std::scientific << nL1A << reset << std::endl;          
    trb->SaveScanStepConfig(*scanConfig);
    scanConfig->ExpectedL1A = nL1A;	
    
    //-----------------------------------------
    // 2.- SECOND while loop: trigger control
    //-----------------------------------------
    bool nextThreshold(true);
    uint32_t totL1A(0);
    int itercnt(0);   
    
    // module occupancies for this threshold
    for(int i=0; i<8; i++) m_modocc[i]=0;
    
    while( 1 ){
      
      // timer just for this trigger iteration
      Timer *timer = new Timer();
      timer->start();
      
      if(m_printLevel > 0){
	log << std::endl << "# [" << dateStr() << "," << timeStr() << "] " << std::endl; 
	log << std::dec << std::fixed 
	    << "# [" << itercnt << "] allModulesOccTarget=" << allModulesOccTarget 
	    << " nL1A=" << nL1A 
	    << " totL1A="  << std::scientific << (float)totL1A  
	    << " absMaxNL1A=" << m_absMaxNL1A << std::endl;	        
      }
      
      // Conditions to stop scan
      // TBD: modify in case there's a nosy strip preventing occupancy to naturally drop...
      if( allModulesOccTarget || (totL1A > m_absMaxNL1A) || (thrmv >= maxThreshold) ){ 
	log << "STOP SCAN because ";
	if(allModulesOccTarget)        log << "all modules reached occupancies < targetOcc" << std::endl;
	else if(totL1A > m_absMaxNL1A) log << "totL1 = " << totL1A << " > "  << m_absMaxNL1A << std::endl;
	else log << "threshold=" << thrmv << " >= " << maxThreshold << " [mV]" << std::endl;
	fstop=true; 
	break; 
      }
      
      //----------------------------------------------
      // 3. L1A loop
      //----------------------------------------------
      
      // update TRB settings
      if( nL1A > lastCalLoopNb ){

	FASER::ConfigReg *cfgReg = trb->GetConfig();
	cfgReg->Set_Global_CalLoopNb(nL1A);
	trb->WriteConfigReg(); 
	
	log << "#### TRB calLoop settings ####" << std::dec << std::endl;
	log << "   - calLoopNb    = " << nL1A << std::endl;
	log << "   - calLoopDelay = " << m_calLoopDelay << std::endl;
	log << "   - dynamicDelay = " << m_dynamicDelay << std::endl;
	std::cout << std::dec << std::setprecision(3);
	
	lastCalLoopNb = nL1A;
      }
      
      const unsigned int ncalModes = ith >= 25 ? 1 : 4;
      for(unsigned int ical=0; ical<ncalModes; ical++){ 
	if(m_printLevel > 0) 
	  log << " - ical=" << ical << " nL1A=" << nL1A << std::endl;
	scanConfig->CalMode = ical;
	trb->SaveScanStepConfig(*scanConfig);

	// enable all channels
	if( ncalModes == 1 ){
	  for( auto mod : modList )
	    for( auto chip : mod->Chips() )
	      chip->enableAllChannelsMaskWords();	
	}
	
	// configure chips of modules
	for( auto mod : modList ){ 
	  for( auto chip : mod->Chips() ){ 	
	    trb->GetSCTSlowCommandBuffer()->SetStripMask(chip->address(), chip->getMaskWords(ical));
	    trb->WriteSCTSlowCommandBuffer();
	    trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	    
	    if(m_printLevel > 2 ){
	      std::vector<uint16_t> vec = chip->getMaskWords(ical);	  
	      std::cout << "Mask pattern for ical = "  << ical << std::endl;	  
	      for(auto v : vec ){ std::bitset<16> bits(v); std::cout << split(bits,4) << std::endl;  }
	    }
	    
	    if( ical==0 && nextThreshold ){ // update threshold just when needed
	      //std::cout << "Setting treshold " << thrmv << std::endl;
	      chip->setThreshold(thrmv); // in mV
	      trb->GetSCTSlowCommandBuffer()->SetThreshold(chip->address(), chip->threshcalReg());
	      trb->WriteSCTSlowCommandBuffer();
	      trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	    } 	
	  } // end loop in chips
	}// end loop in modules

	//--------------------------------
	// 3.- readout sequence
	//--------------------------------
	// FIFO reset + enable data-taking + SR + Start Readout
	trb->FIFOReset();
	trb->SCT_EnableDataTaking(m_globalMask);
	trb->GenerateSoftReset(m_globalMask);	
	trb->StartReadout();

	int totEvents(0);
	bool stop(false);// currently unused (only meant to be used when threading)
	
	/*	std::thread decodeThread(&NoiseOccupancy::decode, this, 
				 std::ref(trbEventData), 
				 std::ref(ith), 				 
				 std::ref(totEvents),
				 std::ref(stop)
				 );
	*/
	
	//	FASER::TRBEventDecoder *ed = new FASER::TRBEventDecoder();
	trb->GenerateL1A(m_globalMask, true);

	//for(){

	while( trb->IsCalibrationLoopRunning() ){
	  std::this_thread::sleep_for(std::chrono::milliseconds(100));
	  //mutex.lock();
	   trbEventData = trb->GetTRBEventData();	  
	  // std::cout << "Main " << trbEventData.size() << std::endl;

	   decode(trbEventData,ith,totEvents,stop);

	  //mutex.unlock();	 
	  //	  usleep(100);
	}
	//usleep(200000);

	//trbEventData = trb->GetTRBEventData();
	//decode(trbEventData,ith,totEvents,stop);

	//}

	
	//bool isLoopRunning=false;
	//do{ isLoopRunning = trb->IsCalibrationLoopRunning(); } while( isLoopRunning );
	
	// stop readout
	trb->StopReadout();

	trbEventData = trb->GetTRBEventData();
	decode(trbEventData,ith,totEvents,stop);

	// stop thread
	//stop=true;	
	//if( !decodeThread.joinable() ){ 
	//log << "[NoiseOccupancy::execute] decoding thread not joinable !! " << std::endl;
	//return 0;
	//}
	//decodeThread.join();
	
	//std::cout << "Total events from thread = " << totEvents << std::endl;
	
	/* show how many triggers we have sent for the module. Since we are 
	   looping in calmodes, just show this info once */
	if(ical == 0){ totL1A += nL1A; }
	
	//-------------------------
	// 4. check number of hits
	//-------------------------
	/*for(unsigned int imodule=0; imodule<8; imodule++){
	  if( m_modocc[imodule] != 0) 
	    std::cout << "[" << std::dec << std::setw(6) << sgs_evntcnt-1 << "]" 
		      << " mod=" << imodule << " cumulated_nhits = " << m_modocc[imodule] << std::endl;
		      }*/

	//	if(ed != nullptr)
	//	  delete ed;

      } // end loop in calmodes

      //-------------------------------
      // 5. check module occupancies 
      //-------------------------------
      allModulesOccTarget=true;
      for(auto mod : modList){    
	int modnum = mod->trbChannel();
	double x = (float)m_modocc[modnum]/double(nactiveChannels[modnum])/(double)(totL1A);

	std::cout << "[SGS] modocc[" << modnum << "] = " << m_modocc[modnum] << std::endl;
	std::cout << "[SGS] nactiveChannels[" << modnum << "] = " << nactiveChannels[modnum] << std::endl;
	std::cout << "[SGS] totL1A = " << totL1A << std::endl;

	log << "# mod "  << std::dec << modnum << "  nL1A=" << nL1A 
	    << "  totL1A=" << totL1A 
	    << bold << green << "  NO=" << std::scientific << x << reset 
	    << "  belowTargetOcc=" << (x < m_targetOcc) 
	    << std::endl;
	log << std::fixed;

	if(x > m_targetOcc) 
	  allModulesOccTarget=false;

	// eventually mask noisy channels !!!
	if( totL1A != 0 && m_autoMask ){
	  int nmasked = 1536 - mod->nActiveChannels();
	  for(auto chip : mod->Chips()){	    
	    int ilink = chip->address() < 40 ? 0 : 1;      
	    int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;
	    for(int istrip=0; istrip<NSTRIPS; istrip++){	
	      double z = (double)m_hits[ith][modnum][ilink][ichip][istrip] / (double)totL1A;
	      if( z > 1.5 ){
		chip->setChannelMask(istrip,1);
		nmasked++;
		log << bold << red << "# masking channel [" << modnum << " , " 
		    << ilink << " , " << ichip << " , " << std::setw(3) << istrip << "] gchan=" 
		    << std::setw(3) << NSTRIPS*ichip+istrip 
		    << " nMasked=" << nmasked 
		    << reset << std::endl;
	      }	      
	    }
	  }
	}
      } // end loop in modules 
      
      //------------------------------------------------
      // 6.- check minimum number of hits condition // [200820]
      //------------------------------------------------
      if(m_printLevel > 0) log << "# Analyzing data..." << std::endl;
      bool allModulesWithMinHits(true);
      for(auto mod : modList){    
	int modnum = mod->trbChannel();
	int nactive50 = (int)(0.5*mod->nActiveChannels());
	
	int totChansWithMinHits(0);
	for( auto chip : mod->Chips() ){ 
	  int ilink = chip->address() < 40 ? 0 : 1;      
	  int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;

	  for(int istrip=0; istrip<NSTRIPS; istrip++){	
	    // check only non-masked channels
	    if( !chip->isChannelMasked(istrip) ){
	      if(m_printLevel > 2)
		log << " - [modnum " << modnum << ", link " << ilink 
		    << ", chip " << ichip << ", chan " << std::setw(3) << istrip 
		    << " ] gchan=" << std::setw(3) << NSTRIPS*ichip+istrip
		    << " isMasked=" << chip->isChannelMasked(istrip)
		    << " hits=" << m_hits[ith][modnum][ilink][ichip][istrip] << std::endl;	   
	      
	      if( m_hits[ith][modnum][ilink][ichip][istrip] >= m_minNumHits )
		totChansWithMinHits++;	      
	    }
	  } // end loop in channels	
	} // end loop in chips

	allModulesWithMinHits = (allModulesWithMinHits && (totChansWithMinHits >= nactive50) );

	if( m_printLevel > 0 ){
	  log << " - mod="   << std::dec << std::fixed << modnum << " :"
	      << " minHits requirement is nactive50=" << nactive50 << "."
	      << " totChansWithMinHits=" << std::setfill(' ') << std::setw(4)  << totChansWithMinHits << " channels";
	  if(totChansWithMinHits >= nactive50) log << " => OK !" << std::endl;
	  else log << " => Not yet there..." << std::endl;
	}
	
	if( !allModulesWithMinHits ){
	  log << "# This module did not pass minHits requirement: send new bunch of triggers..." << std::endl
	      << "# allModulesWithMinHits=" << allModulesWithMinHits << std::endl;

	  /* Multiply by 2 the number of triggers to be sent in next iteration within trigger loop.
	     If we end up being above the maximum, then set nL1A to the maximum. */
	  nL1A *= 2;
	  if( nL1A >= m_maxNumTrig ){ nL1A = m_maxNumTrig; }      
	  log << "==> nL1A for next iteration should be " <<  nL1A << std::endl;
	  
	  break; // break module loop
	  
	}	
      } // end for loop in modules
      
      timer->stop();
      log << "# Elapsed time: " << timer->printElapsed() << std::endl;
      if(timer) delete timer;
      
      nextThreshold = allModulesWithMinHits; 
      if( nextThreshold ){     
	log << "# allModulesWithMinHits=" << allModulesWithMinHits 
	    << " totL1A sent so far : " << std::dec << std::fixed << totL1A << std::endl
	    << "# MOVING TO NEXT THRESHOLD ! " << std::endl << std::endl;
	
	break; // break trigger loop
      }
      
      itercnt++;    

  } // end while trigger control
    
    if( !fstop ){
      // at this point we have ended with this threshold. Update tree array.
      t_threshold[ith] = 2.5*ith;
      m_triggers[ith] = (float)totL1A;
      t_threshold_n++;     
      //std::cout << "t_threshold_n=" << t_threshold_n 
      //	<< "  t_threshold[" << ith << "]=" << t_threshold[ith]
      //	<< "  m_triggers[" << ith << "]=" << (int)m_triggers[ith] 
      //	<< std::endl;
      ith++;
    }

  } // end of while threshold loop

  trbEventData.clear();
  
  delete scanConfig;
  return 1;
}

//------------------------------------------------------
int TrackerCalib::NoiseOccupancy::execute2(FASER::TRBAccess *trb,
					  std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << dateStr() << "," << timeStr() << "] " 
      << std::endl << red << bold << "** [NoiseOccupancy::execute]" << reset << std::endl << std::endl;
  
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

  //-------------------------------------------
  // initialize variables
  //-------------------------------------------
  int ith(0); // threshold counter
  bool fstop(false); // global flag to stop scan
  uint32_t nL1A(128000);
  uint32_t lastCalLoopNb(0);

  // array with module occupancies for a given threshold
  //  double modocc[8], previous[8];
  double previous[8];
  for(int i=0; i<8; i++){
    m_modocc[i]=0;
    previous[i]=1;
  }
  
  //const double maxThreshold=150;
  //const double maxThreshold=110;  // update 02.10.20 
  const double maxThreshold=70;  // 17.11.20

  bool allModulesOccTarget(false); // have all modules reached the target occupancy ? 
  log << "(-) Conditions for end-of-scan (ored with allModulesOccTarget):" << std::endl;
  log << "   - absMaxNL1A   = " << std::scientific << m_absMaxNL1A << std::endl;
  log << "   - maxThreshold = " << std::fixed << std::dec << (int)maxThreshold << " [mV]" << std::endl << std::endl;

  // low-occupancy region
  const double lowOccupancyVal = 0.05; 
  log << "(-) lowOccupancyVal = " << std::fixed << std::dec << lowOccupancyVal << std::endl << std::endl;

  // Conditions to enable calLoop (short delay) 
  const uint32_t nL1A_shortCalLoop(1e5);
  //const float thr_shortCalLoop(75);
  const float thr_shortCalLoop(100);
  log << "(-) Conditions to enable short-delay calLoop (ored with allModulesAtLowOccupancy):" << std::endl;
  log << "   - nL1A_shortCalLoop = " << std::scientific << nL1A_shortCalLoop << std::endl;
  log << "   - thr_shortCalLoop  = " << std::fixed << std::dec << (int)thr_shortCalLoop << " [mV]" << std::endl << std::endl;

  // cache number of active channels per module
  int nactiveChannels[8];
  for(int i=0; i<8; i++) nactiveChannels[i]=0;
  for(auto mod : modList)
    nactiveChannels[mod->trbChannel()] = mod->nActiveChannels();

  ///bool shortCalLoopDelayDone(false);
  //bool sleepDone(false);

  //-------------------------------------------
  // 1.- FIRST while loop: threshold control
  //-------------------------------------------
  while( 1 ){

    // stop scan ? 
    if(fstop == true) break;

    double thrmv = 2.5*ith;
    if(m_printLevel > 0) 
      log << blue << bold << std::fixed << std::dec << "   - threshold[" << ith << "] = " << thrmv 
		<< " [mV] ; nL1A=" << std::scientific << nL1A << reset << std::endl;          
    trb->SaveScanStepConfig(*scanConfig);
    scanConfig->ExpectedL1A = nL1A;	

    //-----------------------------------------
    // 2.- SECOND while loop: trigger control
    //-----------------------------------------
    bool nextThreshold(true);
    uint32_t totL1A(0);
    int itercnt(0);   

    // module occupancies for this threshold
    for(int i=0; i<8; i++) m_modocc[i]=0;
    
    while( 1 ){
      
      // timer just for this trigger iteration
      Timer *timer = new Timer();
      timer->start();
      
      if(m_printLevel > 0){
	log << std::endl << "# [" << dateStr() << "," << timeStr() << "] " << std::endl; 
	log << std::dec << std::fixed 
	    << "# [" << itercnt << "] allModulesOccTarget=" << allModulesOccTarget 
	    << " nL1A=" << nL1A << std::endl;
	log << "# totL1A="  << std::scientific << (float)totL1A  << " absMaxNL1A=" << m_absMaxNL1A << std::endl;	        }
      
      // Conditions to stop scan
      // TBD: modify in case there's a nosy strip preventing occupancy to naturally drop...
      if( allModulesOccTarget || (totL1A > m_absMaxNL1A) || (thrmv >= maxThreshold) ){ 
	log << "STOP SCAN because ";
	if(allModulesOccTarget)        log << "all modules reached occupancies < targetOcc" << std::endl;
	else if(totL1A > m_absMaxNL1A) log << "totL1 = " << totL1A << " > "  << m_absMaxNL1A << std::endl;
	else log << "threshold=" << thrmv << " >= " << maxThreshold << " [mV]" << std::endl;
	fstop=true; 
	break; 
      }
      
      // check if we are in the low-occupancy region
      bool allModulesAtLowOccupancy(true);
      //std::vector<int> vModsNotAtLowOcc;
      for( auto mod : modList ){ 
	int modnum = mod->trbChannel();
	if( previous[modnum] > lowOccupancyVal ){
	  //vModsNotAtLowOcc.push_back(modnum);
	  allModulesAtLowOccupancy=false;
	}	
      }      
      // inspect in case of noisy strip spoiling module occupancies...
      // for( auto modnum : vModsNotAtLowOcc ){}      
      //vModsNotAtLowOcc.clear();      
      if(m_printLevel > 0)
	log << "# allModulesAtLowOccupancy=" << allModulesAtLowOccupancy << std::endl;
      
      //----------------------------------------------
      // 3. calibrationLoop
      //----------------------------------------------
      bool doShortCalLoop = (nL1A >= nL1A_shortCalLoop) || allModulesAtLowOccupancy || (thrmv >= thr_shortCalLoop);
      if(m_printLevel > 0 ){
	if(doShortCalLoop){
	  log << "# shortCalLoop enabled because ";
	  if(nL1A >= nL1A_shortCalLoop) log << "nL1A = " << nL1A << " >= " << nL1A_shortCalLoop << std::endl;
	  else if( allModulesAtLowOccupancy ) log << "allModulesAtLowOccupancy = 1" << std::endl;
	  else log << "threshold=" << thrmv << " >= " << thr_shortCalLoop << " [mV]" << std::endl;
	}
	else log << "# shortCalLoop disabled" << std::endl;
      }
      
      if( doShortCalLoop ){ 
	
	//if( sleepDone == false ){
	//  log << "# Sleeping for 1s.."  << std::endl;
	// sleep(1);
	// sleepDone=true;
	//}
	
	FASER::ConfigReg *cfgReg=0;
	
	/*
	  if( thrmv >= 90 && shortCalLoopDelayDone==false){
	  log << "# Update CalLoopDelay value... "  << std::endl;
	  cfgReg = trb->GetConfig();
	  cfgReg->Set_Global_CalLoopDelay(200);
	  trb->WriteConfigReg(); // this sets std::hex to std::cout without resetting it !!!
	  std::cout << std::dec << std::setprecision(3);
	  shortCalLoopDelayDone=true;
	  usleep(200);
	  }
	*/
	
	if( (nL1A!=m_maxNumTrig) || (nL1A>lastCalLoopNb) ){
	  uint16_t calLoopDelay(100);
	  bool dynamicDelay(true);

	  cfgReg = trb->GetConfig();
	  cfgReg->Set_Global_CalLoopNb(nL1A);
	  cfgReg->Set_Global_CalLoopDelay(calLoopDelay);
	  cfgReg->Set_Global_DynamicCalLoopDelay(dynamicDelay);
	  trb->WriteConfigReg(); 
	  
	  log << "#### TRB calLoop settings ####" << std::dec << std::endl;
	  log << "   - calLoopNb    = " << nL1A << std::endl;
	  log << "   - calLoopDelay = " << calLoopDelay << std::endl;
	  log << "   - dynamicDelay = " << dynamicDelay << std::endl;
	  std::cout << std::dec << std::setprecision(3);

	  lastCalLoopNb = nL1A;
	}
      } // end if( doShortCalDelay )
      
      /* switch to enable all channels when low occupancies, 
	 or when calibrationLoop has been enabled */
      const unsigned int ncalModes = (allModulesAtLowOccupancy || doShortCalLoop) ? 1 : 4;
      if( ncalModes == 1 ){
	for( auto mod : modList )
	  for( auto chip : mod->Chips() )
	    chip->enableAllChannelsMaskWords();	
      }

      /* Loop in calmodes. eventhough we do not inject charge, limitations in the FIFO forces us 
         to mask one out of four channels as if we were doing a standard threshold scan. */     
      for(unsigned int ical=0; ical<ncalModes; ical++){ 
	if(m_printLevel > 0) 
	  log << " - ical=" << ical << " nL1A=" << nL1A << " doShortCalLoop=" << doShortCalLoop << std::endl;
	scanConfig->CalMode = ical;
	trb->SaveScanStepConfig(*scanConfig);
	
	// configure chips of modules
	for( auto mod : modList ){ 
	  for( auto chip : mod->Chips() ){ 	
	    chip->setCalAmp(0); 

	    trb->GetSCTSlowCommandBuffer()->SetStripMask(chip->address(), chip->getMaskWords(ical));
	    trb->WriteSCTSlowCommandBuffer();
	    trb->SendSCTSlowCommandBuffer(mod->moduleMask());

	    if(m_printLevel > 2 ){
	      std::vector<uint16_t> vec = chip->getMaskWords(ical);	  
	      std::cout << "Mask pattern for ical = "  << ical << std::endl;	  
	      for(auto v : vec ){ std::bitset<16> bits(v); std::cout << split(bits,4) << std::endl;  }
	    }
	    
	    if( ical==0 && nextThreshold ){ // update threshold just when needed
	      //std::cout << "Setting treshold " << thrmv << std::endl;
	      chip->setThreshold(thrmv); // in mV
	      trb->GetSCTSlowCommandBuffer()->SetThreshold(chip->address(), chip->threshcalReg());
	      trb->WriteSCTSlowCommandBuffer();
	      trb->SendSCTSlowCommandBuffer(mod->moduleMask());
	    } 	
	  } // end loop in chips
	}// end loop in modules

	//--------------------------------
	// 3.- readout sequence
	//--------------------------------
	// FIFO reset + enable data-taking + SR + Start Readout
	trb->FIFOReset();
	trb->SCT_EnableDataTaking(m_globalMask);
	trb->GenerateSoftReset(m_globalMask);	
	trb->StartReadout();

	/*	if( ! doCalLoop ){ // send N x L1A
	  for (unsigned int i=0; i<nL1A; i++) 
	    trb->GenerateL1A(m_globalMask);
	}
	else{ 
	  trb->SCT_CalibrationPulse(m_globalMask, m_l1delay, doCalLoop);
	  usleep(50);
	  bool isLoopRunning=false;
	  do{ isLoopRunning = trb->IsCalibrationLoopRunning(); usleep(50); } while( isLoopRunning );
	}
	*/

	// always calLoop, even if at low occupancies !!
	/*trb->SCT_CalibrationPulse(m_globalMask, m_l1delay, true);
	//	usleep(50);
	bool isLoopRunning=false;
	//	do{ isLoopRunning = trb->IsCalibrationLoopRunning(); usleep(50); } while( isLoopRunning );
	do{ isLoopRunning = trb->IsCalibrationLoopRunning(); } while( isLoopRunning );
	*/

	trb->GenerateL1A(m_globalMask, true);
	bool isLoopRunning=false;
	do{ isLoopRunning = trb->IsCalibrationLoopRunning(); } while( isLoopRunning );
	
	// stop readout
	trb->StopReadout();
	
	/* show how many triggers we have sent for the module. Since we are 
	   looping in calmodes, just show this info once */
	if(ical == 0){ totL1A += nL1A; }
	
	//-------------------------
	// 4. check number of hits
	//-------------------------
	FASER::TRBEventDecoder *ed = new FASER::TRBEventDecoder();
	ed->LoadTRBEventData(trb->GetTRBEventData());      
	auto evnts = ed->GetEvents();
	//std::cout << "Number of events = " << evnts.size() << std::endl;
	int sgs_evntcnt(0);
	for(auto evnt : evnts){ 	
	  for(unsigned int imodule=0; imodule<8; imodule++){
	    auto sctEvent = evnt->GetModule(imodule);	  
	    if (sctEvent != nullptr){
	      m_modocc[imodule] += sctEvent->GetNHits(); /// check this is OK

	      //std::cout << "[" << std::dec << std::setw(6) << sgs_evntcnt << "]" 
	      //	<< " mod=" << imodule << " nhits=" << sctEvent->GetNHits() 
	      //	<< " ; cumulated nhits = " << m_modocc[imodule] << std::endl;
	    

	      auto hits = sctEvent->GetHits(); 
	      int chipcnt(0);
	      //int sgscnt(0);
	      for(auto hit : hits){ 
		int link = (int)(chipcnt/6);		  
		int ichip = link > 0 ? chipcnt - 6 : chipcnt;		
		//std::cout << " - [" << link << "," << ichip << "] => " << hit.size() << " hits" << std::endl;
		//sgscnt+=hit.size();
		for(auto h : hit){ 
		  int strip = (int)h.first;		  
		  m_hits[ith][imodule][link][ichip][strip]++;		
		}
		chipcnt++;
	      }
	      //std::cout << " - sgscnt = " << std::dec << sgscnt << std::endl;
	    }
	  }// end loop in modules 
	  sgs_evntcnt++;
	}// end loop in events

	for(unsigned int imodule=0; imodule<8; imodule++){
	  if( m_modocc[imodule] != 0) 
	    std::cout << "[" << std::dec << std::setw(6) << sgs_evntcnt-1 << "]" 
		      << " mod=" << imodule << " cumulated_nhits = " << m_modocc[imodule] << std::endl;
	}

	if(ed != nullptr)
	  delete ed;

      } // end loop in calmodes

      //-------------------------------
      // 5. check module occupancies 
      //-------------------------------
      allModulesOccTarget=true;
      for(auto mod : modList){    
	int modnum = mod->trbChannel();
	double x = (float)m_modocc[modnum]/double(nactiveChannels[modnum])/(double)(totL1A);
	previous[modnum]=x;

	//std::cout << "[SGS] modocc[" << modnum << "] = " << m_modocc[modnum] << std::endl;
	//std::cout << "[SGS] nactiveChannels[" << modnum << "] = " << nactiveChannels[modnum] << std::endl;
	//std::cout << "[SGS] totL1A = " << totL1A << std::endl;

	log << "# mod "  << std::dec << modnum << "  nL1A=" << nL1A 
	    << "  totL1A=" << totL1A << "  doShortCalLoop=" << doShortCalLoop
	    << bold << green << "  NO=" << std::scientific << x << reset 
	    << "  AtLowOccupancy=" << (x < lowOccupancyVal) 
	    << "  belowTargetOcc=" << (x < m_targetOcc) 
	    << std::endl;
	log << std::fixed;

	if(x > m_targetOcc) 
	  allModulesOccTarget=false;

	// eventually mask noisy channels !!!
	if( totL1A != 0 && m_autoMask ){
	  int nmasked = 1536 - mod->nActiveChannels();
	  for(auto chip : mod->Chips()){	    
	    int ilink = chip->address() < 40 ? 0 : 1;      
	    int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;
	    for(int istrip=0; istrip<NSTRIPS; istrip++){	
	      double z = (double)m_hits[ith][modnum][ilink][ichip][istrip] / (double)totL1A;
	      if( z > 1.5 ){
		chip->setChannelMask(istrip,1);
		nmasked++;
		log << bold << red << "# masking channel [" << modnum << " , " 
		    << ilink << " , " << ichip << " , " << std::setw(3) << istrip << "] gchan=" 
		    << std::setw(3) << NSTRIPS*ichip+istrip 
		    << " nMasked=" << nmasked 
		    << reset << std::endl;
	      }	      
	    }
	  }
	}
      } // end loop in modules 
      
      //------------------------------------------------
      // 6.- check minimum number of hits condition // [200820]
      //------------------------------------------------
      if(m_printLevel > 0) log << "# Analyzing data..." << std::endl;
      bool allModulesWithMinHits(true);
      for(auto mod : modList){    
	int modnum = mod->trbChannel();
	int nactive50 = (int)(0.5*mod->nActiveChannels());
	
	int totChansWithMinHits(0);
	for( auto chip : mod->Chips() ){ 
	  int ilink = chip->address() < 40 ? 0 : 1;      
	  int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;

	  for(int istrip=0; istrip<NSTRIPS; istrip++){	
	    // check only non-masked channels
	    if( !chip->isChannelMasked(istrip) ){
	      if(m_printLevel > 2)
		log << " - [modnum " << modnum << ", link " << ilink 
		    << ", chip " << ichip << ", chan " << std::setw(3) << istrip 
		    << " ] gchan=" << std::setw(3) << NSTRIPS*ichip+istrip
		    << " isMasked=" << chip->isChannelMasked(istrip)
		    << " hits=" << m_hits[ith][modnum][ilink][ichip][istrip] << std::endl;	   
	      
	      if( m_hits[ith][modnum][ilink][ichip][istrip] >= m_minNumHits )
		totChansWithMinHits++;	      
	    }
	  } // end loop in channels	
	} // end loop in chips

	allModulesWithMinHits = (allModulesWithMinHits && (totChansWithMinHits >= nactive50) );

	if( m_printLevel > 0 ){
	  log << " - mod="   << std::dec << std::fixed << modnum << " :"
	      << " minHits requirement is nactive50=" << nactive50 << "."
	      << " totChansWithMinHits=" << std::setfill(' ') << std::setw(4)  << totChansWithMinHits << " channels";
	  if(totChansWithMinHits >= nactive50) log << " => OK !" << std::endl;
	  else log << " => Not yet there..." << std::endl;
	}
	
	if( !allModulesWithMinHits ){
	  log << "# This module did not pass minHits requirement: send new bunch of triggers..." << std::endl
	      << "# allModulesWithMinHits=" << allModulesWithMinHits << std::endl;

	  /* Multiply by 2 the number of triggers to be sent in next iteration within trigger loop.
	     If we end up being above the maximum, then set nL1A to the maximum. */
	  nL1A *= 2;
	  if( nL1A >= m_maxNumTrig ){ nL1A = m_maxNumTrig; }      
	  log << "==> nL1A for next iteration should be " <<  nL1A << std::endl;
	  
	  break; // break module loop
	  
	}	
      } // end for loop in modules
      
      timer->stop();
      log << "# Elapsed time: " << timer->printElapsed() << std::endl;
      if(timer) delete timer;
      
      nextThreshold = allModulesWithMinHits; 
      if( nextThreshold ){     
	log << "# allModulesWithMinHits=" << allModulesWithMinHits 
	    << " totL1A sent so far : " << std::dec << std::fixed << totL1A << std::endl
	    << "# MOVING TO NEXT THRESHOLD ! " << std::endl << std::endl;
	
	break; // break trigger loop
      }
      
      itercnt++;    

    } // end while trigger control
    
    if( !fstop ){
      // at this point we have ended with this threshold. Update tree array.
      t_threshold[ith] = 2.5*ith;
      m_triggers[ith] = (float)totL1A;
      t_threshold_n++;     
      //std::cout << "t_threshold_n=" << t_threshold_n 
      //	<< "  t_threshold[" << ith << "]=" << t_threshold[ith]
      //	<< "  m_triggers[" << ith << "]=" << (int)m_triggers[ith] 
      //	<< std::endl;
      ith++;
    }

  } // end of while threshold loop
  
  delete scanConfig;
  return 1;
}

  
//------------------------------------------------------
int TrackerCalib::NoiseOccupancy::finalize(FASER::TRBAccess *trb,
					  std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << std::endl << red << bold << "** [NoiseOccupancy::finalize]" << reset << std::endl;  
  char name[256], title[256];
  
  //
  // 1.- Create output ROOT file
  //
  m_outFilename = (m_outDir+"/NoiseOccupancy_"+m_postfix+".root");
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
  log << "# Filling histograms..."<< std::endl;
  TH2F *thocc[MAXMODS][NLINKS];  
  TH1D *projLink[MAXMODS][NLINKS];  
  TH1D *projChip[MAXMODS][NLINKS][NCHIPS];  
  for(int j=0; j<MAXMODS; j++){
    for(int k=0; k<NLINKS; k++){

      // occupancy vs channel
      sprintf(name,"thocc_m%d_l%d",j,k);
      sprintf(title,"NoiseOccupancy [module=%d, link=%d]",j,k);
      thocc[j][k] = new TH2F(name, title, 768, -0.5, 767.5, 256, -1.25, 638.75);
      thocc[j][k]->SetXTitle("Channel number");
      thocc[j][k]->SetYTitle("Threshold [mV]");
      thocc[j][k]->SetZTitle("Occupancy");

      bool isLinkPresent(false);
      for(auto mod : modList){
	if( mod->trbChannel() == j ){
	  for(auto chip : mod->Chips()){
	    if( (k==0 && chip->address()<40 ) || (k==1 && chip->address()>=40 ) ) 
	      isLinkPresent=true;
	  }
	}
      }

      int nActiveChannelsLink(0);
      
      if( isModulePresent(j,m_globalMask) && isLinkPresent ){

	for(unsigned int l=0; l<NCHIPS; l++){
	  
	  // now check if chip is present
	  bool isChipPresent(false);
	  int nActiveChannelsChip(0);
	  int first = k == 0 ? 32 : 40; // first chip address in link
	  for(auto mod : modList){
	    if( mod->trbChannel() == j ){
	      for(auto chip : mod->Chips()){
		if( (chip->address()-first) == l ){
		  isChipPresent=true;			  
		  nActiveChannelsChip  = NSTRIPS - chip->nMasked();
		  nActiveChannelsLink += nActiveChannelsChip;
		}
	      }
	    }
	  }
	  
	  if(m_printLevel > 2)
	    log << "Checking chip "  << l << " isPresent=" << isChipPresent 
		<< " nActiveChannelsChip=" << nActiveChannelsChip << std::endl;
	  
	  if( isChipPresent ){
	    
	    for(unsigned int i=0; i<MAXTHR; i++){
	      
	      // debug information
	      if( (i<t_threshold_n) && (m_printLevel>1) )
		log << "[" << j << "," << k << "," << l << "] thres[" 
		    << i << "]=" << t_threshold[i] << " mV, ntrigs=" << (int)m_triggers[i] << std::endl;
	      
	      // fill occupancy histogram
	      for(int m=0; m<NSTRIPS; m++){	    
		if( m_triggers[i] != 0 ){
		  float occ =(float)m_hits[i][j][k][l][m] / (float)m_triggers[i];
		  thocc[j][k]->SetBinContent(NSTRIPS*l+m+1, i+1, occ);      
		  if( m_printLevel > 2)
		    log << " - " << m << " -> " << std::setprecision(2) << std::scientific << occ << std::endl;
		}
	      }
	    } // end loop in thresholds
	    
	    // projection for this chip
	    int bin = NSTRIPS*l+1;
	    sprintf(name,"proj_m%d_l%d_c%d",j,k,l);
	    sprintf(title,"NoiseOccupancy [module=%d, link=%d, chip=%d]",j,k,l);
	    projChip[j][k][l] = thocc[j][k]->ProjectionY(name,bin,bin+128);
	    if(nActiveChannelsChip!=0) projChip[j][k][l]->Scale(1./nActiveChannelsChip);	    
	    projChip[j][k][l]->SetXTitle("Threshold [mV]");
	    projChip[j][k][l]->SetYTitle("Occupancy");
	    projChip[j][k][l]->SetTitle(title);
	    projChip[j][k][l]->Write();
	    
	  } // end if( isChipPresent )

	} // end loop in chips
	
	// projection for link
	sprintf(name,"proj_m%d_l%d",j,k);
	sprintf(title,"NoiseOccupancy [module=%d, link=%d]",j,k);
	projLink[j][k] = thocc[j][k]->ProjectionY(name,1,768);
	if(nActiveChannelsLink!=0) projLink[j][k]->Scale(1./nActiveChannelsLink);
	projLink[j][k]->SetXTitle("Threshold [mV]");
	projLink[j][k]->SetYTitle("Occupancy");
	projLink[j][k]->SetTitle(title);
	projLink[j][k]->Write();
	
      } // end if (modulePresent & LinkPresent)
      
      // just write always occupancy histogram, from which everytyhing else can be regenerated offline. 
      thocc[j][k]->Write();  

    } // end loop in links 
  } // end loop in modules 


  //
  // 4.- summary of results
  //
  std::string line="-";
  for(int i=0; i<60; i++) line += "-";
  log << std::endl << line << std::endl;
  log << "                  SUMMARY " << std::endl;
  log << line << std::endl;

  for(auto mod : modList){ 
    int modnum = mod->trbChannel();
    for(auto chip : mod->Chips()){
      int ilink = chip->link();      
      int ichip = chip->chipIdx();
      
      // get threshold index corresponding to 1fC
      int thidx(0);
      double x = chip->fC2mV(1);
      log << " - 1fC threshold = " << x << std::endl;
      for(unsigned int i=0; i<t_threshold_n; i++){
	if(t_threshold[i] > x){
	  thidx=i;
	  break;
	}
      }

      // get measured occupancy at 1fC threshold (if available)
      double occ1fC = (thidx != 0) ? 
	projChip[modnum][ilink][ichip]->GetBinContent(thidx) : 0;

      log << "[Mod " << modnum << ", Link " << chip->link() 
	  << ", Chip " << chip->chipIdx() << "]" 
	  << " th(1fC) = " << chip->fC2mV(1) 
	  << " occu@1fC=" << std::scientific << occ1fC  
	  << std::endl;      
    }
  }
  log << line << std::endl;
  
  int cnt=0;  
  log << "Modules config after test '" << testName() << "' :"<< std::endl;
  log << " - MODULES    : " << modList.size() << std::endl;
  std::vector<Module*>::const_iterator mit;
  for(mit=modList.begin(); mit!=modList.end(); ++mit){
    log << "   * mod [" << cnt << "] : " << (*mit)->print() << std::endl;
    cnt++;
  }  
  
  //
  // 5.- close file
  //
  outfile->Close();       
  log << std::endl << bold << "File '" << m_outFilename 
	    << "' created OK" << reset << std::endl;  

  return 1;
}
