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
from os import environ as env
import redis
sys.path.append("../RunControl")

from runcontrol import RunControl

mattermost_hook=""

r = redis.Redis(host='localhost', port=6379, db=3,charset="utf-8", decode_responses=True)

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
    def __init__(self,config,runControl:RunControl,seqnumber,seqstep, socketio = None):
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
        # self.runnumber = 100000000 #NOTE : For testing only
        self.rc=runControl
        self.socketio = socketio
    
    def sleep(self,seconds :int) -> None:
        """
        If the sequencer script is not used as a standalone script (without the rcgui), it will use the regular "time.sleep()" function,
        instead of the sleep function provied by the flask-socketio package.
        """
        if self.socketio is not None: self.socketio.sleep(seconds)
        else : time.sleep(seconds)
        
        
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
            self.sleep(5)

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
        self.sleep(1)
        rc=self.start()
        if not rc:
            print("Failed to start run")
            return False
        print("RUNNING")
        self.sleep(5)
        rc=self.waitStop()
        if not rc:
            print("Failed to reach run conditions")
            return False

        rc=self.stop()
        if not rc:
            print("Failed to stop")
            return False
        print("STOPPED")
        self.sleep(5)
        rc=self.shutdown()
        if not rc:
            print("Failed to shutdown - retry again in 10 seconds")
            self.sleep(10)
            rc=self.shutdown()
            if not rc:
                print("Failed to shutdown - giving up")
                return False
        print("SHUTDOWN")
        for ii in range(5):
            if self.checkState()[0]=="DOWN": break
            self.sleep(1)
        if self.checkState()[0]!="DOWN":
            print("Failed to shutdown",self.checkState())
            return False
        # self.sleep(5) # NOTE : for testing only
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

def load_steps(configName:str) :  
    """
    Loads the configuration from the provided configuration filename and builds the steps from the config. 
    If there is a problem during the process, it will throw an RunTimeError -> best is to wrap the function with an try except block.  
    -> Requires the env variable DAQ_SEQUENCE_CONFIG_DIR. 
    Return :
    ---
    - config :  dictionnary of the used config
    - cfgs : array of the differents steps of the sequence.  
    """
    
    with open(os.path.join(env["DAQ_SEQUENCE_CONFIG_DIR"],configName), "r") as fp: 
        config=json.load(fp)

    cfgs=[]
    if "template" in config:
        if "steps" in config:
            print("ERROR: cannot specify both 'template' and 'steps'")
            raise RuntimeError("Cannot specify both 'template' and 'steps'")
        steps=[]
        tempVars={}
        varLen=0
        for var in config['template']['vars']:
            varDef=config['template']['vars'][var]
            if type(varDef)==str:
                varDef=list(range(*eval(varDef)))
            if type(varDef)!=list:
                print(f"Template variable {var} not properly defined")
                raise RuntimeError(f"Template variable {var} not properly defined")
            if varLen and len(varDef)!=varLen:
                print(f"Template variable {var} does not have the proper number of entries")
                raise RuntimeError(f"Template variable {var} does not have the proper number of entries")
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
                    raise RuntimeError(f"Did not find value for '{par}'")
                cfg[par]=config['defaults'][par]
        cfgs.append(cfg)
    return config, cfgs


def update_rcgui( sequenceName:str, totalStepsNumber:int, stepNumber = 0, seqNumber = 0, socketio = None, end = False) -> None:
    """
    Emits "sequenceUpdate" event to RCGUI and update infos in REDIS.
    If 'end' argument is False, the function will just delete the entry in REDIS.
    """
    if end :
        r.delete("sequencerState")
        if socketio is not None : 
            socketio.emit("sequencerStateChange",{}, broadcast=True)
        return None

    updateDict  = {
        "sequenceName" : sequenceName,
        "seqNumber" : seqNumber,
        "stepNumber" : stepNumber,
        "totalStepsNumber" : totalStepsNumber,
    } 
    r.hmset("sequencerState", updateDict)
    if socketio is not None : 
        socketio.emit("sequencerStateChange",updateDict, broadcast=True)
    return None    


def main(args, socketio=None):
    """
    Runs the sequencer.
    The parameter socketio is used when the function is called directly from the RunControl, which represents the socketio instance. 
    startStep begins at 1.
    """
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
    
    #TODO: add try except to return 1 or 0 for the main function
    config, cfgs = load_steps(args[0])

    hostUrl=config["hostUrl"]
    maxTransitionTime=config["maxTransitionTime"]
    runControl=RunControl(hostUrl,maxTransitionTime)
    if runControl.getState()['runOngoing']:
        print("Run already on-going - bailing out")
        return 1

    update_rcgui(args[0], len(cfgs), 0, seqnumber,socketio=socketio)
    if "initCommand" in config:
        print("Running INIT command")
        rc=os.system(config["initCommand"])
        if rc:
            print("Failed in init command")
            return 1

    message(f"Starting a sequence run ({args[0]}) with {len(cfgs)-startStep+1} steps")
    for step in range(startStep-1,len(cfgs)):
        update_rcgui(args[0],len(cfgs), step+1,seqnumber, socketio)
        print(f"Running step {step+1}")
        cfg=cfgs[step]
        run=runner(cfg,runControl,seqnumber,step+1, socketio)
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

    update_rcgui(args[0],len(cfgs),0, seqnumber, socketio, end=True)
    message("Sequence run completed")
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))


