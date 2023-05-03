#!/usr/bin/env python3
#
# Copyright (C) 2019-2023 CERN for the benefit of the FASER collaboration
#

# simple script to grab all recent histograms from Redis and dump as one json file

import gzip
import json
import redis
import sys
import time

r = redis.Redis(host='localhost', port=6379, db=1,charset="utf-8", decode_responses=True)

if (len(sys.argv)!=3):
    print("ERROR: incorrect number of arguments")
    sys.exit(1)

max_age=int(sys.argv[1])
file_name=sys.argv[2]

hists={}
now=time.time()
for app in r.keys():
    hists[app]={}
    for hist in r.hkeys(app):
        raw=r.hget(app,hist)
        ts=float(raw.split(":")[0])
        if now-ts>max_age: 
            continue
        data=json.loads(raw[raw.find("{"):])
        data["ts"]=ts
        hists[app][hist]=data
    if not hists[app]:
        del hists[app]

js=json.dumps(hists)
gz=gzip.compress(bytes(js,'utf8'))
open(file_name,"wb").write(gz)
sys.exit(0)
