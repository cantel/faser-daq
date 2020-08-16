from flask import render_template, url_for, flash, redirect, request, Response, jsonify
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
import os 

r = redis.Redis(host='localhost', port=6379, db=0,charset="utf-8", decode_responses=True)
r5 = redis.Redis(host='localhost', port=6379, db=5,charset="utf-8", decode_responses=True)


#pas fini
@app.route("/getIDs",methods=["GET"])
def getIDs():
  modules= getModules().get_json() 
  keys = []
  for module in modules:
    histnames = r.hkeys(module)
    for histname in histnames:
      if "h_" in histname:
        keys.append(f"{module}-{histname[2:]}")
  return jsonify(keys)

@app.route("/storeDefaultTagsAndIDs",methods=["GET"] )
def storeDefaultTagsAndIDs():
  ids = getIDs().get_json()
  for ID in ids:
    module,histname = ID.split("-")
    r5.sadd(f"tag:{module}",ID)
    r5.sadd(f"tag:{histname}",ID)
    r5.sadd(f"id:{ID}", module, histname)
  return "true"

@app.route("/getAllTags", methods = ["GET"])
def getAllTags():
  tags = r5.keys("tag*")
  tags = [s.replace("tag:","") for s in tags]
  return jsonify(tags)

@app.route("/getAllIDs")
def getAllIDs():
  ids = r5.keys("id:*")
  ids = [s.replace("id:","") for s in ids]
  return jsonify(ids)


def getTagsByID(ids):
  packet = {}
  for ID in ids:
    module, histname = ID.split("-")
    packet[ID] = list(r5.smembers(f"id:{ID}"))
    packet[ID].pop(packet[ID].index(module))
    packet[ID].pop(packet[ID].index(histname))
  return packet

@app.route("/getModules", methods= ["GET"])
def getModules():
  modules = r.keys("*monitor*")
  return jsonify(modules)

@app.route("/saveDB")
def saveDB():
  # r5.saveBG ## vérifier la commande 
  return "true"


@app.route("/addTag", methods=["GET"])
def addTag():
  tag_to_add = request.args.get("tag")
  ID, tag = tag_to_add.split(",")
  r5.sadd(f"id:{ID}", tag )
  r5.sadd(f"tag:{ tag }", ID)
  return "true"


@app.route("/removeTag", methods=["GET"])
def removeTag():
  tag_to_remove = request.args.get("tag")
  ID,tag = tag_to_remove.split(",")
  r5.srem(f"id:{ID}",tag)
  r5.srem(f"tag:{tag}",ID)
  return "true"

@app.route("/renameTag", methods=["GET"])
def renameTag():
  tag = request.ars.get("tag")
  ID,oldname,newname = tag.split(",")
  r5.srem(f"id:{ID}", oldname)
  r5.sadd(f"id:{ID}", newname)
  r5.srem(f"tag:{oldname}",ID)
  r5.sadd(f"tag:{ newname }", ID)

  return "true"

# pas fini
@app.route("/update")
def update():
  # update modules et tags si besoin?
  pass
  #return redirect?

@app.route("/home")
def home():
  modules = getModules().get_json()
  return render_template('home.html')

@app.route('/')
def load():
  print("Doing the busy stuff")
  storeDefaultTagsAndIDs() ## put the basic tags module and histname 
  # récupérer sauvegarde si elle existe
  return redirect(url_for('home'))

@app.route("/source/<path:path>")
def lastHistogram(path):
  def getHistogram():
    while True:
      packet = {}
      ids = path.split("/")
      for ID in ids:
        source,histname = ID.split("-")
        histobj=r.hget(source,histname) #lrange: one value: the first in the list.
        if histobj != None:
          timestamp = histobj[:histobj.find(':')]
          # print(timestamp)
          histobj = json.loads(histobj[histobj.find("{"):])

          # print("histobj = ",histobj)
      
          xaxis= dict(title = dict(text=histobj["xlabel"]))
          yaxis= dict(title = dict(text=histobj["ylabel"]))

          layout = dict( 
            # title = dict(text=histobj['name']),
            autosize = False,
            uirevision = True,
            # width = 1024,
            # height = 600,
            xaxis = xaxis,
            yaxis = yaxis,
            margin=dict(l=50,r=50,b=50,t=50,pad=4)
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
            xarray = [ xmin+xstep*xbin for xbin in range(xbins) ]
            ymin = float(histobj["ymin"])
            ymax = float(histobj["ymax"])
            ybins = int(histobj["ybins"])
            ystep = (ymax-ymin)/float(ybins)
            yarray = [ ymin+ystep*ybin for ybin in range(ybins) ]
            # print("xbins:", xbins)
            # print("xstep:", xstep)
            # print("ybins:", ybins)
            # print("ystep:", ystep)
            # print("xarray", len(xarray))
            # print("yarray", len(xarray))
            # print("\n \n \n ")
            
            # data = [dict(xbins=dict(start=xmin, end=xmax,size=xstep), ybins=dict(start=ymin, end=ymax,size=ystep), z=zarray[106:], type="heatmap")]
            # data = [dict(x0 = xmin, dx = xstep, y0 = ymin, dy = ystep, z = zarray, type="heatmap")]
            data = [dict(x = xarray, y = yarray, z = zarray, type=" heatmap")]
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
          fig = dict(data=data, layout=layout, config = {"responsive": True, 'displayModeBar': True} )
          packet[ID] = {'figure': fig, 'time': float(timestamp)}


                #print(f"data from lastHistogram:{json.dumps(fig)}")
      yield f"data:{json.dumps(packet)}\n\n"
      time.sleep(30)
  return Response(getHistogram(), mimetype='text/event-stream')

@app.route("/monitor", methods=["GET", "POST"])
def monitor():
  if request.method == "GET":
    if request.args.get("selected_module"):
      selected_module = request.args.get("selected_module")
      ids = r5.smembers(f"tag:{selected_module}")
      modules=getModules().get_json()
      tagsbyID = getTagsByID(ids)
      return render_template("monitor.html", selected_module = selected_module, tagsByID = tagsbyID, ids = ids)
    elif request.args.getlist("selected_tags"):
      selected_tags = request.args.getlist("selected_tags")
      selected_tags = [ f"tag:{s}" for s in selected_tags]
      ids = r5.sunion(selected_tags)
      tagsbyID = getTagsByID(ids)
      return render_template("monitor.html", tagsByID = tagsbyID, ids = ids)


@app.context_processor
def context_processor():
  modules=getModules().get_json()
  tags = getAllTags().get_json()
  return dict(modules = modules, tags = tags)