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
sys.path.append("../RunControl/daqControl")
from helpers import LogLevel, logAndEmit
import subprocess

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

def check_stopFlag() -> bool : 
    """
    Returns True if the stop flag  (redis key "stopSequencer") is set, False otherwise
    """
    if r.get("stopSequencer") is not None:
        return 1
    else :
        return 0

class runner:
    def __init__(self,config,runControl:RunControl,seqnumber,seqstep, socketio = None, logger = None):
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
        self.runnumber = 1000000000 #NOTE : For testing only ? None wouldnt't work for REDIS
        self.rc=runControl
        self.socketio = socketio
        self.logger = logger
    
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
            # checking if stop flag is raised
            if r.get("stopSequencer") is not None:
                print("Stopping current run for the current step")
                return True                 

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
            log_and_print(f"System is not shutdown - will not start run",LogLevel.ERROR,self.socketio, self.logger) 
            return False

        if self.preCommand:
            log_and_print(f"Running pre-command",LogLevel.INFO,self.socketio, self.logger) 
            try :
                run_commands(self.preCommand, self.socketio, self.logger)
            except RuntimeError :
                log_and_print(f"Failed to pre-command",LogLevel.ERROR,self.socketio, self.logger) 
                return False

        log_and_print(f"Initializing...",LogLevel.INFO,self.socketio, self.logger) 
        rc=self.initialize()
        if not rc:
            log_and_print(f"Failed to initialize",LogLevel.ERROR,self.socketio, self.logger) 
            return False
        self.sleep(1)
        log_and_print(f"Starting...",LogLevel.INFO,self.socketio, self.logger) 
        rc=self.start()
        if not rc:
            log_and_print(f"Failed to start run",LogLevel.INFO,self.socketio, self.logger) 
            # print("Failed to start run")
            return False
        log_and_print(f"Running...",LogLevel.INFO,self.socketio, self.logger) 
        self.sleep(5)
        rc=self.waitStop()
        if not rc:
            # print("Failed to reach run conditions")
            log_and_print(f"Failed to reach run conditions",LogLevel.ERROR,self.socketio, self.logger) 
            return False
        log_and_print(f"Stopping...",LogLevel.INFO,self.socketio, self.logger) 
        rc=self.stop()
        if not rc:
            # print("Failed to stop")
            log_and_print(f"Failed to stop",LogLevel.ERROR,self.socketio, self.logger) 
            return False
        self.sleep(5)
        log_and_print(f"Shutting down...",LogLevel.INFO,self.socketio, self.logger) 
        rc=self.shutdown()
        if not rc:
            log_and_print(f"Failed to shutdown - retry again in 10 seconds",LogLevel.WARNING,self.socketio, self.logger) 
            self.sleep(10)
            rc=self.shutdown()
            if not rc:
                log_and_print(f"Failed to shutdown - giving up",LogLevel.ERROR,self.socketio, self.logger) 
                return False
        for ii in range(5):
            if self.checkState()[0]=="DOWN": break
            self.sleep(1)
        if self.checkState()[0]!="DOWN":
            log_and_print(f"Failed to shutdown {self.checkState()}",LogLevel.ERROR,self.socketio, self.logger) 
            # print("Failed to shutdown",self.checkState())
            return False
        # self.sleep(5) # NOTE : for testing only
        if self.postCommand:
            log_and_print(f"Running post-command",LogLevel.INFO,self.socketio, self.logger) 
            try :
                run_commands(self.postCommand, self.socketio, self.logger)
            except RuntimeError :
                log_and_print(f"Failed to post-command",LogLevel.ERROR,self.socketio, self.logger) 
                return False
        return True

def usage():
    print("./sequencer.py [-S <seqNumber> -s <stepNumber> ] <configuration.json>")
    sys.exit(1)

def load_steps(configName:str) :  
    """
    Loads the configuration from the provided configuration filename and builds the steps from the config. 
    If there is a problem during the process, it will throw an RunTimeError -> best is to wrap the function with an try except block.  
    -> Requires the env variable DAQ_SEQUENCE_CONFIG_DIR. 
    
    Return
    ------
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
                if isinstance(config['template']['step'][item], list) : 
                    step[item] = [el.format(**values) for el in config['template']['step'][item]]
                else:
                    step[item] = config['template']['step'][item].format(**values)
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
    The function also stores permanently the last state of the Sequencer with the key : "lastSequencerState". In contrary, "sequencerState" is destroyed when sequencer finished. 
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
    r.hmset("lastSequencerState", updateDict) # the key stores the last state of the sequencer. 
    if socketio is not None : 
        socketio.emit("sequencerStateChange",updateDict, broadcast=True)
    return None

def log_and_print(msg:str, level:LogLevel, socketio=None, logger=None):
    if socketio is not None :
        logAndEmit(msg=msg.strip().replace("\n", " "), configName="SEQUENCER", level =level, socketio = socketio, logger = logger) 
    print(msg)
        
def run_commands(cmds:list, socketio = None, logger = None, stop_if_failure = True) : 
    failed_commands = []
    for cmd in cmds :
        log_and_print(f"Executing command : {cmd}", LogLevel.INFO, socketio, logger)
        process = subprocess.run(cmd,shell=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if process.returncode != 0 : 
            log_and_print(process.stdout, LogLevel.ERROR, socketio, logger)
            if stop_if_failure : 
                raise RuntimeError(f"Failed to run command : {cmd}")
            failed_commands.append(cmd)
        else :
            log_and_print(process.stdout, LogLevel.INFO, socketio, logger) # NOTE : if the script prints nothing, it will log a blank line 
   
    if len(failed_commands) != 0 :
        msg = "The following command(s) failed :\n"
        for cmd in failed_commands :
            msg+= f"//// {cmd}\n"
        log_and_print(msg, LogLevel.ERROR, socketio, logger)
        raise RuntimeError()
    
def finalize_and_clean( finalizeCommands:list, **args): 
    """
    if finalizeCommand is True : it will try to run the finalizeCommands. 
    **args : 
     - socketio
     - logger
    """
    if finalizeCommands is not None : 
        try :
            run_commands(finalizeCommands, socketio = args.get("socketio"),logger= args.get("logger"),stop_if_failure=False)
        
        except RuntimeError : 
            log_and_print(f"Failed in finalize command",LogLevel.ERROR,args.get("socketio"), args.get("logger")) 
            message("Failed in finalize Command after error - please check")
            log_and_print(f"Failed at sequencer stop - please check",LogLevel.ERROR, args.get("socketio"), args.get("logger")) 
    update_rcgui("",0,0,0,args.get("socketio"),end=True) 
        
    
    
def main(args, socketio=None, logger = None):
    """
    Runs the sequencer.
    The parameter socketio is used when the function is called directly from the RunControl, which represents the socketio instance. 
    startStep begins at 1.
    """
    r.delete("stopSequencer") # delete any previous stop Flags
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
    
    config, cfgs = load_steps(args[0])

    hostUrl=config["hostUrl"]
    maxTransitionTime=config["maxTransitionTime"]
    runControl=RunControl(hostUrl,maxTransitionTime)
    if runControl.getState()['runOngoing']:
        log_and_print("Run already on-going - bailing out",LogLevel.ERROR, socketio, logger)
        return 1
    update_rcgui(args[0], len(cfgs), 0, seqnumber,socketio=socketio)
    if "initCommand" in config:
        log_and_print("Running init command",LogLevel.INFO, socketio, logger)
        try :
            run_commands(config["initCommand"], socketio, logger)
        except RuntimeError :
            log_and_print("Failed in init command, giving up...",LogLevel.ERROR, socketio, logger)
            finalize_and_clean(config.get("finalizeCommand"), socketio= socketio, logger = logger)
            return 1

    message(f"Starting a sequence run ({args[0]}) with {len(cfgs)-startStep+1} steps")
    log_and_print(f"Starting a sequence run ({args[0]}) with {len(cfgs)-startStep+1} steps",LogLevel.INFO,  socketio, logger) 
    for step in range(startStep-1,len(cfgs)):

        # checking for stop flag
        if check_stopFlag():
            log_and_print(f"Stopping at step {step + 1}",LogLevel.INFO, socketio, logger)
            break

        update_rcgui(args[0],len(cfgs), step+1,seqnumber, socketio)
        print(f"Running step {step+1}")
        log_and_print(f"Running step {step+1}",LogLevel.INFO, socketio, logger) 
        cfg=cfgs[step]
        run=runner(cfg,runControl,seqnumber,step+1, socketio, logger)
        rc=run.run()
        if rc:
            log_and_print(f"Successful run",LogLevel.INFO, socketio, logger) 
        else:
            message("Failed to start run from sequencer - please check")
            log_and_print(f"Failed to start run from sequencer - please check ;\nTo redo from this step, start a new sequence with the following parameters : Sequence : {args[0]}, Sequence Number : {seqnumber}, Step : {step+1}. It is also possible to start a new run using the command : ./sequencer.py -S {seqnumber} -s {step+1} {args[0]}",LogLevel.ERROR, socketio, logger) 
            finalize_and_clean(config.get("finalizeCommand"), socketio= socketio, logger = logger)
            return 1
        if seqnumber==0:
            seqnumber=run.runnumber

    if "finalizeCommand" in config:
        log_and_print(f"Running finalize command", LogLevel.INFO,socketio, logger) 

        try :
            run_commands(config["finalizeCommand"], socketio, logger)
        except RuntimeError :
            log_and_print(f"Failed in finalize command",LogLevel.ERROR,socketio, logger) 
            message("Failed at sequencer stop - please check")
            log_and_print(f"Failed at sequencer stop - please check",LogLevel.ERROR, socketio, logger) 
            update_rcgui("",0,0,0,socketio= socketio,end=True) 
            return 1

    update_rcgui(args[0],len(cfgs),0, seqnumber, socketio, end=True)
    if r.get("stopSequencer") is not None:
        message("Sequence run completed after manual stop")
        log_and_print(f"Sequence run completed after manual stop",LogLevel.INFO, socketio, logger) 
    else : 
        message("Sequence run completed")
        log_and_print(f"Sequence run completed",LogLevel.INFO, socketio, logger) 
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))


