import time
from typing import Union
import requests


class RunControl:

    def __init__(self,baseUrl:str, requestTimeout:int =40):

        self.__s = requests.Session()
        self.__baseUrl = baseUrl
        self.__requestTimeout = requestTimeout
        
    
    def __sendRequest(self, typeR,route:str, params = None) -> Union[requests.Response,None]:
        """
        Return : dict of the response data if success, False otherwise.
        """
        try : 
            if typeR =="GET":
                r = self.__s.get(self.__baseUrl + route, params = params, timeout=self.__requestTimeout)
            elif typeR == "POST":
                r = self.__s.post(self.__baseUrl + route, json = params, timeout=self.__requestTimeout)

            if r.status_code != 200:
                print("ERROR : Failed to get the requests info")
                return False
            else : 
                return r.json()

        except requests.exceptions.Timeout : print("ERROR : Timeout"); return False
        except requests.exceptions.ConnectionError : print("ERROR : Connection Error"); return False


    def __sendCommand(self, params = None) :
        """
        Return : True if completed, False if error. 
        """
        try :  
            r = self.__s.post(self.__baseUrl + "/processROOTCommand", json=params, timeout=self.__requestTimeout)
            if r.status_code != 200:
                print("ERROR : Failed to process command")
                return False
            else : return True
        except requests.exceptions.Timeout : print("ERROR : Timeout"); return False
        except requests.exceptions.ConnectionError : print("ERROR : Connection Error, failed to send command"); return False

        
    def initialise(self):
        return self.__sendCommand({"bot":True, "command": "INITIALISE"})
    
    def start(self, runType, startComment, seqnumber=None, seqstep=0, seqsubstep = 0):
        return self.__sendCommand({"bot":True, "command": "START", "runType" : runType, "runComment" : startComment, "seqnumber" : seqnumber, "seqstep": seqstep, "seqsubstep":seqsubstep }  )
    
    
    def stop(self, runType, stopComment):
        return self.__sendCommand({"bot":True, "command": "STOP", "runType" : runType, "runComment" : stopComment})
    
    def shutdown(self):
        return self.__sendCommand({"bot":True, "command": "SHUTDOWN"})

    def getState(self):
        """
        Return a dictionnary with the following keys: 
            - runOngoing : is it running ? (True/False)
            - loadedConfig : the name of the current config
            - runState : root state of the run
            - whoInterlocked : who currently has control ? 
            - errors : modules with errors {'1':[...], '2': [...]}
            - crashedM : crashed Modules
            - localOnly : if the run control is in local mode
            - runType : the type of the run
            - runComment : the current run comment 
            - runNumber : the current run number
        """
        r = self.__sendRequest("GET","/appState")
        return r.json()

    
def example():
    control = RunControl(baseUrl = "http://faser-daqvm-000.cern.ch:5000")
    control.initialise()
    time.sleep(2)
    control.start("Test", "remote control test start")
    time.sleep(10)
    control.stop("Test", "remote control test stop")
    time.sleep(10)
    control.shutdown()
    

if __name__ == "__main__":
    example()


