from flask import Flask, render_template, request, jsonify, send_file, abort, session
from flask_scss import Scss
import json
#from jsonschema import validate
import os
from os import environ as env
import threading
import sys
import daqcontrol
import jsonschema
from jsonschema import validate
import shutil

from monitoring.metric import metric_blueprint


app  = Flask(__name__) 
app.register_blueprint(metric_blueprint)
app.debug = True
Scss(app, static_dir='static/css', asset_dir='assets/scss')
#sass.compile(dirname=('assets/scss', 'static/css'))

app.secret_key = 'verySecret:wq'

def read(fileName):
	session['fileName'] = fileName
	if(os.path.exists(env['DAQ_CONFIG_DIR'] + fileName)):
		with open(env['DAQ_CONFIG_DIR'] + fileName) as f:
			data = json.load(f)
		f.close()
	else:
		data = {}
		write(data)
	
	with open(env['DAQ_CONFIG_DIR'] + "json-config.schema") as f:
		 schema = json.load(f)
	f.close()
	return data

def write(d):
	with open(env['DAQ_CONFIG_DIR'] + 'current.json', 'w+') as f:
		json.dump(d, f)

	
def findIndex(boardName):
	index = 0
	d = session.get("data")
	for p in d['components']:
		#print("in findIndex, the index in for: ", index)
		if p['name'] == boardName:
			break
		else:
			index += 1
	return index


def readSchema(boardType):
	schemaFileName = env['DAQ_CONFIG_DIR'] + "schemas/" + boardType
	f = open(schemaFileName)
	#print(f.read())
	schema = json.load(f)
	f.close()
	#print(schema)
	
	#customizedFileName = env['DAQ_CONFIG_DIR'] + "customized/" + "customized.json"
	#f = open(customizedFileName)
	#custom = json.load(f)
	#f.close()

	#schema['properties']['host']['enum'].extend(custom['hostOptions'])
	return schema

def createDaqInstance():
	d = session.get("data")
	group =  d['group']
	dir = env['DAQ_BUILD_DIR']
	exe = "bin/main_core"
	lib_path = 'LD_LIBRARY_PATH='+env['LD_LIBRARY_PATH']
	dc = daqcontrol.daqcontrol(group, lib_path, dir, exe)
	return dc
	

def spawnJoin(list, func):
	threads = []
	for p in list:
		t = threading.Thread(target=func, args=(p,))
		t.start()
		threads.append(t)
	for t in threads:
		t.join()

@app.route("/hostOptions")
def getHostChoices():
	with open(env['DAQ_CONFIG_DIR'] + 'customized/customized.json') as f:
		hostChoices = json.load(f)
	f.close()
	return jsonify(hostChoices)

@app.route("/configFileNames")
def getConfigFileNames():
	entries = os.listdir(env['DAQ_CONFIG_DIR'])
	#print(entries)
	fileNames = []
	for e in entries:
		if(e.endswith(".json")):
			fileNames.append({'name': e})
	return jsonify({'configFileNames' : fileNames})
		
@app.route("/")
def launche():
	
	#session["dc"] = daqcontrol.daqcontrol(group, lib_path, dir, exe)

	session["data"] = read("current.json")

	return render_template('softDaqHome.html')
def translateStatus(rawStatus, timeout):
	translatedStatus = rawStatus
	if(timeout):
		translatedStatus = "TIMEOUT"
	else:
		if(rawStatus == b'not_added'):
			translatedStatus = "DOWN"
		elif(rawStatus == b'added'):
			translatedStatus = "ADDED"
		elif(rawStatus == b'ready'):	
			translatedStatus = "READY"
		elif(rawStatus == b'running'):	
			translatedStatus = "RUN"
	return translatedStatus
@app.route("/status")
def sendStatusJsonFile():
	
	dc = createDaqInstance()
	d = session.get("data")
	#print("in status data read from session", d)
	statusArr = []
	#group = session.get("group")
	#print("group in status", group)
	for p in d['components']:	
		rawStatus, timeout = dc.getStatus(p)
		status = translateStatus(rawStatus, timeout)
		print('status of this componen ', p['name'],"is: ", status)
		statusArr.append({'name' : p['name'] , 'state' : str(status)})
	return jsonify({'allStatus' : statusArr})
			
#@app.route("/status/eventbuilder01")
#def getEventBuilderStatus():	
#	dc = createDaqInstance()
#	d = session.get("data")
#	index = findIndex("eventbuilder01")
#	p = d['components'][index]
#	rawStatus, timeout = dc.getStatus(p)
#	status = translateStatus(rawStatus, timeout)
#	return jsonify({ 'state' : status })

@app.route("/initialise")
def initialise():
		
	print("reboot button pressed")	
	dc = createDaqInstance()
	d = session.get('data')
	
	logfiles = dc.addProcesses(d['components'], False)
	session['logfiles'] = logfiles
	#print("after adding")
	#print('in initilaise the logfiles are : ', logfiles)
	spawnJoin(d['components'], dc.configureProcess)
	
	return "true"

@app.route("/start")
def start():
	print("start button pressed")
	d = session.get('data')	
	dc = createDaqInstance()
	spawnJoin(d['components'], dc.startProcess)
	return "true"

@app.route("/stop")
def stop():
	print("stop button pressed")	
	dc = createDaqInstance()
	d = session.get('data')
	spawnJoin(d['components'], dc.stopProcess)
	return "true"

@app.route("/shutdown")
def shutdown():
	print("shutdown button pressed")	
	dc = createDaqInstance()
	d = session.get('data')	
	spawnJoin(d['components'], dc.shutdownProcess)
	dc.removeProcesses(d['components'])	
	return "true"

@app.route('/config/<boardName>')
def config(boardName):
	d = session.get("data")
	#print(boardName)	
	index = findIndex(boardName)

	component = d['components'][index]
	#print(component)
	boardName = d['components'][index]['name']
	boardType = component['type']
	
	schema = readSchema(boardType + ".schema")
	
	return render_template('config.html', pageName='Data', component = d['components'][index], schema = schema, flag= 0, schemaChoices = {}, boardName = boardName)

@app.route("/config/<boardName>/removeBoard")
def removeBoard(boardName):
	d = session.get("data")
	index = findIndex(boardName)
	d['components'].pop(index)
	session['data'] = d
	write(d)
	
@app.route("/config/<boardName>/changeConfigFile", methods=['GET', 'POST'])
def changeConfigFile(boardName):
	if (request.method == 'POST'):
		#print(request.content_type)
		submittedValue = request.json
		#print("submittedValue: ", submittedValue)
		index = findIndex(boardName)
		d = session.get("data")
		d['components'][index] = submittedValue
		#print(d)
		session['data'] = d
		write(d)
		
		
	return "true"



@app.route("/add/addBoard", methods=['GET', 'POST'])
def writeBoardToFile():
	if (request.method == 'POST'):
		#print(request.content_type)
		submittedValue = request.json
		#print("submittedValue: ", submittedValue)
		d = session.get("data")
		#print(d)
		for p in d['components']:
			if(submittedValue['name'] == p['name']):
				return jsonify({ "message": "this board name already exists"})
		d['components'].append(submittedValue)
		session['data'] = d
		write(d)
		
		
	return jsonify({"message":"board successfully added"})

@app.route("/add")
def addBoard():
		
	schemas = os.listdir(env['DAQ_CONFIG_DIR'] +'schemas')
	#print(schemas)
	schemaChoices = []
	for s in schemas:
		if(s.endswith(".schema")):
			schemaChoices.append({'name': s})
		#print("hello")
	schemaNames = {'schemaChoices' : schemaChoices}
	#print(schemaNames)	
	return render_template('config.html', pageName='Add Board', component = {}, schemaChoices = schemaNames, schema={}, flag = 1, boardName="")
@app.route("/add/<schematype>")
def getschema(schematype):
	schema = readSchema(schematype)
	return jsonify(schema)
	
@app.route('/configurationFile/<fileName>')
def getConfigFile(fileName):
	session["data"] = read(fileName)
	#print(fileName)
	#print(session.get("data"))
	return jsonify(session.get("data"))

@app.route('/saveConfigFile/<newFileName>')
def saveNewConfigFile(newFileName):
	if (os.path.exists(env['DAQ_CONFIG_DIR'] + newFileName)):
		return jsonify({ "message" : 0})
	else:
		shutil.copy(env['DAQ_CONFIG_DIR'] + "current.json", env['DAQ_CONFIG_DIR'] + newFileName + ".json", follow_symlinks=False)
		return jsonify({"message" : 1})
		
		
@app.route('/log/<boardName>')
def logfile(boardName):
	#x = open(logfiles[0][0])
	#logfiles = session.get("logfiles", None)
	logfiles = session.get('logfiles')
	index = findIndex(boardName)
	try:
		return send_file(logfiles[index][1], cache_timeout=-1)
	except IndexError as error:
		abort(404)
	except FileNotFoundError:
		abort(404)


if __name__ == '__main__':
	#debug=True inorder to be able to update without recompiling
	app.run(debug=True)


####main####
#stateOfApp = "DOWN"

