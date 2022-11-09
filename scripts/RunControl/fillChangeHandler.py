#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

import json
import os
import requests
import sys
import redis
import time

cfgFile="combinedTI12"
runtype="Physics"
startcomment="Restart for fill {0}"
endcomment="End of fill"
url="http://faser-daq-010.cern.ch:5000"
minRunTime=30

r = redis.Redis(host='localhost', port=6379, db=0,
                charset="utf-8", decode_responses=True)

from runcontrol import RunControl

if os.access("/etc/faser-secrets.json",os.R_OK):
    secrets=json.load(open("/etc/faser-secrets.json"))
    mattermost_hook=secrets.get("MATTERMOST","")

mattermost_hook=""

def message(msg):
    if mattermost_hook:
        try:
            req = requests.post(mattermost_hook,json={"text": msg})
            if req.status_code!=200:
                print("Failed to post message below. Error code:",req.status_code)
                print(msg)
        except Exception as e:
            print("Got exception when posting message",e)
    else:
        print(msg)

def error(msg):
    if mattermost_hook:
        try:
            req = requests.post(mattermost_hook,json={"text": msg,"channel": "faser-ops-alerts"})
            if req.status_code!=200:
                print("Failed to post message below. Error code:",req.status_code)
                print(msg)
        except Exception as e:
            print("Got exception when posting message",e)
    else:
        print(msg)

class FillNo:

    def __init__(self):
        self.fillNo=self.getFillNo()

    def getFillNo(self):
        data=r.hget('digitizer01','LHC_fillnumber')
        if not data: return 0
        try:
            return int(data.split(":")[1])
        except ValueError:
            return 0


    def checkNewFill(self):
        fillno=self.getFillNo()
        if fillno>self.fillNo:
            self.fillNo=fillno
            return fillno
        return None

def stateCheck(state):
    if (state and
        state['runOngoing'] and
        state['loadedConfig'].startswith(cfgFile) and
        state['runState']=='RUN' and
        not state['crashedM'] and
        state['runType']==runtype and
        time.time()-float(state['runStart'])>minRunTime):
        return True
    return False

if __name__ == '__main__':
    active=False
    test=False
    if "--active" in sys.argv:
        print("Running in active mode - will stop/start runs")
        active=True
    if "--test" in sys.argv:
        test=True
    fill=FillNo()
    print(f"Starting at fill number {fill.fillNo}")
    rc=RunControl(baseUrl = url)
    print("Starting DAQ state:",rc.getState())
    while True:
        newFillNo=fill.checkNewFill()
        if test:
            newFillNo=input("Enter new fill number: ")
        if newFillNo:
            message(f"LHC fill number has changed to {newFillNo}")
            state=rc.getState()
            if stateCheck(state):
                # do action to stop/start run if in combined run
                if active:
                    if not rc.stop(runtype,endcomment):
                        error("Failed to stop run cleanly - please check")
                    elif not rc.start(runtype,startcomment.format(newFillNo)):
                        error("Failed to start new run cleanly - please check")
                else:
                    print("should change run")

            else:
                message(f"No physics run seems to be going right now")


        time.sleep(5)
