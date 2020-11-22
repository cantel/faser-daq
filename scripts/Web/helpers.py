import os
from os import environ as env
import threading
import sys
import jsonschema
import json
import jsonref
import daqcontrol
import statetracker

#reads a configuration json file, handles the errors and validate with general schema
def read(fileName):
        if(os.path.exists(env['DAQ_CONFIG_DIR'] + fileName)):
                with open(env['DAQ_CONFIG_DIR'] + fileName) as f:
                        try:
                                refobj = jsonref.load(f)
                                if "configuration" in refobj:
                                        data = daqcontrol.jsonref_to_json(refobj)["configuration"]
                                        #print(json.dumps(data, sort_keys=True, indent=4))
                                else:
                                        data = refobj
                                f.close()
                                schema=env['DAQ_CONFIG_DIR'] + "schemas/validation-schema.json"
                                if(os.path.exists(schema)):
                                        with open(schema) as f:
                                                try:
                                                        schema = json.load(f)
                                                
                                                        try:
                                                                jsonschema.validate(instance=data, schema=schema)
                                                        except jsonschema.exceptions.ValidationError as e:
                                                                print(e)
                                                                data= "NOTCOMP"
                                                        except:
                                                                print("Unexpected error:", sys.exc_info())
                                                                raise
                                                except:
                                                        print("Unexpected error 2:", sys.exc_info())
                                                        data= "BADSCHEMA"
                                        f.close()

                                else:
                                        data= "NOSCHEMA"
                        except:
                                data = "BADJSON"
        else:
                data = {}
                write(data)
        
                
        return data
#rewrites the current.json
def write(d):
        with open(env['DAQ_CONFIG_DIR'] + 'current.json', 'w+') as f:
                json.dump(d, f)
        statetracker.updateConfig()



#reads the schema for the requested board type
def readSchema(boardType):
        try:
                schemaFileName = env['DAQ_CONFIG_DIR'] + "schemas/" + boardType
                f = open(schemaFileName)
                schema = json.load(f)
                f.close()
        except:
                schema= "error"
        return schema


def readGeneral():
        try:
                schemaFileName = env['DAQ_CONFIG_DIR'] +  "json-config.schema"
                f = open(schemaFileName)
                schema = json.load(f)
                f.close()
        except:
                schema= "error"
        return schema




def translateStatus(rawStatus, timeout):
        translatedStatus = str(rawStatus)
        if(timeout):
                translatedStatus = "TIMEOUT"
        else:
                if(rawStatus == 'not_added'):
                        translatedStatus = "DOWN"
                elif(rawStatus == 'added'):
                        translatedStatus = "ADDED"
                elif(rawStatus == 'booted'):
                        translatedStatus = "BOOTED"
                elif(rawStatus == 'ready'):    
                        translatedStatus = "READY"
                elif(rawStatus == 'running'):  
                        translatedStatus = "RUN"
                elif(rawStatus == 'paused'):   
                        translatedStatus = "PAUSED"
        return translatedStatus


#takes the one thousand first lines of the log file
def tail(file, n=1, bs=1024):

        f = open(file)
        f.seek(0,2)
        l = 1-f.read(1).count('\n')
        B = f.tell()
        while n >= l and B > 0:
                block = min(bs, B)
                B -= block
                f.seek(B, 0)
                l += f.read(block).count('\n')
        f.seek(B, 0)
        l = min(l,n)
        lines = f.readlines()[-l:]
        f.close()
        return lines    
                
#returns the index number of the boardName from the json d      
def findIndex(boardName, d):
        index = 0
        for p in d['components']:
                if p['name'] == boardName:
                        break
                else:
                        index += 1
        return index


def createDaqInstance(d):
        group =  d['group']
        dc = daqcontrol.daqcontrol(group)
        return dc
        
#creates a thread for the function passed on argument
def spawnJoin(list, func):
        threads = []
        for p in list:
                t = threading.Thread(target=func, args=(p,))
                t.start()
                threads.append(t)
        for t in threads:
                t.join()

