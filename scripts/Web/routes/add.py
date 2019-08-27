
import json
import redis

import os
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


add_blueprint = Blueprint('add',__name__, url_prefix='/add',
      static_url_path='',
      static_folder='static',
      template_folder='templates')


@add_blueprint.route("/addBoard", methods=['GET', 'POST'])
def writeBoardToFile():
	if (request.method == 'POST'):
		submittedValue = request.json
		d = session.get("data")
		for p in d['components']:
			if(submittedValue['name'] == p['name']):
				return jsonify({ "message": "this board name already exists"})
		d['components'].append(submittedValue)
		session['data'] = d
		h.write(d)
		
		
	return jsonify({"message":"board successfully added"})

@add_blueprint.route("/")
def addBoard():
	schemas = os.listdir(env['DAQ_CONFIG_DIR'] +'schemas')
	schemaChoices = []
	for s in schemas:
		if(s.endswith(".schema")):
			schemaChoices.append({'name': s})
	schemaNames = {'schemaChoices' : schemaChoices}
	return render_template('config.html', pageName='Add Board', component = {}, schemaChoices = schemaNames, schema={}, flag = 1, boardName="")

@add_blueprint.route("/<schematype>")
def getschema(schematype):
	
	schema = h.readSchema(schematype)
	return jsonify(schema)
	
