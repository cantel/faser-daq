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

from supervisor_wrapper import supervisor_wrapper
import zmq
import json
import jsonref
from time import sleep
from xmlrpc.client import ServerProxy

## DAQ control class
#  Handles communication with Supervisor and the Command XML-RPC server of DAQling processes.
class daqcontrol:
  ## Constructor
  #  @param group The Supervisor registered group to which to add processes
  #  @use_supervisor If False doesn't rely on Supervisor to spawn and check the process status
  def __init__(self, group, use_supervisor=True):
    self.group = group
    self.context = zmq.Context()
    self.use_supervisor = use_supervisor

  ## Get all names and process IDs
  #  Returns a map of process names and correspondent process IDs.
  def getAllNameProcessID(self, host):
    sw = supervisor_wrapper(host, self.group)
    try:
      return {info['name']: info['pid'] for info in sw.getAllProcessInfo()}
    except Exception as e:
      raise Exception(e)

  ## Remove a single process
  def removeProcess(self, host, name):
    sw = supervisor_wrapper(host, self.group)
    try:
      if sw.getProcessState(name)['statename'] == 'RUNNING':
        try:
          sw.stopProcess(name)
        except Exception as e:
          raise Exception(e, ": cannot stop process",
                name, "(probably already stopped)")
      sw.removeProcess(name)
    except Exception as e:
      raise Exception(e, ": Couldn't get process state",name, "probably already removed")

  ## Removes multiple processes
  #  @param components The JSON array of components to remove
  def removeProcesses(self, components):
    for p in components:
      self.removeProcess(p['host'], p['name'])

  ## Add a single process
  #  @param exe The executable to spawn, with relative path from dir
  #  @param dir The absolute path of the directory from which exe is evaluated
  #  @param lib_path The LD_LIBRARY_PATH to pass to the executable
  #  @param command The command to use to run a script (example: python3)
  def addProcess(self, host, name, exe, dir, lib_path="", command=""):
    sw = supervisor_wrapper(host, self.group)
    try:
      _, log_file = sw.addProgram(name, exe, dir, lib_path, command)
      return log_file
    except Exception as e:
      raise Exception(e, ": cannot add program", name, "(probably already added)")

  ## Boot a single process
  #  Executes a process if it has been already added.
  def bootProcess(self, host, name):
    sw = supervisor_wrapper(host, self.group)
    try:
      sw.startProcess(name)
    except Exception as e:
      raise Exception(e, ": cannot start program", name, "(probably already started)")

  ## Adds multiple components (processes)
  #  @param components The JSON array of components to add
  #  @param exe The executable to spawn, with relative path from dir
  #  @param dir The absolute path of the directory from which exe is evaluated
  #  @param lib_path The LD_LIBRARY_PATH to pass to the executable
  def addComponents(self, components, exe, dir, lib_path):
    log_files = []
    for p in components:
      name = p['name']
      port = p['port']
      loglvl_core = p['loglevel']['core']
      loglvl_module = p['loglevel']['module']
      loglvl_connection = p['loglevel']['connection']
      full_exe = exe+" --name "+name+" --port "+str(port)+" --core_lvl "+loglvl_core+" --module_lvl "+loglvl_module+" --connection_lvl "+loglvl_connection
      log_files.append(self.addProcess(p['host'], name, full_exe, dir, lib_path=lib_path))
    return log_files

  ## Adds multiple scripts (processes)
  #  @param scripts The JSON array of scripts to add
  #  @param script_dir The absolute path of the directory from which the relative paths are evaluated
  def addScripts(self, scripts, script_dir):
    log_files = []
    for s in scripts:
      name = s['name']
      command = s['command']
      exe = s['executable']
      dir = script_dir+s['directory']
      log_files.append(self.addProcess(s['host'], name, exe, dir, command=command))
    return log_files

  ## Handles XML-RPC requests
  def handleRequest(self, host, port, request, *args):
    proxy = ServerProxy("http://"+host+":"+str(port)+"/RPC2")
    return getattr(proxy, request)(*args)

  def configureProcess(self, p):
    req = 'configure'
    config = json.dumps(p)
    return self.handleRequest(p['host'], p['port'], req, config)

  def unconfigureProcess(self, p):
    req = 'unconfigure'
    return self.handleRequest(p['host'], p['port'], req)

  def startProcess(self, p, run_num=0):
    req = 'start'
    return self.handleRequest(p['host'], p['port'], req, int(run_num))

  def stopProcess(self, p):
    req = 'stop'
    return self.handleRequest(p['host'], p['port'], req)

  def shutdownProcess(self, p):
    req = 'down'
    return self.handleRequest(p['host'], p['port'], req)

  def customCommandProcess(self, p, command, arg=None):
    req = 'custom'
    return self.handleRequest(p['host'], p['port'], req, command, arg)

  def getStatus(self, p):
      sw = supervisor_wrapper(p['host'], self.group)
      req = 'status'
      status = []
      module_names = []
      try:
        status = self.handleRequest(p['host'], p['port'], req)
        if(status==[]):
          for mod in p['modules']:
            status.append('booted')
            module_names.append(mod['name'])
        else:
          module_names, status = map(list, zip(*(x.split(' , ') for x in status)))
      except:
        if self.use_supervisor:
          for mod in p['modules']:
            status.append('added')
            module_names.append(mod['name'])
          try:
            sw.getProcessState(p['name'])['statename']
          except:
            status=[]
            module_names = []
            for mod in p['modules']:
              status.append('not_added')
              module_names.append(mod['name'])
      return status, module_names

