#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time

voltages={4:1610,
          5:1495,
          6:1510,
          7:1460,
          8:1655,
          9:1525,
          10:1575,
          11:1500,
          12:1515,
          13:1495}

offset=int(sys.argv[1])

if offset<0 or offset>600:
    print("ERROR in offset",offset)
    sys.exit(1)

for ch in voltages:
    volt=voltages[ch]-offset
    rc=os.system(f"snmpset -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputVoltage.u9{ch:02d} F {volt}")
    if rc:
        print("ERROR in channel ",ch)
        sys.exit(1)

time.sleep(5)
redo=True
while redo:
    redo=False
    for ch in voltages:
        volt=voltages[ch]-offset
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
