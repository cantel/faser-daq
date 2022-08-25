import redis
import json

r = redis.Redis(host='localhost', port=6379, db=0,charset="utf-8", decode_responses=True)


def detectorList(config):
    config = json.loads(config)
    detList=[]
    for comp in config["components"]:
        # print(comp)
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
