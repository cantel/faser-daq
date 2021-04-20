#!/usr/bin/env python3

#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#


import getopt
import json
import os
import requests
import sys
import time

class runner:
    def __init__(self,config,hostUrl,maxTransitionTime):
        self.cfgFile=config["cfgFile"]
        self.runtype=config["runtype"]
        self.startcomment=config["startcomment"]
        self.endcomment=config["endcomment"]
        self.maxRunTime=config["maxRunTime"]
        self.maxEvents=config["maxEvents"]
        self.preCommand=config["preCommand"]
        self.postCommand=config["postCommand"]
        self.url=hostUrl
        self.maxTransitionTime=maxTransitionTime
    
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
    
    def initialize(self):
        return self.doTransition(f"initialise/{self.cfgFile}","READY",None)

    def start(self):
        return self.doTransition(f"start","RUN",
                                 {"runtype": self.runtype,
                                  "startcomment": self.startcomment,
                              })

    def stop(self):
        return self.doTransition(f"stop","READY",
                                 {"runtype": self.runtype,
                                  "endcomment": self.endcomment,
                              })

    def shutdown(self):
        return self.doTransition(f"shutdown","DOWN",None)


    def checkState(self):
     state=requests.get(self.url+"/stateNow")
     if state.status_code!=200: return "UNKNOWN"
     return state.json()["globalStatus"]

    def getEvents(self):
     response=requests.get(self.url+"/monitoring/eventCounts")
     if response.status_code!=200: return 0
     return response.json()

    def waitStop(self):
        now=time.time()
        while(True):
            state=self.checkState()
            if state!="RUN":
                print(f"State changed to '{state}' - bailing out")
                return False
            events=self.getEvents()
            if events["Events_sent_Physics"]>=self.maxEvents: return True
            if time.time()-now>self.maxRunTime: return True
            print(f'State: {state}, events: {events["Events_sent_Physics"]}, time: {time.time()-now} seconds')
            time.sleep(5)

    def run(self):
        if self.checkState()!="DOWN":
            print("System is not shutdown - will not start run")
            return False

        if self.preCommand:
            rc=os.system(self.preCommand)
            if rc:
                print("Failed to pre-command:",self.preCommand)
                return False
            print("Ran precommand")

        rc=self.initialize()
        if not rc:
            print("Failed to initialize")
            return False
        print("INITIALIZED")

        rc=self.start()
        if not rc:
            print("Failed to start run")
            return False
        print("RUNNING")

        rc=self.waitStop()
        if not rc:
            print("Failed to reach run conditions")
            return False

        rc=self.stop()
        if not rc:
            print("Failed to stop")
            return False
        print("STOPPED")

        rc=self.shutdown()
        if not rc:
            printf("Failed to shutdown - retry again in 10 seconds")
            time.sleep(10)
            rc=self.shutdown()
            if not rc:
                printf("Failed to shutdown - giving up")
                return False
        print("SHUTDOWN")

        if self.postCommand:
            rc=os.system(self.postCommand)
            if rc:
                print("Failed to post-command:",self.postCommand)
                return False
            print("Ran post-command")

        return True

def usage():
    print("./sequencer.py [-s <stepNumber>] <configuration.json>")
    sys.exit(1)

def main(args):
    
    startStep=1
    try:
        opts, args = getopt.getopt(args,"s:",[])
    except getopt.GetoptError:
        usage()
    for opt,arg in opts:
        if opt=="-s":
            startStep=int(arg)
    if len(args)!=1:
        usage()


    config=json.load(open(args[0]))
    hostUrl=config["hostUrl"]
    maxTransitionTime=config["maxTransitionTime"]
    cfgs=[]
    for cfg in config["steps"]:
        for par in ["cfgFile",
                    "runtype",
                    "startcomment",
                    "endcomment",
                    "maxRunTime",
                    "maxEvents",
                    "preCommand",
                    "postCommand"]:
            if not par in cfg:
                if not par in config:
                    print(f"Did not find value for '{par}'")
                    return 1
                cfg[par]=config[par]
        cfgs.append(cfg)
    
    for step in range(startStep-1,len(cfgs)):
        print(f"Running step {step+1}")
        cfg=cfgs[step]
        run=runner(cfg,hostUrl,maxTransitionTime)
        rc=run.run()
        if rc:
            print("Successful run")
        else:
            print("Failed to run, please check")
            return 1
    return 0

if __name__ == "__main__":
   sys.exit(main(sys.argv[1:]))


