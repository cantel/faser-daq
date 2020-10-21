#!/usr/bin/env python3

# Author : Brian Petersen <Brian.Petersen@cern.ch>

# Description : This converts logically written statements for the four trigger streams
#               into the look up table (LUT) that is used to configure the TLB.

# Usage : You must provide four strings (TBP{0,1,2,3}) which correspond to the 
#         four trigger streams and will map into four bits for each line of the 
#         LUT.  Each of these triggers should be logical combinations of the eight 
#         LVDS trigger bits from the Digitizer Tin{0-7}.  If you do not need all 
#         four trigger streams, you can just enter "0" for a given TBP stream.




#signals assumed:
#Tin0: first veto layer
#Tin1: second veto layer
#Tin2: timing layer top
#Tin3: timing layer bottom
#Tin4: preshower layer
#Tin5: calorimeter bottom
#Tin6: calorimeter top
#Tin7: Unused

#trigger items:

##any scintillator trigger:
#TBP0="Tin0|Tin1|Tin2|Tin3|Tin4"
#
##any calo trigger:
#TBP1="Tin5|Tin6"
#
##good muon trigger:
#TBP2="Tin0&Tin1&Tin2&Tin3&Tin4"
#
##dark photon signal trigger (scintillator only):
#TBP3="(Tin2|Tin3)&Tin4&notTin0&notTin1"



import argparse
parser = argparse.ArgumentParser()
parser.add_argument("-square", type=int, default=2, help="display a square of a given number")
parser.add_argument("-t0",     type=str, default="0",   help="Trigger0 logical combination")
parser.add_argument("-t1",     type=str, default="0",   help="Trigger1 logical combination")
parser.add_argument("-t2",     type=str, default="0",   help="Trigger2 logical combination")
parser.add_argument("-t3",     type=str, default="0",   help="Trigger3 logical combination")
parser.add_argument("-o",      type=str, default="LUT_Output.txt", help="Output look up table")

parser.add_argument("-v", "--verbose", action="store_true", help="increase output verbosity")

args = parser.parse_args()

TBP0   = args.t0
TBP1   = args.t1
TBP2   = args.t2
TBP3   = args.t3
output = args.o

print("TBP0   : ",TBP0  )
print("TBP1   : ",TBP1   )  
print("TBP2   : ",TBP2   )  
print("TBP3   : ",TBP3   )  
print("output : ",output )  

#TBP0="Tin0&notTin1&notTin2&notTin3&notTin4&notTin5&notTin6&notTin7"
#TBP1="0"
#TBP2="0"
#TBP3="0"
#
#TBP0="Tin0&Tin1"
#TBP1="0"
#TBP2="0"
#TBP3="0"
#
print(globals())

# check if output file exists



fout = open(output,"w")

for inp in range(256):
    #print("Line : ",inp)
    for ibit in range(8):
        val=0
        if inp&(1<<ibit): 
          val=1

        globals()["Tin"+str(ibit)]=val
        globals()["notTin"+str(ibit)]=1-val

    TBP=""

    for obit in range(4):
        TBP=str(eval(eval("TBP"+str(obit))))+TBP

    print(TBP)
