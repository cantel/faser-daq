import sys
import zmq
import msgpack
import supervisord
import json

group = 'rd51'
dir = '/home/engamber/workspace/daq/build/bin'
exe = '/home/engamber/workspace/daq/build/bin/test_plugin'

with open('config.json') as f:
    data = json.load(f)

print(data)

arg = "complete"
if len(sys.argv) == 1:
    print("Available: 'remove' 'supervisor'. Going for complete service")
else:
    arg = sys.argv[1]

for p in data:
    sd = supervisord.supervisord(p['host'], group)

    info = sd.getAllProcessInfo()
    for i in info:
        print(i)
        if i['statename'] == 'RUNNING':
            print('Stop', sd.stopProcess(i['name']))
            print('State', sd.getProcessState(i['name'])['statename'])
        print('Remove', sd.removeProcessFromGroup(i['name']))

if arg == "remove":
    exit(0)

for p in data:
    sd = supervisord.supervisord(p['host'], group)

    print("Add", sd.addProgramToGroup(p['name'], exe+" "+p['name'], dir) )


if arg == "supervisor":
    exit(0)

for p in data:
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://rd51:5557")
    p['command']='configure'
    s = json.dumps(p)
    print(s)
    socket.send_string(s)
    print("sent")

    reply = socket.recv()
    print(reply)
