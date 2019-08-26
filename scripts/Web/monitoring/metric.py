"""
 Copyright (C) 2019 CERN
 
 DAQling is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 DAQling is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with DAQling. If not, see <http://www.gnu.org/licenses/>.
"""

__author__ = "Roland Sipos"
__credits__ = [""]
__version__ = "0.0.1"
__maintainer__ = "Roland Sipos"
__email__ = "roland.sipos@cern.ch"

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
from flask import jsonify
from flask import render_template
from flask_restful import Api, Resource, reqparse
from flask import Blueprint, render_template


#import queries
#import backend as db


metric_blueprint = Blueprint('metric',__name__, url_prefix='/monitoring',
      static_url_path='',
      static_folder='static',
      template_folder='templates')
#api = Api(metric)
parser = reqparse.RequestParser()
parser.add_argument('value')


r = redis.Redis(host='localhost', port=6379, db=0)
#r= redis.StrictRedis('localhost', 6379, db=0, charset="utf-8", decode_responses=True)
#r.delete('datarate', 'size')
#r.flushdb()

class Add(Resource):
  def post(self, metric):
    args = parser.parse_args()
    print(args)
    value = str(args['value'])
    r.rpush(metric, str(time.time()*1000)+':'+value)
#    r.expire(metric, 5);


@metric_blueprint.route("/graph")
def graph():
  metrics = [x.decode() for x in r.keys() if x.startswith(b"History")]
  #print(metrics)
  tabNames = []
  for metric in metrics:
    if(not metric.startswith("History")):
      metrics.remove(metric)
  for metric in metrics:
    tabNames.append(metric.split("_")[1])
  #print(metrics)
  #print("///////////tabnames:",tabNames)
  return render_template('graph2.html', metrics=metrics, tabNames=tabNames)


@metric_blueprint.route("/data/<string:metric>")
def data(metric):
  metric_array = []
  for x in r.lrange(metric, 0, -1):
    #print("x: ",x)
    value = x.decode().split(':')
    #print("value: ", value)
    metric_array.append([1000.*float(value[0]), float(value[1])])
  #return json.dumps(metric_array[-1000:])
  metric_array.reverse()
  #print(metric,metric_array)
  return json.dumps(metric_array)


@metric_blueprint.route("/lastMeas/<string:metric>")
def lastMeas(metric):
  value = r.lrange(metric, 0, 0)
  if(len(value) > 0):
    value = value[0].decode().split(':')
    #print(value)
    return json.dumps([1000.*float(value[0]), float(value[1])])
  return "0"

@metric_blueprint.route("/metrics")
def metric():
  return json.dumps([s.decode() for s in r.keys()])

#@metric_blueprint.route("/values")
#def listvalues():
#  metrics={}
#  for key in sorted(r.keys()):
#    if (key.startswith(b"History:")) or (key.startswith(b"Subset:")): continue
#    metrics[key.decode()]=str(r.hlen(key))
#  #print("metrics", metrics)
#  return render_template('overview.html', metrics=metrics)
  

@metric_blueprint.route("/info/<string:source>")
def values(source):
  values={}
  dbVals=r.hgetall(source)
  for key in sorted(dbVals):
    values[key.decode()]=(dbVals[key].split(b':')[1].decode(),time.ctime(float(dbVals[key].split(b':')[0])))
  #print("source", source)
  #print("values", values)
  if (source.startswith("frontendreceiver")):
    return render_template('tracker.html', source=source,values=values)
  return render_template('values.html', source=source,values=values)
 
@metric_blueprint.route("/info/<string:source>/dataUpdate")
def getValues(source):
  values=[]
  dbVals=r.hgetall(source)
  #print("dbVals in dataUpdate", dbVals)
  for key in sorted(dbVals):
    #print("keyyyyyyy:" ,key)
    values.append({  'key':key.decode(), 'value': dbVals[key].split(b':')[1].decode(), 'time': time.ctime(float(dbVals[key].split(b':')[0]))  })
  return jsonify({"values" : values})



@metric_blueprint.route("/info/updateStatus")
def getStatus():
  allStatus = []
  #print("r: ", r)
  for key in sorted(r.keys()):
    #print(key)
    if (key.startswith(b"History")) or (key.startswith(b"Subset")): continue
    source = key.decode()
    #print("source in stat: ", source)
    dbVals = r.hgetall(source)
    #dbVals = sorted(dbVals)
    #print("*********bdVals:", dbVals)
    if(b'Status' in sorted(dbVals)):
      val =  dbVals[b'Status'].split(b':')[1].decode()
    else: 
      val = -1
    #print("status number",val )
    allStatus.append({ "source": source, "statusVal": val })
  #print("allStatus", allStatus) 
  return jsonify({"allStatus" : allStatus})


@metric_blueprint.route("/info/<source>/<moduleId>")
def moduloTemplate(source, moduleId):
  return render_template("module.html", source=source, moduleId= moduleId)


@metric_blueprint.route("/info/<source>/<moduleId>/getData")
def getModuloData(source, moduleId): 
  values=[]
  hashName = "Subset:" + source + ":" + moduleId
  dbVals=r.hgetall(hashName)
  #print("dbVals in dataUpdate", dbVals)
  for key in sorted(dbVals):
    #print("keyyyyyyy:" ,key)
    values.append({  'key':key.decode(), 'value': dbVals[key].split(b':')[1].decode(), 'time': time.ctime(float(dbVals[key].split(b':')[0]))  })
  return jsonify({"values" : values})
#api.add_resource(Add, "/add/<string:metric>", methods=['POST'])
#api.add_resource(Metrics, "/metrics", methods=['GET'])

# As a testserver.
#app.run(host= '0.0.0.0', port=5000, debug=True)
# Normally spawned by gunicorn

