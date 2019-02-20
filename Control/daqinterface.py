import zmq
import msgpack
import supervisord

host = 'rd51'
group = 'rd51'
name = 'daq'
exe = '/usr/local/bin/srs.py'

sd = supervisord.supervisord(host, group)

info = sd.getAllProcessInfo()
if len(info) == 0:
    print('Add', sd.addProgramToGroup(name, exe))

for i in info:
    if i['statename'] == 'RUNNING':
        print('Stop', sd.stopProcess(name))
        print('State', sd.getProcessState(name)['statename'])
        print('Remove',sd.removeProcessFromGroup(name))
    elif i['statename'] == 'STOPPED':
        print('Start', sd.startProcess(name))
        print('State', sd.getProcessState(name)['statename'])

exit(0)

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

msg = msgpack.packb(["configuration", "configure", "BoardReader"])
socket.send(msg)

# socket = context.socket(zmq.REP)
# socket.bind("tcp://localhost:5555")
reply = socket.recv()
unpacked = msgpack.unpackb(reply, raw=False)
print(unpacked)
