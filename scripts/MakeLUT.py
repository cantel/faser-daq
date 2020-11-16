#!/usr/bin/env python3

# Author : Brian Petersen <Brian.Petersen@cern.ch>

# Description : This converts logically written statements for the four trigger streams
#               into the look up table (LUT) that is used to configure the TLB.

# Usage : You must provide four strings (TBP{0,1,2,3}) which correspond to the 
#         four trigger streams and will map into four bits for each line of the 
#         LUT.  Each of these triggers should be logical combinations of the eight 
#         LVDS trigger bits from the Digitizer Tin{0-7}.  If you do not need all 
#         four trigger streams, you can just enter "0" for a given TBP stream.

import argparse
import os

def main():
  parser = argparse.ArgumentParser(description='Generate FASER look up table (LUT) from a set of logical input\n' \
                                                'trigger signals for each of the four trigger combinations. The\n' \
                                                'eight possible trigger bits as input are :\n' \
                                                ' \n' \
                                                'Tin0: first veto layer \n' \
                                                'Tin1: second veto layer \n' \
                                                'Tin2: timing layer top \n' \
                                                'Tin3: timing layer bottom \n' \
                                                'Tin4: preshower layer \n' \
                                                'Tin5: calorimeter bottom \n' \
                                                'Tin6: calorimeter top \n' \
                                                'Tin7: Unused \n' \
                                                ' \n' \
                                                'These are then combined in each of the four "-t{0,1,2,3}" command line \n' \
                                                'arguments.  If you explicitly want a veto on one of the trigger bits, \n' \
                                                'then the trigger bit is negated by changing TinX to notTinX. Combinations \n' \
                                                'of triggers are performed using logical operators "&" (and) and "|" (or) \n' \
                                                'along with "()" to specify groupings. \n' \
                                                ' \n' \
                                                'Example : If you want to configure the following four triggers \n' \
                                                ' \n' \
                                                '  (TBP0) any scintillator trigger: "Tin0|Tin1|Tin2|Tin3|Tin4" \n' \
                                                '  (TBP1) any calo trigger: "Tin5|Tin6" \n' \
                                                '  (TBP2) good muon trigger: "Tin0&Tin1&Tin2&Tin3&Tin4" \n' \
                                                '  (TBP3) dark photon signal trigger (scintillator only): "(Tin2|Tin3)&Tin4&notTin0&notTin1" \n' \
                                                ' \n' \
                                                'then this would be formatted as follows to produce the output file \n' \
                                                'myLUT.txt in the local directory \n' \
                                                ' \n' \
                                                'python MakeLUT.py -t0 Tin0|Tin1|Tin2|Tin3|Tin4 -t1 Tin5|Tin6 -t2 Tin0&Tin1&Tin2&Tin3&Tin4 -t3 (Tin2|Tin3)&Tin4&notTin0&notTin1 -o myLUT.txt \n'                                              
                                                , formatter_class=argparse.RawTextHelpFormatter)
  parser.add_argument("-t0",     type=str, default="0",   help="Trigger0 logical combination")
  parser.add_argument("-t1",     type=str, default="0",   help="Trigger1 logical combination")
  parser.add_argument("-t2",     type=str, default="0",   help="Trigger2 logical combination")
  parser.add_argument("-t3",     type=str, default="0",   help="Trigger3 logical combination")
  parser.add_argument("-o",      type=str, default="LUT_Output.txt", help="Output look up table")
  parser.add_argument("-f", "--force",   action="store_true", help="Force overwrite of output file")
  parser.add_argument("-v", "--verbose", action="store_true", help="increase output verbosity")

  args = parser.parse_args()

  TBP0    = args.t0
  TBP1    = args.t1
  TBP2    = args.t2
  TBP3    = args.t3
  output  = args.o
  force   = args.force
  verbose = args.verbose

  print("TBP0    : ",TBP0  )
  print("TBP1    : ",TBP1   )  
  print("TBP2    : ",TBP2   )  
  print("TBP3    : ",TBP3   )  
  print("output  : ",output )  
  print("force   : ",force )  
  print("verbose : ",verbose )  

  if(verbose):
    print(globals())

  # check if output file exists
  if os.path.isfile(output):
    print(">>> This file already exists : ",output)
    if force:
      print(">>> Forcefully removing file")
      os.remove(output)
    else:
      print(">>> Remove this file or use the [--force] command line argument")
      print(">>> to force an overwriting of this file. Exitting.")
      return 9  


  # output file
  fout = open(output,"w")

  # going through and configuring each of the trigger bits
  for inp in range(256):
    if(verbose):
      print("Line : ",inp)

    for ibit in range(8):
      val=0
      if inp&(1<<ibit): 
        val=1

      globals()["Tin"+str(ibit)]=val
      globals()["notTin"+str(ibit)]=1-val
      
      if(verbose):
        print(globals())

    # this stacks the four trigger combinations together using eval()
    # https://www.programiz.com/python-programming/methods/built-in/eval
    TBP=""
    for obit in range(4):
      TBP=str(eval(eval("TBP"+str(obit))))+TBP

    if(verbose):
      print(TBP)
      
    # write the output to the LUT file
    fout.write(TBP+'\n')
    
  # close the LUT file before exitting
  fout.close()
  
  print(">>> Successfully wrote LUT to : ",output)




if __name__ == '__main__':
  main()