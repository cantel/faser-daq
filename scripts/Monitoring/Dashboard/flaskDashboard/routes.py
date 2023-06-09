#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
# @mainpage Histogram Monitoring server

# @file routes.py

# @brief Defines all the flask routes and the api endpoints

import json
from flask.helpers import flash
import redis
from flask import render_template, request, Response, jsonify, send_file
import numpy as np
import atexit
import itertools
from apscheduler.schedulers.background import BackgroundScheduler
from flaskDashboard import app, interfacePlotly, redis_interface, checker, config_manager
from time import sleep
from datetime import datetime
from concurrent.futures import as_completed, ProcessPoolExecutor
import zipfile
from io import BytesIO

cfg = config_manager.loadConfig("config.json")
## Redis database where the last histograms are published.
r = redis.Redis(
    host=cfg['redis']['host'], port=cfg['redis']['port'], db=cfg['redis']['pub_histo_db'], charset="utf-8", decode_responses=True
)
## Redis database where the current run informations are stored 
r2 = redis.Redis(
    host=cfg["redis"]["host"], port=cfg["redis"]["port"], db=cfg["redis"]["run_info_db"], charset="utf-8", decode_responses=True

)
## Redis database where the tags are stored.
r5 = redis.Redis(
     host=cfg["redis"]["host"], port=cfg["redis"]["port"], db=cfg["redis"]["tags_db"], charset="utf-8", decode_responses=True

)

## Redis database where the previous histograms are stored.
r7 = redis.Redis(
    host=cfg["redis"]["host"], port=cfg["redis"]["port"], db=cfg["redis"]["old_histo_db"], charset="utf-8", decode_responses=True
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
            if r7.hlen(f"old:{ID}")>=168: # 24 (hours) * 7 (days) 
                key_to_remove = sorted(r7.hkeys(f"old:{ID}"))[0]
                r7.hdel(f"old:{ID}", key_to_remove)
            r7.hset(f"old:{ID}", f"old:{timestamp}R{runNumber}", json.dumps(hist))

# setting up the background jobs for storing old histograms
scheduler = BackgroundScheduler()
scheduler.add_job(func=histograms_save, trigger="interval", seconds=cfg["general"]['short_save_histo_rate'])
scheduler.add_job(func=old_histogram_save, trigger="interval", seconds=cfg['general']['long_save_histo_rate'])
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
@app.route("/home")
def home():
    """! Renders the home.html """
    storeDefaultTagsAndIDs()
    return render_template("home.html")


@app.route('/stream_histograms/<string:ids_string>')
def stream_histograms(ids_string):
    def get_histograms():
        packet={}
        ids = ids_string.split("&")
        while True:
            for ID in ids :
                checker_obj = checker.Checker()
                runNumber = r2.get("runNumber")
                source, histname = ID.split("-")
                histobj = r.hget(source, f"h_{histname}")
                if histobj is not None:
                    data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
                    config = {"filename":f"{ID}+{timestamp}"} 
                    fig = dict(data=data,layout=layout, config=config)
                    tags = get_tags_by_ID(ID)
                    checker_obj.check(histname, layout, data)
                packet[ID] = {"timestamp":float(timestamp), "fig":fig, "ID":ID, "tags":tags, "runNumber" : runNumber, "flags" : checker_obj.flags}
            yield f"data:{json.dumps(packet)}\n\n"
            sleep(cfg['general']['update_refresh_rate'])
    return Response(get_histograms(), mimetype="text/event-stream")

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
        t = []
        for tag in tags:
            if "*" in tag :
                temp_t = r5.keys(f"tag:{tag}")
                t = t + temp_t
            else:
                t.append(f"tag:{tag}")
        IDs = list(r5.sunion(t)) if t else []
        keys = r5.keys("id:*")
        keys = [ key[3:] for key in keys ]
        # we take the intersection of the two sets
        valid_IDs  = list(set(keys) & set(IDs))
        valid_IDs.sort()
        valid_IDs = split_into_pages(valid_IDs, cfg["general"]["max_hist_per_page"])
    return jsonify(valid_IDs)

def split_into_pages(IDs, nb):
    """
    IDs : the ids to split into pages
    nb : number of histogramm per page
    """ 
    if len(IDs) > nb : 
        nbEqualparts= len(IDs)//nb
        remainingElements = len(IDs)%nb
        arr = None
        if remainingElements == 0:
            arr = np.array_split(IDs,nbEqualparts)
        else:
            arr = np.array_split(IDs[:-remainingElements],nbEqualparts)
        arr = [ list(a) for a in arr]
        if remainingElements == 0:
            return arr 
        arr.append(IDs[-remainingElements:])
        return arr
    else : 
        return [IDs]
        

@app.route("/histogram_from_ID", methods=["GET"])
def histogram_from_ID():
    """! Get the histogram from redis database 1 associated with the given ID.
    @return JSON containing the timestamp, the plotly figure, the ID, the associated tags and the runNumber of the histogram.
    """
    checker_obj = checker.Checker()
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
        checker_obj.check(histname, layout, data)
        packet = {"timestamp":float(timestamp), "fig":fig, "ID":ID, "tags":tags, "runNumber" : runNumber, "flags" : checker_obj.flags}
    return jsonify(packet)

## Tag sections 

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
    return render_template("historyView.html",ID=ID)


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

@app.route("/compare_histograms", methods=["POST"])
def compare_histograms():

    data = request.get_json() 
    ID = data["ID"]
    ts1 = data["ts1"]
    ts2 = data["ts2"]

    hist1 = redis_interface.get_stored_histogram(r7,ID,ts1)
    hist2 = redis_interface.get_stored_histogram(r7,ID,ts2)

    diff_histo = hist1 # create dict object based on hist1 
    if hist1["figure"]["data"][0]["type"] == 'bar':
        diff_histo["figure"]["data"][0]["y"] = np.abs(np.array(hist1["figure"]["data"][0]["y"]) - np.array(hist2["figure"]["data"][0]["y"])).tolist()
    elif hist1["figure"]["data"][0]["type"] == 'heatmap':
        diff_histo["figure"]["data"][0]["z"] = np.abs(np.array(hist1["figure"]["data"][0]["z"]) - np.array(hist2["figure"]["data"][0]["z"])).tolist()
    return jsonify(diff_histo)

@app.route("/download_plots", methods=["POST"])
def download_plots():
    data = request.get_json()
    IDs = data["IDs"] 
    # runNumber = r2.get("runNumber")
    listOfFigs={}
    for ID in IDs: 
        source, histname = ID.split("-")
        histobj = r.hget(source, f"h_{histname}")
        if histobj is not None:
            data, layout, timestamp = interfacePlotly.convert_to_plotly(histobj)
            t = datetime.fromtimestamp(timestamp).strftime('%d%m%y_%H%M%S')
            filename = f"{ID}_{t}"
            fig = interfacePlotly.plot_js_to_plot_py(ID, data,layout)
            listOfFigs[filename]=fig
    binary_data = []
    with ProcessPoolExecutor() as executor:
        results = [executor.submit(figToPng,filename, fig ) for filename, fig in listOfFigs.items()]
        for f in as_completed(results):
            binary_data.append(f.result())
    # binary_data = [(filename,fig.to_image(format="png")) for filename, fig in listOfFigs.items()]
    mem_zip = BytesIO() 
    with zipfile.ZipFile(mem_zip, mode="w",compression=zipfile.ZIP_DEFLATED) as zf:
        for filename, binary in binary_data:
            zf.writestr(f"{filename}.png", binary)
    mem_zip.seek(0)
    return send_file(mem_zip, attachment_filename='plots.zip', as_attachment=True, mimetype='application/zip')

## functions

def figToPng(filename,fig):
    bts = fig.to_image(format="png")
    return (filename, bts)


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
