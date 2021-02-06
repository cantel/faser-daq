#!/usr/bin/env python3
#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

import json
import redis
import sys
import time
import zmq
import random
config=json.load(open(sys.argv[1]))
r = redis.Redis(host='localhost', port=6379, db=0,
                charset="utf-8", decode_responses=True)

r1=redis.Redis(host='localhost', port=6379, db=2, charset="utf-8", decode_responses=True)
#r= redis.StrictRedis('localhost', 6379, db=0,
#                     charset="utf-8", decode_responses=True)

r.flushdb()
r1.flushdb()

context = zmq.Context()
poller = zmq.Poller()
nameMap={}

#r1.hset("runningFile", "fileName", "current.json")
#r1.hset("runningFile", "isRunning", 0)

FakeData=False

for comp in config["components"]:
  if not "settings" in comp: continue
  if not "stats_uri" in comp["settings"]: continue
  uri=comp["settings"]["stats_uri"]
  uri=uri.replace("*",comp["host"])
  name=comp["name"]
  print("Listening to %s (%s)" % (name,uri))
  sock = context.socket(zmq.SUB)
#  socket.connect(uri)
  uri=uri.replace("localhost","127.0.0.1") # has to be numeric address for bind
  sock.bind(uri)
  sock.setsockopt_string(zmq.SUBSCRIBE,"")
  nameMap[sock]=name
  poller.register(sock, zmq.POLLIN)
  
while True:
  try:
    socks = dict(poller.poll())
  except KeyboardInterrupt:
    break

  #temporary : simulating the frontEndReciever data

  if FakeData:
    for comp in config["components"]:
      if comp["type"].startswith("FrontEndReceiver"):
        for i in range(1,9):
          #print("fake")
          source = comp["name"]
          moduleName = "Module" + str(i)
          valueName = "hits"
          val = str(time.time())+":"+str(random.randint(1,20))
          name = moduleName + "_" + valueName
          r.hset(source, name, val)
          r.hset("Subset:"+ source + ":" + moduleName, valueName, val)

          valueName = "errors"
          val = str(time.time())+":"+str(random.randint(1,10))
          name = moduleName + "_" + valueName
          r.hset(source, name, val)
          r.hset("Subset:"+ source + ":" + moduleName, valueName, val)
          #metric = source + ":" + name
          #r.lpush(metric, val)

          for j in range(1,13):
            valueName1 = "Chip" + str(j)
            valueName2 = "hits"
            val = str(time.time())+":"+str(random.randint(1,20))
            name = valueName1 + "_" + valueName2
            r.hset("Subset:" + source + ":" + moduleName, name, val)


            valueName2 = "errors"
            val = str(time.time())+":"+str(random.randint(1,10))
            name = valueName1 + "_" + valueName2
            r.hset("Subset:" + source + ":" + moduleName, name, val)
           # metric = source + ":" + name
            #r.lpush(metric, val)
  for sock in socks:
    source=nameMap[sock]
    print("Message from: "+source)
    data=sock.recv()
    print(data)
    
    name,value=data.split(b': ')
    name=name.decode()
    value=value.decode()
    val=str(time.time())+":"+value
    r.hset(source,name,val)
    if "Rate" in name: # this should be configurable
      metric="History:"+source+"_"+name
      r.lpush(metric,val)
      r.ltrim(metric,0,9) # this should be configurable


