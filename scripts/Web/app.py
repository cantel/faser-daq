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
@app.route("/sessionTesting")
def sessionsShow():
	if (not "visits" in session):
		session['visits'] = 1
	else:
		session['visits'] = session.get('visits') + 1
	return "Total visits: {}".format(session.get('visits'))

@app.route("/")
def launche():
	#print(app.secret_key)
	try:
		r1.ping()
	except:
		return "redis database isn't working"
	#session["data"] = read("current.json")
	#r1.hset("runningFile", "isRunning", 0)
	if not r1.exists("runningFile"):	
		r1.hset("runningFile", "fileName", "current.json")
		r1.hset("runningFile", "isRunning", 0)

	#print("running file info: ", r1.hgetall("runningFile"))
	#print("isRunning", r1.hgetall("runningFile")["isRunning"] )
	#print("fileNAme: ", r1.hgetall("runningFile")["fileName"])
	
	#selectedFile = "current.json"
	#if(r1.hgetall("runningFile")["isRunning"] == 1):
	selectedFile = r1.hgetall("runningFile")["fileName"]
	#ielse:
	#	selectedFile = "current.json"
	#print(selectedFile)
	#return "the key runningFile does not exist on the redis db 2" 
	return render_template('softDaqHome.html', selectedFile = selectedFile)
@app.route("/initialise")
def initialise():
		
	print("reboot button pressed")
	
	d = session.get('data')
	dc = h.createDaqInstance(d)
	
	
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
	statusArr = []
	session_permanent = True
	d = session.get("data")
	#print("data", d)
	#print("session; ", id(session))
	if not d == {}:
		dc = h.createDaqInstance(d)
		for p in d['components']:	
			rawStatus, timeout = dc.getStatus(p)
			status = h.translateStatus(rawStatus, timeout)
			#print('status of this componen ', p['name'],"is: ", status)
			statusArr.append({'name' : p['name'] , 'state' : str(status)})
	return jsonify({'allStatus' : statusArr})

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

@app.route('/runningFile')
def runningFileInfo():
	info = r1.hgetall("runningFile");
	return(jsonify(info))


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

	
if __name__ == '__main__':
	#debug=True inorder to be able to update without recompiling
	app.run(threaded=True)
	#app.run(processe=19)


####main####

