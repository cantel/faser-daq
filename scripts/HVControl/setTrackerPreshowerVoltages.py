#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time

if len(sys.argv)!=3:
    print("Usage: setTrackerPreshowerVoltages.py <TrackerVoltage> <PreshowerOffset>")
    sys.exit(1)

try:
    trackervoltage=int(sys.argv[1])
except ValueError:
    print("not a valid setting: {sys.argv[1]}")
    sys.exit(1)

try:
    preshoweroffset=int(sys.argv[2])
except ValueError:
    print("not a valid setting: {sys.argv[2]}")
    sys.exit(1)
    
    
rc=os.system(f"/home/shifter/software/faser-daq/scripts/HVControl/setTrackerHighVoltages.py {trackervoltage}")
if rc:
    print("ERROR in setting tracker voltage")
    sys.exit(1)
            
rc=os.system(f"/home/shifter/software/faser-daq/scripts/HVControl/setPreshowerVoltages.py medium {preshoweroffset}")
if rc:
    print("ERROR in setting preshower offset")
    sys.exit(1)

       
time.sleep(1)
