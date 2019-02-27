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

if len(sys.argv) == 1:
    print("Available: remove supervisor complete")

for p in data:
    sd = supervisord.supervisord(p['host'], group)

    info = sd.getAllProcessInfo()
    for i in info:
        print(i)
        if i['statename'] == 'RUNNING':
            print('Stop', sd.stopProcess(i['name']))
            print('State', sd.getProcessState(i['name'])['statename'])
        print('Remove', sd.removeProcessFromGroup(i['name']))

if sys.argv[1] == "remove":
    exit(0)

for p in data:
    sd = supervisord.supervisord(p['host'], group)

    print("Add", sd.addProgramToGroup(p['name'], exe+" "+p['name'], dir) )


if sys.argv[1] == "supervisor":
    exit(0)

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://rd51:5557")
s = json.dumps(data)
print(s)
socket.send_string(s)
print("sent")

reply = socket.recv()
print(reply)
