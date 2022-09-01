from typing import Union
import requests
import json




class RunControl:
    def __init__(self,baseUrl:str, requestTimeout:int =40):
        
        self.__s = requests.Session()
        # self.__login()
        self.__baseUrl = baseUrl
        self.__requestTimeout = requestTimeout
        
    
    def __getRequest(self,route:str, params = None) -> Union[requests.Response,None]:
        """
        Return None if error, array else
        """
        try : 
            r = self.__s.get(self.__baseUrl + route, params = params, timeout=self.__requestTimeout)

            if r.status_code != 200:
                print("ERROR : Failed to get the requests info")
            else : 
                return r
        except requests.exceptions.Timeout as e : raise e
        except requests.exceptions.ConnectionError as e : raise e
    
    def __login(self):
        self.__getRequest("/botLogin")
    
    def get_configs(self) -> Union[list,None]:
        """
        Get a list of the all the valid configurations
        """
        return self.__getRequest("/configDirs").json() 


    def load_config(self, config:str):
        self.__login()
        appState = self.__getRequest("/appState").json()
        if appState["runOngoing"] != 0:
            raise RuntimeError("Unable to load the config, run is already started")
        self.__getRequest("/initConfig",{"configName": config})


control = RunControl(baseUrl = "http://faser-daqvm-000.cern.ch:5000")

print(control.get_configs())
control.load_config("emulatorLocalhostold")




