# enrico.gamberini@cern.ch

import sys
import zmq
import supervisord
import json

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
    for p in data:
        sd = supervisord.supervisord(p['host'], group)

        info = sd.getAllProcessInfo()
        for i in info:
            print(i)
            if i['statename'] == 'RUNNING':
                print('Stop', sd.stopProcess(i['name']))
                print('State', sd.getProcessState(i['name'])['statename'])
            print('Remove', sd.removeProcessFromGroup(i['name']))

if arg == 'supervisor' or arg == 'complete':
    for p in data:
        sd = supervisord.supervisord(p['host'], group)

        print("Add", sd.addProgramToGroup(p['name'], exe+" "+str(p['port']), dir) )


if arg == 'jzonmq' or arg == 'complete':
    context = zmq.Context()
    for p in data:
        socket = context.socket(zmq.REQ)
        socket.RCVTIMEO = 3000
        socket.connect("tcp://"+p['host']+":"+str(p['port']))
        p['command']='configure'
        s = json.dumps(p)
        print(s)
        socket.send_string(s)
        print("sent")

        try:
            reply = socket.recv()
            print(reply)
        except:
            print("Timeout occurred")


