import time
import json
import redis
from flask import render_template
from flask import url_for, redirect, request, Response, jsonify
from flaskDashboard import app
import numpy as np

# from tags_conditions import *

r = redis.Redis(
    host="localhost", port=6379, db=0, charset="utf-8", decode_responses=True
)
r5 = redis.Redis(
    host="localhost", port=6379, db=5, charset="utf-8", decode_responses=True
)


# pas fini
@app.route("/getIDs", methods=["GET"])
def getIDs():
    modules = getModules().get_json()
    keys = []
    for module in modules:
        histnames = r.hkeys(module)
        for histname in histnames:
            if "h_" in histname:
                keys.append(f"{module}-{histname[2:]}")

    return jsonify(keys)


@app.route("/storeDefaultTagsAndIDs", methods=["GET"])
def storeDefaultTagsAndIDs():
    ids = getIDs().get_json()
    for ID in ids:
        module, histname = ID.split("-")
        r5.sadd(f"tag:{module}", ID)
        r5.sadd(f"tag:{histname}", ID)
        r5.sadd(f"id:{ID}", module, histname)
    return "true"


@app.route("/getAllTags", methods=["GET"])
def getAllTags():
    tags = r5.keys("tag*")
    tags = [s.replace("tag:", "") for s in tags]
    return jsonify(tags)


@app.route("/getAllIDs")
def getAllIDs():
    ids = r5.keys("id:*")
    ids = [s.replace("id:", "") for s in ids]
    return jsonify(ids)


def getTagsByID(ids):
    packet = {}
    for ID in ids:
        module, histname = ID.split("-")
        packet[ID] = list(r5.smembers(f"id:{ID}"))
        packet[ID].pop(packet[ID].index(module))
        packet[ID].pop(packet[ID].index(histname))
    return packet


@app.route("/getModules", methods=["GET"])
def getModules():
    modules = r.keys("*monitor*")
    modules = [module for module in modules if r.type(module) == "hash"]
    return jsonify(modules)


@app.route("/addTag", methods=["GET"])
def addTag():
    exists = "false"
    tag_to_add = request.args.get("tag")
    ID, tag = tag_to_add.split(",")
    if r5.exists(f"tag:{tag}"):
        exists = "true"
    r5.sadd(f"id:{ID}", tag)
    r5.sadd(f"tag:{ tag }", ID)
    print(exists)
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
    # return f"{oldtagexists},{newtagexists}"
    return jsonify(packet)


# pas fini
@app.route("/update")
def update():
    # update modules et tags si besoin?
    pass
    # return redirect?


@app.route("/home")
def home():
    return render_template("home.html")


@app.route("/")
def load():
    print("Doing the busy stuff")
    storeDefaultTagsAndIDs()  # puts the basic tags module and histname
    return redirect(url_for("home"))


@app.route("/source/<path:path>")
def lastHistogram(path):
    def getHistogram():
        while True:
            packet = {}
            ids = path.split("/")
            for ID in ids:
                autotags = []
                source, histname = ID.split("-")
                histobj = r.hget(source, f"h_{histname}")

                if histobj is not None:
                    timestamp = histobj[: histobj.find(":")]
                    # print(timestamp)
                    histobj = json.loads(histobj[histobj.find("{") :])

                    xaxis = dict(title=dict(text=histobj["xlabel"]))
                    yaxis = dict(title=dict(text=histobj["ylabel"]))

                    layout = dict(
                        autosize=False,
                        uirevision=True,
                        # width = 1024,
                        # height = 600,
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
                        print(
                            "xmin = %f, xbins = %i, len(xarray) = %i"
                            % (xmin, xbins, len(xarray))
                        )
                        print(
                            "ymin = %f, ybins = %i, len(yarray) = %i"
                            % (ymin, ybins, len(yarray))
                        )
                        print("len(zarray) = %i" % (len(zarray)))

                        print("zarray.shape = ", zarray.shape)
                        zmatrix = zarray.reshape(len(yarray), len(xarray))
                        print("zmatrix.shape = ", zmatrix.shape)
                        data = [
                            dict(
                                x=xarray, y=yarray, z=zmatrix.tolist(), type="heatmap",
                            )
                        ]

                        # elif "2d" in hist_type:
                        # zarray = histobj["zvalues"]
                        # xmin = float(histobj["xmin"])
                        # xmax = float(histobj["xmax"])
                        # xbins = int(histobj["xbins"])
                        # xstep = (xmax - xmin) / float(xbins + 2)
                        # xarray = [xmin + xstep * xbin for xbin in range(xbins)]
                        # xmin -= xstep

                        # ymin = float(histobj["ymin"])
                        # ymax = float(histobj["ymax"])

                        # ybins = int(histobj["ybins"])
                        # ystep = (ymax - ymin) / float(ybins + 2)

                        # ymin -= ystep
                        # yarray = [ymin + ystep * ybin for ybin in range(ybins)]
                        # # data = [dict(xbins=dict(start=xmin, end=xmax,size=xstep), ybins=dict(start=ymin, end=ymax,size=ystep), z=zarray[106:], type="heatmap")]
                        # data = [
                        # dict(
                        # x0=xmin,
                        # dx=xstep,
                        # y0=ymin,
                        # dy=ystep,
                        # z=zarray,
                        # type="heatmap",
                        # )
                        # ]
                        # data = [dict(x=xarray, y=yarray, z=zarray, type="heatmap")]

                        # print(histname)
                        # print("xbins = ", xbins)
                        # print("len(xarray) =", len(xarray))
                        # print("ybins = ", ybins)
                        # print("len(yarray) =", len(yarray))
                        # print("len(zvalues) = ", len(zarray))
                        # print("\n")
                    else:

                        if "_ext" in hist_type:
                            print("ext hist")
                            xmin = float(histobj["xmin"])
                            xmax = float(histobj["xmax"])
                            xbins = int(histobj["xbins"])
                            step = (xmax - xmin) / float(xbins)
                            xarray = [xmin + step * xbin for xbin in range(xbins)]
                            yarray = histobj["yvalues"]
                            # data = [dict(x0=xmin, dx=step, y=yarray, type="bar")]
                            data = [dict(x=xarray, y=yarray, type="bar")]
                            print(histname)
                            print("xbins = ", xbins)
                            print("len(xarray) =", len(xarray))
                            # print("ybins = ", ybins)
                            print("len(yarray) =", len(yarray))
                            # print("len(zvalues) = ", len(zarray))
                            print("\n")

                        else:  # histogram with underflow and overflow
                            print("Overflow")
                            xmin = float(histobj["xmin"])
                            xmax = float(histobj["xmax"])
                            xbins = int(histobj["xbins"])
                            step = (xmax - xmin) / float(xbins + 2)
                            xmin -= step
                            xmax += step
                            xarray = [xmin + step * xbin for xbin in range(xbins + 2)]
                            yarray = histobj["yvalues"]

                            # if len(xarray) != len(yarray):  # check if overflow or not
                            # autotags.append("overflow,danger")
                            # yarray = yarray.pop()
                            # data = [dict(x0=xmin, dx=step, y=yarray, type="bar")]
                            data = [dict(x=xarray, y=yarray, type="bar")]
                            print(histname)
                            print("xbins = ", xbins)
                            print("len(xarray) =", len(xarray))
                            # print("ybins = ", ybins)
                            print("len(yarray) =", len(yarray))
                            # print("len(zvalues) = ", len(zarray))
                            print("\n")

                    # print(ID,len(xarray))

                    fig = dict(data=data, layout=layout, config={"responsive": True})
                    packet[ID] = {
                        "figure": fig,
                        "time": float(timestamp),
                        "autotags": autotags,
                    }
            yield f"data:{json.dumps(packet)}\n\n"
            time.sleep(3)

    return Response(getHistogram(), mimetype="text/event-stream")


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


@app.route("/time_view/<string:id>")
def timeView(id):
    # return render_template("time_view")

    return f"{id}"


@app.context_processor
def context_processor():
    modules = getModules().get_json()
    tags = getAllTags().get_json()
    return dict(modules=modules, tags=tags)


def addAutomaticTags(fig):
    returned_tags = {}

    return returned_tags
