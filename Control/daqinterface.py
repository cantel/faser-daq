import zmq
#<<<<<<< HEAD
#import msgpack
#import supervisord
import time
#=======
import msgpack
import supervisord
import json
#>>>>>>> a1e37d0873b916bc76955a2d284721ce489b25f1

group = 'rd51'
dir = '/home/engamber/workspace/daq/build/bin'
exe = '/home/engamber/workspace/daq/build/bin/test_plugin'

#<<<<<<< HEAD
#sd = supervisord.supervisord(host, group)

#=======
with open('config.json') as f:
    data = json.load(f)

print(data)


for p in data:
    sd = supervisord.supervisord(p['host'], group)

    info = sd.getAllProcessInfo()
    for i in info:
        print(i)
        if i['statename'] == 'RUNNING':
            print('Stop', sd.stopProcess(i['name']))
            print('State', sd.getProcessState(i['name'])['statename'])
        print('Remove', sd.removeProcessFromGroup(i['name']))

for p in data:
    sd = supervisord.supervisord(p['host'], group)

    print("Add", sd.addProgramToGroup(p['name'], exe+" "+p['name'], dir) )

# sd = supervisord.supervisord(host, group)
# sd.addProgramToGroup(name, exe)
# info = sd.getAllProcessInfo()
# if len(info) == 0:
#     print('Add', sd.addProgramToGroup(name, exe))

# for i in info:
#     if i['statename'] == 'RUNNING':
#         print('Stop', sd.stopProcess(name))
#         print('State', sd.getProcessState(name)['statename'])
#         print('Remove',sd.removeProcessFromGroup(name))
#     elif i['statename'] == 'STOPPED':
#         print('Start', sd.startProcess(name))
#         print('State', sd.getProcessState(name)['statename'])

# exit(0)
#>>>>>>> a1e37d0873b916bc76955a2d284721ce489b25f1

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://188.185.65.114:5557")
<<<<<<< HEAD
print("Connected");
#time.sleep(60)


#msg = msgpack.packb(["configuration", "configure", "BoardReader"])
socket.send_string("HELLO")

reply = socket.recv()
print("Reply: ", reply) 

#time.sleep(10)

# socket = context.socket(zmq.REP)
# socket.bind("tcp://localhost:5555")
#reply = socket.recv()
#unpacked = msgpack.unpackb(reply, raw=False)
#print(unpacked)
#=======

# msg = msgpack.packb(["configuration", "configure", "BoardReader"])
socket.send_json(data)
print("sent")

# socket = context.socket(zmq.REP)
# socket.bind("tcp://localhost:5555")
reply = socket.recv()
# unpacked = msgpack.unpackb(reply, raw=False)
print(reply)
#>>>>>>> a1e37d0873b916bc76955a2d284721ce489b25f1

