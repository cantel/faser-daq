#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time


#gain is wrt to above voltages

if len(sys.argv)!=2:
    print("Usage: setCaloVoltage.py <voltage>")
    sys.exit(1)

volt=float(sys.argv[1])

if volt<100 or volt>1600:
    print(f"{volt} V is out of range")
    sys.exit(1)

newTargets={}

for ch in range(4):
    newTargets[ch]=volt
    rc=os.system(f"snmpset -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputVoltage.u9{ch:02d} F {volt}")
    if rc:
        print("ERROR in channel ",ch)
        sys.exit(1)

time.sleep(5)
redo=True
while redo:
    redo=False
    for ch in range(4):
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


