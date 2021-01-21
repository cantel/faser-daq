#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
import json
import redis
from os import environ as env
import flask
from flask import Flask
from flask import Response
from flask import jsonify
from flask import render_template
from flask import Blueprint
from flask import request, send_file, abort


import sys
sys.path.append('../')
import helpers as h

config_blueprint = Blueprint('config',__name__, url_prefix='/config',
      static_url_path='',
      static_folder='static',
      template_folder='templates')


@config_blueprint.route("/hostOptions")
def getHostChoices():
	with open(env['DAQ_CONFIG_DIR'] + 'customized/host.json') as f:
		hostChoices = json.load(f)
	f.close()
	return jsonify(hostChoices)

@config_blueprint.route('/<fileName>/<boardName>')
def config(fileName, boardName):
	d = h.read(fileName)
	index = h.findIndex(boardName, d)

	component = d['components'][index]
	boardName = d['components'][index]['name']
	boardType = component['type']
	
	schema = h.readSchema(boardType + ".schema")
	generalSchema = h.readGeneral()	
	return render_template('config.html', pageName='Config Editor - '+boardName, component = d['components'][index], schema = schema, flag= 0, schemaChoices = {}, boardName = boardName, fileName= fileName, generalSchema= generalSchema)

@config_blueprint.route("/<fileName>/<boardName>/removeBoard")
def removeBoard(fileName, boardName):
	d = h.read(fileName)
	index = h.findIndex(boardName, d)
	d['components'].pop(index)
	h.write(d)
	
@config_blueprint.route("/<fileName>/<boardName>/changeConfigFile", methods=['GET', 'POST'])
def changeConfigFile(fileName, boardName):
	if (request.method == 'POST'):
		submittedValue = request.json
		d = h.read(fileName)
		index = h.findIndex(boardName, d)
		#print(submittedValue)
		#print("***************")
		#print(	d['components'][index] )
		d['components'][index] = submittedValue
		h.write(d)
		
		
	return "true"

