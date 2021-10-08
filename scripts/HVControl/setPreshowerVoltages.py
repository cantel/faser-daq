#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time

voltageSettings={"high": 
                 {
                   12: 1100,
                   13: 1100,
                  },
                 "medium": 
                 {
                   12: 1100,
                   13: 1100,
               },
                 "low": 
                 {
                   12: 1100,
                   13: 1100,
               }
             }


#gain is wrt to above voltages

if len(sys.argv)!=3:
    print("Usage: setPreshowerVoltages.py <Gain> <Offset>")
    sys.exit(1)

gainName=sys.argv[1]
if not gainName in voltageSettings:
    print(f"Wrong gain name: {gainName}")
    sys.exit(1)

try:
    offset=float(sys.argv[2])
except ValueError:
    print(f"not a valid offset: {sys.argv[2]}")
    sys.exit(1)

if offset<-500:
    print("Invalid offset, only positive values allowed")
    sys.exit(1)
if offset>900:
    print("Invalid offset, values >700 V not allowed")
    sys.exit(1)

voltages=voltageSettings[gainName]

offsetPreshower=offset

newTargets={}

for ch in voltages:
    if ch>10:
        volt=voltages[ch]-offsetPreshower
    else:
        volt=voltages[ch]-offset
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
