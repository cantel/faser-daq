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


histogram_blueprint = Blueprint('histogram',__name__, url_prefix='/histogramming',
      static_url_path='',
      static_folder='static',
      template_folder='templates')

r = redis.Redis(host="localhost", port= 6379, db=0, charset="utf-8", decode_responses=True)


@histogram_blueprint.route("<string:sourceid>/<string:histname>/lastValues/<float:sleeptime>")
def lastHistogram(sourceid, histname,sleeptime):
  def getHistogram():
    source = sourceid
    histName = histname
    while True:
      histobj=r.hget(source,histName) #lrange: one value: the first in the list.
      if histobj != None:
        histobj = json.loads(histobj)
        print("histobj = ",histobj)
            
        xaxis= dict(
          title = dict(text=histobj["xlabel"])
        )
        yaxis= dict(
          title = dict(text=histobj["ylabel"])
        )
 
        layout = dict( 
          title = dict(title=dict(text=histobj['name'])),
          width = 1024,
          height = 600,
          xaxis = xaxis,
          yaxis = yaxis
        )

        hist_type = histobj["type"]
        data=[]
    
        if "categories" in hist_type:
           xarray= histobj["categories"]
           yarray = histobj["yvalues"]
           data = [dict(x=xarray, y=yarray, type="bar")]
        #elif "2d" in hist_type:
        #  zarray = histobj["zvalues"]
        #  xmin = float(histobj["xmin"])
        #  xmax = float(histobj["xmax"])
        #  xbins = int(histobj["xbins"])
        #  xstep = (xmax-xmin)/float(xbins)
        #  ymin = float(histobj["ymin"])
        #  ymax = float(histobj["ymax"])
        #  ybins = int(histobj["ybins"])
        #  ystep = (ymax-ymin)/float(ybins)
        #  data = [dict(xbins=dict(start=xmin, end=xmax,size=xstep), ybins=dict(start=ymin, end=ymax,size=ystep), z=zarray[106:], type="histogram2d")]
        #  print("xbins = ", xbins)
        #  print("ybins = ", ybins)
        #  print("len(zvalues) = ", len(zarray))
        else:
          xmin = float(histobj["xmin"])
          xmax = float(histobj["xmax"])
          xbins = int(histobj["xbins"])
          step = (xmax-xmin)/float(xbins)
          xarray = [ xmin+step*xbin for xbin in range(xbins) ]
          yarray = histobj["yvalues"]
          data = [dict(x0=xmin, dx=step, y=yarray, type="bar")]

        fig = dict(data=data, layout=layout)
        #print(f"data from lastHistogram:{json.dumps(fig)}")
        yield f"data:{json.dumps(fig)}\n\n"
      time.sleep(sleeptime)
  return Response(getHistogram(), mimetype='text/event-stream')

