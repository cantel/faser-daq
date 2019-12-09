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
    selectedFile = r1.hgetall("runningFile")["fileName"]
    schemas = os.listdir(env['DAQ_CONFIG_DIR'] +'schemas')
    schemaChoices = []
    for s in schemas:
        if(s.endswith(".schema")):
            schemaChoices.append(s.replace(".schema",""))
    return render_template('softDaqHome.html', selectedFile = selectedFile, schemaChoices=schemaChoices)


@app.route("/initialise")
def initialise():
    print("reboot button pressed")
    
    d = session.get('data')
    dc = h.createDaqInstance(d)

    r1.publish("stateAction","initialize "+session.get("selectedFile"))
    
    r1.hset("runningFile", "fileName", session.get("selectedFile")) 

    logfiles = dc.addProcesses(d['components'])
        
    for logfile in logfiles:
        name = logfile[1][5:].split("-")[0]
        r1.hset("log", name, logfile[0] + logfile[1])
    h.spawnJoin(d['components'], dc.configureProcess)
            
    return "true"

@app.route("/start")
def start():
    print("start button pressed")
    runNumber=int(r1.get("runNumber"))
    runNumber+=1
    r1.set("runNumber",runNumber)
    r1.set("runStart",time.time())
    d = session.get('data') 
    dc = h.createDaqInstance(d)
#       h.spawnJoin(d['components'], dc.startProcess)
    h.spawnJoin(d['components'], functools.partial(dc.startProcess,arg=str(runNumber)))
    return "true"

@app.route("/pause")
def pause():
    print("pause button pressed")
    d = session.get('data') 
    dc = h.createDaqInstance(d)
    h.spawnJoin(d['components'], functools.partial(dc.customCommandProcess,command="disableTrigger"))
    return "true"

@app.route("/ecr")
def ecr():
    print("ECR button pressed")
    d = session.get('data') 
    dc = h.createDaqInstance(d)
    h.spawnJoin(d['components'], functools.partial(dc.customCommandProcess,command="ECR"))
    return "true"

@app.route("/unpause")
def unpause():
    print("unpause (start) button pressed")
    d = session.get('data') 
    dc = h.createDaqInstance(d)
    h.spawnJoin(d['components'], functools.partial(dc.customCommandProcess,command="enableTrigger"))
    return "true"


@app.route("/stop")
def stop():
    print("stop button pressed")    
    d = session.get('data')
    dc = h.createDaqInstance(d)
    h.spawnJoin(d['components'], dc.stopProcess)
    return "true"

@app.route("/shutdown")
def shutdown():
    print("shutdown button pressed")    
    d = session.get('data') 
    dc = h.createDaqInstance(d)
    h.spawnJoin(d['components'], dc.shutdownProcess)
    dc.removeProcesses(d['components'])     
    return "true"

@app.route("/refreshState")
def refresh():
    r1.publish("status","new") # force update
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
    
    try:
        r1.ping()
    except:
           print("Redis database is not running - please start it")
           #FIXME: print instructions here
           sys.exit(1)

    if not r1.exists("runningFile"):    
        r1.hset("runningFile", "fileName", "current.json")
    if not r1.exists("runNumber"):
        r1.set("runNumber",99)
        r1.set("runStart",0)

    metrics=metricsHandler.Metrics(app.logger)
    state=statetracker.State(r1,app.logger)

    print("Connect browser to:")
    print(" http://%s:%d" % (platform.node(),port))
    print(" - If connection does not work make sure browser runs on computer on CERN network")
    print("   and that firewall is open for the port:")
    print("   > sudo firewall-cmd --list-ports")
    from werkzeug.serving import run_simple
    run_simple("0.0.0.0",port,app,threaded=True)
    #app.run(host="0.0.0.0",port=5002,threaded=True)
    print("The end! Wrapping up remaining threads, please standby ")
    state.stop()
    metrics.stop()
	#app.run(processe=19)


####main####

