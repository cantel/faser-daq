#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time

highGainVoltages={0: 1450,
                  1: 1420,
                  2: 1430,
                  3: 1405}

lowGainVoltages={0: 855,
                 1: 840,
                 2: 855,
                 3: 840}


#gain is interpolation between the above voltages

if len(sys.argv)!=5:
    print("Usage: setCaloGainBeam.py <GF0> <GF1> <GF2> <GF3>")
    sys.exit(1)

gainfactors=[float(arg) for arg in sys.argv[1:5]]

if min(gainfactors)<-0.1:
    print("too low gain factor")
    sys.exit(1)

if max(gainfactors)>1.1:
    print("too high gain factor")
    sys.exit(1)

newTargets={}

for ch in lowGainVoltages:
    volt=int(lowGainVoltages[ch]+gainfactors[ch]*(highGainVoltages[ch]-lowGainVoltages[ch]))
    newTargets[ch]=volt
    rc,output=subprocess.getstatusoutput(f"snmpset -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputVoltage.u9{ch:02d} F {volt}")
    if rc:
        print("ERROR in channel ",ch)
        sys.exit(1)
    print(f"Set HV channel {ch} to {volt} volts")

time.sleep(5)
redo=True
timeout=30
while redo:
    redo=False
    for ch in lowGainVoltages:
        volt=newTargets[ch]
        rc,output=subprocess.getstatusoutput(f"snmpget -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputMeasurementSenseVoltage.u9{ch:02d}")
        #print(output)
        if rc:
            print("ERROR in channel ",ch)
            sys.exit(1)
        newValue=-float(output.split()[-2])
        if abs(volt-newValue)>5:
            #print("Not ready yet:",volt,newValue)
            redo=True
            time.sleep(1)
    print("Desired HV not yet reached - waiting a bit longer")
    timeout-=1
    if timeout==0:
        print("Failed to set desired HV - please check")
        sys.exit(1)

time.sleep(10) # give time to stabilize a bit
