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


def removeProcesses(components):
  for p in components:
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


def addProcesses(components, debug):
  for p in components:
    sd = supervisord.supervisord(p['host'], group)
    if debug is True:
      print("Add", sd.addProgramToGroup(
          p['name'], exe+" "+str(p['port'])+" debug", dir, env))
    else:
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
    req = json.dumps(self.p)
    rv = handleRequest(self.p['host'], self.p['port'], req)
    if rv != b'Success':
      print("Error", self.p['name'], rv)


class startProcess (threading.Thread):
  def __init__(self, p):
    threading.Thread.__init__(self)
    self.p = p

  def run(self):
    req = json.dumps({'command': 'start'})
    rv = handleRequest(self.p['host'], self.p['port'], req)
    if rv != b'Success':
      print("Error", self.p['name'], rv)


class stopProcess (threading.Thread):
  def __init__(self, p):
    threading.Thread.__init__(self)
    self.p = p

  def run(self):
    req = json.dumps({'command': 'stop'})
    rv = handleRequest(self.p['host'], self.p['port'], req)
    if rv != b'Success':
      print("Error", self.p['name'], rv)


class shutdownProcess (threading.Thread):
  def __init__(self, p):
    threading.Thread.__init__(self)
    self.p = p

  def run(self):
    req = json.dumps({'command': 'shutdown'})
    rv = handleRequest(self.p['host'], self.p['port'], req)
    if rv != b'Success':
      print("Error", self.p['name'], rv)


class statusCheck (threading.Thread):
  def __init__(self, p):
    threading.Thread.__init__(self)
    self.p = p

  def run(self):
    status = ""
    while(not exit):
      sleep(0.5)
      req = json.dumps({'command': 'status'})
      new_status = handleRequest(self.p['host'], self.p['port'], req)
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
debug = False

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
    debug = True

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
  removeProcesses(data['components'])
  quit()

if arg == 'add' or arg == 'complete':
  addProcesses(data['components'], debug)
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
  text = input("(config) | start | stop | down\n")
  print("Executing", text)
  command_threads = []
  if text == "config":
    spawnJoin(data['components'], configureProcess)
  elif text == "start":
    spawnJoin(data['components'], startProcess)
  elif text == "stop":
    spawnJoin(data['components'], stopProcess)
  elif text == "down":
    spawnJoin(data['components'], shutdownProcess)
    if arg != 'configure':
      removeProcesses(data['components'])
    exit = True

for t in threads:
  t.join()

quit()
