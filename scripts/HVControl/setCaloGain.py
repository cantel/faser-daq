#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time

voltages={ 0:1040,
           1:965,
           2:980,
           3:995}

#gain is wrt to above voltages

if len(sys.argv)!=5:
    print("Usage: setCaloGain.py <GF0> <GF1> <GF2> <GF3>")
    sys.exit(1)

gainfactors=[float(arg) for arg in sys.argv[1:5]]

if min(gainfactors)<0.1:
    print("too low gain factor")
    sys.exit(1)

if max(gainfactors)>11:
    print("too high gain factor")
    sys.exit(1)

newTargets={}

for ch in voltages:
    volt=voltages[ch]*(gainfactors[ch]**(1/6.6))
    newTargets[ch]=volt
    rc=os.system(f"snmpset -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputVoltage.u9{ch:02d} F {volt}")
    if rc:
        print("ERROR in channel ",ch)
        sys.exit(1)

time.sleep(5)
redo=True
while redo:
    redo=False
    for ch in voltages:
        volt=newTargets[ch]
        rc,output=subprocess.getstatusoutput(f"snmpget -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputMeasurementSenseVoltage.u9{ch:02d}")
        print(output)
        if rc:
            print("ERROR in channel ",ch)
            sys.exit(1)
        newValue=-float(output.split()[-2])
        if abs(volt-newValue)>5:
            print("Not ready yet:",volt,newValue)
            redo=True
            time.sleep(1)

time.sleep(30) # give time to stabilize a bit
