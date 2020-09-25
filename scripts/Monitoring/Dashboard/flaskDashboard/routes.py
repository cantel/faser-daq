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


INTERVAL_TASK_ID = "interval-task-id"

r = redis.Redis(
    host="localhost", port=6379, db=0, charset="utf-8", decode_responses=True
)
r5 = redis.Redis(
    host="localhost", port=6379, db=5, charset="utf-8", decode_responses=True
)

r6 = redis.Redis(
    host="localhost", port=6379, db=6, charset="utf-8", decode_responses=True
)

def recent_histograms_save():
    with app.app_context():
        IDs = getAllIDs().get_json()
    for ID in IDs:
        module, histname = ID.split("-")
        histobj = r.hget(module, f"h_{histname}")
        data, layout, timestamp = convert_to_plotly(histobj)
        figure = dict(data=data, layout=layout)
        hist = dict(timestamp=timestamp, figure=figure)
        entry = {json.dumps(hist): float(timestamp)}
        r6.zadd(ID, entry)
        if r6.zcard(ID) >= 6:  # Should be 31
            r6.zremrangebyrank(ID, 0, 0)


def historic_histograms_save():
    with app.app_context():
        IDs = getAllIDs().get_json()
    for ID in IDs:
        module, histname = ID.split("-")
        histobj = r.hget(module, f"h_{histname}")
        data, layout, timestamp = convert_to_plotly(histobj)
        figure = dict(data=data, layout=layout)
        hist = dict(timestamp=timestamp, figure=figure)
        entry = {json.dumps(hist): float(timestamp)}
        r6.zadd(f"historic:{ID}", entry)


scheduler = BackgroundScheduler()

scheduler.add_job(func=recent_histograms_save, trigger="interval", seconds=60)
scheduler.add_job(func=historic_histograms_save, trigger="interval", seconds=1800)
scheduler.start()
atexit.register(lambda: scheduler.shutdown())



@app.route("/getIDs", methods=["GET"])
def getIDs():
    modules = getModules().get_json()

    keys = []
    for module in modules:
        histnames = r.hkeys(module)
        for histname in histnames:
            if "h_" in histname:
                keys.append(f"{module}-{histname[2:]}")

    keys.sort()
    return jsonify(keys)

#### tags section ###

def getTagsByID(ids):
    packet = {}
    for ID in ids:
        module, histname = ID.split("-")
        packet[ID] = list(r5.smembers(f"id:{ID}"))
        packet[ID].sort()
        packet[ID].pop(packet[ID].index(module))
        packet[ID].pop(packet[ID].index(histname))

    return packet

@app.route("/storeDefaultTagsAndIDs", methods=["GET"])
def storeDefaultTagsAndIDs():
    ids = getIDs().get_json()
    for ID in ids:
        module, histname = ID.split("-")
        r5.sadd(f"tag:{module}", ID)
        r5.sadd(f"tag:{histname}", ID)
        r5.sadd(f"id:{ID}", module, histname)
    return "true"


@app.route("/addTag", methods=["GET"])
def addTag():
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
    tags = r5.keys("tag*")
    tags = [s.replace("tag:", "") for s in tags]
    tags.sort()
    return jsonify(tags)


@app.route("/getAllIDs")
def getAllIDs():
    ids = r5.keys("id:*")
    ids = [s.replace("id:", "") for s in ids]
    ids.sort()
    return jsonify(ids)


@app.route("/getModules", methods=["GET"])
def getModules():
    modules = r.keys("*monitor*")
    modules = [module for module in modules if r.type(module) == "hash"]
 
    modules.sort()
    return jsonify(modules)


## Time_view ###
@app.route("/delete_histos", methods=["GET"])
def delete_histos():
    r6.flushdb()
    return "true"

@app.route("/change_histo", methods = ["GET"])
def change_histo():
    timestamp = request.args.get("selected")
    ID = request.args.get("ID")
    newGraph =""
    if "old" in timestamp:
        newGraph = r6.zrangebyscore(f"historic:{ID}",(timestamp.replace("old:","")),(timestamp.replace("old:","")))
    else:

        newGraph = r6.zrangebyscore(f"{ID}",timestamp, (timestamp))

    return jsonify(newGraph[0])

@app.route("/")
def load():
    print("Doing the busy stuff")
    storeDefaultTagsAndIDs()  # puts the basic tags module and histname
    return redirect(url_for("home"))


@app.route("/home")
def home():
    return render_template("home.html")


@app.route("/monitor", methods=["GET", "POST"])
def monitor():
    if request.method == "GET":
        if request.args.get("selected_module"):
            selected_module = request.args.get("selected_module")
            ids = r5.smembers(f"tag:{selected_module}")
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
            tagsbyID = getTagsByID(ids)
            return render_template("monitor.html", tagsByID=tagsbyID, ids=ids)


@app.route("/time_view/<string:ID>")
def timeView(ID):
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
    return render_template("single_view.html", ID = ID)


@app.route("/download_tags_json")
def download_tags_json():
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
    if request.method == 'POST':
            file = request.files['filename']
            if file:
                if file.filename.split(".")[1] == "json":
                    content = file.read()
                    content  = json.loads(content.decode("utf-8"))
                    for key in content.keys():
                        r5.sadd(f"id:{key}", *content[key])
                        for tag in content[key]:
                            r5.sadd(f"tag:{tag}", key)
                else:
                    flash("Wrong filetype, upload a .json file", category = "danger")
            else: 
                flash("No chosen file", category =  "danger")
    return redirect(url_for("home"))


@app.route("/source/<string:ID>")
def lastHistogram(ID):
    def getHistogram():
        while True:
            packet = {}
            source, histname = ID.split("-")
            histobj = r.hget(source, f"h_{histname}")
            if histobj is not None:
                data, layout, timestamp = convert_to_plotly(histobj)
                fig = dict(data=data, layout=layout, config={"responsive": True})
                packet = {
                    "figure": fig,
                    "time": float(timestamp)}
            yield f"data:{json.dumps(packet)}\n\n"

            time.sleep(3)

    return Response(getHistogram(), mimetype="text/event-stream")


def convert_to_plotly(histobj):
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
    modules = getModules().get_json()
    tags = getAllTags().get_json()
    return dict(modules=modules, tags=tags)

@app.template_filter("ctime")
def timectime(s):
    d = time.localtime(s)
    datestring = time.strftime("%x %X", d)
    return datestring
