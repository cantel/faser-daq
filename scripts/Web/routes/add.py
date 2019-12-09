
import json
import redis

import os
from os import environ as env
import flask
from flask import Flask
from flask import Response
from flask import jsonify
from flask import render_template
from flask_restful import Api, Resource, reqparse
from flask import Blueprint, render_template
from flask import request, jsonify, send_file, abort

import sys
sys.path.append('../')
import helpers as h


add_blueprint = Blueprint('add',__name__, url_prefix='/add',
      static_url_path='',
      static_folder='static',
      template_folder='templates')


@add_blueprint.route("/addBoard/<fileName>", methods=['GET', 'POST'])
def writeBoardToFile(fileName):
        if (request.method == 'POST'):
                submittedValue = request.json
                d = h.read(fileName)
                for p in d['components']:
                        if(submittedValue['name'] == p['name']):
                                return jsonify({ "message": "this board name already exists"})
                d['components'].append(submittedValue)
                h.write(d)
                
                
        return jsonify({"message":"board successfully added"})

@add_blueprint.route("/<boardType>")
def getschema(boardType):
        fileName='ToBeAdded'
        schema = h.readSchema(boardType + ".schema")
        generalSchema = h.readGeneral() 
        boardName='New'+boardType
        return render_template('config.html', pageName='Add Board - '+boardType, component = {}, schemaChoices = {}, schema=schema, flag = 1, fileName=fileName, boardName=boardName, generalSchema = generalSchema)


        
