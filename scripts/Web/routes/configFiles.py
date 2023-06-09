#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
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
		if(e.endswith(".json") and not e=="top.json"):
			fileNames.append({'name': e})
	fileNames.sort(key=lambda a: a['name'])
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
        res = h.read(fileName)
        session["selectedFile"] = fileName;
#        print("Selected",fileName,res)
        if(res == "NOJSON" or res == "BADSCHEMA" or res == "NOSCHEMA" or res == "NOTCOMP" or res == "BADJSON"):
                print("ERROR",res)
                pass #FIXME do something here?
        return jsonify(res)

