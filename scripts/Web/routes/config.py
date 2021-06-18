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
    boardType = component['modules'][0]['type'] # FIXME: does not support multiple modules

    config={}
    config["name"]=boardName
    config["host"]=component['host']
    config["port"]=component['port']
    config["type"]=component['modules'][0]['type']
    config["settings"]=component['modules'][0]['settings']
    schema = h.readSchema(boardType + ".schema")
    generalSchema = h.readGeneral()    #FIXME we actually don't use this anymore
    return render_template('config.html', pageName='Config Editor - '+boardName, component = config, schema = schema, flag= 0, schemaChoices = {}, boardName = boardName, fileName= fileName, generalSchema= generalSchema)

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
        #print(    d['components'][index] )
        component=d['components'][index]
        component['name']= submittedValue['name']
        component['host']= submittedValue['host']
        component['port']= submittedValue['port']
        component['modules'][0]['settings']=submittedValue["settings"]
        
        h.write(d)
        
        
    return "true"

