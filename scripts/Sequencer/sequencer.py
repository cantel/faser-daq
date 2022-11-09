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

sys.path.append("../RunControl")

from runcontrol import RunControl

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


class runner:
    def __init__(self,config,runControl,seqnumber,seqstep):
        self.cfgFile=config["cfgFile"]
        self.runtype=config["runtype"]
        self.startcomment=config["startcomment"]
        self.endcomment=config["endcomment"]
        self.maxRunTime=int(config["maxRunTime"])
        self.maxEvents=int(config["maxEvents"])
        self.preCommand=config["preCommand"]
        self.postCommand=config["postCommand"]
        self.seqnumber=seqnumber
        self.seqstep=seqstep
        self.runnumber=None
        self.rc=runControl
    
    
    def initialize(self):
        status=self.rc.change_config(self.cfgFile.replace(".json",""))
        if not status: return False
        return self.rc.initialise()

    def start(self):
        return self.rc.start(self.runtype,
                             self.startcomment,
                             self.seqnumber,
                             self.seqstep,
                             0 # seqsubstep - not supported yet
                         )

    def stop(self):
        return self.rc.stop(self.runtype,self.endcomment)

    def shutdown(self):
        return self.rc.shutdown()


    def checkState(self):
     state=self.rc.getState()
     return state["runState"],state["runNumber"]

    def getEvents(self):
        data=self.rc.getInfo("eventbuilder01")
        if "Events_sent_Physics" in data:
            return int(data["Events_sent_Physics"])
        else:
            return 0

    def waitStop(self):
        now=time.time()
        while(True):
            state,runnumber=self.checkState()
            if state!="RUN":
                print(f"State changed to '{state}' - bailing out")
                return False
            self.runnumber=runnumber
            events=self.getEvents()
            if events>=self.maxEvents: return True
            if time.time()-now>self.maxRunTime: return True
            print(f'Run {runnumber}, State: {state}, events: {events}, time: {time.time()-now} seconds')
            time.sleep(5)

    def run(self):
        if self.checkState()[0]!="DOWN":
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
        time.sleep(1)
        rc=self.start()
        if not rc:
            print("Failed to start run")
            return False
        print("RUNNING")
        time.sleep(5)
        rc=self.waitStop()
        if not rc:
            print("Failed to reach run conditions")
            return False

        rc=self.stop()
        if not rc:
            print("Failed to stop")
            return False
        print("STOPPED")
        time.sleep(5)
        rc=self.shutdown()
        if not rc:
            print("Failed to shutdown - retry again in 10 seconds")
            time.sleep(10)
            rc=self.shutdown()
            if not rc:
                print("Failed to shutdown - giving up")
                return False
        print("SHUTDOWN")
        for ii in range(5):
            if self.checkState()[0]=="DOWN": break
            time.sleep(1)
        if self.checkState()[0]!="DOWN":
            print("Failed to shutdown",self.checkState())
            return False

        if self.postCommand:
            rc=os.system(self.postCommand)
            if rc:
                print("Failed to post-command:",self.postCommand)
                return False
            print("Ran post-command")

        return True

def usage():
    print("./sequencer.py [-S <seqNumber> -s <stepNumber> ] <configuration.json>")
    sys.exit(1)

def main(args):
    
    startStep=1
    seqnumber=0 
    try:
        opts, args = getopt.getopt(args,"s:S:",[])
    except getopt.GetoptError:
        usage()
    for opt,arg in opts:
        if opt=="-s":
            startStep=int(arg)
        if opt=="-S":
            seqnumber=int(arg)
    if len(args)!=1:
        usage()
    if startStep!=1 and seqnumber==0:
        usage() #FIXME, maybe one should be able to look up last step?


    config=json.load(open(args[0]))
    hostUrl=config["hostUrl"]
    maxTransitionTime=config["maxTransitionTime"]
    cfgs=[]
    if "template" in config:
        if "steps" in config:
            print("ERROR: cannot specify both 'template' and 'steps'")
            return 1
        steps=[]
        tempVars={}
        varLen=0
        for var in config['template']['vars']:
            varDef=config['template']['vars'][var]
            if type(varDef)==str:
                varDef=list(range(*eval(varDef)))
            if type(varDef)!=list:
                print(f"Template variable {var} not properly defined")
                return 1
            if varLen and len(varDef)!=varLen:
                print(f"Template variable {var} does not have the proper number of entries")
                return 1
            varLen=len(varDef)
            tempVars[var]=varDef
        for idx in range(varLen):
            values={}
            for var in tempVars:
                values[var]=tempVars[var][idx]
            step={}
            for item in config['template']['step']:
                step[item]=config['template']['step'][item].format(**values)
            steps.append(step)
        config["steps"]=steps

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
                if not par in config['defaults']:
                    print(f"Did not find value for '{par}'")
                    return 1
                cfg[par]=config['defaults'][par]
        cfgs.append(cfg)

    runControl=RunControl(hostUrl,maxTransitionTime)
    if runControl.getState()['runOngoing']:
        print("Run already on-going - bailing out")
        return 1

    if "initCommand" in config:
        print("Running INIT command")
        rc=os.system(config["initCommand"])
        if rc:
            print("Failed in init command")
            return 1

    message(f"Starting a sequence run ({args[0]}) with {len(cfgs)-startStep+1} steps")
    for step in range(startStep-1,len(cfgs)):
        print(f"Running step {step+1}")
        cfg=cfgs[step]
        run=runner(cfg,runControl,seqnumber,step+1)
        rc=run.run()
        if rc:
            print("Successful run")
        else:
            message("Failed to start run from sequencer - please check")
            print("Failed to run, please check")
            print("To redo from this step run:")
            print(f"./sequencer.py -S {seqnumber} -s {step+1} {args[0]}")
            return 1
        if seqnumber==0:
            seqnumber=run.runnumber

    if "finalizeCommand" in config:
        print("Running FINALIZE command")
        rc=os.system(config["finalizeCommand"])
        if rc:
            print("Failed in finalize command")
            message("Failed at sequencer stop - please check")
            return 1

    message("Sequence run completed")
    return 0

if __name__ == "__main__":
   sys.exit(main(sys.argv[1:]))


