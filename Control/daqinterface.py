import zmq
#import msgpack
#import supervisord
import time

host = 'rd51'
group = 'rd51'
name = 'daq'
exe = '/usr/local/bin/srs.py'

#sd = supervisord.supervisord(host, group)


context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://188.185.65.114:5557")
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
