
import os
from os import environ as env
import threading
import sys
from jsonschema import validate
import json
import daqcontrol

def read(fileName):
	if(os.path.exists(env['DAQ_CONFIG_DIR'] + fileName)):
		with open(env['DAQ_CONFIG_DIR'] + fileName) as f:
			try:
				data = json.load(f)

				f.close() 	
				if(os.path.exists(env['DAQ_CONFIG_DIR'] + "json-config.schema")):
					with open(env['DAQ_CONFIG_DIR'] + "json-config.schema") as f:
						schema = json.load(f)
					f.close()

					try:
						validate(instance=data, schema=schema)
					except:
						data= "NOTSCHEMA"
				else:
					data= "NOSCHEMA"
			except:
				data = "NOTJSON"
	else:
		data = {}
		write(data)
	
		
	return data

def write(d):
	with open(env['DAQ_CONFIG_DIR'] + 'current.json', 'w+') as f:
		json.dump(d, f)




def readSchema(boardType):
	try:
		schemaFileName = env['DAQ_CONFIG_DIR'] + "schemas/" + boardType
		f = open(schemaFileName)
		schema = json.load(f)
		f.close()
	except:
		schema= "error"
	return schema



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



def tail(file, n=1, bs=1024):

	f = open(file)
	f.seek(0,2)
	l = 1-f.read(1).count('\n')
	B = f.tell()
	while n >= l and B > 0:
		block = min(bs, B)
		B -= block
		f.seek(B, 0)
		l += f.read(block).count('\n')
	f.seek(B, 0)
	l = min(l,n)
	lines = f.readlines()[-l:]
	f.close()
	return lines	
		
	
def findIndex(boardName, d):
	index = 0
	for p in d['components']:
		if p['name'] == boardName:
			break
		else:
			index += 1
	return index


def createDaqInstance(d):
	group =  d['group']
	dir = env['DAQ_BUILD_DIR']
	exe = "bin/daqling"
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

