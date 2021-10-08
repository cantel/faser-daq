#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time

voltageSettings={"high": 
                 { 0: 1400*1.109,
                   1: 1400*1.018,
                   2: 1400*1.022,
                   3: 1400,
                   4: 1400*1.004,
                   5: 1400*1.116},
                 "medium": 
                 { 0: 1100*1.109,
                   1: 1100*1.018,
                   2: 1100*1.022,
                   3: 1100,
                   4: 1100*1.004,
                   5: 1100*1.116},
                 "low": 
                 { 0: 700,
                   1: 700,
                   2: 700,
                   3: 700,
                   4: 700,
                   5: 700
               }
             }


#gain is wrt to above voltages

if len(sys.argv)!=8:
    print("Usage: setCaloHV.py <Gain> <Offset0> <Offset1> <Offset2> <Offset3> <Offset4> <Offset5>")
    sys.exit(1)

gainName=sys.argv[1]
if not gainName in voltageSettings:
    print(f"Wrong gain name: {gainName}")
    sys.exit(1)

try:
    offsets=[float(arg) for arg in sys.argv[2:]]
except ValueError:
    print(f"not a valid offset: {sys.argv[2:]}")
    sys.exit(1)

if min(offsets)<-210:
    print("Invalid offset, only positive values allowed")
    sys.exit(1)

if max(offsets)>900:
    print("Invalid offset, values >700 V not allowed")
    sys.exit(1)

voltages=voltageSettings[gainName]

newTargets={}

for ch in voltages:
    volt=voltages[ch]-offsets[ch]
    newTargets[ch]=volt
    rc=os.system(f"snmpset -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-test outputVoltage.u9{ch:02d} F {volt}")
    if rc:
        print("ERROR in channel ",ch)
        sys.exit(1)
                   
time.sleep(5)
redo=True
while redo:
    redo=False
    for ch in voltages:
        volt=newTargets[ch]
        rc,output=subprocess.getstatusoutput(f"snmpget -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-test outputMeasurementSenseVoltage.u9{ch:02d}")
        print(output)
        if rc:
            print("ERROR in channel ",ch)
            sys.exit(1)
        newValue=-float(output.split()[-2])
        if abs(volt-newValue)>5:
            print("Not ready yet:",volt,newValue)
            redo=True
            time.sleep(5)

time.sleep(30) # give time to stabilize a bit
