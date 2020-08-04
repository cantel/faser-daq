from flask import render_template, url_for, flash, redirect, request, Response, jsonify, session
from flaskDashboard import app
import plotly
import plotly.graph_objs as go
import pandas as pd
import numpy as np
import json
import redis
from plotly.subplots import make_subplots
import plotly.graph_objects as go
import time
import requests

def getselectedModuleInfoFUNCTION(module):
  data = r.hgetall(module)
  keys = []
  for key in data.keys():
    if data[key].find('{') == -1:
      continue
    else:
      keys.append(key)
  packet = {'name': module, 'info': keys}
  return packet

def getcurrentModulesFUNCTION():
  database_modules = r.scan() ##returns the number of the database and the modules in it
  modules = [] 
# Add only modules with hash data
  for module in database_modules[1]:
    try: 
      r.hgetall(module)
    except redis.exceptions.ResponseError:
      print(f"{module} doesn't contain hash data")
      continue
    modules.append(module)
  packet = {"data": modules}
  return packet

plotly_types = {'num_fixedwidth': "bar", '2d_num_fixedwidth': "histogram2d"}
r = redis.Redis(host='localhost', port=6379, db=0,charset="utf-8", decode_responses=True)

@app.route("/", methods=["GET", "POST"])
def home():
  if not request.form.get('module_select'):
    print("Doing the busy stuff")
    dataModules = getcurrentModulesFUNCTION()
    session['modules'] = dataModules
    return render_template('home.html', modules = dataModules["data"])

  elif request.method == 'POST':
    selectedModule = request.form.get('module_select')
    return redirect(url_for('monitormodule', module = selectedModule))


@app.route('/monitor/<string:module>', methods=["GET", "POST"])
def monitormodule(module):
  moduleData = getselectedModuleInfoFUNCTION(module)
  modules = session.get('modules')
  return render_template('module.html', modules = modules, selected_module = moduleData)

  
@app.route("/<string:sourceid>/<string:nameshisto>")
def lastHistogram(sourceid, nameshisto):
  def getHistogram():
    source = sourceid
    histnames = nameshisto.split("-")
    # print(histnames)
    while True:
      packet = {}
      for histname in histnames :
        histobj=r.hget(source,histname) #lrange: one value: the first in the list.
        if histobj != None:
          timestamp = histobj[:histobj.find(':')]
          # print(timestamp)
          histobj = json.loads(histobj[histobj.find("{"):])

          # print("histobj = ",histobj)
      
          xaxis= dict(title = dict(text=histobj["xlabel"]))
          yaxis= dict(title = dict(text=histobj["ylabel"]))

          layout = dict( 
            title = dict(text=histobj['name']),
            width = 1024,
            height = 600,
            xaxis = xaxis,
            yaxis = yaxis
          )
          data =[]
          hist_type = histobj["type"]
          if "categories" in hist_type:
            xarray= histobj["categories"]
            yarray = histobj["yvalues"]
            data = [dict(x=xarray, y=yarray, type="bar")]

          elif "2d" in hist_type:
            zarray = histobj["zvalues"]
            xmin = float(histobj["xmin"])
            xmax = float(histobj["xmax"])
            xbins = int(histobj["xbins"])
            xstep = (xmax-xmin)/float(xbins)
            ymin = float(histobj["ymin"])
            ymax = float(histobj["ymax"])
            ybins = int(histobj["ybins"])
            ystep = (ymax-ymin)/float(ybins)
            data = [dict(xbins=dict(start=xmin, end=xmax,size=xstep), ybins=dict(start=ymin, end=ymax,size=ystep), z=zarray[106:], type="histogram2d")]
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
          packet[histname] = {'figure': fig, 'time': float(timestamp)}


                #print(f"data from lastHistogram:{json.dumps(fig)}")
      yield f"data:{json.dumps(packet)}\n\n"
      time.sleep(10)
  return Response(getHistogram(), mimetype='text/event-stream')
