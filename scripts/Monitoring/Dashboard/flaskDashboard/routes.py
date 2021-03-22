#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
# @mainpage Histogram Monitoring server

# @file routes.py

# @brief Defines all the flask routes and the api endpoints


import time
import json
from flask.helpers import flash
import redis
from flask import render_template
from flask import url_for, redirect, request, Response, jsonify
from flaskDashboard import app
import numpy as np
import atexit
from apscheduler.schedulers.background import BackgroundScheduler
from flaskDashboard import interfacePlotly 

## ID used for backround jobs (can it be deleted ?)
INTERVAL_TASK_ID = "interval-task-id" # can be deleted ?

## Redis database where the last histograms are published.
r = redis.Redis(
    host="localhost", port=6379, db=1, charset="utf-8", decode_responses=True
)
## Redis database where the tags are stored.
r5 = redis.Redis(
    host="localhost", port=6379, db=5, charset="utf-8", decode_responses=True
)
## Redis database where the old histograms are stored (--> for time view).
r6 = redis.Redis(
    host="localhost", port=6379, db=6, charset="utf-8", decode_responses=True
)
def recent_histograms_save():
    """! Defines the job that will be executed every 60 seconds
    It saves the current histograms in the redis database 1 in the redis database 6
    """
    with app.app_context():
        IDs = getAllIDs().get_json() # we get all the IDs
    for ID in IDs:
        module, histname = ID.split("-")
        histobj = r.hget(module, f"h_{histname}")
        if histobj != None:
            data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
            figure = dict(data=data, layout=layout)
            hist = dict(timestamp=timestamp, figure=figure)
            entry = {json.dumps(hist): float(timestamp)}
            r6.zadd(ID, entry)
            if r6.zcard(ID) >= 31:
                r6.zremrangebyrank(ID, 0, 0)

def historic_histograms_save():
    """! Defines the job that will be executed every 30 minutes.
    It saves the current histograms in the redis database 1 in the redis database 6
    """
    with app.app_context():
        IDs = getAllIDs().get_json()
    for ID in IDs:
        module, histname = ID.split("-")
        histobj = r.hget(module, f"h_{histname}")
        if histobj != None:
            data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
            figure = dict(data=data, layout=layout)
            hist = dict(timestamp=timestamp, figure=figure)
            entry = {json.dumps(hist): float(timestamp)}
            r6.zadd(f"historic:{ID}", entry)


# setting up the background jobs
scheduler = BackgroundScheduler()
scheduler.add_job(func=recent_histograms_save, trigger="interval", seconds=60)
scheduler.add_job(func=historic_histograms_save, trigger="interval", seconds=1800)
scheduler.start()
atexit.register(lambda: scheduler.shutdown())


@app.route("/getIDs", methods=["GET"])
def getIDs():
    """! Returns an JSON response with an list with the ID (<moduleName>-<histogramName>) of all histograms
    (sorted alphabetically).

    @return keys: JSON response with a list of all IDs.
    """
    modules = getModules().get_json()

    keys = []
    for module in modules:
        histnames = r.hkeys(module)
        for histname in histnames:
            if "h_" in histname:
                keys.append(f"{module}-{histname[2:]}")

    keys.sort()
    return jsonify(keys)


def getTagsByID(ids):
    """! Returns a dict with the ID as the key, and the tags as the associated values.

        @param ids The list of all IDs.

        @return The dictionary which contains the ID and the set of tags.
    """
    packet = {}
    for ID in ids:
        module, histname = ID.split("-")
        # get the members associated with the key id:<histID> in the redis database
        packet[ID] = list(r5.smembers(f"id:{ID}"))
        packet[ID].sort()
        # remove the default tags of the histogram (module name and histogram name)
        packet[ID].pop(packet[ID].index(module))
        packet[ID].pop(packet[ID].index(histname))
    return packet


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


@app.route("/addTag", methods=["GET"])
def addTag():
    """! Add the tag sent with a GET request from the client in the redis database.

            @return "true" if it already exists, "false" otherwise.
    """
    exists = "false"
    tag_to_add = request.args.get("tag")
    ID, tag = tag_to_add.split(",")
    if r5.exists(f"tag:{tag}"):
        exists = "true"
    r5.sadd(f"id:{ID}", tag)
    r5.sadd(f"tag:{ tag }", ID)
    return exists


@app.route("/removeTag", methods=["GET"])
def removeTag():
    """! Remove the tag sent through a GET request.
        @return "true" if the tag exists after the delete, "false" otherwise.
    """
    exists = "true"
    tag_to_remove = request.args.get("tag")
    ID, tag = tag_to_remove.split(",")
    r5.srem(f"id:{ID}", tag)
    r5.srem(f"tag:{tag}", ID)
    if not r5.exists(f"tag:{tag}"):
        exists = "false"
    return exists


@app.route("/renameTag", methods=["GET"])
def renameTag():
    """! Renames the tag sent through a GET request.
        If the old tag name still exists, oldtagexists is set to "true", "false" otherwise.
        If the new tag name already exists, "newtagexists" is set to "true", "false" otherwise.

        @return JSON response: the dictionary containing oldtagexists and newtagexists.
    """
    oldtagexists = "true"
    newtagexists = "false"
    tag = request.args.get("tag")
    ID, oldname, newname = tag.split(",")
    if r5.exists(f"tag:{newname}"):
        newtagexists = "true"
    r5.srem(f"id:{ID}", oldname)
    r5.sadd(f"id:{ID}", newname)
    r5.srem(f"tag:{oldname}", ID)
    r5.sadd(f"tag:{ newname }", ID)
    if not r5.exists(f"tag:{oldname}"):
        oldtagexists = "false"
    packet = dict(oldtagexists=oldtagexists, newtagexists=newtagexists)
    return jsonify(packet)


@app.route("/getAllTags", methods=["GET"])
def getAllTags():
    """! Returns a JSON response with all the tags stored in the redis database."""
    tags = r5.keys("tag*")
    tags = [s.replace("tag:", "") for s in tags]
    tags.sort()
    return jsonify(tags)


@app.route("/getAllIDs")
def getAllIDs():
    """! Returns a JSON response with all IDs stored in the redis database. """
    ids = r5.keys("id:*")
    ids = [s.replace("id:", "") for s in ids]
    ids.sort()
    return jsonify(ids)


@app.route("/getModules", methods=["GET"])
def getModules():
    """! Gets the keys from the redis database containing the "monitor" expression and whose data type is hash.

    @return JSON response : list of all valid modules.
    """
    modules = r.keys("*monitor*")
    modules = [module for module in modules if r.type(module) == "hash"]
 
    # modules.sort()
    return jsonify(modules)


@app.route("/flush_tags", methods=["GET"])
def flush_tags():
    """! Flushes the tags redis database (db 5)."""
    r5.flushdb()
    return "true"


@app.route("/flush_current_histograms", methods=["GET"])
def flush_current_histograms():
    """! Flushes the last histograms redis database (db 1)."""
    r.flushdb()
    return "true"


@app.route("/delete_histos", methods=["GET"])
def delete_histos():
    """! Flushes the saved histograms from the redis database (db 6). """
    r6.flushdb()
    return "true"


@app.route("/change_histo", methods = ["GET"])
def change_histo():
    """! Gets the selected histogram in the time view from a GET request and return the select."""
    timestamp = request.args.get("selected")
    ID = request.args.get("ID")
    newGraph = ""     # can be removed ?
    if "old" in timestamp:
        newGraph = r6.zrangebyscore(f"historic:{ID}",(timestamp.replace("old:","")),(timestamp.replace("old:","")))
    else:
        newGraph = r6.zrangebyscore(f"{ID}",timestamp, (timestamp))
    return jsonify(newGraph[0])



@app.route("/home")
def home():
    """! Renders the default web page."""
    storeDefaultTagsAndIDs()
    return render_template("home.html")


@app.route("/monitor", methods=["GET", "POST"])
def monitor():
    """! Renders the monitor webpage for a specified module or tag(s)."""
    if request.method == "GET":
        if request.args.get("selected_module"):
            selected_module = request.args.get("selected_module")
            ids = r5.smembers(f"tag:{selected_module}")
            ids = list(ids)
            tagsbyID = getTagsByID(ids)
            return render_template(
                "monitor.html",
                selected_module=selected_module,
                tagsByID=tagsbyID,
                ids=ids,
            )
        elif request.args.getlist("selected_tags"):
            selected_tags = request.args.getlist("selected_tags")
            selected_tags = [f"tag:{s}" for s in selected_tags]
            ids = r5.sunion(selected_tags)
            ids = list(ids)
            tagsbyID = getTagsByID(ids)
            return render_template("monitor.html", tagsByID=tagsbyID, ids=ids)

        else: 
            return render_template("home.html")


@app.route("/time_view/<string:ID>")
def timeView(ID):
    """! Renders the time view webpage for a specified histogram ID."""

    data = r6.zrangebyscore(ID, "-inf", "inf", withscores=True)
    if len(data) == 0: 
        return render_template("time_view.html", ID = ID)
    timestamps = [element[1] for element in data]
    lastHisto = data[0][0]
    historic_data = r6.zrangebyscore(f"historic:{ID}", "-inf", "inf", withscores=True)
    historic_timestamps = [element[1] for element in historic_data]
    
    return render_template(
        "time_view.html",
        historic_timestamps=historic_timestamps,
        timestamps=timestamps,
        lastHisto=lastHisto,
        ID = ID
    )


@app.route("/single_view/<string:ID>")
def single_view(ID):
    """! Renders the single view webpage for a specified histogram ID. """
    return render_template("single_view.html", ID=ID)

# @app.route("/settings")
# def settings():

#     return render_template("")


@app.route("/download_tags_json")
def download_tags_json():
    """! Gets all ids with their associated tags and return a Response object containing the JSON file. """
    json_file = {}
    keys = r5.keys("id*")
    for key in keys: 
        json_file[key.replace("id:", "")] = list(r5.smembers(key))
    json_file = json.dumps(json_file)
    return Response(
        json_file,
        mimetype="text/json",
        headers={"Content-disposition":
                 "attachment; filename=tags.json"})


@app.route("/load_tags_from_file", methods = ["POST"])
def load_tags_from_file():
    """! Gets the JSON file with a POST request, read it and store the information in the redis database (db 5). """
    if request.method == 'POST':
        file = request.files['filename']
        if file:
            if file.filename.split(".")[1] == "json":
                content = file.read()
                content = json.loads(content.decode("utf-8"))
                for key in content.keys():
                    r5.sadd(f"id:{key}", *content[key])
                    for tag in content[key]:
                        r5.sadd(f"tag:{tag}", key)
            else:
                flash("Wrong filetype, upload a .json file", category="danger")
        else:
            flash("No chosen file", category =  "danger")
    return redirect(url_for("home"))


@app.route("/source",methods=["GET"])
def lastHistogram():
    """! Returns the last histogram associated to the ID given in the GET request as a dictionary.
     The dictionary contains the figure dictionary compatible with the  Plotly library and its timestamp."""
    ID = request.args.get("ID")
    packet = {}
    source, histname = ID.split("-")
    histobj = r.hget(source, f"h_{histname}")
    if histobj is not None:
        data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
        fig = dict(data=data, layout=layout, config={"responsive": True})
        packet = {
                "figure": fig,
                "time": float(timestamp)}
    return jsonify(packet)





@app.context_processor
def context_processor():
    """! Returns variable that will be global for Jinja (the templating language)."""
    modules = getModules().get_json()
    tags = getAllTags().get_json()
    return dict(modules=modules, tags=tags)


@app.template_filter("ctime")
def timectime(s):
    """! Defines a custom filter for Jinja that returns a custom timestamp format."""
    d = time.localtime(s)
    datestring = time.strftime("%x %X", d)
    return datestring

### Migrating to Vue.js ### 
@app.route("/")
@app.route("/vue_home", methods = ["GET"])
def vue_home():
    return render_template("vue_home.html")
    
@app.route("/getModulesAndTags", methods = ["GET"])
def get_modules_and_tags():
    packet = {}
    modules = r.keys("*monitor*")
    tags = r5.keys("tag*")

    packet["modules"] = [module for module in modules if r.type(module) == "hash"]
    packet["tags"] = [s.replace("tag:", "") for s in tags]
    return jsonify(packet)

@app.route("/IDs_from_tags",methods=["POST"])
def IDs_from_tags():
    request_data = request.get_json()
    tags = request_data["tags"]
    if len(tags) == 0:
        return jsonify([])
    else:
        selected_tags = [f"tag:{s}" for s in tags]
        IDs = list(r5.sunion(selected_tags))
    return jsonify(IDs)

@app.route("/histograms_from_IDs",methods=["POST"])
def histograms_from_IDs():
    packet={}
    request_data = request.get_json()
    IDs = request_data["IDs"]
    for ID in IDs:
        source, histname = ID.split("-")
        histobj = r.hget(source, f"h_{histname}")
        if histobj is not None:
            data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
            fig = dict(data=data, layout=layout, config={"responsive": True})
            packet[ID] = dict(timestamp = timestamp,fig = fig)
    return jsonify(packet)

@app.route("/histogram_from_ID",methods=["GET"])
def histogram_from_ID():
    packet={}
    ID = request.args.get("ID")
    source, histname = ID.split("-")
    histobj = r.hget(source, f"h_{histname}")
    if histobj is not None:
        data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
        fig = dict(data=data, layout=layout, config={"responsive": False})
        tags = get_tags_by_ID(ID)
        packet = dict(timestamp = float(timestamp), fig = fig, ID = ID, tags = tags)
    return jsonify(packet)

## Tag section 

@app.route("/add_tag", methods=["POST"])
def add_tag():
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
    """! Remove the tag sent through a GET request.
        @return "true" if the tag exists after the delete, "false" otherwise.
    """
    exists = True
    data = request.get_json()
    ID = data["ID"]
    tag = data["tag"]
    r5.srem(f"id:{ID}", tag)
    r5.srem(f"tag:{tag}", ID)
    if not r5.exists(f"tag:{tag}"):
        exists = False
    return jsonify({"exists":exists})

## functions 

def get_tags_by_ID(ID):
    module, histname = ID.split("-")
    # get the members associated with the key id:<histID> in the redis database
    tags = list(r5.smembers(f"id:{ID}"))
    # remove the default tags of the histogram (module name and histogram name)
    tags.pop(tags.index(module))
    tags.pop(tags.index(histname))
    return tags





