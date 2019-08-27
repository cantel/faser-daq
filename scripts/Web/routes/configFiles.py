import json
import redis
import os
import shutil
from os import environ as env
import flask
from flask import Flask,session
from flask import Response
from flask import jsonify
from flask import render_template
from flask_restful import Api, Resource, reqparse
from flask import Blueprint, render_template
from flask import Flask, render_template, request, jsonify, send_file, abort, session, Response


import sys
sys.path.append('../')
import helpers as h

configFiles_blueprint = Blueprint('configFiles',__name__, url_prefix='/configurationFiles',
      static_url_path='',
      static_folder='static',
      template_folder='templates')


@configFiles_blueprint.route("/fileNames")
def getConfigFileNames():
	entries = os.listdir(env['DAQ_CONFIG_DIR'])
	#print(entries)
	fileNames = []
	for e in entries:
		if(e.endswith(".json")):
			fileNames.append({'name': e})
	return jsonify({'configFileNames' : fileNames})
		

@configFiles_blueprint.route('/save/<newFileName>')
def saveNewConfigFile(newFileName):
	if (os.path.exists(env['DAQ_CONFIG_DIR'] + newFileName)):
		return jsonify({ "message" : 0})
	else:
		shutil.copy(env['DAQ_CONFIG_DIR'] + "current.json", env['DAQ_CONFIG_DIR'] + newFileName + ".json", follow_symlinks=False)
		return jsonify({"message" : 1})
	
@configFiles_blueprint.route('/<fileName>')
def getConfigFile(fileName):
		
	session['fileName'] = fileName
	res = h.read(fileName)
	session["selectedFile"] = fileName;
	if(res == "NOTJSON" or res == "NOTSCHEMA" or res == "NOSCHEMA"):
		session["data"]={}
	else:
		session["data"] = res
	
	return jsonify(res)

