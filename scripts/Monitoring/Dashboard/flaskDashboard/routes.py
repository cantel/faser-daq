#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
# @mainpage Histogram Monitoring server

# @file routes.py

# @brief Defines all the flask routes and the api endpoints

import json
from flask.helpers import flash
import redis
from flask import render_template
from flask import url_for, redirect, request, Response, jsonify
from flaskDashboard import app
import numpy as np
import atexit
import itertools
from apscheduler.schedulers.background import BackgroundScheduler
from flaskDashboard import interfacePlotly



## Redis database where the last histograms are published.
r = redis.Redis(
    host="localhost", port=6379, db=1, charset="utf-8", decode_responses=True
)
## Redis database where the current run informations are stored 
r2 = redis.Redis(
    host="localhost", port=6379, db=2, charset="utf-8", decode_responses=True
)
## Redis database where the tags are stored.
r5 = redis.Redis(
    host="localhost", port=6379, db=5, charset="utf-8", decode_responses=True
)

## Redis database where the previous histograms are stored.
r7 = redis.Redis(
    host="localhost", port=6379, db=7, charset="utf-8", decode_responses=True
)

def histograms_save():
    """! Defines the job that will be executed every 60 seconds
    It saves the current histograms in redis database 1 to redis database 6
    """
    with app.app_context():
        IDs = getAllIDs().get_json()  # we get all the IDs
    for ID in IDs:
        module, histname = ID.split("-")
        histobj = r.hget(module, f"h_{histname}")
        if histobj != None:
            runNumber = r2.get("runNumber")
            data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
            config = {"filename":f"{ID}+{timestamp}"}  
            figure = dict(data=data, layout=layout, config=config)
            hist = dict(timestamp=timestamp, figure=figure)
            if r7.hlen(ID) >= 30:
                key_to_remove = sorted(r7.hkeys(ID))[0]
                r7.hdel(ID, key_to_remove)
            r7.hset(ID, f"{timestamp}R{runNumber}", json.dumps(hist))

            
def old_histogram_save():
    """! Defines the job that will be executed every 30 minutes.
    It saves the current histograms in the redis database 1 in the redis database 7
    """
    with app.app_context():
        IDs = getAllIDs().get_json()
    for ID in IDs:
        module, histname = ID.split("-")
        histobj = r.hget(module, f"h_{histname}")
        if histobj != None:
            runNumber = r2.get("runNumber")
            data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
            config = {"filename":f"{ID}+{timestamp}"} 
            figure = dict(data=data, layout=layout, config=config)
            hist = dict(timestamp=timestamp, figure=figure)
            if r7.hlen(f"old:{ID}")>=168 : # 24 (hours) * 7 (days) 
                key_to_remove = sorted(r7.hkeys(f"old:{ID})"))[0]
                r7.hdel(f"old:{ID}", key_to_remove)
            r7.hset(f"old:{ID}", f"old:{timestamp}R{runNumber}", json.dumps(hist)) 

# setting up the background jobs for storing old histograms
scheduler = BackgroundScheduler()
scheduler.add_job(func=histograms_save, trigger="interval", seconds=60)
scheduler.add_job(func=old_histogram_save, trigger="interval", seconds=3600)
scheduler.start()
atexit.register(lambda: scheduler.shutdown())


@app.route("/getIDs", methods=["GET"])
def getIDs():
    """! Get the ID of all histograms sorted alphabetically.

    @return JSON: list of all IDs from redis database 1.
    """
    modules = getModules().get_json()
    keys = []
    for module in modules:
        histnames = r.hkeys(module)
        for histname in histnames:
            if "h_" in histname:
                keys.append(f"{module}-{histname[2:]}")

    keys = sorted(keys)
    return jsonify(keys)


@app.route("/storeDefaultTagsAndIDs", methods=["GET"])
def storeDefaultTagsAndIDs():
    """! Stores in the Redis database the default tags for all the histograms.

    @return The string "true".
    """
    ids = getIDs().get_json()
    for ID in ids:
        module, histname = ID.split("-")
        r5.sadd(f"tag:{module}", ID)
        r5.sadd(f"tag:{histname}", ID)
        r5.sadd(f"id:{ID}", module, histname)
    return "true"



@app.route("/getAllIDs")
def getAllIDs():
    """! Gets the IDs stored in the redis database 5. 

    @return JSON: all IDs 
    """
    ids = r5.keys("id:*")
    ids = [s.replace("id:", "") for s in ids]
    ids.sort()
    return jsonify(ids)


@app.route("/getModules", methods=["GET"])
def getModules():
    """! Gets the keys from the redis database containing the "monitor" expression and whose data type is hash.

    @return JSON: all valid modules.
    """
    modules = r.keys("*monitor*")
    modules = [module for module in modules if r.type(module) == "hash"]

    # modules.sort()
    return jsonify(modules)


@app.route("/")
@app.route("/vue_home", methods=["GET"])
def vue_home():
    """! Renders the vue_home.html"""
    storeDefaultTagsAndIDs()
    return render_template("vue_home.html")


@app.route("/getModulesAndTags", methods=["GET"])
def get_modules_and_tags():
    """! Gets all module names used for monitoring (contain the keyword "monitor") and all the tags associated to the modules.
    @return JSON : JSON response with all modules name and tags
    """
    packet = {}
    modules = r.keys("*monitor*")
    tags = r5.keys("tag*")
    packet["modules"] = [module for module in modules if r.type(module) == "hash"]
    packet["tags"] = [s.replace("tag:", "") for s in tags]
    return jsonify(packet)


@app.route("/IDs_from_tags", methods=["POST"])
def IDs_from_tags():
    """! Gets the IDs of the histograms associated with the given tags.
    @return list: list of IDs.
    """
    request_data = request.get_json()
    tags = request_data["tags"]
    if len(tags) == 0:
        return jsonify([])
    else:
        selected_tags = [f"tag:{s}" for s in tags]
        IDs = list(r5.sunion(selected_tags))
        keys = r5.keys("id:*")
        keys = [ key[3:] for key in keys ]
        # we take the intersection of the two sets
        valid_IDs  = list(set(keys) & set(IDs))

    return jsonify(valid_IDs)



@app.route("/histogram_from_ID", methods=["GET"])
def histogram_from_ID():
    """! Get the histogram from redis database 1 associated with the given ID.
    @return JSON containing the timestamp, the plotly figure, the ID, the associated tags and the runNumber of the histogram.
    """
    runNumber = r2.get("runNumber")
    packet = {}
    ID = request.args.get("ID")
    source, histname = ID.split("-")
    histobj = r.hget(source, f"h_{histname}")
    if histobj is not None:
        data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
        config = {"filename":f"{ID}+{timestamp}"} 
        fig = dict(data=data,layout=layout, config=config)
        tags = get_tags_by_ID(ID)
        packet = dict(timestamp=float(timestamp), fig=fig, ID=ID, tags=tags, runNumber = runNumber)
    return jsonify(packet)

## Tag section

@app.route("/add_tag", methods=["POST"])
def add_tag():
    """! Adds the given tag to redis.
    @return JSON with key "existed". If true, the added tag already existed in the database.
    """
    existed = False
    data = request.get_json()
    ID = data["ID"]
    tag = data["tag"]
    if r5.exists(f"tag:{tag}"):
        existed = True
    r5.sadd(f"id:{ID}", tag)
    r5.sadd(f"tag:{ tag }", ID)
    return jsonify({"existed": existed})


@app.route("/remove_tag", methods=["POST"])
def remove_tag():
    """! Remove the tag sent through a POST request.
    @return JSON with key "exists". If true, the removed tag still exists in the database.
    """
    exists = True
    data = request.get_json()
    ID = data["ID"]
    tag = data["tag"]
    r5.srem(f"id:{ID}", tag)
    r5.srem(f"tag:{tag}", ID)
    if not r5.exists(f"tag:{tag}"):
        exists = False
    return jsonify({"exists": exists})


@app.route("/history_view/<string:ID>")
def history_view(ID):
    """! Renders the history view page."""
    return render_template("vue_historyView.html",ID=ID)


@app.route("/stored_timestamps", methods=["GET"])
def stored_timestamps():
    """! Gets all stored timestamps in the database for a given ID.
    @return JSON with timestamps and old timestamps : {"ts": [...], "ots":[...]}
    """
    ID = request.args.get("ID")
    timestamps = sorted(r7.hkeys(ID), reverse= True)
    old_timestamps = sorted(r7.hkeys(f"old:{ID}"), reverse=True)
    packet = dict(ts=timestamps, ots=old_timestamps)
    return jsonify(packet)


@app.route("/stored_histogram", methods = ["GET"])
def stored_histogram():
    """! Get the histogram from redis database 7 associated with the given timestamp 
    @return JSON: stored histogram  
    """
    hist = ""
    ts_run = str(request.args.get("args"))  # (old:)timestamp&runNumber
    ID = request.args.get("ID")
    print("ts_run",ts_run)
    #ts_run = args.split("&")
    if "old" in ts_run:
        hist = r7.hget(f"old:{ID}", ts_run)
        hist = json.loads(hist)
    else:   
        hist = r7.hget(ID, ts_run)
        hist = json.loads(hist)
    return jsonify(hist)

@app.route("/delete_history", methods=["GET"])
def delete_history():
    """! Flushes the saved histograms from the redis database (db 7). """
    r7.flushdb()
    return jsonify({"response": "OK"})


@app.route("/delete_tags", methods=["GET"])
def delete_tags():
    """! Flushes the tags redis database (db 5)."""
    r5.flushdb()
    return jsonify({"response": "OK"})


@app.route("/delete_current", methods=["GET"])
def delete_current():
    """! Flushes the current histogram redis database (db 1)."""
    r.flushdb() # delete current histograms 
    # delete associated default tags 
    ids = r5.keys("id:*")
    m = map(delete_associated_tags, ids )
    l = list(itertools.chain.from_iterable(m))
    r5.delete(*l)

    r5.delete(*ids) 
    return jsonify({"response": "OK"})

@app.route("/redis_info", methods=["GET"])
def redis_info():
    """! Returns informations about the state of the Redis database. 
    @returns JSON: infos about the database
    """
    r_infos = r.info()
    return jsonify(r_infos)

## functions

def get_tags_by_ID(ID):
    """! Function that gets all tags without the default ones associated with an histogtram ID 
    @returns list: all tags
    """
    module, histname = ID.split("-")    
    # get the members associated with the key id:<histID> in the redis database
    tags = list(r5.smembers(f"id:{ID}"))
    # remove the default tags of the histogram (module name and histogram name)
    tags.pop(tags.index(module))
    tags.pop(tags.index(histname))
    return tags

def delete_associated_tags(ID):
    tags = ID[3:].split("-")
    tags[0] = f"tag:{tags[0]}"
    tags[1] = f"tag:{tags[1]}"
    return tags