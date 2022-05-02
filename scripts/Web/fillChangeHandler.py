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

r = redis.Redis(host='localhost', port=6379, db=0,
                charset="utf-8", decode_responses=True)

mattermost_hook=""

if os.access("/etc/faser-secrets.json",os.R_OK):
    secrets=json.load(open("/etc/faser-secrets.json"))
    mattermost_hook=secrets.get("MATTERMOST","")

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

class RunControl:
    def __init__(self):
        self.cfgFile="combinedTI12Physics.json"
        self.runtype="Physics"
        self.startcomment="Restart for fill {0}"
        self.endcomment="End of fill"
        self.url="http://faser-daq-010.cern.ch:5000"
        self.maxTransitionTime=30
        self.minRunTime=300
    
    def doTransition(self,transition,transitionTarget,args):
        result=requests.get(self.url+"/"+transition,params=args)
        if result.status_code!=200 or result.text!="true": 
            print(f"Failed to send <{transition}> transition")
            return False
        now=time.time()
        while(time.time()-now<self.maxTransitionTime):
            time.sleep(1)
            state=requests.get(self.url+"/stateNow")
            if state.status_code!=200: continue
            state=state.json()
            if state["globalStatus"]==transitionTarget: return True
        print(f"Timeout in <{transition}> transition")
        return False
    
    def start(self,fillno):
        return self.doTransition(f"start","RUN",
                                 {"runtype": self.runtype,
                                  "startcomment": self.startcomment.format(fillno)
                              })

    def stop(self):
        return self.doTransition(f"stop","READY",
                                 {"runtype": self.runtype,
                                  "endcomment": self.endcomment,
                              })

    def checkState(self):
        try:
            state=requests.get(self.url+"/stateNow")
        except requests.exceptions.ConnectionError:
            return "NOCONNECTION",False
        if state.status_code!=200: return "UNKNOWNERROR",False
        state=state.json()
        try:
            return (state["globalStatus"],
                    state["runState"]['fileName']==self.cfgFile and state['runType']==self.runtype and 
                    time.time()-float(state["runStart"])>self.minRunTime)
        except KeyError:
            return "CORRUPTEDINFO",False


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
        

if __name__ == '__main__':
    
    fill=FillNo()
    print(f"Starting at fill number {fill.fillNo}")
    rc=RunControl()
    print("Starting DAQ state:",rc.checkState())
    while True:
        newFillNo=fill.checkNewFill()
###FOR TEST        newFillNo=input("Enter new fill number: ")
        if newFillNo:
            message(f"LHC fill number has changed to {newFillNo}")
            state,good=rc.checkState()
            if state=="RUN" and good:
                print("should change run")
#                message("Consider changing run number (not critical - to be automated)")
                
            # do action to stop/start run if in combined run
###                if not rc.stop():
###                    message("Failed to stop run cleanly - please check")
###                elif not rc.start(newFillNo):
###                    message("Failed to start new run cleanly - please check")
###            else:
###                message(f"No physics run seems to be going right now")

        time.sleep(5)
    
        
