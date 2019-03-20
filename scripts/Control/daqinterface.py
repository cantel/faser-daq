# enrico.gamberini@cern.ch

import sys
import zmq
import supervisord
import json
from time import sleep
import threading

context = zmq.Context()

exit = False


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

        print("Add", sd.addProgramToGroup(
            p['name'], exe+" "+str(p['port']), dir))


def handleRequest(host, port, request):
    socket = context.socket(zmq.REQ)
    socket.setsockopt(zmq.LINGER, 0)
    socket.RCVTIMEO = 2000
    socket.connect("tcp://"+host+":"+str(port))
    socket.send_string(request)
    try:
        reply = socket.recv()
        # print(reply)
        return reply
    except:
        print("Timeout occurred")
        return ""


class configureProcess (threading.Thread):
    def __init__(self, p):
        threading.Thread.__init__(self)
        self.p = p

    def run(self):
        self.p['command'] = 'configure'
        s = json.dumps(self.p)
        # print(s)
        rv = handleRequest(self.p['host'], self.p['port'], s)
        if rv != b'Success':
            print("Error", self.p['name'], rv)


class startProcess (threading.Thread):
    def __init__(self, p):
        threading.Thread.__init__(self)
        self.p = p

    def run(self):
        self.p['command'] = 'start'
        s = json.dumps(self.p)
        rv = handleRequest(self.p['host'], self.p['port'], s)
        if rv != b'Success':
            print("Error", self.p['name'], rv)

class stopProcess (threading.Thread):
    def __init__(self, p):
        threading.Thread.__init__(self)
        self.p = p

    def run(self):
        self.p['command'] = 'stop'
        s = json.dumps(self.p)
        rv = handleRequest(self.p['host'], self.p['port'], s)
        if rv != b'Success':
            print("Error", self.p['name'], rv)


class shutdownProcess (threading.Thread):
    def __init__(self, p):
        threading.Thread.__init__(self)
        self.p = p

    def run(self):
        self.p['command'] = 'shutdown'
        s = json.dumps(self.p)
        rv = handleRequest(self.p['host'], self.p['port'], s)
        if rv != b'Success':
            print("Error", self.p['name'], rv)

class statusCheck (threading.Thread):
    def __init__(self, p):
        threading.Thread.__init__(self)
        self.p = p

    def run(self):
        self.p['command'] = 'status'
        s = json.dumps(self.p)
        status = ""
        while(not exit):
            sleep(0.5)
            new_status = handleRequest(self.p['host'], self.p['port'], s)
            if new_status != status and new_status != "":
                print(self.p['name'], "in status", new_status)
                status = new_status

def spawnJoin(list, func):
    threads = []
    for p in list:
        t = func(p)
        t.start()
        threads.append(t)
    for t in threads:
        t.join()

with open('settings.json') as f:
    settings = json.load(f)
f.close()

group = settings['group']
dir = settings['build_dir']
exe = settings['exe']

arg = "complete"
if len(sys.argv) == 1 or sys.argv[1] == '-h':
    print("First argument must be a .json configuration file. Available options: 'remove' 'supervisor' 'configure' 'complete'")
    quit()
else:
    arg = sys.argv[2]

with open(sys.argv[1]) as f:
    data = json.load(f)
f.close()

if arg == "remove" or arg == 'complete':
    removeProcesses()

if arg == 'supervisor' or arg == 'complete':
    addProcesses()

if arg == 'configure' or arg == 'complete':
    threads = []
    for p in data:
        t = configureProcess(p)
        t.start()
        threads.append(t)
    for t in threads:
        t.join()

#spawn status check threads
threads = []
for p in data:
    t = statusCheck(p)
    t.start()
    threads.append(t)

while(not exit):
    text = input("start | stop | down\n")
    print("Executing", text)
    command_threads = []
    if text == "start":
        spawnJoin(data,startProcess)
    elif text == "stop":
        spawnJoin(data,stopProcess)
    elif text == "down":
        spawnJoin(data,shutdownProcess)
        exit = True

for t in threads:
    t.join()

quit()
