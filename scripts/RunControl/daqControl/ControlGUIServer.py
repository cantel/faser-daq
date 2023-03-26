#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

import subprocess
from flask import Flask, make_response,redirect,url_for,render_template,request,session,jsonify,Response
from datetime import timedelta
import redis
import sys, os, time
import json
import logging
import jsonref
from functools import wraps, update_wrapper
from logging.handlers import RotatingFileHandler
from os import environ as env
from anytree import RenderTree
from anytree.search import find_by_attr
from anytree.importer import DictImporter
from flask_socketio import SocketIO
from keycloak import Client
from datetime import datetime
from copy import deepcopy
from pathlib import Path
import platform
from flask_cors import CORS
from nodetree import NodeTree
from helpers import detectorList, getEventCounts
from daqcontrol import daqcontrol as daqctrl
import metricsHandler
import requests
import socket
import urllib3
from mattermostNotifier import MattermostNotifier
from glob import glob

sys.path.append("../Sequencer")
import sequencer

# patching the original function from NodeTree class from daqLing
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

# monkey-patching the function :
NodeTree.childrenStateChecker = childrenStateChecker


## reading server configuration
serverConfig = {}
with open(os.path.join(os.path.dirname(__file__), 'serverconfiguration.json')) as f:
    serverConfig = json.load(f)

LOGOUT_URL = "https://auth.cern.ch/auth/realms/cern/protocol/openid-connect/logout"
configPath = os.path.join(env["DAQ_CONFIG_DIR"])

# for run service
run_user="FASER"
run_pw="HelloThere"

mattermost_hook=""
influxDB=None
hostname=socket.gethostname()
if os.access("/etc/faser-secrets.json",os.R_OK):
    influxDB=json.load(open("/etc/faser-secrets.json"))
    mattermost_hook=influxDB.get("MATTERMOST","")
    urllib3.disable_warnings()


app = Flask(__name__)
cors = CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")
CONFIG_PATH =  ""
CONFIG_DICT = {}
configTree = None
sysConfig : NodeTree = None # the treeObject for the configuration

PORT = 5000
localOnly = False
testMode = False

# redis
r = redis.Redis(host='localhost', port=6379, db=0,charset="utf-8", decode_responses=True)
r2 = redis.Redis(host='localhost', port=6379, db=3,charset="utf-8", decode_responses=True)

# creating redis default keys 
r2.setnx("runState","DOWN") # create key runState with value DOWN if it doesn't exist
r2.setnx("runOngoing",0) # 
r2.setnx("loadedConfig","")
r2.setnx("errorsM","")
r2.setnx("crashedM","")

# configuring flask server
handler = RotatingFileHandler(serverConfig["serverlog_location_name"], maxBytes=1000000, backupCount=0)
logging.root.setLevel(logging.NOTSET)
handler.setLevel(logging.NOTSET)
app.logger.addHandler(handler)

keycloak_client = Client()
app.secret_key = os.urandom(24)
app.config["PERMANENT_SESSION_LIFETIME"] = timedelta(minutes=serverConfig["timeout_session_expiration_mins"])


def refreshConfig():
    global CONFIG_PATH
    systemConfiguration(CONFIG_PATH)
    socketio.emit("refreshConfig")


def systemConfiguration(configPath):
    global sysConfig
    with open(os.path.join(configPath, "config-dict.json")) as f:
        configJson = json.load(f)
    if sysConfig is None:  # we (re)booted the app 
        print("sysConfig was None")
        try: 
            sysConfig = reinitTree(configJson)
        except Exception as e:
            logAndEmit("general", "ERROR", str(e))
    else : # we changed config
        print("sysConfig is not None and we changed config")
        sysConfig = reinitTree(configJson=configJson, oldRoot=sysConfig)

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

    with open(os.path.join(CONFIG_PATH, configJson["tree"])) as f:
        tree = json.load(f)

    try:
        with open(os.path.join(CONFIG_PATH, configJson["fsm_rules"])) as f:
            fsm_rules = json.load(f)
    except:
        raise Exception("FSM configuration not found")

    state_action = fsm_rules["fsm"]  # allowed action if in state "booted", etc...
    order_rules = fsm_rules["order"]  # order of starting and stopping

    if r2.get("runOngoing") == "1":
        print("Loading from redis")
        configuration = json.loads(r2.get("config"))
    else : 
        with open(os.path.join(CONFIG_PATH, configJson["config"])) as f:
            base_dir_uri = (Path(configPath).as_uri() + "/")
            jsonref_obj = jsonref.load(f, base_uri=base_dir_uri, loader=jsonref.JsonLoader())
 
        if ("configuration" in jsonref_obj):  # faser configuration files have "configuration" but not demo-tree
            # schema with references (version >= 10)
            configuration = deepcopy(jsonref_obj)["configuration"]
        else:
            # old-style schema (version < 10)
            configuration = jsonref_obj
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
    """
    Executes node commands 
    """
    r = ""
    configName = r2.get("loadedConfig")
    logAndEmit(configName,"INFO","User "+ session["user"]["cern_upn"]+ " has sent command "+ action+ " on node "+ ctrl,)
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


def message(msg):
    """
    Send a message to mattermost. 
    """
    if mattermost_hook: 
        try:
            req = requests.post(mattermost_hook,json={"text": msg})
            if req.status_code!=200:
                print("Failed to post message below. Error code:",req.status_code)
                print(msg)
        except Exception as e:
            print("Got exception when posting message",e)
    else:
        print(msg)



def executeCommROOT(action:str, reqDict:dict):
    """
    Execute actions on ROOT node (general commands) and extend the interlock for another <timeout> seconds 
    """
    global sysConfig
    configName =  r2.get("loadedConfig")
    if reqDict.get("bot") :
        ...
    else : 
        r2.expire("whoInterlocked", serverConfig["timeout_interlock_secs"])
        logAndEmit(configName,"INFO","User " + session["user"]["cern_upn"] + " has sent ROOT command " + action)
    setTransitionFlag(1)
    if action == "INITIALISE":
        refreshConfig()
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
            done = waitUntilCorrectState(nextState, serverConfig["timeout_rootCommands_secs"][action])
            if not done: 
                break  # if one step doesn't reach the correct state, end command loop. 

    elif action == "START":
        runNumber = 1000000000 # run number for local run
        runType = reqDict.get("runType","")
        runComment = reqDict.get("runComment","Test")[:500]
        if not localOnly:
            try :
                runNumber = send_start_to_runservice(runType=runType, runComment=runComment, reqDict=reqDict)
                runNumber = int(runNumber)
            except ValueError : # if runNumber is a error message
                return runNumber 
            
        # updating redis database        
        r2.set("runType", runType)
        r2.set("runComment", runComment)
        r2.set("runStart", time.time())
        r2.set("runNumber", runNumber)

        r=sysConfig.executeAction("start",runNumber)
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
        done = waitUntilCorrectState("running", serverConfig["timeout_rootCommands_secs"][action])

    elif action == "STOP":
        runComment = reqDict.get("runComment","Test")[:500]
        runNumber = r2.get("runNumber")
        runType = reqDict.get("runType","") 
        r2.set("runComment", runComment)
        r2.set("runType", runType)
        if not localOnly:
            send_stop_to_runservice(runComment=runComment, runNumber=runNumber, runType=runType)
        try : 
            r=sysConfig.executeAction("stop")
            logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
            done = waitUntilCorrectState("ready", serverConfig["timeout_rootCommands_secs"][action])
        except ConnectionRefusedError:
            setTransitionFlag(0)
            return "Error: connection refused"
            
    elif action == "SHUTDOWN":
        steps = [("unconfigure", "booted"), ("remove", "not_added")]
        
        if r2.get("runState") != "READY" and not localOnly:
            runComment = "Run was shutdown"
            runType = r2.get("runType")
            runNumber = r2.get("runNumber")
            r2.set("runComment", runComment)
            if not localOnly : 
                send_stop_to_runservice(runComment=runComment, runType=runType, runNumber=runNumber, forceShutdown=True)
            
        for step,nextState in steps : 
            r = sysConfig.executeAction(step)
            logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
            done = waitUntilCorrectState(nextState, serverConfig["timeout_rootCommands_secs"][action])
    
        if not done : 
            print("Force shutdown")
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
        rootState = sysConfig.getState()
        if rootState == "running" : state="RUN"
        elif rootState == "not_added" : state="DOWN"; cleanErrors()
        elif rootState == "ready" : state="READY"
        elif rootState == "paused" : state="PAUSED"
        socketio.emit("runStateChng",state , broadcast=True)
        updateRedis(status=state)
        return "Error: The command took to long"
    return "Success"

def post_influxDB_mattermost(action:str,configName,runComment,runType, runNumber, physicsEvents=None):
    """
    Post start, stop and shutdown information on influxDB and (combined run), post a message on mattermost
    """
    runData =""
    
    if action=="start":
        runData=f'runStatus,host={hostname} state="Started",comment="{runComment}",runType="{runType}",runNumber={runNumber}'
    elif action =="stop":
        runData=f'runStatus,host={hostname} state="Stopped",comment="{runComment}",runType="{runType}",runNumber={runNumber},physicsEvents={physicsEvents}'
    elif action == "shutdown":
        runData=f'runStatus,host={hostname} state="Shutdown",comment="Run was shutdown",runNumber={runNumber},physicsEvents={physicsEvents}'
        
    r=requests.post(f'https://dbod-faser-influx-prod.cern.ch:8080/write?db={influxDB["INFLUXDB"]}',
                        auth=(influxDB["INFLUXUSER"],influxDB["INFLUXPW"]),
                        data=runData,
                        verify=False)
    if r.status_code!=204:
        logAndEmit("Failed to post end of run information to influxdb: "+r.text)
        sendInfoToSnackBar("error","Failed to post end of run information to influxdb: "+r.text )
    if configName.startswith("combined"):
        if action == "stop" : message(f"Run {runNumber} stopped\nOperator: {runComment}")
        elif action == "start" : message(f"Run {runNumber} was started\nOperator: {runComment}")
        elif action == "shutdown" : message(f"Run {runNumber} was shutdown ")


def send_start_to_runservice(runType:str,runComment:str, reqDict:dict) -> int:
    """
    Parameters
    ----------
    - runType : type of the run
    - runComment : run comment
    - reqDict : dictionnary of all the run service-related info    
    
    Return
    --------
    - Run Number provided by the run service
    """
    version=subprocess.check_output(["git","rev-parse","HEAD"]).decode("utf-8").strip()
    seqnumber = reqDict.get("seqnumber",None)
    seqstep = reqDict.get("seqstep",0)
    seqsubstep = reqDict.get("seqsubstep",0)
    config = json.loads(r2.get("config"))
    detList = r2.get("detList")
    configName = r2.get("loadedConfig")
    runNumber = None
    
    rsURL =  "http://faser-runnumber.web.cern.ch/NewRunNumber"
    if testMode : 
        rsURL = "http://faser-daq-001.cern.ch:5000/NewRunNumber"
    try : 
        res = requests.post(rsURL,auth=(run_user,run_pw),
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
    print("Fonctionne")
    if influxDB : 
        post_influxDB_mattermost("start", configName, runComment, runType, runNumber)
    return runNumber

def send_stop_to_runservice(runComment, runType, runNumber, forceShutdown=False):
    """
    Parameters
    ----------
    - runType : type of the run
    - runComment : run comment
    - runNumber : run number
    - forceShutdown : if the function is used for a "SHUTDOWN" root command.    
    """
    runinfo = {}
    configName = r2.get("loadedConfig")
    runinfo["eventCounts"] = getEventCounts()
    physicsEvents = runinfo["eventCounts"]["Events_sent_Physics"] # for influxDB
    
    if forceShutdown :
        runComment = "Run was shut down"
    stopMsg = {
                "endcomment" :runComment,
                "type": runType,
                "runinfo": runinfo
            }
    rsURL = f'http://faser-runnumber.web.cern.ch/AddRunInfo/{runNumber}'
    if testMode:
        rsURL = f"http://faser-daq-001.cern.ch:5000/AddRunInfo/{runNumber}"
    try : 
        res = requests.post(rsURL,
                auth=(run_user,run_pw),
                json = stopMsg)

        if res.status_code != 200 :
            logAndEmit("general", "ERROR", "Failed to register end of run information: "+r.text )
            sendInfoToSnackBar( "error", "Failed to register end of run information: "+r.text)

    except requests.exceptions.ConnectionError:
        logAndEmit("general","Error","Could not connect to run service")
        sendInfoToSnackBar( "error","Could not connect to run service")
    
    if influxDB:
        if forceShutdown : 
            post_influxDB_mattermost("shutdown", configName, runComment, runType, runNumber, physicsEvents)  
        else :
            post_influxDB_mattermost("stop", configName, runComment, runType, runNumber, physicsEvents)
 

def sendInfoToSnackBar(typeM:str, message:str):
    """
    Sends a message to the client with message <message> with a color specified by typeM
    """
    valid_types = {"error", "info", "success"}
    if typeM not in valid_types:
        raise ValueError(f"Wrong type specified: {typeM} is not a valid type ")
    socketio.emit("snackbarEmit", {"message": message, "type": typeM})



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
    listState = {}
    if locRoot:
        for _ , _, node in RenderTree(locRoot):
            listState[node.name] = [
                find_by_attr(locRoot, node.name).getState(),
                find_by_attr(locRoot, node.name).inconsistent,
                find_by_attr(locRoot, node.name).included,
            ]

    return listState


def get_crashed_modules():
    crashedModules = []
    for _,_,node in RenderTree(sysConfig):
        a = find_by_attr(sysConfig, node.name).getState()
        if isinstance(a,list):
            if a[0] == "added":
                crashedModules.append(node.name)
    return crashedModules


def stateChecker():
    global CONFIG_DICT, CONFIG_PATH
    # nodesState = TrackChange(initial_values={})
    # transitionFlag = TrackChange(initial_values=r2.get("transitionFlag"))
    # crashedMods = TrackChange(initial_values={})
    # errorsMods = TrackChange(initial_values={})
    # runInfos = TrackChange(initial_values={"runType":"", "runComment":"", "runNumber":None})
    # lockState = TrackChange(initial_values=None)
    # loadedConf = TrackChange(initial_values="")
    l1 = {}
    l2 = {}
    errors = {}
    transitionFlag1 = r2.get("transitionFlag")
    runInfo1= {"runType":"", "runComment":"", "runNumber":None}
    crashedM1 = {}
    loadedConfig =""
    lockState = None
    loaded = False
    mattNotif_crash = MattermostNotifier(mattermost_hook if influxDB else None,
                                         ":warning: Module __{}__ has CRASHED",
                                         time_interval=serverConfig["persistent_notification_delay"],
                                         okAlerts=serverConfig["ok_alerts"])
    mattNotif_error = MattermostNotifier(mattermost_hook if influxDB else None,
                                         ":warning: Module __{}__ is in ERROR STATE",
                                         time_interval=serverConfig["persistent_notification_delay"],
                                         okAlerts=serverConfig["ok_alerts"])
    
    while True:
        ########### Config Change ############
        loadedConfig2 = r2.get("loadedConfig")
        if loadedConfig2 != loadedConfig :
            socketio.emit("configChng", loadedConfig2, broadcast = True)
            loadedConfig = loadedConfig2

        ############# States ###############
        transitionFlag2 = r2.get("transtionFlag")
        if sysConfig:
            l2 = getStatesList(sysConfig)
            if (l2 != l1) or (transitionFlag2 != transitionFlag1):
                socketio.emit("stsChng", l2, broadcast=True)
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
                transitionFlag1 = transitionFlag2

            errors2 = modulesWithError(sysConfig)
            if r2.get("runState") == "RUN":
                mattNotif_error.check(errors2["2"])
            if errors2 != errors:
                socketio.emit("errorModChng", errors2, broadcast =True)
                r2.set("errorsM", json.dumps(errors2) )
                errors = errors2
            
            
            crashedM2 =  get_crashed_modules()
            if r2.get("runState") == "RUN" or r2.get("runState") == "READY":
                mattNotif_crash.check(crashedM2)
                if  crashedM2 != crashedM1 :
                    if r2.get("transitionFlag") == "0":
                        socketio.emit("crashModChng",crashedM2, broadcast=True)
                    r2.set("crashedM", json.dumps(crashedM2))
                    crashedM1 = crashedM2

        elif r2.get("loadedConfig") and not loaded: 
            print("loading the config") 
            CONFIG_PATH = os.path.join(env["DAQ_CONFIG_DIR"], r2.get("loadedConfig"))
            with open(os.path.join(CONFIG_PATH, "config-dict.json")) as f:
                CONFIG_DICT = json.load(f)
            systemConfiguration(CONFIG_PATH)
            loaded = True 

        ####### change of lock State ###########
        lockState2 = r2.get("whoInterlocked")
        if lockState2 !=lockState:
            socketio.emit("interlockChng", lockState2, broadcast = True)
            if not lockState2:
                logAndEmit(loadedConfig2, "INFO", "Interlock has been released because of TIMEOUT")
            lockState = lockState2

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
    r2.delete("errorsM")
    socketio.emit("errorModChng", {"1":[],"2":[]} , broadcast=True)


def modulesWithError(tree):
    errorList = {"1":[],"2":[]}
    for _,_, node in RenderTree(tree):
        if not node.children : # if the node has no children -> not a category
            if r.hget(node.name, "Status"):
                status =  r.hget(node.name, "Status").split(":")[1] # returns 0, 1 or 2 (1 and 2 -> warning and error)
                if status != "0" :
                    errorList[status].append(node.parent.name)
                    errorList[status].append(node.name) 
    errorList["1"] =  list(set(errorList["1"]))
    errorList["2"] = list(set(errorList["2"]))
    return errorList

            
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
    
    errors = r2.get("errorsM")
    packet["errors"] = json.loads(errors) if errors else {'1':[],'2':[]}

    crashedModules = r2.get("crashedM")
    packet["crashedM"] = json.loads(crashedModules) if (crashedModules!="" or crashedModules) else []
    packet["localOnly"] = localOnly
    packet["runStart"] = r2.get("runStart")
    packet["sequencerState"] = r2.hgetall("sequencerState")
    print(r2.hgetall("sequencerState"))
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
    Returns the full log. (Works only if the logs are on the same server)
    """
    path = getLogPath(module)
    with open(path, "r") as f:
        log = f.readlines()
    return Response(log, mimetype='text/plain')


@app.route("/log", methods=["GET"])
def log():
    with open(serverConfig["serverlog_location_name"]) as f:
        ret = f.readlines()
    return jsonify(ret[-20:]) # return the last 20 element of the log


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
            return jsonify("unlocked file")

        else: # someone else try to unlock file         
            logAndEmit(r2.get("loadedConfig"),"INFO","User "+ session["user"]["cern_upn"]+ "  ATTEMPTED to take control of configuration "+ r2.get("loadedConfig")+f"from {r2.get('whoInterlocked')}",)
            return jsonify("you can't")


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
    return redirect(f"{LOGOUT_URL}?redirect_uri={url_for('index', _external=True)}")


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


@app.route("/configDirs", methods=["GET", "POST"])
def configDirs():
    return jsonify(getConfigsInDir(configPath))


@app.route("/initConfig", methods=["GET", "POST"])
def initConfig():
    global CONFIG_PATH,CONFIG_DICT
    configName  =  request.args.get("configName")
    if configName == r2.get("loadedConfig") and sysConfig is not None :
        print("Do nothing")
    else :             
        CONFIG_PATH = os.path.join(env["DAQ_CONFIG_DIR"], configName)
        if request.args.get("bot") is None: 
            logAndEmit(CONFIG_PATH,"INFO","User "+ session["user"]["cern_upn"] + " has switched to configuration "+ configName,)
        try : 

            with open(os.path.join(CONFIG_PATH, "config-dict.json")) as f:
                CONFIG_DICT = json.load(f)
            systemConfiguration(CONFIG_PATH)
            r2.set("loadedConfig", configName)
        except Exception as e:
            print("Exception when trying to change configuration file : ", e)
            return jsonify(f"Error: {e}")
    return jsonify("Success")


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
        # overwriting the default callback_uri : 
        # Can be for example : http://faser-daqvm-000:5000/login/callback
        #                      http://localhost:1234/login/callback (for port forwarding and only port 1234)
        keycloak_client.callback_uri = request.url_root + "login/callback"
        auth_url, state = keycloak_client.login()
            
        session["state"] = state
        return redirect(auth_url)


@app.route("/login/callback", methods=["GET"])
def login_callback():
    state = request.args.get("state", "unknown")
    _state = session.pop("state", None)
    if state != _state:
        return render_template("invalidState.html", url=f"http://{platform.node()}.cern.ch:{PORT}")
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
    return jsonify(getInfo(module))


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
    The function could be optimized
    """
    packet = {"values": {}, "graphs":{}}
    keys = ["RunStart","RunNumber",
            "Events_received_Physics","Event_rate_Physics",
            "Events_received_Calibration", "Event_rate_Calibration",
            "Events_received_TLBMonitoring","Event_rate_TLBMonitoring"]

    #The next line avoids the need to hardcode the name of the EventBuilder module, but it still needs to have eventbuilder in the name.  
    eventBuilderName = r.keys("eventbuilder*")[0]
    results = [int(float(metric.split(":")[1])) for metric in r.hmget(eventBuilderName, keys )]
    data = dict(zip(keys,results))
    data["RunStart"] = datetime.fromtimestamp(float(data["RunStart"])).strftime('%d/%m %H:%M:%S')  if data["RunStart"] != 0 else "-"
    packet["values"] = data
    
    # for the graphs
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
    Returns : a stream  from the EventBuilder module with various monitoring data
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
        # The next line avoids the need to hardcode the name of the module, but it still needs to have eventbuilder in the name.  
        eventBuilderName = r.keys("eventbuilder*")[0]

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
            socketio.sleep(1) # Could : could be configurable
    return Response(getValues(), mimetype="text/event-stream")


@app.route("/logURL", methods=["GET"])
def returnLogURL():
    module = request.args.get("module")
    group =  r2.get("group")
    url = f"http://{platform.node()}:9001/logtail/{group}:{module}"
    return jsonify(url)

# ---------------- Sequencer -------------- # 

@app.route("/getSequencerConfigs", methods =["GET"])
def getSequencerConfigs():
    files = sorted(glob(os.path.join(env["DAQ_SEQUENCE_CONFIG_DIR"],"*.json")))
    # keeping only the filename instead of full path
    files = [os.path.basename(file) for file in files]
    return jsonify(files) 


@app.route("/loadSequencerConfig", methods=["GET"])
def loadSequencerConfig():
    sequencerConfigName = request.args.get("sequencerConfigName") 
    response = {}
    try : 
        config, steps = sequencer.load_steps(sequencerConfigName)
    except (RuntimeError, FileNotFoundError, KeyError)  as err : 
        response["status"] = "error"
        response["data"] = repr(err)
    except json.decoder.JSONDecodeError :
        response["status"] = "error"
        response["data"] = "An error occured when parsing the JSON file."
    else : 
        response["status"] = "success"
        response["data"] = {"config": config, "steps" : steps}
    return jsonify(response)


@app.route("/startSequencer", methods=["POST"])
def startSequencer() : 
   requestData = request.get_json() 
   configName = requestData["configName"]
   startStep = requestData["startStep"]
   seqNumber = requestData["seqNumber"]
   # FIXME: The sequence Numbrer should be given by the runNumber
   argsSequencer = ['-S', seqNumber,        # sequence Number
                    '-s', startStep, # startStep
                    configName]
   socketio.start_background_task(sequencer.main, argsSequencer,socketio)
   return jsonify({"status" : "success"})

    
    
    
@socketio.on_error()
def error_handler(e):
    """ Handles the default namespace """
    print(e)


if __name__ == "__main__":
    if len(sys.argv) != 1 :
        if sys.argv[1] == "-l": # local mode
            print("Running in local mode - no run number InfluxDB archiving")
            localOnly = True
        elif sys.argv[1] == "-t" : #  test mode
            print("Running in test mode (using the test run service server")
            testMode = True
    else:
        print("Running in production mode")

    print(f"Connect with the browser to http://{platform.node()}:{PORT}")
    metrics=metricsHandler.Metrics(app.logger)
    socketio.start_background_task(stateChecker)
    socketio.run(app, host="0.0.0.0", port=PORT, debug=True, use_reloader = False)
    print("Stopping...")
    metrics.stop()

