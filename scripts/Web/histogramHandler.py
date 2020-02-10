import redis
import threading
import time
import zmq
import json

histoAddress="tcp://127.0.0.1:8001"   #for now the port is hardcoded to 8001 on localhost
historyLength=9                         # this should be configurable

r = redis.Redis(host='localhost', port=6379, db=0,
                charset="utf-8", decode_responses=True)

def histoHandler(stopEvent,logger):
    context = zmq.Context()

    sock = context.socket(zmq.SUB)
    sock.bind(histoAddress)
    sock.setsockopt_string(zmq.SUBSCRIBE,"")

    while not stopEvent.isSet():
        events=sock.poll(timeout=1000)
        if not events: continue
        data=sock.recv()
        try:
            histname,content=data.decode().split(":",1)
            source = "unknown"
            if "tracker" in histname:
              source = "tracker"
              #print("source = ", source)
            elif "tlb" in histname:
              source="tlb"
              #print("source = ", source)
            else:
              source="unknown"
            if source == "unknown": continue
            r.hset(source,histname,content)
            #if "h_tracker_payloadsize" in histname:
            #   print("data:",data)
            #   with open('data/h_tracker_payloadsize.json', 'w') as outfile:
            #     outfile.write(content)
        except ValueError:
            logger.error("Failed to decode histo data: %s",data)



class Histo:
    def __init__(self,logger):
        self.event = threading.Event()
        self.histo = threading.Thread(name="histoHandler",target=histoHandler,args=(self.event,logger))
        self.histo.start()

    def stop(self):
        self.event.set()
        self.histo.join()
