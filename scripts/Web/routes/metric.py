#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

__author__ = "Elham Amin Mansour"
__credits__ = [""]
__version__ = "0.0.1"
__maintainer__ = "Brian Petersen"
__email__ = "Brian.Petersen@cern.ch"

import ctypes
from multiprocessing.pool import ThreadPool
from collections import deque
import threading
from datetime import datetime, timedelta
import time
import json
import redis

import flask
from flask import Flask
from flask import Response
from flask import request
from flask import jsonify
from flask import render_template
from flask_restful import Api, Resource, reqparse
from flask import Blueprint, render_template


rateSource="eventbuilder01" #FIXME: this is hardcoded in several places

#import queries
#import backend as db


metric_blueprint = Blueprint('metric',__name__, url_prefix='/monitoring',
      static_url_path='',
      static_folder='static',
      template_folder='templates')

r = redis.Redis(host='localhost', port=6379, db=0)

@metric_blueprint.route("/metrics")
def metric():
  return json.dumps([s.decode() for s in r.keys()])

@metric_blueprint.route("/data/<string:metric>")  # used to get rate history data
def data(metric):
  metric_array = []
  for x in r.lrange(metric, 0, -1):
    value = x.decode().split(':')
    metric_array.append([1000.*float(value[0]), float(value[1])])
  metric_array.reverse()
  return json.dumps(metric_array)

@metric_blueprint.route("/lastRates/<float:sleeptime>")
def lastRates(sleeptime):
  types=["Physics","Calibration","TLBMonitoring"]
  def getRates():
    prevValues={}
    while True:
      values={}
      for valType in ["Event_rate_","Events_received_"]:
        for rateType in types:
          name=valType+rateType
          value=r.hget(rateSource,name)
          if value and value!=prevValues.get(name,""):
            prevValues[name]=value
            value = value.decode().split(':')
            values[valType+rateType]=[1000.*float(value[0]),float(value[1])]
      if values:
        yield f"data:{json.dumps(values)}\n\n"
      time.sleep(sleeptime)
  return Response(getRates(), mimetype='text/event-stream')
    

@metric_blueprint.route("/lastMeas/<string:metric>")
def lastMeas(metric):
  value = r.lrange(metric, 0, 0)
  if(len(value) > 0):
    value = value[0].decode().split(':')
    return json.dumps([1000.*float(value[0]), float(value[1])])
  return "0"

@metric_blueprint.route("/info/<boardType>/<string:source>")
def values(boardType, source):
  module=request.args.get('module','')
  return render_template('values.html',
                         source=source,
                         content="custom/"+boardType+".html",
                         boardType=boardType,
                         module=module
  )
 
@metric_blueprint.route("/infoTable/<string:source>")
def sendValues(source):
  def getValues(source):
    oldValues=None
    while True:
      values=[]
      dbVals=r.hgetall(source)
      if dbVals==oldValues: continue
      oldValues=dbVals
      now=time.time()
      for key in sorted(dbVals):
        vTime=float(dbVals[key].split(b':')[0])
        if now-vTime>3600: continue  #skip old measurements
        values.append({
          'key':key.decode(),
          'value': dbVals[key].split(b':')[1].decode(),
          'time': time.ctime(vTime)
        })
      yield f"data:{json.dumps(values)}\n\n"
      time.sleep(1)
  return Response(getValues(source), mimetype='text/event-stream')


def getBoardStatus(boardname):
  state=r.hget(boardname,'Status')
  if not state: return -1
  return int(state.decode().split(":")[1])

def getEventCounts():
  types=["Physics","Calibration","TLBMonitoring"]
  values={}
  for rateType in types:
    name="Events_sent_"+rateType
    value=r.hget(rateSource,name)
    value = value.decode().split(':')
    values[name]=int(value[1])
  return values
