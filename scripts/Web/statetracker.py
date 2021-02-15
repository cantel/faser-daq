#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
import functools
import json
import os
import redis
import requests
import subprocess
import threading
import time
from os import environ as env

import helpers as h
from routes.metric import getBoardStatus,getEventCounts

r1=redis.Redis(host="localhost", port= 6379, db=2, charset="utf-8", decode_responses=True)

#cfg=json.load(open("../RunService/runservice.config"))
#run_user=cfg['user']
#run_pw=cfg["pw"]
run_user="FASER"
run_pw="HelloThere"

#FIXME: this should probably be in daqcontrol
import supervisor_wrapper
def removeProcess(p,group,logger):
    sd = supervisor_wrapper.supervisor_wrapper(p['host'], group)
    try:
        if sd.getProcessState(p['name'])['statename'] == 'RUNNING':
            try:
                sd.stopProcess(p['name'])
            except Exception as e:
                logger.error("Exception"+str(e)+": cannot stop process"+
                  p['name']+ "(probably already stopped)")
        sd.removeProcessFromGroup(p['name'])
    except Exception as e:
        logger.error("Exception"+str(e)+": Couldn't get process state")

def stateTracker(logger):
    pubsub=r1.pubsub()
    pubsub.subscribe("stateAction")
    oldState={}
    configName=""
    config={}
    oldStatus=[]
    status=[]
    daq=None
    overallState="DOWN"
    cmd=None
    while True:
        update=False
        runState=r1.hgetall("runningFile")
        runNumber=r1.get("runNumber")
        runStart=r1.get("runStart")
        if not oldState==runState:
            oldState=runState
            update=True
        if runState["fileName"]!=configName:
            configName=runState["fileName"]
            config=h.read(configName)
            #FIXME: add handling of failed reads here
            daq=None
        if not daq and type(config)==dict:
            daq = h.createDaqInstance(config)
        h.checkThreads()
        if daq:
            if cmd=="initialize":
                daqdir = env['DAQ_BUILD_DIR']
                exe = "bin/daqling"
                lib_path = 'LD_LIBRARY_PATH='+env['LD_LIBRARY_PATH']
                logfiles = daq.addComponents(config['components'],exe, daqdir, lib_path)
                for logfile in logfiles:
                    name = logfile[1][5:].split("-")[0]
                    r1.hset("log", name, logfile[0] + logfile[1])
                time.sleep(1.5)  #this should really check status of components
                detList=h.detectorList(config)
                logger.info("Included detector components: "+",".join(detList))
                r1.set("detList",json.dumps(detList))
                logger.info("Calling configure")
                h.spawnJoin(config['components'], daq.configureProcess)
                logger.info("Configure done")
            elif cmd=="start":
                h.spawnJoin(config['components'], functools.partial(daq.startProcess,run_num=runNumber))
            elif cmd=="pause":
                h.spawnJoin(config['components'], functools.partial(daq.customCommandProcess,command="disableTrigger",arg=""))
            elif cmd=="ECR":
                h.spawnJoin(config['components'], functools.partial(daq.customCommandProcess,command="ECR",arg=""))
                update=True
            elif cmd=="unpause":
                h.spawnJoin(config['components'], functools.partial(daq.customCommandProcess,command="enableTrigger",arg=""))
            elif cmd=="stop":
                logger.info("Calling Stop")
                h.spawnJoin(config['components'], daq.stopProcess)
                if int(runNumber)!=1000000000:
                    stopMsg=json.loads(m['data'][5:])
                    runinfo={}
                    runinfo["eventCounts"]=getEventCounts()
                    stopMsg["runinfo"]=runinfo
                    r1.set("runType",stopMsg["type"])
                    r = requests.post(f'http://faser-runnumber.web.cern.ch/AddRunInfo/{runNumber}',
                                      auth=(run_user,run_pw),
                                      json = stopMsg)
                    if r.status_code!=200:
                        logger.error("Failed to register end of run information: "+r.text)
                logger.info("Stop done")
            elif cmd=="shutdown":
                logger.info("Calling shutdown")
                h.spawnJoin(config['components'], daq.shutdownProcess)
                logger.info("Sleep for a bit")
                cnt=20
                while cnt:
                    time.sleep(0.1)
                    if h.checkThreads()==0: break
                    cnt-=1
                logger.info("Calling remove components")
                #could not use daqling loop as it stops on first exception
                for comp in config['components']:
                    try:
                        daq.removeProcess(comp['host'],comp['name'])
                    except Exception as e:
                        logger.error("Exception"+str(e)+": Got exception during removal of "+comp['name'])
                logger.info("Shutdown down")
                #h.spawnJoin(config['components'],  functools.partial(removeProcess,group=daq.group,logger=logger))
            status=[]
            overallState=None
            if not config['components']: overallState="DOWN"
            for comp in config['components']:       
                rawStatus = daq.getStatus(comp)
                timeout = None
                state = str(h.translateStatus(rawStatus, timeout))
                if not overallState:
                    overallState=state
                if state!=overallState:
                    if state!="RUN" and overallState!="RUN":
                        overallState="IN TRANSITION"
                appState=getBoardStatus(comp['name'])
                status.append({'name' : comp['name'] , 'state' : state,'infoState':appState})
            if status!=oldStatus:
                oldStatus=status
                logger.info("Status changed to: %s",overallState)
                r1.set("status",json.dumps({'allStatus' : status,
                                'runState' : runState,
                                'runNumber' : runNumber,
                                'runStart' : runStart,
                                'runType'  : r1.get("runType"),
                                'globalStatus': overallState,}))
                update=True
        if update:
               r1.publish("status","new")

        m=pubsub.get_message(timeout=0.5)
        cmd=""
        if m:
            if m['type']=='subscribe': continue
            cmds=m['data'].split()
            cmd=cmds[0]
            if cmd=="quit": break
            elif cmd=="updateConfig": configName=""
            elif cmd=="initialize":
                if overallState!="DOWN":
                    logger.warn("Tried to initialize in state: "+str(overallState))
                    cmd=""
                else:
                    r1.hset("runningFile", "fileName", cmds[1])
                    configName="" #force a re-read
            elif cmd=="start":
                if overallState!="READY":
                    logger.warn("Tried to start run in state: "+overallState)
                    cmd=""
                else:
                    detList=json.loads(r1.get("detList"))
                    subInfo=json.loads(m['data'][6:])
                    version=subprocess.check_output(["git","rev-parse","HEAD"]).decode("utf-8").strip()
                    runNumber=1000000000
                    runType="Unspecified"
                    try:
                        runType=subInfo['runtype']
                        r= requests.post('http://faser-runnumber.web.cern.ch/NewRunNumber',
                                         auth=(run_user,run_pw),
                                         json = {
                                             'version':    version,
                                             'type':       subInfo['runtype'],
                                             'username':       os.getenv("USER"),
                                             'startcomment':    subInfo['startcomment'],
                                             'detectors':  detList,
                                             'configName': configName,
                                             'configuration': config
                                         })
                        if r.status_code!=201:
                            logger.error("Failed to get run number: "+r.text)
                        else:
                            try:
                                runNumber=int(r.text)
                            except ValueError:
                                logger.error("Failed to get run number: "+r.text)
                    except requests.exceptions.ConnectionError:
                        logger.error("Could not connect to run service")
                    r1.set("runNumber",runNumber)
                    r1.set("runType",runType)
                    r1.set("runStart",time.time())
            elif cmd=="pause":
                if overallState!="RUN":
                    logger.warn("Tried to pause run in state: "+overallState)
                    cmd=""
            elif cmd=="ECR":
                if overallState!="PAUSED":
                    logger.warn("Tried to send ECR in state: "+overallState)
                    cmd=""
            elif cmd=="unpause":
                if overallState!="PAUSED":
                    logger.warn("Tried to restart run in state: "+overallState)
                    cmd=""
            elif cmd=="stop":
                if overallState!="PAUSED" and overallState!="RUN":
                    logger.warn("Tried to stop run in state: "+overallState)
                    cmd=""
            elif cmd=="shutdown":
                if overallState=="DOWN":
                    logger.warn("Tried to shutdown run in state: "+overallState)
                    cmd=""
            if cmd and cmd!="updateConfig" and cmd!="ECR":
                overallState="IN TRANSITION"
                r1.set("status",json.dumps({'allStatus' : status,
                                'runState' : runState,
                                'runNumber' : runNumber,
                                'runStart' : runStart,
                                'runType'  : r1.get("runType"),
                                'globalStatus': overallState,}))
                r1.publish("status","new")
                

def initialize(fileName):
    r1.publish("stateAction","initialize "+fileName)

def start(reqinfo):
    info={}
    info['startcomment']=reqinfo.get('startcomment',"")[:500]
    info['runtype']=reqinfo.get('runtype',"Test")[:100]
    r1.publish("stateAction","start "+json.dumps(info))

def pause():
    r1.publish("stateAction","pause")

def ecr():
    r1.publish("stateAction","ECR")

def unpause():
    r1.publish("stateAction","unpause")

def stop(reqinfo):
    info={}
    info['endcomment']=reqinfo.get('endcomment',"")[:500]
    info['type']=reqinfo.get('runtype',"")[:100]
    r1.publish("stateAction","stop "+json.dumps(info))

def shutdown():
    r1.publish("stateAction","shutdown")

def getSelectedFile():
    return r1.hgetall("runningFile")["fileName"]

def updateConfig():
    r1.publish("stateAction","updateConfig")

def refresh():
    r1.publish("status","new") # force update
    
class State:
    def __init__(self,logger):
        try:
            r1.ping()
        except:
            print("Redis database is not running - please start it")
            #FIXME: print instructions here
            sys.exit(1)
        if not r1.exists("runningFile"):    
            r1.hset("runningFile", "fileName", "current.json")
        else:
            runState=r1.hgetall("runningFile")
            logger.info("runState: %s",runState)
            name=r1.hget("runningFile","fileName")
            logger.info("Running with: %s",name)
            data=h.read(name)
            if not data:
                logger.warn("Can't find valid input data - resetting")
                r1.hset("runningFile", "fileName", "current.json")
        if not r1.exists("runNumber"):
            r1.set("runNumber",99)
            r1.set("runStart",0)


        self.tracker = threading.Thread(name="stateTracker",target=stateTracker,args=(logger,))
        self.tracker.start()

    def stop(self):
        r1.publish("stateAction","quit")
        self.tracker.join()

