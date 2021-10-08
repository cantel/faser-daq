#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time

voltageSettings={10:   {0: 150,
                   	1: 10,
                   	2: 150,
                   	5: 150,
                   	6: 10,
                   	7: 150},
                 20:   {0: 150,
                   	1: 20,
                   	2: 150,
                   	5: 150,
                   	6: 20,
                   	7: 150},
                 30:   {0: 150,
                   	1: 30,
                   	2: 150,
                   	5: 150,
                   	6: 30,
                   	7: 150},
                 40:   {0: 150,
                   	1: 40,
                   	2: 150,
                   	5: 150,
                   	6: 40,
                   	7: 150},
                 50:   {0: 150,
                   	1: 50,
                   	2: 150,
                   	5: 150,
                   	6: 50,
                   	7: 150},
                 75:   {0: 150,
                   	1: 75,
                   	2: 150,
                   	5: 150,
                   	6: 75,
                   	7: 150},
                 100:  {0: 150,
                   	1: 100,
                   	2: 150,
                   	5: 150,
                   	6: 100,
                   	7: 150},
                 125:  {0: 150,
                   	1: 125,
                   	2: 150,
                   	5: 150,
                   	6: 125,
                   	7: 150},
                 150:  {0: 150,
                   	1: 150,
                   	2: 150,
                   	5: 150,
                   	6: 150,
                   	7: 150},
                 175:  {0: 150,
                   	1: 175,
                   	2: 150,
                   	5: 150,
                   	6: 175,
                   	7: 150},
                 200:  {0: 150,
                   	1: 200,
                   	2: 150,
                   	5: 150,
                   	6: 200,
                   	7: 150},
               	}


#gain is wrt to above voltages

if len(sys.argv)!=2:
    print("Usage: setCaloHV.py <Voltage>")
    sys.exit(1)

try:
    voltage=float(sys.argv[1])
except ValueError:
    print("not a valid setting: {sys.argv[1]}")
    sys.exit(1)

if voltage<0:
    print("Invalid setting, only positive values allowed")
if voltage>200:
    print("Invalid setting, values >200 V not allowed")

voltages=voltageSettings[voltage]

newTargets={}

for ch in voltages:
    volt=voltages[ch]
    newTargets[ch]=volt
    rc=os.system(f"snmpset -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-test outputVoltage.u6{ch:02d} F {volt}")
    if rc:
        print("ERROR in channel ",ch)
        sys.exit(1)
                   
time.sleep(5)
redo=True
while redo:
    redo=False
    for ch in voltages:
        volt=newTargets[ch]
        rc,output=subprocess.getstatusoutput(f"snmpget -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-test outputMeasurementSenseVoltage.u6{ch:02d}")
        print(output)
        if rc:
            print("ERROR in channel ",ch)
            sys.exit(1)
        newValue=float(output.split()[-2])
        if abs(volt-newValue)>5:
            print("Not ready yet:",volt,newValue)
            redo=True
            time.sleep(5)

time.sleep(5) # give time to stabilize a bit
