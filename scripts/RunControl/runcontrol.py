import time
from typing import Union
import requests
import json




class RunControl:

    def __init__(self,baseUrl:str, requestTimeout:int =40):
        
        self.__s = requests.Session()
        # self.__login()
        self.__baseUrl = baseUrl
        self.__requestTimeout = requestTimeout
        
    
    def __sendRequest(self, typeR,route:str, params = None) -> Union[requests.Response,None]:
        """
        Return None if error, array else
        """
        try : 
            if typeR =="GET":
                r = self.__s.get(self.__baseUrl + route, params = params, timeout=self.__requestTimeout)
            elif typeR == "POST":
                r = self.__s.post(self.__baseUrl + route, json = params, timeout=self.__requestTimeout)

            if r.status_code != 200:
                raise RuntimeError("ERROR : Failed to get the requests info")
            else : 
                return r
        except requests.exceptions.Timeout as e : raise e
        except requests.exceptions.ConnectionError as e : raise e
    
    def initialise(self):
        return self.__sendRequest("POST", "/processROOTCommand", {"bot":True, "command": "INITIALISE"})
    
    def start(self, runType, startComment, seqnumber=None, seqstep=0, seqsubstep = 0):
        return self.__sendRequest("POST","/processROOTCommand", {"bot":True, "command": "START", "runType" : runType, "runComment" : startComment, "seqnumber" : seqnumber, "seqstep": seqstep, "seqsubstep":seqsubstep }  )
    
    
    def stop(self, runType, stopComment):
        return self.__sendRequest("POST","/processROOTCommand", {"bot":True, "command": "STOP", "runType" : runType, "runComment" : stopComment})
    
    def shutdown(self):
        return self.__sendRequest("POST","/processROOTCommand", {"bot":True, "command": "SHUTDOWN"})
    
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


