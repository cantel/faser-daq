#include "TrackerCalibration/TrimScan.h"
#include "TrackerCalibration/Logger.h"
#include "TrackerCalibration/Module.h"
#include "TrackerCalibration/ThresholdScan.h"
#include "TrackerCalibration/Utils.h"

#include <TFile.h>
#include <TH1F.h>
#include <TF1.h>
#include <TGraphErrors.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#ifdef _MACOSX_
#include <unistd.h>
#endif

using namespace std;

//------------------------------------------------------
TrackerCalib::TrimScan::TrimScan() :
  ITest(TestType::TRIM_SCAN),
  m_ntrig(100)
{
  clear();
  initArrays();
}

//------------------------------------------------------
TrackerCalib::TrimScan::TrimScan(unsigned int ntrig) :
  ITest(TestType::TRIM_SCAN), 
  m_ntrig(ntrig)
{
  clear();
  initArrays();
}

//------------------------------------------------------
TrackerCalib::TrimScan::~TrimScan() 
{
  clear();
}

//------------------------------------------------------
void TrackerCalib::TrimScan::clear(){
  for(int r=0; r<MAXTRIMRANGE; r++){
    for(int d=0; d<MAXTRIMDAC; d++){
      for(int j=0; j<MAXMODS; j++){
	for(int k=0; k<NLINKS; k++){
	  for(int l=0; l<NCHIPS; l++){
	    for(int m=0; m<NSTRIPS; m++){
	      m_vt50[r][d][j][k][l][m]=0;
	      m_evt50[r][d][j][k][l][m]=0;
	      m_sigma[r][d][j][k][l][m]=0;	      
	    }
	  }
	}
      }
    }
  }
}

//------------------------------------------------------
void TrackerCalib::TrimScan::initArrays(){
  /* 
     Set trimDac values for each of the four ranges
     - Range 0: use all trimDacs (until wwe cross-check DAC linearity)
     - Other ranges: use reduced set of trimDac
  */
  /*  for(int i=0; i<16; i++)
    m_vtrimdac[0].push_back(i);
  
  for(int r=1; r<4; r++){
    m_vtrimdac[r].push_back(3);
    m_vtrimdac[r].push_back(7);
    m_vtrimdac[r].push_back(11);
    m_vtrimdac[r].push_back(15);
  }
  */
  
  for(int r=0; r<2; r++){
    m_vtrimdac[r].push_back(3);
    m_vtrimdac[r].push_back(7);
    m_vtrimdac[r].push_back(11);
    m_vtrimdac[r].push_back(15);
  }
}

//------------------------------------------------------
const std::string TrackerCalib::TrimScan::print(int indent){
  std::string blank="";
  for(int i=0; i<indent; i++)
    blank += " ";
  
  std::ostringstream out;
  out << ITest::print(indent);
  out << blank << "   - ntrig        : " << m_ntrig << std::endl;
  for(int r=0; r<4; r++){
    out << blank << "   - TRange " << r << "     : ";
    const int ntdacs=(int)m_vtrimdac[r].size();
    if (ntdacs == 0) out << "none" << std::endl;
    int cnt(0);
    for(auto d : m_vtrimdac[r]){
      out << d;
      if(cnt+1 < ntdacs) out << ", ";
      else out << std::endl;
      cnt++;
    }
  }
  return out.str();
}

//------------------------------------------------------
int TrackerCalib::TrimScan::run(FASER::TRBAccess *trb,
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
      << " Ntrig=" << std::dec << m_ntrig 
      << " printLevel="  << m_printLevel
      << " outDir="  << m_outDir
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
int TrackerCalib::TrimScan::initialize(FASER::TRBAccess *trb,
					    std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [TrimScan::initialize]" << reset << std::endl;
  return 1;
}

//------------------------------------------------------
int TrackerCalib::TrimScan::execute(FASER::TRBAccess *trb,
					 std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [TrimScan::execute]" << reset << std::endl;
  
  // check eventual TRB emulation
  if( m_emulateTRB ){
    log << "emulateTRB = " << m_emulateTRB << ". Nothing to do here... " << std::endl;
    return 1;
  }

  //
  // 1.- loop in trimRanges
  ///
  for(unsigned int r=0; r<MAXTRIMRANGE; r++){
    //if( r == 0 || r > 1 ) continue;
    //    if( r == 0 ) continue;

    //
    // 2.-loop in trimDacs
    //
    for(auto d : m_vtrimdac[r]){
      log << std::endl << "[TrimRange " << r << " , TrimDac " << d << "]"  << std::endl;
      
      // create threshold scan object
      ThresholdScan *thscan = new ThresholdScan(50, // ntrig
						1.0, // fC
						true, // autostop
						ABCD_ReadoutMode::LEVEL, // X1X
						false); // edge-mode
      
      // TBD: adjust start threshold based on TrimRange
      thscan->setStart(0);
      thscan->setStop(120);
      thscan->setStep(1);  
      thscan->setGlobalMask(m_globalMask);  
      thscan->setCalLoop(m_calLoop); 
      thscan->setLoadTrim(true); // force loading of TrimDac values
      thscan->setSaveDaq(m_saveDaq); 
      thscan->setPrintLevel(m_printLevel);  
      thscan->setOutputDir(m_outDir);
      thscan->setRunNumber(m_runNumber);  

      // update trimRange and trimData in chips
      for( auto mod : modList ){ 
	for( auto chip : mod->Chips() ){ 	
	  chip->setTrimRange(r);	  
	  for(unsigned int strip=0; strip<NSTRIPS; strip++)
	    chip->setTrimDac(strip,d);
	}
      } 
      
      //
      // 3.- run threshold scan for this trimRange and trimDac
      //
      if( !thscan->run(trb,modList) ) return 0;

      //
      // 4.- get output data      
      //
      for(auto mod : modList){    
	int modnum = mod->trbChannel();

	for(auto chip : mod->Chips()){
	  int ilink = chip->address() < 40 ? 0 : 1;      
	  int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;
	  
	  // TBD fix mod-id and chip address: set via map
	  for(int istrip=0; istrip<NSTRIPS; istrip++){	  
	    
	    // TBD skip if strip is masked

	    m_vt50[r][d][modnum][ilink][ichip][istrip] = 
	      thscan->vt50(mod->trbChannel(), ilink, ichip, istrip);

	    m_evt50[r][d][modnum][ilink][ichip][istrip] = 
	      thscan->evt50(mod->trbChannel(), ilink, ichip, istrip);

	    m_sigma[r][d][modnum][ilink][ichip][istrip] = 
	      thscan->sigma(mod->trbChannel(), ilink, ichip, istrip);
	  }   	  
	}
      }

      m_filesSummary[r].push_back(thscan->outputFile());
      
      delete thscan;
            
    } // end loop in TrimDac
  } // end loop in TrimRanges

  return 1;
}

//------------------------------------------------------
int TrackerCalib::TrimScan::finalize(FASER::TRBAccess *trb,
				     std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [TrimScan::finalize]" << reset << std::endl;
  char name[256], title[256];
 
  //--------------------------------------------------
  // 1.- show summary of output files
  //--------------------------------------------------
  std::string line="-";
  for(int i=0; i<120; i++) line += "-";

  log << "Summary of output files: " << std::endl;  
  log << line << std::endl;
  log << std::setfill(' ');
  log << std::setw(13) << " TrimRange ";
  log << " | ";
  log << std::setw(10)  << " TrimDac ";
  log << " | ";
  log << std::setw(50) << " Output file "  << std::endl;
  log << line << std::endl;
  for(int r=0; r<MAXTRIMRANGE; r++){
    // TBD: fixme
    //if( r == 0 || r > 1 ) continue;
    const int ntrimdac = (int)m_vtrimdac[r].size();
    for(int i=0; i<ntrimdac; i++){
      int d = m_vtrimdac[r].at(i);
      std::string filename = m_filesSummary[r].at(i);
      log << std::setw(13) << r << " | " 
		<< std::setw(13) << d << " => "
		<< filename  << std::endl;
    }
  }
  log << line << std::endl;
  
  // Create output ROOT file
  std::string outFilename(m_outDir+"/TrimScan_"+dateStr()+"_"+timeStr()+".root");
  TFile* outfile = new TFile(outFilename.c_str(), "RECREATE");  

  //--------------------------------------------------
  // 2.- initialize arrays
  //--------------------------------------------------
  
  // list of target thresholds [mV]
  const int ntargets(MAXTHR);
  float target[MAXTHR];
  for(int t=0; t<ntargets; t++)
    target[t]=2.5*t;

  // number of trimmable channels for each module, range and target threshold
  int nTrimmedChans[MAXTRIMRANGE][MAXMODS][MAXTHR]; 
  for(unsigned int r=0; r<MAXTRIMRANGE; r++){
    for(int j=0; j<MAXMODS; j++){
      for(int t=0; t<ntargets; t++){
      nTrimmedChans[r][j][t]=0;
      }
    }
  }

  // TBD: write me
  double max[MAXTRIMRANGE][MAXMODS];
  double minTarget[MAXTRIMRANGE][MAXMODS];
  double minTargetIdx[MAXTRIMRANGE][MAXMODS];
  for(unsigned int r=0; r<MAXTRIMRANGE; r++){
    for(int j=0; j<MAXMODS; j++){
      max[r][j]=0;
      minTarget[r][j]=0;
      minTargetIdx[r][j]=0;
    }
  }
  
  double slope[MAXTRIMRANGE][MAXMODS][NLINKS][NCHIPS][NSTRIPS];
  double offset[MAXTRIMRANGE][MAXMODS][NLINKS][NCHIPS][NSTRIPS];
  for(int r=0; r<MAXTRIMRANGE; r++){
    for(int j=0; j<MAXMODS; j++){
      for(int k=0; k<NLINKS; k++){
	for(int l=0; l<NCHIPS; l++){
	  for(int m=0; m<NSTRIPS; m++){
	    slope[r][j][k][l][m]=0;
	    offset[r][j][k][l][m]=0;
	  }
	}
      }
    }
  }

  //--------------------------------------------------
  // 3.- loop in data, fill graphs and do fits
  //--------------------------------------------------
  log << "[SGS] ana1" << std::endl;

  // loop in trimRanges  
  for(unsigned int r=0; r<MAXTRIMRANGE; r++){
    log << "[SGS] trimRange " << r << std::endl;
    //    if( r == 0) continue;
    
    const int ntrimdac = (int)m_vtrimdac[r].size();
    double x[ntrimdac], y[ntrimdac], ex[ntrimdac], ey[ntrimdac];
    for(int i=0; i<ntrimdac; i++){
      x[i] = m_vtrimdac[r].at(i);
      ex[i] = 0;
    }
    
    // loop in modules
    for(auto mod : modList){    
      int modnum = mod->trbChannel();
      
      sprintf(name,"hnTrimmedChans_r%d_m%d", r, modnum);      
      sprintf(title, "Number of Trimmable Channels [module %d] TrimRange=%d", modnum, r);
      TH1F* hnTrimmedChans = new TH1F(name, title, ntargets, -1.25, ntargets*2.5-1.25);
      hnTrimmedChans->GetXaxis()->SetTitle("Target threshold [mV]");
      hnTrimmedChans->GetYaxis()->SetTitle("Number of trimmable channels");      

      // loop in chips
      for(auto chip : mod->Chips() ){    
	int ilink = chip->address() < 40 ? 0 : 1;      
	int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;	
	log << "- [SGS] chip " << chip->address() << " [" << ilink << "," << ichip << std::endl;
	
	// loop in channels
	for(int istrip=0; istrip<NSTRIPS; istrip++){
	  log << "  * [SGS] channel " << istrip << std::endl;	  
	  
	  for(int i=0; i<ntrimdac; i++){
	    int d = m_vtrimdac[r].at(i);
	    y[i]  = m_vt50[r][d][modnum][ilink][ichip][istrip];
	    ey[i] = m_evt50[r][d][modnum][ilink][ichip][istrip];
	  }
	  
	  // declare histograms / graphs
	  sprintf(name,"hvt50_r%d_m%d_l%d_c%d_s%d", r, modnum, ilink, ichip, istrip);      
	  sprintf(title, "vt50 [module %d, link %d, chip %d, strip %d] TrimRange=%d", 
		  modnum, ilink, ichip, istrip, r);
	  TGraphErrors* gr_vt50 = new TGraphErrors(ntrimdac, x, y, ex, ey);
	  gr_vt50->SetMarkerStyle(20);
	  gr_vt50->SetMarkerSize(1.0);
	  gr_vt50->GetXaxis()->SetLimits(-0.5, 15.5);
	  gr_vt50->SetTitle(title);	  

	  // linear fit
	  sprintf(name,"linefit_r%d_m%d_l%d_c%d_s%d", r, modnum, ilink, ichip, istrip);  
	  TF1 *func = new TF1(name,"pol1", 0, 15);
	  gr_vt50->Fit(name,"NQ");
	  slope[r][modnum][ilink][ichip][istrip] = func->GetParameter(1);
	  offset[r][modnum][ilink][ichip][istrip] = func->GetParameter(0);

	  // TBD: explain more
	  // loop in targets
	  for(int t=0; t<ntargets; t++){	    
	    float s = slope[r][modnum][ilink][ichip][istrip];
	    float o = offset[r][modnum][ilink][ichip][istrip];	    
	    int dac = (s!= 0) ? int((target[t]-o)/s) : -1;
	    if(dac >= 0 && dac<=15 ) nTrimmedChans[r][modnum][t]++;
	    log << "    - [SGS] target [" << t <<"]=" << target[t] << " s=" << s << " o="  << o << " dac=" << dac 
		      << " nTrimmedChans[" << r << "][" << modnum << "][" << t << "] = " << nTrimmedChans[r][modnum][t] << std::endl;
	  }
	  
	  gr_vt50->Write();
	  delete gr_vt50;
	  delete func;
	  
	} // end loop in channnels
      } // end loop in chips
      
      for(int t=0; t<ntargets; t++)
	hnTrimmedChans->SetBinContent(t+1, nTrimmedChans[r][modnum][t]);

      log << "# [SGS] bins of hnTrimmedChans" << std::endl;
      for(int bx=1; bx<=hnTrimmedChans->GetNbinsX(); ++bx)
	log << " - bin " << bx << " const=" << hnTrimmedChans->GetBinContent(bx) << std::endl;
      
      max[r][modnum] = hnTrimmedChans->GetMaximum();
      log << "[SGS] max[" << r << "][" << modnum << "]=" << max[r][modnum] << std::endl;
      for(int bx=1; bx<=hnTrimmedChans->GetNbinsX(); ++bx){
	if( hnTrimmedChans->GetBinContent(bx) == max[r][modnum] ){
	  minTarget[r][modnum] = hnTrimmedChans->GetBinCenter(bx);
	  minTargetIdx[r][modnum] = bx-1;
	  break;
	}
      }
      hnTrimmedChans->Write();
      delete hnTrimmedChans;

      log << std::endl << "# [SGS] minTarget[" << r << "][" << modnum << "]=" << minTarget[r][modnum] << std::endl;
      log << std::endl << "# [SGS] minTargetIdx[" << r << "][" << modnum << "]=" << minTargetIdx[r][modnum] << std::endl;

    } // end loop in modules
  } // end loop in TrimRanges

  // 
  // TBD: expand explanation
  // determine optimum target threshold and minimum range
  //-------------------------------------
  // 4.- Perform analysis
  //-------------------------------------
  if(m_printLevel > 0) log << "Doing analysis..." << std::endl;
  
  // loop in modules
  for(auto mod : modList){    
    int modnum = mod->trbChannel();
    
    if(m_printLevel > 0)
      log << std::endl << mod->print() << std::endl;
    
    // find maximum of maximums
    float maxmax=0;
    for(unsigned int r=0; r<MAXTRIMRANGE; r++)
      if(max[r][modnum] > maxmax) maxmax = max[r][modnum];
    log << "[SGS] Maximum of maximums=" << maxmax << std::endl;
    
    // find minimum range for which we have a maximum number of trimmable channels
    int minRange=-1;
    for(unsigned int r=0; r<MAXTRIMRANGE; r++){
      if( max[r][modnum] == maxmax ){
	minRange = r;
	break;
      }      
    }
    log << "[SGS] minRange=" << minRange << std::endl;
    
    // find minimum target within selected range
    float minTargetMod = minTarget[minRange][modnum];
    int minTargetIdxMod = minTargetIdx[minRange][modnum];

    log << std::endl 
	      << "====> Module " << modnum 
	      << " minTrimRange=" << minRange
	      << " target=" << minTargetMod
	      << " targetIdx=" << minTargetIdxMod
	      << " maxTrimmable=" << maxmax
	      << std::endl;

    // loop in modules
    for(auto chip : mod->Chips() ){    
      if(m_printLevel > 0)
	log << "* Configuring chip " << chip->address() << "..." << std::endl;

      chip->setThreshold(minTargetMod);	
      chip->setTrimTarget(minTargetMod);
      chip->setTrimRange(minRange);

      // write configuration register to update trimRange
      trb->GetSCTSlowCommandBuffer()->SetConfigReg(chip->address(), chip->cfgReg());
      trb->WriteSCTSlowCommandBuffer();
      trb->SendSCTSlowCommandBuffer(mod->moduleMask());

      if(m_printLevel > 0)
	log << "   " << chip->print() << std::endl;
      
      int ilink = chip->address() < 40 ? 0 : 1;      
      int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;	
      
      for(int istrip=0; istrip<NSTRIPS; istrip++){	  
	float s = slope[minRange][modnum][ilink][ichip][istrip];
	float o = offset[minRange][modnum][ilink][ichip][istrip];
	int dac = (s!=0) ? int((target[minTargetIdxMod]-o)/s) : -1;
	if(s < 0)       dac=0;
	else if(s > 15) dac=15;
	chip->setTrimDac(istrip,dac);
	
	if(m_printLevel > 0)
	  log << "  - channel " << std::setw(3) << istrip << " trimDac=" << s << std::endl;

	// send command to update trimDac values
	trb->GetSCTSlowCommandBuffer()->SetTrimDac(chip->address(), chip->trimWord(istrip));
	trb->WriteSCTSlowCommandBuffer();
	trb->SendSCTSlowCommandBuffer(mod->moduleMask());	  

      } // end loop in channels
    } // end loop in chips    
  } // end loop in modules

 
  //--------------------------------------------------
  // close output file
  //--------------------------------------------------
  outfile->Close();       
  log << "File '" << outFilename << "' created OK" << std::endl;  
  return 1;
}

