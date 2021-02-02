#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
#!/usr/bin/env python3

import copy
from flask import Flask, render_template, request, jsonify, send_file, abort, session, Response
from datetime import datetime
from flask_scss import Scss
import functools
import getopt
import json
from jsonschema import validate
import os
from os import environ as env
import platform
import threading
import sys
import daqcontrol
import jsonschema
import shutil
import redis
import time

import metricsHandler
import statetracker

from routes.metric import metric_blueprint
from routes.config import config_blueprint
from routes.add import add_blueprint
from routes.configFiles import configFiles_blueprint

import helpers as h

__author__ = "Elham Amin Mansour"
__version__ = "0.0.2"
__maintainer__ = "Brian Petersen"
__email__ = "Brian.Petersen@cern.ch"

app  = Flask(__name__) 
app.register_blueprint(metric_blueprint)
app.register_blueprint(config_blueprint)
app.register_blueprint(add_blueprint)
app.register_blueprint(configFiles_blueprint)

app.debug = True
Scss(app, static_dir='static/css', asset_dir='assets/scss')
r1 = redis.Redis(host="localhost", port= 6379, db=2, charset="utf-8", decode_responses=True)

app.secret_key = os.urandom(24)

@app.route("/")
def launch():
    selectedFile = statetracker.getSelectedFile()
    schemas = os.listdir(env['DAQ_CONFIG_DIR'] +'schemas')
    schemaChoices = []
    for s in schemas:
        if(s.endswith(".schema")):
            schemaChoices.append(s.replace(".schema",""))
    return render_template('softDaqHome.html', selectedFile = selectedFile, schemaChoices=schemaChoices)


@app.route("/initialise")
def initialise():
    app.logger.info("Initialize button pressed")
    selectedFile=session.get("selectedFile")
    if not selectedFile:
        app.logger.error("Failed to get selected file")
        return "false"
    statetracker.initialize(selectedFile)
    return "true"

@app.route("/start")
def start():
    app.logger.info("Start button pressed")
    statetracker.start(request.args)
    return "true"

@app.route("/pause")
def pause():
    app.logger.info("Pause button pressed")
    statetracker.pause()
    return "true"

@app.route("/ecr")
def ecr():
    app.logger.info("ECR button pressed")
    statetracker.ecr()
    return "true"

@app.route("/unpause")
def unpause():
    app.logger.info("Unpause button pressed")
    statetracker.unpause()
    return "true"


@app.route("/stop")
def stop():
    app.logger.info("stop button pressed")    
    statetracker.stop()
    return "true"

@app.route("/shutdown")
def shutdown():
    app.logger.info("shutdown button pressed")
    statetracker.shutdown()
    return "true"

@app.route("/refreshState")
def refresh():
    statetracker.refresh()
    return "true"

@app.route("/state")
def sendState():
    def getState():
        pubsub=r1.pubsub()
        pubsub.subscribe("status")
        while True:
            json_data = r1.get("status")
            yield f"data:{json_data}\n\n"
            m=pubsub.get_message(timeout=10)
#            print(m)
    return Response(getState(), mimetype='text/event-stream')

@app.route('/log/<boardName>')
def logfile(boardName):
    try:
        logfile = r1.hgetall("log")[boardName]
        logfile = logfile[len(logfile.split("/")[0]):]
        data = h.tail(logfile, 1000)
        return Response(data, mimetype='text/plain')
    except KeyError:
        return Response("No logfile available", mimetype='text/plain')
    except IndexError as error:
        abort(404)
    except FileNotFoundError:
        abort(404)


def usage():
    print("Usage:")
    print("  app.py [-p <port>]")
    sys.exit(0)
    
if __name__ == '__main__':
    try:
        opts, args= getopt.getopt(sys.argv[1:],"p:",[])
    except getopt.GetoptError:
        usage()
    port=5000
    for opt,arg in opts:
        if opt=="-p":
            port=int(arg)
            if port<1024:
                print("Port number has to be >1023")
                sys.exit(1)
    

    state=statetracker.State(app.logger)
    metrics=metricsHandler.Metrics(app.logger)

    print("Connect browser to:")
    print(" http://%s:%d" % (platform.node(),port))
    print(" - If connection does not work make sure browser runs on computer on CERN network")
    print("   and that firewall is open for the port:")
    print("   > sudo firewall-cmd --list-ports")
    from werkzeug.serving import run_simple
    run_simple("0.0.0.0",port,app,threaded=True)
    #app.run(host="0.0.0.0",port=5000,threaded=True)
    print("The end! Wrapping up remaining threads, please standby ")
    state.stop()
    metrics.stop()
	#app.run(processe=19)


####main####

