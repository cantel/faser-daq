#include "TrackerReadout/TRBAccess.h"
#include "TrackerReadout/TRB_ConfigRegisters.h"
#include <iostream>

using namespace FASER;
using namespace std;

//------------------------------------------------------
int main(int argc, char *argv[]){

  int sourceId(0);
  TRBAccess trb(sourceId,false);
  //  trb.SetDebug(1);
 
  return 0;
}
