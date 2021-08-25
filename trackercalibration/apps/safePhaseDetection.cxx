/** \file safePhaseDetection.cxx
 * Simple standalone program to determine the finePhase settings of the TRB.
 */

#include "TrackerReadout/TRBAccess.h"
#include "TrackerReadout/TRB_ConfigRegisters.h"
#include <iostream>
#include <iomanip>
#include <unistd.h>

#define N_STEPS 64
#define N_FINE_PHASE_CLK 2

using namespace std;

//------------------------------------------------------
int main(int argc, char *argv[]){
  
  std::cout << std::endl;
  std::cout << "====================================================================" << std::endl << std::endl;
  std::cout << "BEFORE CONTINUING MAKE SURE THAT:" << std::endl << std::endl;
  std::cout << " 1.- A module is connected to trbChannel 0" << std::endl;
  std::cout << " 2.- The module has been power-cycled" << std::endl << std::endl;
  std::cout << "====================================================================" << std::endl;
  std::cout << "When above is done, press enter to continue..." << std::endl;
  while(std::cin.get()!='\n'); 
  
  int v0[N_FINE_PHASE_CLK][N_STEPS];
  int v1[N_FINE_PHASE_CLK][N_STEPS];
  for(int i=0; i<N_FINE_PHASE_CLK; i++){
    for(int j=0; j<N_STEPS; j++){
      v0[i][j]=-1;
      v1[i][j]=-1;
    }
  }

  FASER::TRBAccess *trb = new FASER::TRBAccess(0,false);
  FASER::PhaseReg *phaseReg = trb->GetPhaseConfig();
  
  uint16_t status;    
  unsigned int finePhaseClk[N_FINE_PHASE_CLK]={0,16};
  for(unsigned int idx=0; idx<N_FINE_PHASE_CLK; idx++){
    std::cout << "[" << idx << "] running FinePhaseClk = " 
	      << std::dec << finePhaseClk[idx] << "..." << std::endl;
    phaseReg->SetFinePhase_Clk0(finePhaseClk[idx]);
    phaseReg->SetFinePhase_Clk1(finePhaseClk[idx]);
    trb->WritePhaseConfigReg();
    trb->ApplyPhaseConfig();
    
    for(unsigned int step=0; step<64; step++){
      phaseReg->SetFinePhase_Led0(step);
      phaseReg->SetFinePhase_Led1(step);
      trb->WritePhaseConfigReg();
      trb->ApplyPhaseConfig();

      usleep(20);

      trb->ReadStatus(status);
      v0[idx][step] = int(status & 0x10) != 0 ? 1 : 0;
      v1[idx][step] = int(status & 0x20) != 0 ? 1 : 0;
    }
  }

  std::cout << "----------------" << std::endl;
  std::cout << "Results : " << std::endl;
  unsigned int idx(0), idxbef(0), transition(0);
  for(unsigned int step=0; step<64; step++){
    idx = v0[1][step];
    
    if(step >= 1){
      idxbef = v0[1][step-1];
      if(idx != idxbef){
	transition=step-1;
      }
    }

    std::cout << " step " << std::setfill('0') << std::setw(2) << std::dec << step << " =>";    
    std::cout << " SafePhaseDetect [0] = [" << v0[0][step] << v0[1][step] << "] ;";
    std::cout << " SafePhaseDetect [1] = [" << v1[0][step] << v1[1][step] << "]";
    std::cout << std::endl;
  }

  cout << "Transition at : " << transition << endl;

  cout << endl;
  cout << "---------------------------------------------" << endl;
  cout << "Optimal finePhase led = " << (transition + 32) % 64 << endl;
  cout << "---------------------------------------------" << endl;

  cout << endl << "Bye ! " << endl;
  
  delete trb;
  return 0;
}
