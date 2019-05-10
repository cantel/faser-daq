from xmlrpc.client import ServerProxy
import datetime
import getpass

class supervisord:
  def __init__(self, host, group):
    print("host", host)
    self.group = group
    self.server = ServerProxy('http://'+host+':9001/RPC2')

  def getAllProcessInfo(self):
    return self.server.supervisor.getAllProcessInfo()

  def startProcess(self, name):
    return self.server.supervisor.startProcess(self.group+":"+name)

  def stopProcess(self, name):
    return self.server.supervisor.stopProcess(self.group+":"+name)

  def getProcessState(self, name):
    return self.server.supervisor.getProcessInfo(self.group+":"+name)

  def addProgramToGroup(self, name, exe, dir, env):
    now = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    user = getpass.getuser()
    log_file = "/log/"+name+"-"+user+"-"+now+".log"
    settings = {
        'command': exe,
        'directory': dir,
        'autorestart': 'false',
        'environment': env,
        'user': user,
        'stdout_logfile': log_file,
        'stdout_logfile_maxbytes': '0',
        'stdout_logfile_backups': '0',
        'stderr_logfile': log_file
        # 'redirect_stderr': 'true' # TODO apparently supervisor 3.1.4 ignores this.
    }
    return self.server.twiddler.addProgramToGroup(self.group, name, settings)

  def removeProcessFromGroup(self, name):
    return self.server.twiddler.removeProcessFromGroup(self.group, name)
