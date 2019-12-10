import functools
import json
import redis
import threading
import time

import helpers as h
from routes.metric import getBoardStatus

r1=redis.Redis(host="localhost", port= 6379, db=2, charset="utf-8", decode_responses=True)

def stateTracker(logger):
    pubsub=r1.pubsub()
    pubsub.subscribe("stateAction")
    oldState={}
    configName=""
    config={}
    oldStatus=[]
    daq=None
    overallState=None
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
        if daq:
            if cmd=="initialize":
                logfiles = daq.addProcesses(config['components'])
                for logfile in logfiles:
                    name = logfile[1][5:].split("-")[0]
                    r1.hset("log", name, logfile[0] + logfile[1])
                h.spawnJoin(config['components'], daq.configureProcess)
            elif cmd=="start":
                h.spawnJoin(config['components'], functools.partial(daq.startProcess,arg=runNumber))
            elif cmd=="pause":
                h.spawnJoin(config['components'], functools.partial(daq.customCommandProcess,command="disableTrigger"))
            elif cmd=="ECR":
                h.spawnJoin(config['components'], functools.partial(daq.customCommandProcess,command="ECR"))
            elif cmd=="unpause":
                h.spawnJoin(config['components'], functools.partial(daq.customCommandProcess,command="enableTrigger"))
            elif cmd=="stop":
                h.spawnJoin(config['components'], daq.stopProcess)
            elif cmd=="shutdown":
                h.spawnJoin(config['components'], daq.shutdownProcess)
                daq.removeProcesses(config['components'])
            status=[]
            overallState=None
            if not config['components']: overallState="DOWN"
            for comp in config['components']:       
                rawStatus, timeout = daq.getStatus(comp)
                state = str(h.translateStatus(rawStatus, timeout))
                if not overallState:
                    overallState=state
                if state!=overallState:
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
                    logger.warn("Tried to initialize in state: "+overallState)
                    cmd=""
                else:
                    r1.hset("runningFile", "fileName", cmds[1])
                    configName="" #force a re-read
            elif cmd=="start":
                if overallState!="READY":
                    logger.warn("Tried to start run in state: "+overallState)
                    cmd=""
                else:
                    runNumber=int(runNumber)+1
                    r1.set("runNumber",runNumber)
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
            if cmd and cmd!="updateConfig":
                overallState="IN TRANSITION"
                r1.set("status",json.dumps({'allStatus' : status,
                                'runState' : runState,
                                'runNumber' : runNumber,
                                'runStart' : runStart,
                                'globalStatus': overallState,}))
                r1.publish("status","new")
                

def initialize(fileName):
    r1.publish("stateAction","initialize "+fileName)

def start():
    r1.publish("stateAction","start")

def pause():
    r1.publish("stateAction","pause")

def ecr():
    r1.publish("stateAction","ECR")

def unpause():
    r1.publish("stateAction","unpause")

def stop():
    r1.publish("stateAction","stop")

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
        if not r1.exists("runNumber"):
            r1.set("runNumber",99)
            r1.set("runStart",0)


        self.tracker = threading.Thread(name="stateTracker",target=stateTracker,args=(logger,))
        self.tracker.start()

    def stop(self):
        r1.publish("stateAction","quit")
        self.tracker.join()

