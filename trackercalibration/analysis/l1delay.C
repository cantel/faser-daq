void l1delay(const char *filename){

  // open file
  TFile *f = new TFile(filename);
  if(f->IsZombie()){
    cout << "Could not open file '" << filename << "'. Exit " << endl;
    return;
  } cout << "File '" << filename << "' opened OK" << endl;

  int address[12] = { 
    32, 33, 34, 35, 36, 37,
    40, 41, 42, 43, 44, 45 
  };
  
  char name[200];
  const int module = 2;


  // retrieve histograms
  sprintf(name,"l1delay_m%d", module);
  TH1F *hNhits = (TH1F*)f->Get(name);
  if(!hNhits){ cout << "Could not find histogram '"<< name << "'. continue" << endl; return; }
  hNhits->SetDirectory(0);    
  hNhits->SetLineWidth(2);        
  hNhits->GetXaxis()->SetNdivisions(hNhits->GetNbinsX());

  TH1F *h1[12];    
  for(int i=0; i<12; i++){
    sprintf(name,"l1delay_m%d_c%d", module, address[i]);
    h1[i] = (TH1F*)f->Get(name);
    if(!h1[i]){ cout << "Could not find histogram '"<< name << "'. continue" << endl; return; }
    h1[i]->SetDirectory(0);    
    h1[i]->SetLineWidth(2);        
  }

  // styles
  gStyle->SetPalette(1);

  //
  // plots
  //
  TCanvas *c1 = new TCanvas("c1", "c1", 800, 600);
  hNhits->Draw("hist");

  TCanvas *c2 = new TCanvas("c2", "c2", 1400, 900);
  c2->Draw();
  c2->Divide(6,2);
  for(int iside=0; iside<2; iside++){
    for(int ichip=0; ichip<6; ichip++){
      int idx = 6*iside + ichip;
      c2->cd(idx+1);
      h1[idx]->Draw("hist");
    }
  }
  c2->Update();



}
