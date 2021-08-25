void thscan(const char *filename){

  gStyle->SetOptFit(000000);

  // open file
  TFile *f = new TFile(filename);
  if(f->IsZombie()){
    cout << "Could not open file '" << filename << "'. Exit " << endl;
    return;
  } cout << "File '" << filename << "' opened OK" << endl;

  gStyle->SetOptFit(000000);

  char name[200];

  // retrieve histograms
  sprintf(name,"thscan_m2_l0_c32");
  TH2F *h2 = (TH2F*)f->Get(name);
  if(!h2){ cout << "Could not find histogram '"<< name << "'. continue" << endl; return; }
  h2->SetDirectory(0);    
  int min(1);
  for(int by=1; by<=h2->GetNbinsY(); ++by){
    double tot(0); min=by;
    for(int bx=1; bx<=h2->GetNbinsX(); ++bx)
      tot += h2->GetBinContent(bx,by);    
    if(tot != 0)  break;
  }
  h2->GetYaxis()->SetRange(min,h2->GetNbinsY());
  h2->SetStats(kFALSE);  

  // projections 
  /*  TH1D *proj= new TH1D("proj", NULL, h2->GetNbinsY(), h2->GetYaxis()->GetXmin(), h2->GetYaxis()->GetXmax());
  proj->SetLineColor(2);
  proj->SetLineWidth(2);
  proj->SetMarkerStyle(20);
  proj->SetMarkerSize(1);
  */

  //  const int maxbins = 128;    
  //  TH1D* projY[maxbins];
  //  for (int cha=1; cha<=h2->GetXaxis()->GetNbins(); cha++){
  //    sprintf(name, "proj_chan%d", cha-1);
  //    projY[cha-1] = h2->ProjectionY(name, cha, cha);      
  //  }

  //  double val=0;
  
  // loop in thresholds
  /*  for (int bin=1; bin<=projY[0]->GetXaxis()->GetNbins(); bin++){ 
    //      cout << "[" << hyb << "," << col << "," << chip << "] bin=" << bin << endl;
    val=0;
    for(int cha=0; cha<128; cha++){ // loop in channels
      val += projY[cha]->GetBinContent(bin);
      //      cout << "channel " << cha << " bin " 
      // << bin << " " << projY[cha]->GetBinCenter(bin) 
      // << " " << projY[cha]->GetBinContent(bin) << endl;
    }
    val/=128;
    proj->SetBinContent(bin, val);
  }// end loop in thresholds
}  */

  // get thresholds

  // The fitting function for scurve
  //  float thrmin=thr[0];
  //  float thrmax=thr[nthr-1];
  TF1 *fscurve = new TF1("fscurve","0.5*TMath::Erfc((x-[0])/(sqrt(2)*[1]))", 20, 140);
  fscurve->SetParameters(25, 10);
  fscurve->SetParNames("vt50","onoise");

  int chan_color[128];
  Color_t color[7]={kRed,kYellow,kGreen,kCyan,kBlue,kMagenta,kPink};
  for(int chan=0; chan<128; chan++){   
    int v = chan/16;
    int z = chan%16;
    int w = z<10 ? -1*z : (z%10); 
    if(w==5) w-=1;
    int val = (v==0) ? 11+z : color[v-1]+w;
    chan_color[chan] = val;
  }
  

  float thr[256], val[256];
  TGraph *gr[128];
  const int nthr(h2->GetYaxis()->GetNbins());

  for(int l=0; l<128; l++){ 

    for(int by=1; by<=h2->GetYaxis()->GetNbins(); ++by){
      val[by-1] = h2->GetBinContent(l+1,by);
      thr[by-1] = h2->GetYaxis()->GetBinCenter(by);
    }
    
    gr[l] = new TGraph(nthr, thr, val);
    gr[l]->SetMarkerStyle(20);
    gr[l]->SetMarkerSize(0.8);
    gr[l]->SetMarkerColor(chan_color[l]);                  
  }
  
          // display progress
          //      if(!(cnt%step))
          //        cout << "" << 10-int(double(10.*cnt)/total) << 

  // box for threshold scan
  TH2F *kip = new TH2F("kip", NULL, 150, 0, 150, 2, 0, 1.05);
  kip->SetXTitle("Threshold (mV)");
  kip->SetYTitle("Occupancy");
  kip->SetStats(kFALSE);

  //
  // plots
  //
  gStyle->SetPalette(1);  

  //  TCanvas *c1 = new TCanvas("c1", "c1", 800, 600);  h2->Draw("colz");
  
  TCanvas *c2 = new TCanvas("c2", "c2", 800, 600);
  kip->Draw();
  TF1 *fit;
  for(int i=0; i<1; i++){
    gr[i]->Fit(fscurve,"RMQ");
    gr[i]->SetMarkerColor(1);
    gr[i]->SetMarkerSize(1.2);
    fit = gr[i]->GetFunction("fscurve");
    //    fit->SetLineColor(chan_color[i]);
    fit->SetLineColor(2);
    //    gr[i]->SetMarkerStyle(20);    
    gr[i]->Draw("Psame");
    fit->Draw("sameR");             
  }

  


}
