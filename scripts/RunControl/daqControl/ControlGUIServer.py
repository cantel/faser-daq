from cmath import exp
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

maxTransitionTime = 30 # TODO: have to be in the configuration file (has to be more)


sys.path.append(os.path.join(os.path.dirname(__file__), "..", "..", "Control"))
from nodetree import NodeTree
from daqcontrol import daqcontrol as daqctrl
from flask_cors import CORS
import metricsHandler

configPath = os.path.join(env["DAQ_CONFIG_DIR"])

with open(os.path.join(os.path.dirname(__file__), "serverconfiguration.json")) as f:
    serverConfigJson = json.load(f)

app = Flask(__name__)
cors = CORS(app)
socketio = SocketIO(app, cors_allowed_origins="*")
thread_lock = threading.Lock()
thread = None
sysConf = {}
root = threading.local()
mainRoot = None
ALLOWED_EXTENSIONS = set(serverConfigJson["ALLOWED_EXTENSIONS"])
whoInterlocked = {}
whoInterlocked[""] = [None, 0]
TIMEOUT = serverConfigJson["timeout_for_requests_secs"]
LOGOUT_URL = serverConfigJson["LOGOUT_URL"]
CONFIG_PATH =  ""
CONFIG_DICT = {}
configTree = None

localOnly = False

r = redis.Redis(host='localhost', port=6379, db=0,charset="utf-8", decode_responses=True)

r2 = redis.Redis(host='localhost', port=6379, db=3,charset="utf-8", decode_responses=True)


r2.setnx("runState","DOWN") # create key runState with value DOWN if it doesn't exist
r2.setnx("runOngoing",0) # 
r2.setnx("runningFile","") 



def systemConfiguration(configName, configPath):
    global sysConf
    with open(os.path.join(configPath, "config-dict.json")) as f:
        configJson = json.load(f)
    if configName not in sysConf:
        try:
            sysConf[configName] = reinitTree(configJson)
        except Exception as e:
            logAndEmit("general", "ERROR", str(e))


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
    print("try reinit tree")
    if oldRoot != None:
        for pre, _, node in RenderTree(oldRoot):
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
        # base_dir_uri = Path(session['configPath']).as_uri() + '/' # file:///home/egalanta/latest/faser-daq/configs/demo-tree/
        base_dir_uri = (
            Path(configPath).as_uri() + "/")  # file:///home/egalanta/latest/faser-daq/configs/demo-tree/
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

    for pre, _, node in RenderTree(newRoot):
        for c in components:
            if node.name == c["name"]:
                node.configure(order_rules,state_action,pconf=c,exe=exe,dc=dc,dir=dir,lib_path=lib_path,)
            else:
                node.configure(order_rules, state_action)  # only loads order_rules and state_actions for that node

        node.startStateCheckers()
    return newRoot


def executeComm(ctrl, action):
    global sysConf,currentConfig
    r = ""
    configName = r2.get("runningFile")

    logAndEmit(
        configName,"INFO","User "+ session["user"]["cern_upn"]+ " has sent command "+ action+ " on node "+ ctrl,)
    try:
        if action == "exclude":
            r = find_by_attr(sysConf[configName], ctrl).exclude()
        elif action == "include":
            r = find_by_attr(sysConf[configName], ctrl).include()
        else:
            r = find_by_attr(sysConf[configName], ctrl).executeAction(action)
    except Exception as e:
        logAndEmit(configName, "ERROR", ctrl + ": " + str(e))
    if r != "":
        logAndEmit(configName, "INFO", ctrl + ": " + str(r))
    return r


def executeCommROOT(action:str):
    """
    Execute actions on ROOT node (general commands) and extend the interlock for another <timeout> seconds 
    Returns : location of the module specific log files 
    """
    global sysConf
    r2.expire("whoInterlocked", 60) #TODO: Should be configurable
    configName =  r2.get("runningFile")
    logAndEmit(configName,"INFO","User " + session["user"]["cern_upn"] + " has sent ROOT command " + action)
    setTransitionFlag(1)
    if action == "INITIALISE":
        steps = [("add", "booted"), ("configure","ready")]
        for step,nextState in steps :
            r = sysConf[configName].executeAction(step)
            logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))

            if step == "add" and r[0] != "Action not allowed" :
                logPaths = listLogs(r)
                r2.delete("log")
                r2.sadd("log", *logPaths)
            done = waitUntilCorrectState(nextState, 30 ) # 10 seconds for each steps 
            if not done: 

                break  # one step doesn't reach the correct state, end command loop. 

    elif action == "START": 
        r=sysConf[configName].executeAction("start")
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))

        done = waitUntilCorrectState("running", 30)

    elif action == "STOP": 
        r=sysConf[configName].executeAction("stop")
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))

        done = waitUntilCorrectState("ready", 30)

    elif action == "SHUTDOWN":
        steps = [("unconfigure", "booted"), ("shutdown","added"), ("remove", "not_added")]
        for step,nextState in steps :
            r = sysConf[configName].executeAction(step)
            logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
            done = waitUntilCorrectState(nextState, 1)
        if not done : 
            # if timeout, force remove all the modules 
            print("Force shutdown")
            r = sysConf[configName].executeAction("remove")
            done = True
        # real shutdown -> function ? 
        updateRedis(file=r2.get("runningFile"),status="DOWN")
        socketio.emit("runStateChng",{'runState': "DOWN", 'file': r2.get("runningFile")} , broadcast=True)


    elif action == "PAUSE":
        r=sysConf[configName].executeAction("disableTrigger")
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
        done = waitUntilCorrectState("paused", 30)
    
    elif action == "RESUME":
        r=sysConf[configName].executeAction("enableTrigger")
        if "Action not allowed" in r:
            r=sysConf[configName].executeAction("resume")

            
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
        done = waitUntilCorrectState("running", 30)
    
    elif action == "ECR":
        r=sysConf[configName].executeAction("ECR")
        logAndEmit(configName, "INFO", "ROOT" + ": " + str(r))
        done = waitUntilCorrectState("paused", 30)
    




    
    setTransitionFlag(0)
    if not done :
        return "ERROR: TIMEOUT"
    return "success" # success 

def setTransitionFlag(flag:int):
    r2.set("transitionFlag", flag)

def waitUntilCorrectState(state:str, timeout=30):
    """
    The function will wait until the state <state> matches the root node state while being consistant. 
    Returns True if the transition succeded, False if it reached timeout.
    Default value for timeout is 30 seconds.  
    """
    global sysConf
    now = time.time()
    configName = r2.get("runningFile")
    while (time.time()-now < timeout):
        if (sysConf[configName].getState() == state)  and  (not sysConf[configName].inconsistent):
            return True
        time.sleep(0.5)
    return False

def listLogs(r):
    """
    Transforms a list of tuples to a list of string with all the log paths
    """
    final = []
    for sublist in r:
        if isinstance(sublist, tuple): final.append(sublist[1])
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
    global whoInterlocked
    for pre, _, node in RenderTree(locRoot):
        list[node.name] = [
            find_by_attr(locRoot, node.name).getState(),
            find_by_attr(locRoot, node.name).inconsistent,
            find_by_attr(locRoot, node.name).included,
        ]
    return list

def printTree(root):
    print(RenderTree(root, style=AsciiStyle()))


def stateChecker():
    # global whoInterlocked
    l1 = {}
    l2 = {}
    # whoValue = {}
    rState1 = {}
    rState2 = {}
    runningFile =""
    lockState = None
    errors = []

    while True:
        for file in sysConf:
            l2[file] = getStatesList(sysConf[file])
            rState2[file] = [sysConf[file].getState(), sysConf[file].inconsistent]
            try:
                if l1[file] != l2[file]:
                    socketio.emit(f"stsChng{file}", l2[file], broadcast=True)
                    # if l1[file]['Root'][0] != l2[file]

            except KeyError:
                print(f"File: {file} not registred")
                socketio.emit(f"stsChng{file}", l2[file], broadcast=True)

            l1[file] = l2[file]
            
            try : 
                if (rState2[file] != rState1[file]) and (rState2[file][1] == False ) and (rState2[file][0] not in ['booted','added']):
                    printTree(sysConf[file])
                    if rState2[file][0] == 'running' : state="RUN"
                    elif rState2[file][0] == 'not_added':
                        state = "DOWN"
                        cleanRedis()
                    elif rState2[file][0] == 'ready': state = "READY"
                    elif rState2[file][0] == 'paused' : state = "PAUSED"
                    updateRedis(file=file,status=state)
                    logAndEmit(file,"INFO",f"ROOT element is now in state {state}")
                    socketio.emit("runStateChng",{'runState': state, 'file': file} , broadcast=True)

                #intentional inconsistent (transition state)
                elif (rState2[file] != rState1[file]) and (rState2[file][1] == True ) and (r2.get("transitionFlag")== "1"):
                    printTree(sysConf[file])
                    state = "IN TRANSITION"
                    updateRedis(file=file,status=state)
                    socketio.emit("runStateChng",{'runState': state, 'file': file} , broadcast=True)

                # elif (rState2[file] != rState1[file]) and (rState2[file][1] == True ) and (r2.get("transitionFlag")== "0"):
                #     print("CRASH")
                #     # modulesCrashed = modulesWithError(sysConf[file])
                #     # socketio.emit("errorModChng", modulesCrashed , broadcast=True)
                #     r2.set("modulesErrors", str(modulesCrashed))


            except KeyError : 
                print(f"rState: {file} not registred")
            rState1[file] = rState2[file]

        runningFile2 = r2.get("runningFile")
        if runningFile2 != runningFile :
            socketio.emit("configChng", runningFile2, broadcast = True)
            runningFile = runningFile2

        lockState2 = r2.get("whoInterlocked")
        if lockState2 !=lockState:
            socketio.emit("interlockChng", lockState2, broadcast = True)
            if not lockState2:
                logAndEmit(runningFile2, "INFO", "Interlock has been released because of TIMEOUT")
            lockState = lockState2

        if runningFile2 and runningFile2 in sysConf:
            errors2 = modulesWithError(sysConf[runningFile2])
            if errors2 != errors:
                socketio.emit("errorModChng", errors2, broadcast =True)
                r2.delete("modulesErrors")
                print(errors2)
                if len(errors2) != 0:
                    r2.sadd("modulesErrors", *errors2)
                errors = errors2
        time.sleep(0.5)


def cleanRedis():
    r2.delete("modulesErrors")
    socketio.emit("errorModChng", [] , broadcast=True)

     

def modulesWithError(tree):
    errorList = []
    rootNodeState = tree.getState()
    for _,_, node in RenderTree(tree):
        if not node.children : # if the node has no children -> not a category
            if r.hget(node.name, "Status"):
                status =  bool(int(r.hget(node.name, "Status").split(":")[1])) # returns 0, 1 or 2 (1 and 2 -> warning and error)
                if status != 0 :
                    errorList.append(node.parent.name)
                    errorList.append(node.name) 
    return list(set(errorList)) 

            
            
def logAndEmit(configtype, type, message):
    now = datetime.now()
    timestamp = now.strftime("%d/%m/%Y, %H:%M:%S")
    if type == "INFO":
        app.logger.info("[" + configtype + "] " + timestamp + " " + type + ": " + message)
    elif type == "WARNING":
        app.logger.warning("[" + configtype + "] " + timestamp + " " + type + ": " + message)
    elif type == "ERROR":
        app.logger.error("[" + configtype + "] " + timestamp + " " + type + ": " + message)
    socketio.emit("logChng","[" + configtype + "] " + timestamp + " " + type + ": " + message,broadcast=True,)


def updateRedis(file:str=None, status:str = None):
    if status:
        r2.set('runState', status) 
        if status == "DOWN":
            r2.set('runOngoing',0)
        else:
            r2.set('runOngoing',1)



keycloak_client = Client(callback_uri=serverConfigJson["callbackUri"])
app.secret_key = os.urandom(24)
app.config["PERMANENT_SESSION_LIFETIME"] = timedelta(minutes=serverConfigJson["timeout_session_expiration_mins"])


@app.route("/appState", methods=["GET"])
def appState():
    packet ={}
    packet["runOngoing"] = bool(int(r2.get("runOngoing")))
    packet["runningFile"] = r2.get('runningFile')
    packet["runState"] = r2.get("runState")
    packet["user"] = {"name":session["user"]["cern_upn"], "logged" : True if "cern_gid" in session["user"] else False }
    packet["whoInterlocked"] = r2.get("whoInterlocked")
    errors = r2.smembers("modulesErrors")
    packet["errors"] = list(errors) if errors else []
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
    if keys == []: # if module is not in the redis database
        return []
    keys.sort()
    vals = r.hmget(module, keys=keys)
    
    info = [{"key":key, "value": val.split(":")[1],"time": datetime.fromtimestamp(int(float(val.split(":")[0]))).strftime('%a %d/%m/%y %H:%M:%S ') } for key,val in zip(keys,vals)]
    return info
 
@app.route("/fullLog/<string:module>")
def fullLog(module:str):
    """
    Returns the full log for the current run #NOTE: Is it really just the current run ? 
    (Works only if the logs are on the same server)
    """
    path = getLogPath(module)
    with open(path, "r") as f:
        log = f.readlines()
    return Response(log, mimetype='text/plain')

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
    session["configName"] = ""
    session["configPath"] = ""
    session["configDist"] = ""
    logAndEmit("general", "INFO", "User connected: " + session["user"]["cern_upn"])
    return redirect(url_for('index'))


@app.route("/log", methods=["GET"])
def log():
    with open(serverConfigJson["serverlog_location_name"]) as f:
        ret = f.readlines()
    return jsonify(ret[-20:])


@app.route("/serverconfig")
def serverconfig():
    return jsonify(serverConfigJson)


@app.route("/interlock", methods=["POST"])
def interlock():
    global whoInterlocked
    requestData  = request.get_json()
    cern_upn = requestData["cern_upn"]
    action = requestData["action"] # lock or unlock 

    
    if action == "lock": 
        if r2.set("whoInterlocked", session["user"]["cern_upn"], ex=60,nx=True): 
            logAndEmit(r2.get("runningFile"),"INFO","User "+ session["user"]["cern_upn"]+ " has TAKEN control of configuration "+ r2.get("runningFile"),)
            return jsonify("File locked successfully")
        else : 
            # quel'qun est a déjà locked la config 
            logAndEmit(r2.get("runningFile"),"INFO","User "+ session["user"]["cern_upn"]+ " tried to take control configuration "+ r2.get("runningFile")+f"from {r2.get('whoInterlocked')}",)
            return jsonify(f"Sorry, the config is already locked by {r2.get('whoInterlocked')}")

    else: # action == "unlock"
        if session["user"]["cern_upn"] == r2.get("whoInterlocked"): # if is the right person
            r2.delete("whoInterlocked")            
            logAndEmit(r2.get("runningFile"),"INFO","User " + session["user"]["cern_upn"]+ " has RELEASED control of configuration"+ r2.get('runningFile'))
            return jsonify(f"unlocked file")

        else: # someone else try to unlock file         
            logAndEmit(r2.get("runningFile"),"INFO","User "+ session["user"]["cern_upn"]+ "  ATTEMPTED to take control of configuration "+ r2.get("runningFile")+f"from {r2.get('whoInterlocked')}",)
            return jsonify(f"you can't")

@app.route("/ajaxParse", methods=["POST"])
def ajaxParse():
    postData = request.get_json()
    node = postData["node"]
    command = postData["command"]
    configName =  r2.get("runningFile")
    r2.expire("whoInterlocked",maxTransitionTime) # adds a new timeout

    try:
        r = executeComm(node, command)
    except Exception as e:
        logAndEmit(configName, "ERROR", str(e))
    # time.sleep(TIMEOUT)
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
    print(session)
    try:
        with open(os.path.join(CONFIG_PATH, CONFIG_DICT["fsm_rules"])) as f:
            fsmRules = json.load(f)
    except Exception as e:
        raise e
    return jsonify(fsmRules["fsm"])


@app.route("/grafanaJson", methods=["GET", "POST"])
def grafanaJson():
    try:
        with open(
            os.path.join(CONFIG_PATH, CONFIG_DICT["grafana"])
        ) as f:
            grafanaConfig = json.load(f)
        f.close()
    except:
        return "error"
    return jsonify(grafanaConfig)


@socketio.on("connect", namespace="/")
def connect():
    print("Client connected")


@app.before_first_request
def startup():
    global thread
    # for file in getConfigsInDir(configPath):
        # whoInterlocked[file] = [None, 0]
    with thread_lock:
        if thread is None:
            thread = socketio.start_background_task(stateChecker)
    handler = RotatingFileHandler(serverConfigJson["serverlog_location_name"], maxBytes=1000000, backupCount=0)
    logging.root.setLevel(logging.NOTSET)
    handler.setLevel(logging.NOTSET)
    app.logger.addHandler(handler)


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
    logAndEmit(CONFIG_PATH,"INFO","User "+ session["user"]["cern_upn"] + " has switched to configuration "+ session["configName"],)
    with open(os.path.join(CONFIG_PATH, "config-dict.json")) as f:
        CONFIG_DICT = json.load(f)
        print(CONFIG_DICT)

    systemConfiguration(configName, CONFIG_PATH)
    r2.set("runningFile", configName)
    return "Success"


@app.route("/urlTreeJson")
def urlTreeJson():
    print("urlTreeJson function")
    # sessionStatus()
    try:
        with open(
            os.path.join(CONFIG_PATH, CONFIG_DICT["tree"]), "r"
        ) as file:
            return jsonify(json.load(file))
    except Exception as e:

        return "error"


@app.route("/login", methods=["GET", "POST"])
def login():
    if serverConfigJson["SSO_enabled"] == 0:
        session["user"]["cern_upn"] = "offlineUser"
        session["user"]["cern_gid"] = "12345"
        return redirect(url_for("index"))
    else : 
        auth_url, state = keycloak_client.login()
        session["state"] = state
        return redirect(auth_url)


@app.route("/")
def index():
    if "user" in session: # if user is already connected
        # return render_template('index.html', usr=session['user']['cern_upn'], wholocked=whoInterlocked[session['configName']][0], currConfigFile = session['configName'], statesGraphics=serverConfigJson['states'], displayName=serverConfigJson['displayedName'] )
        return render_template("index.html", usr=session["user"]["cern_upn"])
    return redirect(url_for('localLogin'))


@app.route("/localLogin")
def localLogin():
    session["user"] = {}
    session["user"]["cern_upn"] = "local_user"
    session["configName"] = ""
    session["configPath"] = ""
    session["configDict"] = ""
    #logAndEmit("","INFO","User " + session["user"]["cern_upn"] + " connected ")
    return redirect(url_for('index'))



def sessionStatus():
    print("sysConf", sysConf)
    print("whoInterlocked", whoInterlocked)
    try:
        print("configPath", session["configPath"])
    except KeyError:
        print("Pas de configPath")
    try:
        print("configName", session["configName"])
    except KeyError:
        print("Pas de configName")
    try:
        print("user", session["user"])
    except KeyError:
        print("Pas de user")
    try:
        print("configDict", session["configDict"])
    except KeyError:
        print("Pas de configDict")


@app.route("/statesList", methods=["GET"])
def statesList():
    global sysConf
    list = {}
    print("stateList", r2.get("runningFile"))
    list = getStatesList(sysConf[r2.get("runningFile")])
    # list["whoLocked"] = whoInterlocked[session["configName"]][0]
    return jsonify(list)


@app.route("/processROOTCommand", methods=["GET"])
def rootCommand():
    command = request.args.get("command")
    r = executeCommROOT(command)
    print(r)
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
    NOTE: the function can be optimized
    """
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
            time.sleep(1) # NOTE:Can be configurable
    return Response(getValues(), mimetype="text/event-stream")


@app.route("/logURL", methods=["GET"])
def return_logURL():
    module = request.args.get("module")
    groupName =  "faser"  #TODO: Should not be hardcoded
    # nodes = getListNodes(sysConf[session["configName"]])
    url = f"http://{platform.node()}:9001/logtail/faser:{module}"

    return jsonify(url)




def getListNodes(locRoot):
    return [node.name for pre,_,node in RenderTree(locRoot)]



if __name__ == "__main__":


    if len(sys.argv) != 1 :
        if sys.argv[1] == "-l":
            print("Running in local mode - no run number InfluxDB archiving")
            localOnly = True

    print(f"Connect with the browser to http://{platform.node()}:5000")
    metrics = metricsHandler.Metrics(app.logger)

    socketio.run(app, host="0.0.0.0", port=5000, debug=True, use_reloader = False)
    print("Stopping... ")
    metrics.stop()
