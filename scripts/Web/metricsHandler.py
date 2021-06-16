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
        mapping={}
        for line in data.decode().split('\n'):
            try:
                #FIXME: should do smarter decoding and copy of data to Redis for local copy
                source,rest=line.split("-",1)
                if rest.startswith("h_"): # FIXME Maybe can think of better way of identifying histograms
                    name,value=rest.split(': ',1)
                    val=str(time.time())+":"+value
                    r1.hset(source,name,val)
                else:
                    name,value,timestamp=rest.split(' ')
                    value=value.split('=')[1]
                    val=str(time.time())+":"+value
                    r.hset(source,name,val)
                if "rate" in name: # this should be configurable
                    metric="History:"+source+"_"+name
                    r.lpush(metric,val)
                    r.ltrim(metric,0,historyLength) 
            except ValueError:
                logger.error("Failed to decode metrics data: %s in %s",data,line)



class Metrics:
    def __init__(self,logger):
        self.event = threading.Event()
        self.metrics = threading.Thread(name="metricsHandler",target=metricsHandler,args=(self.event,logger))
        self.metrics.start()

    def stop(self):
        self.event.set()
        self.metrics.join()
