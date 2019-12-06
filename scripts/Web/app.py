#!/usr/bin/env python3

import copy
from flask import Flask, render_template, request, jsonify, send_file, abort, session, Response
from datetime import datetime
from flask_scss import Scss
import functools
import json
from jsonschema import validate
import os
from os import environ as env
import threading
import sys
import daqcontrol
import jsonschema
import shutil
import redis
import time
import threading

from routes.metric import metric_blueprint
from routes.config import config_blueprint
from routes.add import add_blueprint
from routes.configFiles import configFiles_blueprint

import helpers as h

__author__ = "Elham Amin Mansour"
__version__ = "0.0.1"

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
	try:
		r1.ping()
	except:
		return "redis database isn't working"

	if not r1.exists("runningFile"):	
                r1.hset("runningFile", "fileName", "current.json")
                r1.hset("runningFile", "isRunning", 0)

	selectedFile = r1.hgetall("runningFile")["fileName"]
	return render_template('softDaqHome.html', selectedFile = selectedFile)


@app.route("/initialise")
def initialise():
		
	print("reboot button pressed")
	
	d = session.get('data')
	dc = h.createDaqInstance(d)

	r1.publish("stateAction","initialize "+session.get("selectedFile"))
	
	r1.hset("runningFile", "isRunning", 1)
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
	d = session.get('data')	
	dc = h.createDaqInstance(d)
	h.spawnJoin(d['components'], dc.startProcess)
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

@app.route("/status")
def sendStatusJsonFile():
        status=r1.get("status")
        return Response(status, mimetype="application/json")

@app.route("/state")
def sendState():
        def getState():
                pubsub=r1.pubsub()
                pubsub.subscribe("status")
                while True:
                        json_data = r1.get("status")
                        yield f"data:{json_data}\n\n"
                        m=pubsub.get_message(timeout=10)
                        print(m)
        return Response(getState(), mimetype='text/event-stream')

@app.route('/log/<boardName>')
def logfile(boardName):
	try:
		logfile = r1.hgetall("log")[boardName]
		logfile = logfile[len(logfile.split("/")[0]):]
		print("logfile: ", logfile)

		d = session.get('data')	
		index = h.findIndex(boardName, d)
		
		data = h.tail(logfile, 1000)
		return Response(data, mimetype='text/plain')

	except IndexError as error:
		abort(404)
	except FileNotFoundError:
		abort(404)


#on redis, the runningFile is put to 0
@app.route("/shutDownRunningFile")
def shutDownRunningFile():
	
	d = session.get("data")
	dc = h.createDaqInstance(d)
	#print("running File name: ", r1.hgetall("runningFile")["fileName"])
	runningFile = h.read(r1.hgetall("runningFile")["fileName"])
	allDOWN = True
	#print("running File:", runningFile['components'])
	for p in runningFile['components']:	
		rawStatus, timeout = dc.getStatus(p)
		status = h.translateStatus(rawStatus, timeout)
		if(not status == "DOWN"):
			allDOWN = False
	if(allDOWN):
		r1.hset("runningFile", "isRunning", 0)
		r1.hset("runningFile", "fileName", "current.json")
		return jsonify("true")
	else:
		return jsonify("false")

def stateTracker():
        pubsub=r1.pubsub()
        pubsub.subscribe("stateAction")
        oldState={}
        configName=""
        config={}
        oldStatus=[]
        daq=None
        while True:
                update=False
                runState=r1.hgetall("runningFile")
                if not oldState==runState:
                        oldState=runState
                        print("State changed to:",runState)
                        update=True
                if runState["fileName"]!=configName:
                        configName=runState["fileName"]
                        config=h.read(configName)
                        #FIXME: add handling of failed reads here
                        daq=None
                if not daq and type(config)==dict:
                        daq = h.createDaqInstance(config)
                if daq:
                        status=[]
                        for comp in config['components']:	
                                rawStatus, timeout = daq.getStatus(comp)
                                state = h.translateStatus(rawStatus, timeout)
                                status.append({'name' : comp['name'] , 'state' : str(state)})
                        if status!=oldStatus:
                                oldStatus=status
                                print("Status changed to: ",status)
                                r1.set("status",json.dumps({'allStatus' : status,
                                                            'runState' : runState}))
                                update=True
                if update:
                       r1.publish("status","new")

                m=pubsub.get_message(timeout=0.5)
                if m:
                        cmd=m['data']
                        if cmd=="quit": break
                        if cmd=="updateConfig": configName=""
                        print(m)
                        print(cmd)

        
	
if __name__ == '__main__':
	#debug=True inorder to be able to update without recompiling
        t = threading.Thread(target=stateTracker)
        t.start()
        app.run(host="0.0.0.0",port=5002,threaded=True)
        print("the end")
        r1.publish("stateAction","quit")
        t.join()
	#app.run(processe=19)


####main####

