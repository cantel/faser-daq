#include "TrackerCalibration/NPointGain.h"
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
#include <TGraphErrors.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#ifdef _MACOSX_
#include <unistd.h>
#endif

using namespace std;

//------------------------------------------------------
TrackerCalib::NPointGain::NPointGain(TestType type) :
  ITest(type),
  m_ntrig(100)
{
  // check TestType and throw exception if not valid
  if( m_testType != TestType::THREE_POINT_GAIN && m_testType != TestType::RESPONSE_CURVE ){
    throw std::invalid_argument("Invalid TestType argument in NPointGain constructor");
  }
  
  if( m_testType == TestType::THREE_POINT_GAIN ){
    float q[3] = {1.5, 2.0, 2.5};
    for(int i=0; i<3; i++) 
      m_charges.push_back(q[i]);      
  }
  else if( m_testType == TestType::RESPONSE_CURVE ){
    float q[10] = {0.5, 0.75, 1, 1.25, 1.5, 2, 3, 4, 6, 8};
    for(int i=0; i<10; i++) 
      m_charges.push_back(q[i]);  
  }
  ITest::initTree();
  initArrays();
}

//------------------------------------------------------
TrackerCalib::NPointGain::~NPointGain() {
}

//------------------------------------------------------
void TrackerCalib::NPointGain::initArrays(){
  for(int q=0; q<MAXCHARGES; q++){
    for(int j=0; j<MAXMODS; j++){
      for(int k=0; k<NLINKS; k++){
	for(int l=0; l<NCHIPS; l++){
	  for(int m=0; m<NSTRIPS; m++){
	    m_vt50[q][j][k][l][m]=0;
	    m_evt50[q][j][k][l][m]=0;
	    m_sigma[q][j][k][l][m]=0;
	  }
	}
      }
    }
  }
}

//------------------------------------------------------
const std::string TrackerCalib::NPointGain::print(int indent){
  // white space for indent
  std::string blank="";
  for(int i=0; i<indent; i++)
    blank += " ";

  std::ostringstream out;
  out << ITest::print(indent);  
  out << blank << "   - ntrig        : " << m_ntrig << std::endl;
  out << blank << "   - Ncharges     : " << (int)m_charges.size() << " (";
  int cnt=0;
  for(auto q : m_charges){
    out << q;
    if(cnt <= ((int)m_charges.size()-2) )
      out << ", ";
    cnt++;
  }
  out << ")" << std::endl;
  return out.str();
}

//------------------------------------------------------
int TrackerCalib::NPointGain::run(FASER::TRBAccess *trb,
				  std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << green << "# Running "<< testName() << reset << std::endl;

  // initialize arrays
  initArrays();

  //
  // tree variables
  //
  t_l1delay     = m_l1delay;
  t_ntrig       = m_ntrig;
  ABCD_ReadoutMode rmode = ABCD_ReadoutMode::LEVEL;
  t_readoutMode  = (int)rmode;
  t_edgeMode     = false;
  t_planeID      = modList[0]->planeId();
  t_module_n     = modList.size();
  for(unsigned int i=0; i<t_module_n; i++){
    t_sn[i]         = modList[i]->id();
    t_trbChannel[i] = modList[i]->trbChannel();
  }

  // start timer
  m_timer->start(); 
  
  // check TRBAccess pointer is valid
  if( trb == nullptr){
    log << red << bold << "[ERROR] TRBAccess=0. Exit" << reset << std::endl;
    return 0;    
  }

  // loop in charges
  int cntch(0);
  for (auto q : m_charges){
    ThresholdScan *thscan = new ThresholdScan(m_ntrig, // ntrig
					      q, // fC
					      true, // autostop
					      ABCD_ReadoutMode::LEVEL, // X1X
					      false); // edge-mode
    thscan->setStep(1);// DAC
    thscan->setGlobalMask(m_globalMask);  
    thscan->setCalLoop(m_calLoop); 
    thscan->setLoadTrim(m_loadTrim); 
    thscan->setSaveDaq(m_saveDaq); 
    thscan->setPrintLevel(m_printLevel);  
    thscan->setOutputDir(m_outDir);  
    thscan->setRunNumber(m_runNumber);
    
    if( !thscan->run(trb,modList) ) return 0;

    for(auto mod : modList){    
      int modnum = mod->trbChannel();

      for(auto chip : mod->Chips()){
	int ilink = chip->address() < 40 ? 0 : 1;      
	int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;
	
	// TBD fix mod-id and chip address: set via map
	for(int istrip=0; istrip<NSTRIPS; istrip++){	  
	  m_vt50[cntch][modnum][ilink][ichip][istrip]  = thscan->vt50(modnum, ilink, ichip, istrip);
	  m_evt50[cntch][modnum][ilink][ichip][istrip] = thscan->evt50(modnum, ilink, ichip, istrip);
	  m_sigma[cntch][modnum][ilink][ichip][istrip] = thscan->sigma(modnum, ilink, ichip, istrip);
	}
      }
    }

    t_calAmp[cntch] = q;
    t_calAmp_n++;
    cntch++;

    delete thscan;

  } // end loop in charges
  
  if( !finalize(trb,modList) ) return 0;

  // stop timer and show elapsed time
  m_timer->stop(); 
  log << bold << green << "Elapsed time: " << m_timer->printElapsed() << reset << std::endl;

  return 1;
}
  
//------------------------------------------------------
int TrackerCalib::NPointGain::finalize(FASER::TRBAccess *trb,
					  std::vector<Module*> &modList){
  auto &log = TrackerCalib::Logger::instance();
  log << "[" << int2str(m_runNumber) << "-" << dateStr() << ":" << timeStr() << "] " 
      << red << bold << "** [NPointGain::finalize]" << reset << std::endl;  
  char name[256], title[256];

  // 
  // 1.- Create output ROOT file
  //  
  std::string outFilename(m_outDir+"/");
  if(m_testType == TestType::THREE_POINT_GAIN) outFilename += "ThreePtGain_";
  else  outFilename += "ResponseCurve_";
  outFilename += (dateStr()+"_"+timeStr()+".root");
  TFile* outfile = new TFile(outFilename.c_str(), "RECREATE");  

  //
  // 2.- TTree with metadata
  //
  if(m_tree){
    m_tree->Fill();
    m_tree->Write();
  }

  const int ncharges = (int)m_charges.size();
  double x[ncharges], y[ncharges], ex[ncharges], ey[ncharges];
  for(int q=0; q<ncharges; q++){
    x[q] = m_charges[q];
    ex[q] = 0;
  }

  for(auto mod : modList){    
    int modnum = mod->trbChannel();  

    for(auto chip : mod->Chips() ){    
      int ilink = chip->address() < 40 ? 0 : 1;      
      int ichip = chip->address() < 40 ? chip->address() - 32 : chip->address() - 40;

      //cout << "Module " << modnum << ", " << ilink << ", " << ichip << endl;

      //
      // 1. declare histograms
      //
      TH1F *hgain(0); // gain @2fC (3PTG)
      TH1F *hnoise(0); // input noise @2fC (3PTG)
      TH2F *hgainStrip(0); // gain vs strip @2fC (3PTG)
      TH2F *hnoiseStrip(0); // input noise vs strip @2fC (3PTG)

      TH1F *hp0(0); // fit-parameter p0 (RC)
      TH1F *hp1(0); // fit-parameter p1 (RC)
      TH1F *hp2(0); // fit-parameter p2 (RC)
      TH1F* hgainCharge[ncharges]; // gain vs charge (RC)
      TH1F* hnoiseCharge[ncharges]; // input noise vs charge (RC)
      for(int q=0; q<ncharges; q++){
	hgainCharge[q]=0;
	hnoiseCharge[q]=0;
      }

      if( m_testType == TestType::THREE_POINT_GAIN )
	{
	  // histograms with values at 2fC input charge
	  sprintf(name,"hgain_m%d_l%d_c%d", modnum, ilink, ichip);     
	  sprintf(title, "Gain [module %d, link %d, chip %d]", modnum, ilink, ichip);
	  hgain = new TH1F(name, "Gain", 100, 0, 100);
	  hgain->SetXTitle("Gain [mV/fC]");

	  sprintf(name,"hnoise_m%d_l%d_c%d", modnum, ilink, ichip);
	  sprintf(title, "Noise [module %d, link %d, chip %d]", modnum, ilink, ichip);
	  hnoise = new TH1F(name, "ENC", 100, 0, 2500);
	  hnoise->SetXTitle("Noise [ENC]");

	  sprintf(name,"hgainStrip_m%d_l%d_c%d", modnum, ilink, ichip);
	  sprintf(title, "Gain per strip [module %d, link %d, chip %d]", modnum, ilink, ichip);
	  hgainStrip = new TH2F(name, title, 768, -0.5, 767.5, 100, 0, 100);
	  hgainStrip->SetXTitle("Channel number");
	  hgainStrip->SetYTitle("Gain [mV/fC]");

	  sprintf(name,"hnoiseStrip_m%d_l%d_c%d", modnum, ilink, ichip);
	  sprintf(title, "Noise per strip [module %d, link %d, chip %d]", modnum, ilink, ichip);
	  hnoiseStrip = new TH2F(name, title, 768, -0.5, 767.5, 100, 0, 2500);
	  hnoiseStrip->SetXTitle("Channel number");
	  hnoiseStrip->SetYTitle("Noise [ENC]");
      }
      else
	{
	  sprintf(name,"hp0_m%d_l%d_c%d",modnum,ilink,ichip);
	  sprintf(title,"p0 [module %d, link %d, chip %d]", modnum, ilink, ichip);
	  hp0 = new TH1F(name, title, 1000, 0, 3000);
	  hp0->SetXTitle("p0 [mV]");

	  sprintf(name,"hp1_m%d_l%d_c%d",modnum,ilink,ichip); 
	  sprintf(title,"p1 [module %d, link %d, chip %d]", modnum, ilink, ichip);
	  hp1 = new TH1F(name, title, 100, 0, 20);
	  hp1->SetXTitle("p1 [fC]");

	  sprintf(name,"hp2_m%d_l%d_c%d",modnum,ilink,ichip);
	  sprintf(title,"p2 [module %d, link %d, chip %d]", modnum, ilink, ichip);
	  hp2 = new TH1F(name, title, 500, -1500, 0);
	  hp2->SetXTitle("p2 [mV]");

	  for(int q=0; q<ncharges; q++){
	    sprintf(name,"hgainCharge_m%d_l%d_c%d_q%d", modnum, ilink, ichip, q);
	    sprintf(title, "Gain [module %d, link %d, chip %d] q=%2.2f", modnum, ilink, ichip, m_charges[q]);
	    hgainCharge[q] = new TH1F(name, "Gain", 100, 0, 100);
	    hgainCharge[q]->SetXTitle("Gain [mV/fC]");

	    sprintf(name,"hnoiseCharge_m%d_l%d_c%d_q%d", modnum, ilink, ichip, q);
	    sprintf(title, "Noise [module %d, link %d, chip %d] q=%2.2f", modnum, ilink, ichip, m_charges[q]);
	    hnoiseCharge[q] = new TH1F(name, "ENC", 100, 0, 2500);
	    hnoiseCharge[q]->SetXTitle("Noise [ENC]");
	  }
	}

      //
      // 2. loop in strips of chip, do fitting and fill histograms / graphs
      //
      for(int istrip=0; istrip<NSTRIPS; istrip++){

	bool chanGood(true);
	for(int q=0; q<ncharges; q++){
	  y[q]  = m_vt50[q][modnum][ilink][ichip][istrip];
	  ey[q] = m_evt50[q][modnum][ilink][ichip][istrip];

	  if( y[q] == 0 )
	    chanGood=false;
	}

	//cout << "- istrip = " << istrip << " chanGood=" << chanGood << endl;

	if( !chanGood ) continue;

	//for(int q=0; q<ncharges; q++)
	//cout << "[" << q << "] x=" << x[q] << " y=" << y[q] << " ex=" << ex[q] << " ey=" << ey[q] << endl;

	TGraphErrors *gr = new TGraphErrors(ncharges, x, y, ex, ey);	    
	gr->SetMarkerStyle(20);
	gr->SetMarkerSize(1.0);
	gr->GetXaxis()->SetLimits(m_charges[0]-0.2, m_charges[ncharges-1]+0.2);
	sprintf(title,"gain_m%d_l%d_c%d_s%d", modnum, ilink, ichip, istrip);
	gr->SetNameTitle(title,title);
	
	// fit function
	sprintf(name,"func_m%d_l%d_c%d_s%d", modnum, ilink, ichip, istrip);
	TF1 *func = nullptr;

	//////////////////// 3PTGAIN ////////////////////
	if( m_testType == TestType::THREE_POINT_GAIN ){
	  func = new TF1(name,"[0]+[1]*x", 0., 3.);
	  func->SetLineColor(40);
	  func->SetLineWidth(1);

	  gr->Fit(name,"NQ");
	  
	  // fill histograms
	  Double_t par[2];
	  func->GetParameters(&par[0]);	  
	  double gain = par[1];
	  double noise = (gain!=0) ? m_sigma[1][modnum][ilink][ichip][istrip] / gain / 0.0001602 : 0;
	  
	  hgain->Fill(gain);
	  hnoise->Fill(noise);	  
	  hgainStrip->Fill(128*ichip+istrip,gain);
	  hnoiseStrip->Fill(128*ichip+istrip,noise);

	  if(m_printLevel > 0)
	    log << " - fitres [ " 
		<< modnum << " , " 
		<< ilink << ","
		<< std::setw(2) << ichip << " , "
		<< std::setw(3) << istrip 
		<< " ] = " << gain << " , " << noise << std::endl;
	} 
	//////////////////// RESPONSE-CURVE ////////////////////
	else{
	  /*cout << "- name = " << name << endl;
	  cout << "- ncharges = " << ncharges << endl;
	  for(int q=0; q<ncharges; q++)
	  cout << "  * q[" << q << "] = " << m_charges[q] << endl;*/

	  func = new TF1(name,"[0]/(1+TMath::Exp(-x/[1]))+[2]", 0, m_charges[ncharges-1]);
	  func->SetParNames("p0","p1","p2");

	  gr->Fit(func,"NQ");

	  double p0 = func->GetParameter("p0");
	  double p1 = func->GetParameter("p1");
	  double p2 = func->GetParameter("p2");

	  //cout << "- p0 = " << p0 << endl;
	  //cout << "- p1 = " << p1 << endl;
	  //cout << "- p2 = " << p2 << endl;

	  hp0->Fill(p0);
	  hp1->Fill(p1);
	  hp2->Fill(p2);

	  for(int q=0; q<ncharges; q++){
	    double x = TMath::Exp(-m_charges[q]/p1);
	    double gain = p1!=0 ? (p0*x)/(p1*(1+x)*(1+x)) : 0;
	    double noise = (gain!=0) ? m_sigma[q][modnum][ilink][ichip][istrip] / gain / 0.0001602 : 0;
	    
	    hgainCharge[q]->Fill(gain);
	    hnoiseCharge[q]->Fill(noise); 			    
	  } 
	}
	
	if(func != nullptr)
	  delete func;
	
	gr->Write();
	delete gr;
	
      } // end loop in strips

      if( m_testType == TestType::THREE_POINT_GAIN ){
	hgain->Write();
	hnoise->Write();
	hgainStrip->Write();
	hnoiseStrip->Write();
	
	delete hgain;
	delete hnoise;
	delete hgainStrip;
	delete hnoiseStrip;
      }
      else{
	chip->setP0(hp0->GetMean());
	chip->setP1(hp1->GetMean());
	chip->setP2(hp2->GetMean());

	hp0->Write();
	hp1->Write();
	hp2->Write();
	for(int q=0; q<ncharges; q++){
	  hgainCharge[q]->Write();
	  hnoiseCharge[q]->Write();
	}	
	
  	delete hp0;
	delete hp1;
	delete hp2;
	for(int q=0; q<ncharges; q++){
	  delete hgainCharge[q];
	  delete hnoiseCharge[q];
	}	
      }

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
  // 4. Close ROOT file
  //
  outfile->Close();       
  log << "File '" << outFilename << "' created OK" << std::endl;  
  return 1;
}
