#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
import redis
import threading
import time
import zmq

metricsAddress="tcp://127.0.0.1:7000"   #for now the port is hardcoded to 7000 on localhost
historyLength=9                         # this should be configurable

r = redis.Redis(host='localhost', port=6379, db=0,
                charset="utf-8", decode_responses=True)
r1 = redis.Redis(host='localhost', port=6379, db=1,
                charset="utf-8", decode_responses=True)

def metricsHandler(stopEvent,logger):
    context = zmq.Context()

    sock = context.socket(zmq.SUB)
    sock.bind(metricsAddress)
    sock.setsockopt_string(zmq.SUBSCRIBE,"")

    while not stopEvent.isSet():
        events=sock.poll(timeout=1000)
        if not events: continue
        data=sock.recv()
        try:
            source,rest=data.decode().split("-",1)
            name,value=rest.split(': ')
            val=str(time.time())+":"+value
            if name.startswith("h_"): # FIXME Maybe can think of better way of identifying histograms
              r1.hset(source,name,val)
            else:
              r.hset(source,name,val)
            if "rate" in name: # this should be configurable
                metric="History:"+source+"_"+name
                r.lpush(metric,val)
                r.ltrim(metric,0,historyLength) 
        except ValueError:
            logger.error("Failed to decode metrics data: %s",data)



class Metrics:
    def __init__(self,logger):
        self.event = threading.Event()
        self.metrics = threading.Thread(name="metricsHandler",target=metricsHandler,args=(self.event,logger))
        self.metrics.start()

    def stop(self):
        self.event.set()
        self.metrics.join()
