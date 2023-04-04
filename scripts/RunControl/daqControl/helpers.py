#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

from datetime import datetime
import redis
import json
from enum import Enum


class LogLevel(Enum) :
    """
    Avaiable types for logging events from the RCGUI.
    """ 
    INFO = "INFO"
    WARNING = "WARNING"
    ERROR = "ERROR"



r = redis.Redis(host='localhost', port=6379, db=0,charset="utf-8", decode_responses=True)

def detectorList(config):
    config = json.loads(config)
    detList=[]
    for comp in config["components"]:
        compType=comp['modules'][0]['type'] #FIXME: do not support modules
        if compType=="TriggerReceiver": 
            detList.append("TLB")
        elif compType=="DigitizerReceiver": 
            detList.append("DIG00")
        elif compType=="TrackerReceiver":
            boardID=comp['modules'][0]['settings']['BoardID']
            detList.append(f"TRB{boardID:02}")
    return detList


def getEventCounts():
  types=["Physics","Calibration","TLBMonitoring"]
  values={}
  for rateType in types:
    name="Events_sent_"+rateType
    eventbuilderModule = r.keys("eventbuilder*")[0]
    value=r.hget(eventbuilderModule,name)
    value = value.split(':')
    values[name]=int(value[1])
  return values


def logAndEmit(msg:str, configName:str, level :LogLevel, socketio, logger) : 
    """
    Logs the message in the file specified by the logger, while emitting an websocket event. 
    """
    now = datetime.now()
    strTemplate = f"[{configName}] {now.strftime('%d/%m/%Y, %H:%M:%S')} {level.value}: {msg}"
    if level == LogLevel.INFO :
        logger.info(strTemplate)
    elif level == LogLevel.WARNING :
       logger.warning(strTemplate)
    elif level == LogLevel.ERROR :
        logger.error(strTemplate)
    socketio.emit("logChng",strTemplate,broadcast=True)
