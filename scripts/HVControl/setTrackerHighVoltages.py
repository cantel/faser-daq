#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import os
import subprocess
import sys
import time


if len(sys.argv)!=3:
    print("Usage: setTrackerHighVoltages.py <voltage> <layer>")
    sys.exit(1)

volt=float(sys.argv[1])
layer=int(sys.argv[2])

if volt<0:
    print("Invalid setting, only positive values allowed")
    sys.exit(1)

if volt>200:
    print("Invalid setting, values >200 V not allowed")
    sys.exit(1)

if layer<0:
    print("Invalid setting, only positive values allowed")
    sys.exit(1)

if layer>2:
    print("Invalid setting, only layer0, 1, 2 allowed")
    sys.exit(1)

newTargets={}

channel_list = []
if layer == 0:
    #S1_L0_m0_3
    channel_list.append(407)
    #S1_L0_m4_7
    channel_list.append(400)

    #S2_L0_m0_3
    channel_list.append(507)
    #S2_L0_m4_7
    channel_list.append(500)

    #S3_L0_m0_m3
    channel_list.append(602)
    #S3_L0_m4_m7
    channel_list.append(605)

    #S0_L0_m0_m3
    channel_list.append(707)
    #S0_L0_m4_m7
    channel_list.append(700)


elif layer == 1:
    #S1_L1_m0_3
    channel_list.append(406)
    #S1_L1_m4_7
    channel_list.append(401)

    #S2_L1_m0_3
    channel_list.append(506)
    #S2_L1_m4_7
    channel_list.append(501)

    #S3_L1_m0_m3
    channel_list.append(601)
    #S3_L1_m4_m7
    channel_list.append(606)

    #S0_L1_m0_m3
    channel_list.append(706)
    #S0_L1_m4_m7
    channel_list.append(701)


elif layer == 2:
    #S1_L2_m0_3
    channel_list.append(405)
    #S1_L2_m4_7
    channel_list.append(402)

    #S2_L2_m0_3
    channel_list.append(505)
    #S2_L2_m4_7
    channel_list.append(502)

    #S3_L2_m0_m3
    channel_list.append(600)
    #S3_L2_m4_m7
    channel_list.append(607)

    #S0_L2_m0_m3
    channel_list.append(705)
    #S0_L2_m4_m7
    channel_list.append(702)


print(channel_list)

for ch in channel_list:
    newTargets[ch]=volt
    print(volt)
    print(layer)
    print(ch)
    print(f"faser-mpod-00 outputVoltage.u{ch:03d} F {volt}")
    rc=os.system(f"snmpset -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputVoltage.u{ch:03d} F {volt}")
    if rc:
        print("ERROR in channel ",ch)
        sys.exit(1)

time.sleep(5)
redo=True
while redo:
    redo=False
    for ch in channel_list:
        volt=newTargets[ch]
        rc,output=subprocess.getstatusoutput(f"snmpget -v 2c -m +WIENER-CRATE-MIB -c guru faser-mpod-00 outputMeasurementSenseVoltage.u{ch:03d}")
        print(output)
        if rc:
            print("ERROR in channel ",ch)
            sys.exit(1)
        newValue=-float(output.split()[-2])
        if abs(abs(volt)-abs(newValue))>2:
            print("volrage = "+str(volt)+" new value = "+str(newValue))
            print("Not ready yet:",volt,newValue)
            redo=True
            time.sleep(1)


