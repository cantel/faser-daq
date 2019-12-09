import json
import threading

import helpers as h
from routes.metric import getBoardStatus

def stateTracker(r1,logger):
    pubsub=r1.pubsub()
    pubsub.subscribe("stateAction")
    oldState={}
    configName=""
    config={}
    oldStatus=[]
    daq=None
    overallState=None
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
        if m:
            cmd=m['data']
            if cmd=="quit": break
            if cmd=="updateConfig": configName=""
#            print(m)
#            print(cmd)

class State:
    def __init__(self,redisDB,logger):
        self.redis=redisDB
        self.tracker = threading.Thread(name="stateTracker",target=stateTracker,args=(self.redis,logger))
        self.tracker.start()

    def stop(self):
        self.redis.publish("stateAction","quit")
        self.tracker.join()

