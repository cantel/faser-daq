# enrico.gamberini@cern.ch

import sys
import zmq
import supervisord
import json
from jsonschema import validate
from time import sleep
import threading

context = zmq.Context()

exit = False


def removeProcesses():
  for p in data['components']:
    sd = supervisord.supervisord(p['host'], group)
    info = sd.getAllProcessInfo()
    for i in info:
      print(i)
      if i['statename'] == 'RUNNING':
        try:
          print('Stop', sd.stopProcess(i['name']))
        except:
          print("Exception: cannot stop process", i['name'])
        print('State', sd.getProcessState(i['name'])['statename'])
      try:
        print('Remove', sd.removeProcessFromGroup(i['name']))
      except:
        print("Exception: cannot remove process", i['name'])


def addProcesses():
  for p in data['components']:
    sd = supervisord.supervisord(p['host'], group)
    print("Add", sd.addProgramToGroup(
        p['name'], exe+" "+str(p['port']), dir, env))


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


def print_help():
  print("First argument must be a .json configuration file.\n"
        "Available second arguments: 'remove' 'add' 'configure' 'complete'.\n"
        "Add 'dev' in order to suppress production feature.")

with open('settings.json') as f:
  settings = json.load(f)
f.close()

group = settings['group']
dir = settings['build_dir']
exe = settings['exe']
env = settings['env']

validation = True

arg = "complete"
if len(sys.argv) <= 2:
  print_help()
  quit()
elif sys.argv[2] != 'dev':
  arg = sys.argv[2]

for o in sys.argv:
  if o == '-h':
    print_help()
    quit()
  elif o == 'dev':
    print("Developer mode")
    validation = False

with open(sys.argv[1]) as f:
  data = json.load(f)
f.close()

with open("json-config.schema") as f:
  schema = json.load(f)
f.close()

if validation:
  print("Configuration Version:", data['version'])
  validate(instance=data, schema=schema)

if arg == "remove":
  removeProcesses()
  quit()

if arg == 'add' or arg == 'complete':
  addProcesses()
  if arg == 'add':
    quit()

if arg == 'configure' or arg == 'complete':
  threads = []
  for p in data['components']:
    t = configureProcess(p)
    t.start()
    threads.append(t)
  for t in threads:
    t.join()

# spawn status check threads
threads = []
for p in data['components']:
  t = statusCheck(p)
  t.start()
  threads.append(t)

while(not exit):
  text = input("start | stop | down\n")
  print("Executing", text)
  command_threads = []
  if text == "start":
    spawnJoin(data['components'], startProcess)
  elif text == "stop":
    spawnJoin(data['components'], stopProcess)
  elif text == "down":
    spawnJoin(data['components'], shutdownProcess)
    if arg != 'configure':
      removeProcesses()
    exit = True

for t in threads:
  t.join()

quit()
