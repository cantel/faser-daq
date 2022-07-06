#!/usr/bin/env python3

#
#  Copyright (C) 2019-2022 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time

#until June 30, 2022
refVoltagesToJun30={0: 1440,
             1: 1365,
             2: 1380,
             3: 1395,
             #4:1610,
             5:1495,
             6:1510,
             7:1460,
             8:1825,
             9:1700,
             10:1730,
             11:1680,
             12:1110,
             13:1100,
             14: 850,
             15: 850
}

#post june 30
refVoltages={0: 1450,
             1: 1420,
             2: 1430,
             3: 1405,
             #4:1610,
             5:1495,
             6:1525,
             7:1470,
             8:1845,
             9:1715,
             10:1770,
             11:1710,
             12:1110,
             13:1100,
             14: 875,
             15: 860
}

maxVoltages={0: 1500,
             1: 1500,
             2: 1500,
             3: 1500,
             #4: 2100,
             5: 2100,
             6: 2100,
             7: 2100,
             8: 2100,
             9: 2100,
             10: 2100,
             11: 2100,
             12: 2100,
             13: 2100,
             14: 1000,
             15: 1000
}

stepVoltages={0: 50,
              1: 50,
              2: 50,
              3: 50,
              #4: 50,
              5: 50,
              6: 50,
              7: 50,
              8: 50,
              9: 50,
              10: 50,
              11: 50,
              12: 60,
              13: 60,
              14: 25,
              15: 25
}

lowLightOnly=[]
args=sys.argv[1:]
if args[0]=="-l":
  lowLightOnly=[5,15,6,7,12]
  args=args[1:]

step=int(args[0])

if step<0 or step>18:
    print("ERROR in step",step)
    sys.exit(1)

if step==0 or step==18:
    voltages=refVoltages
else:
    voltages=maxVoltages
    for ch in voltages:
        if ch in lowLightOnly:
            voltages[ch]=refVoltages[ch]
        else:
            voltages[ch]-=(step-1)*stepVoltages[ch]

import pprint
pprint.pprint(voltages)

for ch in voltages:
    volt=voltages[ch]
    rc=os.system(f"snmpset -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputVoltage.u9{ch:02d} F {volt}")
    if rc:
        print("ERROR in channel ",ch)
        sys.exit(1)

time.sleep(5)
redo=True
while redo:
    redo=False
    for ch in voltages:
        volt=voltages[ch]
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

print("All done - now sleep")
time.sleep(30) # give time to stabilize a bit
