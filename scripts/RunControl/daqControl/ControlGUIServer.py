# import eventlet
# eventlet.monkey_patch()
from cgitb import reset
from cmath import exp
from http import server
import subprocess
from flask import Flask,redirect,url_for,render_template,request,session,g,jsonify,Response
from datetime import timedelta
import redis
import sys, os, time
import json
import threading
import logging
import jsonref
from functools import wraps, update_wrapper
from logging.handlers import RotatingFileHandler
from os import environ as env
from anytree import RenderTree, AsciiStyle
from anytree.search import find_by_attr
from anytree.importer import DictImporter
from flask_socketio import SocketIO, send, emit
from keycloak import Client
from datetime import datetime
from werkzeug.utils import secure_filename
from copy import deepcopy
from pathlib import Path
import platform
from flask_cors import CORS
from nodetree import NodeTree
from helpers import detectorList, getEventCounts
from daqcontrol import daqcontrol as daqctrl
import metricsHandler
import requests


# rewriting the original function from NodeTree class from daqLing
def childrenStateChecker(self):
    while (self.check):
        states = []
        for c in self.children:
            if c.getIncluded() == True:
                if type(c.getState())==str:
                    states.append(c.getState())
                elif type(c.getState())==list:
                    for item in c.getState(): states.append(item)
                else:
                    raise Exception("getState returned object of invalid type.") 
        # if no children is included
        if len(states) == 0:
            # add back all of them as to have a meaningful state
            for c in self.children:
                states.append(c.getState())
            self.exclude() # automatically exclude parent
        else:
        # if at least a child is included then include self if excluded (parent)
            if self.getIncluded() == False:
                self.included = True
        max_state = max(states, key=lambda state: list(self.state_action.keys()).index(state))
        min_state = min(states, key=lambda state: list(self.state_action.keys()).index(state))
        max_count = 0
        min_count = 0
        for s in states:
            if s == max_state:
                max_count = max_count + 1
            elif s == min_state:
                min_count = min_count + 1
        if max_count == len(states):
            self.state = max_state
            self.inconsistent = False
        elif min_count == len(states):
            self.state = min_state
            self.inconsistent = False
        else:
            # instead of having the lowest state, it stays in the max state until all the states are in the lowest state. 
            # self.state = min_state
            self.state = max_state ## <------
            self.inconsistent = True
        socketio.sleep(0.1)
NodeTree.childrenStateChecker = childrenStateChecker

## reading server configuration
serverConfig = {}
with open(os.path.join(os.path.dirname(__file__), 'serverconfiguration.json')) as f:
    serverConfig = json.load(f)

LOGOUT_URL = serverConfig['LOGOUT_URL']


configPath = os.path.join(env["DAQ_CONFIG_DIR"])

# for run service
run_user="FASER"
run_pw="HelloThere"


app = Flask(__name__)
cors = CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")
CONFIG_PATH =  ""
CONFIG_DICT = {}
configTree = None

localOnly = False

r = redis.Redis(host='localhost', port=6379, db=0,charset="utf-8", decode_responses=True)
r2 = redis.Redis(host='localhost', port=6379, db=3,charset="utf-8", decode_responses=True)

# creating redis default keys 
r2.setnx("runState","DOWN") # create key runState with value DOWN if it doesn't exist
r2.setnx("runOngoing",0) # 
r2.setnx("loadedConfig","") 

sysConfig : NodeTree = None # the treeObject for the configuration



handler = RotatingFileHandler(serverConfig["serverlog_location_name"], maxBytes=1000000, backupCount=0)
logging.root.setLevel(logging.NOTSET)
handler.setLevel(logging.NOTSET)
app.logger.addHandler(handler)

keycloak_client = Client(callback_uri=serverConfig["callbackUri"])
app.secret_key = os.urandom(24)
app.config["PERMANENT_SESSION_LIFETIME"] = timedelta(minutes=serverConfig["timeout_session_expiration_mins"])


def systemConfiguration(configName, configPath):
    global sysConfig
    with open(os.path.join(configPath, "config-dict.json")) as f:
        configJson = json.load(f)
    loadedConfig = r2.get("loadedConfig")
    if sysConfig is None:  # we (re)booted the app 
        print("sysConfig was None")
        try: 
            sysConfig = reinitTree(configJson)
        except Exception as e:
            logAndEmit("general", "ERROR", str(e))
    elif configName != loadedConfig: # we changed config
        print("sysConfig is not None and we changed config")
        sysConfig = reinitTree(configJson=configJson, oldRoot=sysConfig)
    else:
        print("Do nothing")
    

def getConfigsInDir(configPath):
    listOfFiles = []
    for d in os.listdir(configPath):
        conf_path = os.path.join(configPath, d)
        if not os.path.isfile(conf_path) and "config-dict.json" in os.listdir(conf_path):
            listOfFiles.append(d)
    return listOfFiles


def reinitTree(configJson, oldRoot=None):
    """
    configJson: map des différents fichiers json
    """
    if oldRoot != None:
        for _ , _, node in RenderTree(oldRoot):
            node.stopStateCheckers()

    # On récupère le fichier pour le tree
    with open(os.path.join(CONFIG_PATH, configJson["tree"])) as f:
        tree = json.load(f)

    # On récupère le fichier pour fsm
    try:
        with open(os.path.join(CONFIG_PATH, configJson["fsm_rules"])) as f:
            fsm_rules = json.load(f)
    except:
        # raise Exception("Invalid FSM configuration")
        raise Exception("FSM configuration not found")

    state_action = fsm_rules["fsm"]  # allowed action if in state "booted", etc...
    order_rules = fsm_rules["order"]  # ordrer of starting and stopping

    # We open the main config file
    with open(os.path.join(CONFIG_PATH, configJson["config"])) as f:
        base_dir_uri = (Path(configPath).as_uri() + "/")  # file:///home/egalanta/latest/faser-daq/configs/demo-tree/
        jsonref_obj = jsonref.load(f, base_uri=base_dir_uri, loader=jsonref.JsonLoader())

    if ("configuration" in jsonref_obj):  # faser ones have "configuration" but not demo-tree
        # schema with references (version >= 10)
        configuration = deepcopy(jsonref_obj)["configuration"]
    else:
        # old-style schema (version < 10)
        configuration = jsonref_obj
    # except:
    #     raise Exception("Invalid devices configuration")
    # try:
    #     with open(os.path.join(env['DAQ_CONFIG_DIR'], configJson['grafana'])) as f:
    #         grafanaConfig = json.load(f)
    #     f.close()
    # except:
    #     raise Exception("Invalid grafana/kibana nodes configuration:" + e)

    group = configuration["group"]  # for example "daq" or "faser"
    r2.set("group", group) 

    r2.set("config", json.dumps(configuration)) 
 
    if "path" in configuration.keys():
        dir = configuration["path"]
    else:
        dir = env["DAQ_BUILD_DIR"]  # <-- for faser normally
    exe = "/bin/daqling"
    lib_path = ("LD_LIBRARY_PATH="+ env["LD_LIBRARY_PATH"]+ ":"+ dir+ "/lib/,TDAQ_ERS_STREAM_LIBS=DaqlingStreams")
    components = configuration["components"]
    dc = daqctrl(group)

    importer = DictImporter(nodecls=NodeTree)

    newRoot = importer.import_(tree)  # We import the tree json
    for _, _, node in RenderTree(newRoot):
        for c in components:
            if node.name == c["name"]:
                node.configure(order_rules,state_action,pconf=c,exe=exe,dc=dc,dir=dir,lib_path=lib_path,)
            else:
                node.configure(order_rules, state_action)  # only loads order_rules and state_actions for that node

        node.startStateCheckers()
    return newRoot


def executeComm(ctrl, action):
    r = ""
    configName = r2.get("loadedConfig")

    logAndEmit(
        configName,"INFO","User "+ session["user"]["cern_upn"]+ " has sent command "+ action+ " on node "+ ctrl,)
    try:
        if action == "exclude":
            r = find_by_attr(sysConfig, ctrl).exclude()
        elif action == "include":
            r = find_by_attr(sysConfig, ctrl).include()
        else:
            r = find_by_attr(sysConfig, ctrl).executeAction(action)
    except Exception as e:
        logAndEmit(configName, "ERROR", ctrl + ": " + str(e))
    if r != "":
        logAndEmit(configName, "INFO", ctrl + ": " + str(r))
    return r


def executeCommROOT(action:str, reqDict:dict):
    """
    Execute actions on ROOT node (general commands) and extend the interlock for another <timeout> seconds 
    Returns : location of the module specific log files 
    """
    r2.expire("whoInterlocked", serverConfig["timeout_interlock_secs"])
    configName =  r2.get("loadedConfig")
    logAndEmit(configName,"INFO","User " + session["user"]["cern_upn"] + " has sent ROOT command " + action)
    setTransitionFlag(1)
    if action == "INITIALISE":

        detList = detectorList(r2.get("config"))
        r2.set("detList", json.dumps(detList))
        steps = [("add", "booted"), ("configure","ready")]
        for step,nextState in steps :
            r = sysConfig.executeAction(step)
            logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))

            if step == "add" and r[0] != "Action not allowed" :
                logPaths = listLogs(r)
                r2.delete("log")
                r2.sadd("log", *logPaths)
            done = waitUntilCorrectState(nextState, serverConfig["timeout_rootCommands_secs"][action]) # 10 seconds for each steps 
            if not done: 
                break  # one step doesn't reach the correct state, end command loop. 

    elif action == "START":
        runNumber = 1000000000
        runType = reqDict.get("runType","")
        runComment = reqDict.get("runComment","Test")[:500]
        version=subprocess.check_output(["git","rev-parse","HEAD"]).decode("utf-8").strip()
        seqnumber = reqDict.get("seqnumber",None)
        seqstep = reqDict.get("seqstep",0)
        seqsubstep = reqDict.get("seqsubstep",0)
        config = json.loads(r2.get("config"))
        detList = r2.get("detList")

        if not localOnly:
            prodURL = 'http://faser-runnumber.web.cern.ch/NewRunNumber'
            testURL = "http://faser-daq-001.cern.ch:5000/NewRunNumber"
            try : 
                res = requests.post(testURL,
                        auth=(run_user,run_pw),
                        json = {
                            'version':    version,
                            'type':       runType,
                            'username':       os.getenv("USER"),
                            'startcomment':   runComment,
                            'seqnumber': seqnumber,
                            'seqstep': seqstep,
                            'seqsubstep': seqsubstep,
                            'detectors':  detList,
                            'configName': configName,
                            'configuration': config
                        })
                if res.status_code != 201 :
                    logAndEmit("general", "ERROR", "Failed to get run number: "+res.text )
                    setTransitionFlag(0)
                    return "Error: Failed to get run number: "+res.text
                else:
                    try:
                        runNumber=int(res.text)
                    except ValueError:
                        logAndEmit("ERROR","Failed to get run number: "+res.text)
                        setTransitionFlag(0)
                        return "Error: Failed to get run number: "+res.text
            except requests.exceptions.ConnectionError:
                logAndEmit("general","Error","Could not connect to run service")
                setTransitionFlag(0)
                return "Error: Could not connect to run service"

        
        r2.set("runType", runType)
        r2.set("runComment", runComment)
        r2.set("runStart", time.time())
        r2.set("runNumber", runNumber)

        r=sysConfig.executeAction("start",runNumber)
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
        done = waitUntilCorrectState("running", serverConfig["timeout_rootCommands_secs"][action])

    elif action == "STOP":
        runinfo = {}
        runComment = reqDict.get("runComment","Test")[:500]
        runNumber = r2.get("runNumber")
        runType = reqDict.get("runType","") 
        runinfo["eventCounts"] = getEventCounts()
        physicsEvent = runinfo["eventCounts"]["Events_sent_Physics"] # for influxDB

        r2.set("runComment", runComment)
        r2.set("runType", runType)



        seqnumber = reqDict.get("seqnumber",None)
        seqstep = reqDict.get("seqstep",0)
        seqsubstep = reqDict.get("seqsubstep",0)
        config = json.loads(r2.get("config"))
        if not localOnly:
            stopMsg = {
                "endcomment" :runComment,
                "type": runType,
                "runinfo": runinfo

            }
            prodURL = f'http://faser-runnumber.web.cern.ch/AddRunInfo/{runNumber}'
            testURL = f"http://faser-daq-001.cern.ch:5000/AddRunInfo/{runNumber}"
            try : 
                res = requests.post(testURL,
                        auth=(run_user,run_pw),
                        json = stopMsg)

                if res.status_code != 200 :
                    logAndEmit("general", "ERROR", "Failed to register end of run information: "+r.text )
                    sendInfoToSnackBar( "ERROR", "Failed to register end of run information: "+r.text)

            except requests.exceptions.ConnectionError:
                logAndEmit("general","Error","Could not connect to run service")
                sendInfoToSnackBar( "ERROR","Could not connect to run service",)
        try : 
            r=sysConfig.executeAction("stop")
            logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
            done = waitUntilCorrectState("ready", serverConfig["timeout_rootCommands_secs"][action])
        except ConnectionRefusedError:
            setTransitionFlag(0)
            return "Error: connection refused"
            

    elif action == "SHUTDOWN":
        # steps = [("unconfigure", "booted"), ("shutdown","added"), ("remove", "not_added")]
        steps = [("unconfigure", "booted"), ("remove", "not_added")]
        for step,nextState in steps : 
            print(step,nextState)
            r = sysConfig.executeAction(step)
            logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
            done = waitUntilCorrectState(nextState, serverConfig["timeout_rootCommands_secs"][action])
        if not done : 
            print("not done")
            r = sysConfig.executeAction("remove")
            done = True
        
        # to be sure shutdown reset everything
        updateRedis(status="DOWN")
        socketio.emit("runStateChng","DOWN", broadcast=True)


    elif action == "PAUSE":
        r=sysConfig.executeAction("disableTrigger")
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
        done = waitUntilCorrectState("paused", serverConfig["timeout_rootCommands_secs"][action])
    
    elif action == "RESUME":
        r=sysConfig.executeAction("enableTrigger")
        if "Action not allowed" in r:
            r=sysConfig.executeAction("resume")
 
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
        done = waitUntilCorrectState("running", serverConfig["timeout_rootCommands_secs"][action])
    
    elif action == "ECR":
        r=sysConfig.executeAction("ECR")
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
        done = waitUntilCorrectState("paused", serverConfig["timeout_rootCommands_secs"][action])
 
    setTransitionFlag(0)
    if not done :
        return "Error: The command took to long -> TIMEOUT"
    return "Success" # success 

def sendInfoToSnackBar(typeM:str, message:str):
    """
    Sends a message to the client with message <message> with a color specified by typeM
    """
    valid_types = {"ERROR", "INFO", "SUCCESS"}
    if typeM not in valid_types:
        raise ValueError(f"Wrong type specified: {typeM} is not a valid type ")
    socketio.emit("snackbarEmit", {"mesage": message, "type": typeM})



def setTransitionFlag(flag:int):
    r2.set("transitionFlag", flag)

def waitUntilCorrectState(state:str, timeout=30):
    """
    The function will wait until the state <state> matches the root node state while being consistant. 
    Returns True if the transition succeded, False if it reached timeout.
    Default value for timeout is 30 seconds.  
    """
    now = time.time()
    while (time.time()-now < timeout):
        if (sysConfig.getState() == state)  and  (not sysConfig.inconsistent):
            return True
        socketio.sleep(0.2)
    return False

def listLogs(r):
    """
    Transforms a list of tuples to a list of string with all the log paths
    """
    final = []
    for sublist in r:
        if isinstance(sublist, tuple): final.append(sublist[1])
        elif sublist =="Excluded" : pass # ignore nodes that are excluded
        else:
            for item in sublist: final.append(item[1])
    return final

def getLogPath(name:str):
    paths = r2.smembers("log")
    for path in paths:
        if name in path: return path
    return ""
    

def getStatesList(locRoot):
    list = {}
    for _ , _, node in RenderTree(locRoot):
        list[node.name] = [
            find_by_attr(locRoot, node.name).getState(),
            find_by_attr(locRoot, node.name).inconsistent,
            find_by_attr(locRoot, node.name).included,
        ]
    return list


def stateChecker():
    l1 = {}
    l2 = {}
    loadedConfig =""
    lockState = None
    errors = []
    runInfo1= {"runType":"", "runComment":"", "runNumber":None}
    transitionFlag1 = r2.get("transitionFlag")


    while True:
        ########### Change of config ############
        loadedConfig2 = r2.get("loadedConfig")

        if loadedConfig2 != loadedConfig :
            socketio.emit("configChng", loadedConfig2, broadcast = True)
            loadedConfig = loadedConfig2

        ############# States ###############
        transitionFlag2 = r2.get("transtionFlag")
        if sysConfig:
            l2 = getStatesList(sysConfig)
            if (l2 != l1) or (transitionFlag2 != transitionFlag1):
                socketio.emit(f"stsChng", l2, broadcast=True)
                if l1 != {} and l2 !={}:    
                    if l1["Root"] != l2["Root"] or (transitionFlag2 != transitionFlag1): # if Root status changes
                        state=""
                        if l2["Root"][1] and r2.get("transitionFlag") == "1" : # if root is in transition
                            print("ROOT inconsistent")
                            state = "IN TRANSITION"
                            updateRedis(status=state)
                            socketio.emit("runStateChng",state , broadcast=True)
                        else :
                            if l2["Root"][0] == "running" : state="RUN"
                            elif l2["Root"][0] == "not_added" : state="DOWN"; cleanErrors()
                            elif l2["Root"][0] == "ready" : state="READY"
                            elif l2["Root"][0] == "paused" : state="PAUSED"
                            print(f"ROOT Changed state and is not inconsistent:{l2['Root']}", state)
                            
                            if l2["Root"][0] not in ["booted","added"]:
                                updateRedis(status=state)
                                logAndEmit(r2.get("loadedConfig"),"INFO",f"ROOT element is now in state {state}")
                                socketio.emit("runStateChng",state , broadcast=True)
                l1 = l2
                transitionFlag1= transitionFlag2

            errors2 = modulesWithError(sysConfig)
            if errors2 != errors:
                socketio.emit("errorModChng", errors2, broadcast =True)
                r2.delete("modulesErrors")
                if len(errors2) != 0:
                    r2.sadd("modulesErrors", *errors2)
                errors = errors2

            


            

                        
                        


            # if l1[file]['Root'][0] != l2[file]
        
            # if (rState2 != rState1) and (rState2[1] == False ) and (rState2[0] not in ['booted','added']):
            #     # printTree(sysConf[file])
            #     if rState2[0] == 'running' : state="RUN"
            #     elif rState2[0] == 'not_added':
            #         state = "DOWN"
            #         cleanRedis()
            #     elif rState2[0] == 'ready': state = "READY"
            #     elif rState2[0] == 'paused' : state = "PAUSED"
            #     updateRedis(status=state)
            #     logAndEmit(r2.get("loadedConfig"),"INFO",f"ROOT element is now in state {state}")
            #     socketio.emit("runStateChng",state , broadcast=True)

            # #intentional inconsistent (transition state)
            # elif (rState2 != rState1) and (rState2[1] == True ) and (r2.get("transitionFlag")== "1"):
            #     # printTree(sysConfig)
            #     state = "IN TRANSITION"
            #     updateRedis(status=state)
            #     socketio.emit("runStateChng",state , broadcast=True)

        # elif (rState2[file] != rState1[file]) and (rState2[file][1] == True ) and (r2.get("transitionFlag")== "0"):
        #     print("CRASH")
        #     # modulesCrashed = modulesWithError(sysConf[file])
        #     # socketio.emit("errorModChng", modulesCrashed , broadcast=True)
        #     r2.set("modulesErrors", str(modulesCrashed))


        
        ####### change of lock State ###########
        lockState2 = r2.get("whoInterlocked")
        if lockState2 !=lockState:
            socketio.emit("interlockChng", lockState2, broadcast = True)
            if not lockState2:
                logAndEmit(loadedConfig2, "INFO", "Interlock has been released because of TIMEOUT")
            lockState = lockState2

        #### check the status of the modules ### 

        ####### change of runInfo #######
        runInfo2 = getRunInfo()
        if runInfo2 != runInfo1:
            socketio.emit("runInfoChng", runInfo2, broadcast =True)
            runInfo1 = runInfo2

        socketio.sleep(0.3)


def getRunInfo():
    return {
        "runType": r2.get("runType"),
        "runComment": r2.get("runComment"),
        "runNumber": r2.get("runNumber")
    }

def cleanErrors():
    r2.delete("modulesErrors")
    socketio.emit("errorModChng", [] , broadcast=True)

     

def modulesWithError(tree):
    errorList = []
    for _,_, node in RenderTree(tree):
        if not node.children : # if the node has no children -> not a category
            if r.hget(node.name, "Status"):
                status =  bool(int(r.hget(node.name, "Status").split(":")[1])) # returns 0, 1 or 2 (1 and 2 -> warning and error)
                if status != 0 :
                    errorList.append(node.parent.name)
                    errorList.append(node.name) 
    return list(set(errorList)) 

            
            
def logAndEmit(configtype, type:str , message=""):
    now = datetime.now()
    timestamp = now.strftime("%d/%m/%Y, %H:%M:%S")
    if type == "INFO":
        app.logger.info("[" + configtype + "] " + timestamp + " " + type + ": " + message)
    elif type == "WARNING":
        app.logger.warning("[" + configtype + "] " + timestamp + " " + type + ": " + message)
    elif type == "ERROR":
        app.logger.error("[" + configtype + "] " + timestamp + " " + type + ": " + message)
    socketio.emit("logChng","[" + configtype + "] " + timestamp + " " + type + ": " + message,broadcast=True,)


def updateRedis(status:str = None):
    if status:
        r2.set('runState', status) 
        if status == "DOWN":
            r2.set('runOngoing',0)
        else:
            r2.set('runOngoing',1)





@app.route("/appState", methods=["GET"])
def appState():
    packet ={}
    packet["runOngoing"] = bool(int(r2.get("runOngoing")))
    packet["loadedConfig"] = r2.get('loadedConfig')
    packet["runState"] = r2.get("runState")
    if "user" in session:
        packet["user"] = {"name":session["user"]["cern_upn"], "logged" : True if "cern_gid" in session["user"] else False }
    packet["whoInterlocked"] = r2.get("whoInterlocked")
    errors = r2.smembers("modulesErrors")
    packet["errors"] = list(errors) if errors else []
    packet["localOnly"] = localOnly
    packet = {**packet, **getRunInfo()}

    return jsonify(packet)



def nocache(view):
    @wraps(view)
    def no_cache(*args, **kwargs):
        response = make_response(view(*args, **kwargs))
        response.headers["Last-Modified"] = datetime.now()
        response.headers[
            "Cache-Control"
        ] = "no-store, no-cache, must-revalidate, post-check=0, pre-check=0, max-age=0"
        response.headers["Pragma"] = "no-cache"
        response.headers["Expires"] = "-1"
        return response

    return update_wrapper(no_cache, view)


@app.before_request
def func():
    session.modified = True


def getInfo(module:str) -> list:
    """
    Gets the informations about a module in the redis database 
    Returns a list of dictionnaries with {"key": ..., "value": ..., "time": ... (string)}
    """

    # TODO: use hgetall instead of hmget
    keys = r.hkeys(module)
    # if module is not in the redis database
    if keys == []: return []
    keys.sort()
    vals = r.hmget(module, keys=keys)
    
    info = [{"key":key, "value": val.split(":")[1],"time": datetime.fromtimestamp(int(float(val.split(":")[0]))).strftime('%a %d/%m/%y %H:%M:%S ') } for key,val in zip(keys,vals)]
    return info
 
@app.route("/fullLog/<string:module>")
def fullLog(module:str) -> Response:
    """
    Returns the full log for the current run #NOTE: Is it really just the current run ? 
    (Works only if the logs are on the same server)
    """
    path = getLogPath(module)
    with open(path, "r") as f:
        log = f.readlines()
    return Response(log, mimetype='text/plain')

@app.route("/log", methods=["GET"])
def log():
    with open(serverConfig["serverlog_location_name"]) as f:
        ret = f.readlines()
    return jsonify(ret[-20:])


@app.route("/serverconfig")
def serverconfig():
    return jsonify(serverConfig)


@app.route("/interlock", methods=["POST"])
def interlock():
    requestData  = request.get_json()
    action = requestData["action"] # lock or unlock 

    
    if action == "lock": 
        if r2.set("whoInterlocked", session["user"]["cern_upn"], ex=serverConfig["timeout_interlock_secs"], nx=True): 
            logAndEmit(r2.get("loadedConfig"),"INFO","User "+ session["user"]["cern_upn"]+ " has TAKEN control of configuration "+ r2.get("loadedConfig"),)
            return jsonify("File locked successfully")
        else : 
            # someone already locked the config
            logAndEmit(r2.get("loadedConfig"),"INFO","User "+ session["user"]["cern_upn"]+ " tried to take control configuration "+ r2.get("loadedConfig")+f"from {r2.get('whoInterlocked')}",)
            return jsonify(f"Sorry, the config is already locked by {r2.get('whoInterlocked')}")

    else: # action == "unlock"
        if session["user"]["cern_upn"] == r2.get("whoInterlocked"): # if is the right person
            r2.delete("whoInterlocked")            
            logAndEmit(r2.get("loadedConfig"),"INFO","User " + session["user"]["cern_upn"]+ " has RELEASED control of configuration"+ r2.get('loadedConfig'))
            return jsonify(f"unlocked file")

        else: # someone else try to unlock file         
            logAndEmit(r2.get("loadedConfig"),"INFO","User "+ session["user"]["cern_upn"]+ "  ATTEMPTED to take control of configuration "+ r2.get("loadedConfig")+f"from {r2.get('whoInterlocked')}",)
            return jsonify(f"you can't")

@app.route("/ajaxParse", methods=["POST"])
def ajaxParse():
    postData = request.get_json()
    node = postData["node"]
    command = postData["command"]
    configName =  r2.get("loadedConfig")
    r2.expire("whoInterlocked",serverConfig["timeout_interlock_secs"]) # adds a new timeout

    try:
        r = executeComm(node, command)
    except Exception as e:
        logAndEmit(configName, "ERROR", str(e))
    socketio.sleep(1)
    return jsonify(r)


@app.route("/logout", methods=["GET", "POST"])
def logout():
    logAndEmit("general", "INFO", "User logged out: " + session["user"]["cern_upn"])
    session.pop("user", None)
    session.pop("configName", None)
    session.pop("configPath", None)
    session.pop("configDict", None)
    return redirect("{}?redirect_uri={}".format(LOGOUT_URL, url_for("index", _external=True)))

@app.route("/fsmrulesJson", methods=["GET", "POST"])
def fsmrulesJson():
    try:
        with open(os.path.join(CONFIG_PATH, CONFIG_DICT["fsm_rules"])) as f:
            fsmRules = json.load(f)
    except Exception as e:
        raise e
    return jsonify(fsmRules["fsm"])


@socketio.on("connect", namespace="/")
def connect():
    print("Client connected")


# @app.before_first_request
# def startup():
#     global thread
#     with thread_lock:
#         if thread is None:
#             thread = socketio.start_background_task(stateChecker)
#     handler = RotatingFileHandler(serverConfigJson["serverlog_location_name"], maxBytes=1000000, backupCount=0)
#     logging.root.setLevel(logging.NOTSET)
#     handler.setLevel(logging.NOTSET)
#     app.logger.addHandler(handler)


# @app.after_request
# def add_header(response):
#     response.cache_control.no_store = True
#     return response


#####################################################################


@app.route("/configDirs", methods=["GET", "POST"])
def configDirs():
    return jsonify(getConfigsInDir(configPath))


@app.route("/initConfig", methods=["GET", "POST"])
def initConfig():
    global CONFIG_PATH,CONFIG_DICT
    configName  =  request.args.get("configName")
    
    CONFIG_PATH = os.path.join(env["DAQ_CONFIG_DIR"], configName)
    logAndEmit(CONFIG_PATH,"INFO","User "+ session["user"]["cern_upn"] + " has switched to configuration "+ configName,)
    with open(os.path.join(CONFIG_PATH, "config-dict.json")) as f:
        CONFIG_DICT = json.load(f)

    systemConfiguration(configName, CONFIG_PATH)
    r2.set("loadedConfig", configName)
    return "Success"


@app.route("/urlTreeJson")
def urlTreeJson():
    try:
        with open(os.path.join(CONFIG_PATH, CONFIG_DICT["tree"]), "r") as file:
            return jsonify(json.load(file))
    except Exception as e:
        return "error"


@app.route("/login", methods=["GET", "POST"])
def login():
    if serverConfig["SSO_enabled"] == 0:
        session["user"]["cern_upn"] = "offlineUser"
        session["user"]["cern_gid"] = ""
        return redirect(url_for("index"))
    else : 
        auth_url, state = keycloak_client.login()
        session["state"] = state
        return redirect(auth_url)


@app.route("/login/callback", methods=["GET"])
def login_callback():
    state = request.args.get("state", "unknown")
    _state = session.pop("state", None)
    if state != _state:
        return Response("Invalid state: Please retry", status=403)
    code = request.args.get("code")
    response = keycloak_client.callback(code)
    access_token = response["access_token"]
    userinfo = keycloak_client.fetch_userinfo(access_token)
    session["user"] = userinfo
    logAndEmit("general", "INFO", "User connected: " + session["user"]["cern_upn"])
    return redirect(url_for('index'))



@app.route("/")
def index():
    if "user" in session: # if user is already connected
        return render_template("index.html", usr=session["user"]["cern_upn"])
    return redirect(url_for('localLogin'))




@app.route("/botLogin")
def botLogin():
    if session.get("user") is not None:
        session["user"] = {}
        session["user"]["cern_upn"] = "bot"

    return "OK"


@app.route("/localLogin")
def localLogin():
    session["user"] = {}
    session["user"]["cern_upn"] = "local_user"
    return redirect(url_for('index'))

@app.route("/statesList", methods=["GET"])
def statesList():
    list = {}
    list = getStatesList(sysConfig)
    return jsonify(list)


@app.route("/processROOTCommand", methods=["POST"])
def rootCommand():
    reqDict:dict = request.get_json()
    command = reqDict.get("command")
    r = executeCommROOT(command, reqDict)
    return jsonify(r)

@app.route("/info", methods=["GET"])
def info():
    module = request.args.get("module")
    info = getInfo(module)
    return jsonify(info)

@app.route("/infoWindow/<string:module>")
def infoWindow(module):
    """
    Page for displaying the info table in a separate window
    """
    return render_template("infoWindow.html", module = module)


@app.route("/monitoring/initialValues",methods=["GET"])
def monitoringInitialValues():
    """
    For the plots in the monitoring Panel (Run control GUI)
    Returns : some metrics (defined by 'keys') and last values for rates defined in 'graphKeys'
    NOTE: the function can be easily optimized
    """
    def is_recent(value):
        return datetime.now().timestamp() - float(value.split(":")[0]) < 1800

    packet = {"values": {}, "graphs":{}}
    keys = ["RunStart","RunNumber",
            "Events_received_Physics","Event_rate_Physics",
            "Events_received_Calibration", "Event_rate_Calibration",
            "Events_received_TLBMonitoring","Event_rate_TLBMonitoring"]
    # getting the eventbuilder module 
    #NOTE The next line avoids the need to hardcode the name of the module, but it still needs to have eventbuilder in it's name.  
    eventBuilderName = r.keys("eventbuilder*")[0] # should only be one eventbuilder module
    results = [int(float(metric.split(":")[1])) for metric in r.hmget(eventBuilderName, keys )]
    data = dict(zip(keys,results))
    data["RunStart"] =datetime.fromtimestamp(float(data["RunStart"])).strftime('%d/%m %H:%M:%S')  if data["RunStart"] != 0 else "-"
    packet["values"] = data
    
    # for the plot :

    graphKeys = [
        f"History:{eventBuilderName}_Event_rate_Physics",
        f"History:{eventBuilderName}_Event_rate_Calibration",
        f"History:{eventBuilderName}_Event_rate_TLBMonitoring",
                ]
                
    for graphKey in graphKeys:
        values, = sorted(r.lrange(graphKey,0,r.llen(graphKey)))[-20:],
        values = filter(lambda value : datetime.now().timestamp() - float(value.split(":")[0]) < 1800 ,values) # filter old values
        values = [[float(value.split(":")[0]) for value in values], [int(value.split(":")[1]) for value in values]]
        packet["graphs"][graphKey] = values
    return jsonify(packet) 

    

@app.route("/monitoring/latestValues")
def monitoringLatestValues():
    """
    Returns : a stream  from a "eventbuilder" module with various monitoring data

    """
    def getValues():
        keys = ["RunStart","RunNumber",
                "Events_received_Physics","Event_rate_Physics",
                "Events_received_Calibration", "Event_rate_Calibration",
                "Events_received_TLBMonitoring","Event_rate_TLBMonitoring"]
        graphKeys = [
                "Event_rate_Physics",
                "Event_rate_Calibration",
                "Event_rate_TLBMonitoring"]
        # getting the eventbuilder module 
        #NOTE The next line avoids the need to hardcode the name of the module, but it still needs to have eventbuilder in its name.  
        eventBuilderName = r.keys("eventbuilder*")[0] # should only be one eventbuilder module

        d1 = []
        while True :
            packet = {}
            data_results = [int(float(metric.split(":")[1])) for metric in r.hmget(eventBuilderName, keys )]
            graph_data =  r.hmget(eventBuilderName, graphKeys)
            if (d1 != data_results + graph_data): # if data has changed           
                data = dict(zip(keys,data_results))
                data["RunStart"] = datetime.fromtimestamp(float(data["RunStart"])).strftime('%d/%m %H:%M:%S')  if data["RunStart"] != 0 else "-"
                packet["values"] = data
                packet["graphs"] = {key : [float(value.split(":")[0]),int(value.split(":")[1])] for (key,value) in zip(graphKeys,graph_data)}
                d1 = data_results + graph_data
                yield f"data:{json.dumps(packet)}\n\n"
                d1 = data_results
            socketio.sleep(1) # NOTE:Can be configurable
    return Response(getValues(), mimetype="text/event-stream")


@app.route("/logURL", methods=["GET"])
def returnLogURL():
    module = request.args.get("module")
    group =  r2.get("group")
    url = f"http://{platform.node()}:9001/logtail/{group}:{module}"
    return jsonify(url)

@socketio.on_error()        # Handles the default namespace
def error_handler(e):
    print(e)


if __name__ == "__main__":
    if len(sys.argv) != 1 :
        if sys.argv[1] == "-l":
            print("Running in local mode - no run number InfluxDB archiving")
            localOnly = True

    # exit_event = threading.Event()
    # signal.signal(signal.SIGINT, lambda signum,frame : exit_event.set())

    print(f"Connect with the browser to http://{platform.node()}:5000")
    metrics=metricsHandler.Metrics(app.logger)
    socketio.start_background_task(stateChecker)
    # socketio.start_background_task(mH, app.logger)
    socketio.run(app, host="0.0.0.0", port=5000, debug=True, use_reloader = False)

    print("Stopping...")
    metrics.stop()

