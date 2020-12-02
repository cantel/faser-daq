#!/usr/bin/env python3

from flask import Flask, Response, request, render_template
from flask_restful import Resource, Api
from functools import wraps
import json
import logging
import os
import time

import dbaccess

config = json.load(open("runservice.config"))

# Logger
log = logging.getLogger('service_logger')
log.setLevel(logging.DEBUG)
fh = logging.FileHandler(config["logfile"])
fh.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(threadName)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
log.addHandler(fh)
log.addHandler(ch)


dbaccess.init(config)

app = Flask(__name__)
api = Api(app)

def check_auth(user,pw):
    if not user==config["user"]: return False
    if not pw==config["pw"]: return False
    return True

def login_required(f):
    @wraps(f)
    def wrapped_view(self,**kwargs):
        auth = request.authorization
        if not (auth and check_auth(auth.username, auth.password)):
            return ('Unauthorized', 401, {
                'WWW-Authenticate': 'Basic realm="Login Required"'
            })
        return f(self,**kwargs)
    return wrapped_view

@app.route('/')
def runList():
    log.debug("RunList")
    runList=dbaccess.getRunList({})
    if not runList:
        return { "error": "Failed in DB connection"},503
    return render_template("runList.html",runList=runList)

class NewRunNumber(Resource):
    @login_required
    def post(self):
        log.debug('New run number requested: ' + str(request.json))
        log.debug('Requested from: '+ request.remote_addr)
        jsonreq=request.json
        data={}
        for field in ["type","version","configName","configuration"]:
            if not field in jsonreq:
                log.debug('  No "'+field+'" specified')
                data[field]="N/A"
            else:
                if len(jsonreq[field])>100 and field!="configuration":
                    return { "error": field+" field too long (>100 bytes)"},400
                data[field]=jsonreq[field]
        if data["type"]=="N/A":
            return { "error": "No type specified"},400

        runno=dbaccess.insertNewRun(data)
        if runno==0:
            return { "error": "Failed in DB connection"},503
        return runno,201

class AddRunInfo(Resource):
    @login_required
    def post(self,runno):
        log.debug('New info for run '+str(runno)+': ' + str(request.json))
        jsonreq=request.json
        if not "runinfo" in jsonreq:
            jsonreq["runinfo"]=""
        runinfo=jsonreq["runinfo"]
        success=dbaccess.addRunInfo(runno,runinfo)
        if not success:
            return {"error": "Failed to update run info"},400
        return {"success": True }
    
class RunInfo(Resource):
    def get(self,runno):
        log.debug('Run info request for run number: '+str(runno))
        result=dbaccess.getRunInfo(runno)
        if not result:
            return {"error": "No such run number"},400
        result["runnumber"]=runno
        return result
    
api.add_resource(NewRunNumber, '/NewRunNumber')
api.add_resource(RunInfo, '/RunInfo/<int:runno>')
api.add_resource(AddRunInfo, '/AddRunInfo/<int:runno>')

if __name__ == '__main__':
    app.run(debug=True,port=config["port"],host="0.0.0.0")
