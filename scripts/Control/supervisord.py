from xmlrpc.client import ServerProxy

class supervisord:
    def __init__(self, host, group):
        print("host",host)
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

    def addProgramToGroup(self, name, exe, dir):
        settings = {
            'command': exe,
            'directory': dir,
            'autorestart': 'false',
            'environment': "LD_LIBRARY_PATH=/cvmfs/sft.cern.ch/lcg/releases/gcc/6.2.0-b9934/x86_64-centos7/lib64"
            }
        return self.server.twiddler.addProgramToGroup(self.group, name, settings)

    def removeProcessFromGroup(self, name):
        return self.server.twiddler.removeProcessFromGroup(self.group, name)
    
