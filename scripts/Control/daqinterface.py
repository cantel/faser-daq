# enrico.gamberini@cern.ch

import sys
import zmq
import supervisord
import json
from time import sleep
import threading

context = zmq.Context()

def removeProcesses():
    for p in data:
        sd = supervisord.supervisord(p['host'], group)

        info = sd.getAllProcessInfo()
        for i in info:
            print(i)
            if i['statename'] == 'RUNNING':
                print('Stop', sd.stopProcess(i['name']))
                print('State', sd.getProcessState(i['name'])['statename'])
            print('Remove', sd.removeProcessFromGroup(i['name']))

def addProcesses():
    for p in data:
        sd = supervisord.supervisord(p['host'], group)

        print("Add", sd.addProgramToGroup(p['name'], exe+" "+str(p['port']), dir) )


def handleRequest(host, port, request):
    socket = context.socket(zmq.REQ)
    socket.RCVTIMEO = 3000
    socket.connect("tcp://"+host+":"+str(port))
    socket.send_string(request)
    try:
        reply = socket.recv()
        print(reply)
        return reply
    except:
        print("Timeout occurred")

def checkStatus():
    for p in data:
        p['command']='status'
        s = json.dumps(p)
        print(s)
        handleRequest(p['host'], p['port'], s)

def startCommand():
    for p in data:
        p['command']='start'
        s = json.dumps(p)
        print(s)
        handleRequest(p['host'], p['port'], s)

def stopCommand():
    for p in data:
        p['command']='stop'
        s = json.dumps(p)
        print(s)
        handleRequest(p['host'], p['port'], s)


def configureProcesses():
    for p in data:
        p['command']='configure'
        s = json.dumps(p)
        print(s)
        handleRequest(p['host'], p['port'], s)


class statusCheck (threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
    def run(self):
        print ("Starting statusCheck")
        while(True):
            checkStatus()
            sleep(2)
        print ("Exiting " + self.name)

with open('settings.json') as f:
    settings = json.load(f)

group = settings['group']
dir = settings['build_dir']
exe = settings['exe']

with open('config.json') as f:
    data = json.load(f)

arg = "complete"
if len(sys.argv) == 1:
    print("Available: 'remove' 'supervisor' 'jzonmq'. Going for complete service")
elif sys.argv[1] == '-h':
    print("Default complete service. Available options: 'remove' 'supervisor' 'jzonmq'")
    exit(0)
else:
    arg = sys.argv[1]

if arg == "remove" or arg == 'complete':
    removeProcesses()

if arg == 'supervisor' or arg == 'complete':
    addProcesses()

if arg == 'jzonmq' or arg == 'complete':
    configureProcesses()

thread = statusCheck()
thread.start()

while(True):
    text = input("prompt")
    print(text)
    if text == "start":
        startCommand()
    elif text == "stop":
        stopCommand()