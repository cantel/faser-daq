#!/usr/bin/env python3

import json
import redis
import sys
import time
import zmq
import random
config=json.load(open(sys.argv[1]))
r = redis.Redis(host='localhost', port=6379, db=0,
                charset="utf-8", decode_responses=True)
#r= redis.StrictRedis('localhost', 6379, db=0,
#                     charset="utf-8", decode_responses=True)
r.flushdb()

context = zmq.Context()
poller = zmq.Poller()
nameMap={}
for comp in config["components"]:
  if not "settings" in comp: continue
  if not "stats_uri" in comp["settings"]: continue
  uri=comp["settings"]["stats_uri"]
  uri=uri.replace("*",comp["host"])
  name=comp["name"]
  print("Listening to %s (%s)" % (name,uri))
  socket = context.socket(zmq.SUB)
  socket.connect(uri)
  socket.setsockopt_string(zmq.SUBSCRIBE,"")
  nameMap[socket]=name
  poller.register(socket, zmq.POLLIN)
  
while True:
  try:
    socks = dict(poller.poll())
  except KeyboardInterrupt:
    break

  #temporary : simulating the frontEndReciever data

  for comp in config["components"]:
      if comp["name"].startswith("frontendreceiver"):
        for i in range(8):
          source = comp["name"]
          moduleName = "module" + str(i)
          valueName = "hits"
          val = str(time.time())+":"+str(random.randint(1,4000))
          name = moduleName + "_" + valueName
          r.hset(source, name, val)
          metric = source + ":" + name
          #r.lpush(metric, val)

          for j in range(12):
            valueName1 = "chip" + str(j)
            valueName2 = "hits"
            val = str(time.time())+":"+str(random.randint(1,4000))
            name = moduleName + "_" + valueName1 + "_" + valueName2
            r.hset(source, name, val)
            metric = source + ":" + name
            #r.lpush(metric, val)
  for sock in socks:
    source=nameMap[sock]
    print("Message from: "+source)
    data=sock.recv()
    print(data)
    name=data.split(b':')[0].decode()
    value=data.split()[1].decode()
    val=str(time.time())+":"+value
    r.hset(source,name,val)
    if "Rate" in name: # this should be configurable
      metric="History:"+source+"_"+name
      r.lpush(metric,val)
      r.ltrim(metric,0,9) # this should be configurable


