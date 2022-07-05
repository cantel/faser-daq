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

from xmlrpc.client import ServerProxy
import datetime
import getpass

## Supervisor wrapper class
#  Wraps the XML-RPC client calls in order to control Twiddler and Supervisor.
class supervisor_wrapper:
  ## Constructor
  #  @param host The targeted host were the supervisor daemon is running
  #  @param group The registered group to which to add processes
  def __init__(self, host, group):
    self.group = group
    self.host = host
    self.server = ServerProxy('http://'+host+':9001/RPC2')

  def getAllProcessInfo(self):
    return [info for info in self.server.supervisor.getAllProcessInfo() if info['group'] == self.group]

  def startProcess(self, name):
    return self.server.supervisor.startProcess(self.group+":"+name)

  def stopProcess(self, name):
    return self.server.supervisor.stopProcess(self.group+":"+name)

  def getProcessState(self, name):
    return self.server.supervisor.getProcessInfo(self.group+":"+name)

  ## Add program
  #  Adds a program, sets the log name with date and time.
  #  Execution of the added program is automatic.
  #  @param exe The executable to spawn, with relative path from dir
  #  @param dir The absolute path of the directory from which exe is evaluated
  #  @param env The environment variables to pass to the executable
  #  @param command The command to use to run a script (example: python3)
  def addProgram(self, name, exe, dir, env, command=""):
    now = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    user = getpass.getuser()
    log_file = "/log/"+name+"-"+user+"-"+now+".log"
    settings = {
        'command': command+" "+dir+exe,
        'directory': dir,
        'autorestart': 'false',
        'startretries': '0',
        'environment': env,
        'user': user,
        'stdout_logfile': log_file,
        'stdout_logfile_maxbytes': '0',
        'stdout_logfile_backups': '0',
        'stderr_logfile': log_file
        # 'redirect_stderr': 'true' # TODO apparently supervisor 3.1.4 ignores this.
    }
    return (self.server.twiddler.addProgramToGroup(self.group, name, settings), (self.host, log_file))

  def removeProcess(self, name):
    return self.server.twiddler.removeProcessFromGroup(self.group, name)
