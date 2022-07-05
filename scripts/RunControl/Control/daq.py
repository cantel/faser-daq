"""
 Copyright (C) 2019-2021 CERN
 
 DAQling is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 DAQling is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with DAQling. If not, see <http://www.gnu.org/licenses/>.
"""

import sys
import signal
from os import environ as env
import json
import jsonref
import jsonschema
from jsonschema import validate
from functools import partial
from daqcontrol import daqcontrol
import config_interface
import concurrent.futures
import threading
import zmq
from time import sleep
from copy import deepcopy
import argparse

def spawnJoin(list, func):
  futures = []
  rvs = []
  with concurrent.futures.ThreadPoolExecutor() as executor:
    for p in list:
      f = executor.submit(func, p)
      futures.append(f)
    for f in futures:
      try:
        rvs.append(f.result())
      except Exception as e:
        print(e)
        rvs.append(e)
  return rvs


def signal_handler(sig, frame):
  print("Ctrl+C: shutting down")
  sc.stop_check_threads()
  spawnJoin(data['components'], dc.shutdownProcess)
  if args.arg!= 'configure':
    try:
      dc.removeProcesses(data['components'])
    except Exception as e:
      print(e)
    if add_scripts:
      try:
        dc.removeProcesses(data['components'])
      except Exception as e:
        print(e)
  quit()


class statusChecker():
  def __init__(self, dc):
    self.stop_check = False
    self.status_check_threads = []
    self.dc = dc

  def statusCheck(self, p):
    sleep(0.5)
    status, module_names=self.dc.getStatus(p)
    for s,m in zip(status,module_names):
      print(p['name'],"-",m, "in status", s)
    while(not self.stop_check):
      sleep(0.5)
      new_status,module_names = self.dc.getStatus(p)
      for s,ns,m in zip(status,new_status,module_names):
        if ns != s:
          print(p['name'],"-",m, "in status", ns)
      status=new_status

  def startChecker(self, components):
    for p in components:
      t = threading.Thread(target=self.statusCheck, args=(p,))
      t.start()
      self.status_check_threads.append(t)

  def stop_check_threads(self):
    self.stop_check = True
    for t in self.status_check_threads:
      t.join()


## Main
parser = argparse.ArgumentParser(description='Runner service for DAQling.')
parser.add_argument('--remote', dest='json_source', action='store_const',
                    const=True, default=False,
                    help='Gets remote json-configs from database, default is local file.')
parser.add_argument('json_identifier', metavar='config_name', type=str, 
                    help='JSON configuration file or name of DB configuration sets for running DAQling. Use \"--remote list\" to print the list of available configuration sets.')
parser.add_argument('arg', metavar='command', type=str, nargs='?', 
                    default='complete',
                    help="Available commands: 'remove' 'add' 'configure' 'complete'.")
parser.add_argument('--service_config', metavar='CONFIG', type=str, nargs=1, 
                    default=env['DAQ_SCRIPT_DIR']+'Configuration/config/service-config.json', 
                    help='JSON configuration file for config service.')
args = parser.parse_args()

debug = False
with open(args.service_config) as config_file:
  service_config = json.load(config_file)

if args.json_identifier == "list":
  confs = config_interface.printListAndExit(service_config['service_host'], service_config['service_port'])
  print('List of available configurations below:')
  print(confs)
  exit()

if args.json_source:
  conf_dir = config_interface.remote(args.json_identifier, service_config['service_host'], service_config['service_port'])
  args.json_identifier = conf_dir+"config.json"

jsonref_obj = config_interface.local(args.json_identifier)

if "configuration" in jsonref_obj:
  # schema with references (version >= 10)
  data = deepcopy(jsonref_obj)["configuration"]
else:
  # old-style schema (version < 10)
  data = jsonref_obj

add_scripts = False
if "scripts" in data:
  add_scripts = True

# open schema and validate the configuration
with open(env['DAQ_CONFIG_DIR']+'schemas/validation-schema.json') as f:
  schema = json.load(f)
f.close()

print("Schema Version:", data['version'])

resolver=jsonschema.RefResolver(base_uri='file://'+env['DAQ_CONFIG_DIR']+'schemas/',referrer=schema)
validate(instance=data, schema=schema,resolver=resolver)

# define required parameters from configuration
group = data['group']
if 'path' in data.keys():
  dir = data['path']
else:
  dir = env['DAQ_BUILD_DIR']
print("Using path "+dir)
scripts_dir = env['DAQ_SCRIPT_DIR']
exe = "/bin/daqling"
lib_path = 'LD_LIBRARY_PATH='+env['LD_LIBRARY_PATH']+':'+dir+'/lib/,TDAQ_ERS_STREAM_LIBS=DaqlingStreams'

# instanciate a daqcontrol object
if args.arg == "configure":
  dc = daqcontrol(group, False)
else:
  dc = daqcontrol(group)

if args.arg == "remove":
  try:
    dc.removeProcesses(data['components'])
  except Exception as e:
    print(e)
  if add_scripts:
    try:
      dc.removeProcesses(data['scripts'])
    except Exception as e:
      print(e)
  quit()

if args.arg == 'add' or args.arg == 'complete':
  log_files = []
  try:
    log_files = dc.addComponents(data['components'], exe, dir, lib_path)
  except Exception as e:
    print(e)
  if add_scripts:
    try:
      log_files = log_files + dc.addScripts(data['scripts'], scripts_dir)
    except Exception as e:
      print(e)
  for l in log_files:
    print("Host:", l[0], "\n", "tail -f", l[1])
  if args.arg == 'add':
    quit()

# send name+pid info to active psutil-manager(s)
if add_scripts:
  hosts = {s['host'] for s in data['scripts'] if s['executable'] == 'psutil-manager.py'}
  context = zmq.Context()
  # loop on hosts with psutil-manager, get the Name+PID dictionary and send it
  for host in hosts:
    try:
      name_pids = dc.getAllNameProcessID(host)
      socket = context.socket(zmq.PAIR)
      socket.connect("tcp://"+host+":6100")
      socket.send_pyobj(name_pids)
    except Exception as e:
      print(e)

sleep(0.5) # allow time for daqling executables to boot

# spawn status check threads
sc = statusChecker(dc)
sc.startChecker(data['components'])

if args.arg == 'configure' or args.arg == 'complete':
  print(spawnJoin(data['components'], dc.configureProcess))

signal.signal(signal.SIGINT, signal_handler)

# user input loop
while(not sc.stop_check):
  cmd, *cmd_args = input("(config) | start | stop | down | command <cmd>\n").split(' ')
  print("Executing", cmd, ' '.join(cmd_args))
  command_threads = []
  if cmd == "config":
    print(spawnJoin(data['components'], dc.configureProcess))
  if cmd == "unconfig":
    print(spawnJoin(data['components'], dc.unconfigureProcess))
  elif cmd == "start":
    try:
      sp = partial(dc.startProcess, run_num=cmd_args[0])
      print(spawnJoin(data['components'], sp))
    except IndexError:
      print("Run number not specified. Default to 0")
      print(spawnJoin(data['components'], dc.startProcess))
    except Exception as e:
      print(e)
  elif cmd == "stop":
    print(spawnJoin(data['components'], dc.stopProcess))
  elif cmd == "down":
    sc.stop_check_threads()
    print(spawnJoin(data['components'], dc.shutdownProcess))
    if add_scripts:
      try:
        dc.removeProcesses(data['scripts'])
      except Exception as e:
        print(e)
    if args.arg!= 'configure':
      sleep(2) # allow time for daqling executables to exit
      try:
        dc.removeProcesses(data['components'])
      except Exception as e:
        print(e)
  elif cmd == "command":
    try:
      ccp = partial(dc.customCommandProcess, command=cmd_args[0], arg=' '.join(cmd_args[1:]))
      print(spawnJoin(data['components'], ccp))
    except IndexError:
      print("Missing required command argument")


quit()
