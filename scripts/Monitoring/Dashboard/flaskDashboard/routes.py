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
        IDs = getAllIDs().get_json() # we get all the
    for ID in IDs:
        module, histname = ID.split("-")
        histobj = r.hget(module, f"h_{histname}")
        if histobj != None:
            data, layout, timestamp = convert_to_plotly(histobj)
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
            data, layout, timestamp = convert_to_plotly(histobj)
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
 
    modules.sort()
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


@app.route("/")
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
        data, layout, timestamp = convert_to_plotly(histobj)
        fig = dict(data=data, layout=layout, config={"responsive": True})
        packet = {
                "figure": fig,
                "time": float(timestamp)}
    return jsonify(packet)


def convert_to_plotly(histobj):
    """! Converts a "FASER type" histogram to a Plotly compatible histogram."""
    timestamp = float(histobj[: histobj.find(":")])
    histobj = json.loads(histobj[histobj.find("{") :])

    xaxis = dict(title=dict(text=histobj["xlabel"]))
    yaxis = dict(title=dict(text=histobj["ylabel"]))

    layout = dict(
        autosize=False,
        uirevision=True,
        xaxis=xaxis,
        yaxis=yaxis,
        margin=dict(l=50, r=50, b=50, t=50, pad=4),
    )
    data = []
    hist_type = histobj["type"]
    if "categories" in hist_type:
        xarray = histobj["categories"]
        yarray = histobj["yvalues"]
        data = [dict(x=xarray, y=yarray, type="bar")]

    elif "2d" in hist_type:
        zarray = np.array(histobj["zvalues"])
        xmin = float(histobj["xmin"])
        xmax = float(histobj["xmax"])
        xbins = int(histobj["xbins"])
        xstep = (xmax - xmin) / float(xbins)
        ymin = float(histobj["ymin"])
        ymax = float(histobj["ymax"])
        ybins = int(histobj["ybins"])
        ystep = (ymax - ymin) / float(ybins)
        xmin -= xstep
        ymin -= ystep
        xbins += 2
        ybins += 2
        xarray = [xmin + xbin * xstep for xbin in range(xbins)]
        yarray = [ymin + ybin * ystep for ybin in range(ybins)]
        zmatrix = zarray.reshape(len(yarray), len(xarray))
        data = [dict(x=xarray, y=yarray, z=zmatrix.tolist(), type="heatmap",)]
    else:

        if "_ext" in hist_type:
            xmin = float(histobj["xmin"])
            xmax = float(histobj["xmax"])
            xbins = int(histobj["xbins"])
            step = (xmax - xmin) / float(xbins)
            xarray = [xmin + step * xbin for xbin in range(xbins)]
            yarray = histobj["yvalues"]
            data = [dict(x=xarray, y=yarray, type="bar")]

        else:  # histogram with underflow and overflow
            xmin = float(histobj["xmin"])
            xmax = float(histobj["xmax"])
            xbins = int(histobj["xbins"])
            step = (xmax - xmin) / float(xbins)
            xmin -= step
            xbins += 2

            xarray = [xmin + step * xbin for xbin in range(xbins)]
            yarray = histobj["yvalues"]

            data = [dict(x=xarray, y=yarray, type="bar")]
    return data, layout, timestamp


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








