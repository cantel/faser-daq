#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

import os
import sys
import requests
import time

def usage():
    print("Usage: setLEDPulser.py <freq. in Hz> <Ampl. A> <Ample. B>")
    sys.exit(1)

def doSetting(setting):
    url="http://faser-ledcalib-03.cern.ch/"+setting
    r=requests.get(url)
    if r.status_code!=200:
        print("Error setting "+setting+": status_code="+r.status_code)
        sys.exit(1)
    result=r.json()
    if result.get("success",False)!=True:
        print("Error setting "+setting+": result="+result)
        sys.exit(1)
    print(f"Successfully set '{setting}' for LED calibration system")

def main(args):

    if len(args)!=3:
        usage()
    try:
        freq=int(args[0])
        amplA=int(args[1])
        amplB=int(args[2])
    except ValueError:
        print("Could not convert arguments")
        usage()
    
    if freq<0 or freq>20000:
        print("invalid frequency range")
        sys.exit(1)
    
    if amplA<0 or amplA>1000:
        print("invalid A amplitude")
        sys.exit(1)
    if amplB<0 or amplB>3300:
        print("invalid B amplitude")
        sys.exit(1)

    rc=os.system("ping -c 1 faser-ledcalib-03 > /dev/null")
    if rc!=0:
        print("Error communicating with LED calibration system")
        sys.exit(1)

    doSetting(f"amplitude/A/{amplA}")
    doSetting(f"amplitude/B/{amplB}")
    doSetting(f"frequency/{freq}")
    if amplA>0:
        doSetting("enable/A")
    else:
        doSetting("disable/A")
    if amplB>0:
        doSetting("enable/B")
    else:
        doSetting("disable/B")

    if amplA>0 or amplB>0:
        time.sleep(5) # wait a bit for pulse to stabilize

if __name__ == "__main__":
   main(sys.argv[1:])
