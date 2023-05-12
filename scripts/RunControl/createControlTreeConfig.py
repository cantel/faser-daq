#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
from copy import deepcopy
import os,sys
from pathlib import Path
import jsonref, json


def corresponding_type(longType:str):
    """
    Returns the short version type of the module
    """
    shortType = ""
    if "monitor" in longType.lower(): shortType = "monitor"
    elif "archiver" in longType.lower(): shortType = "monitor"
    elif "filewriter" in longType.lower() : shortType = "FW"
    elif "eventbuilder" in longType.lower() : shortType = "EB"
    elif "receiver" in longType.lower() and "trigger" not in longType.lower() and "tracker" not in longType.lower() and "digitizer" not in longType.lower(): shortType = "receiver"
    elif "emulator" in longType.lower(): shortType = "emulator"
    elif "triggergenerator" in longType.lower(): shortType = "TG"
    elif "triggerreceiver" in longType.lower(): shortType = "TR"
    elif "trackerreceiver" in longType.lower() or "digitizer" in longType.lower() : shortType = "TKR"
    else : print("Not a known type, please add it"); exit(0)
    return shortType
        
        
def createTreeJSON(configPath,configsDirPath):
    try :
        with open(configPath, "r") as fp :
            base_dir_uri = f"{Path(configsDirPath).as_uri()}/"
            jsonref_obj = jsonref.load(fp,base_uri=base_dir_uri,loader=jsonref.JsonLoader())
    except FileNotFoundError:
        print("Error, the file does not exist")
        exit(1)
    if "configuration" in jsonref_obj: # faser ones have "configuration" but not demo-tree schema with references (version >= 10) 
        configuration = deepcopy(jsonref_obj)["configuration"]
    else:
        # old-style schema (version < 10)
        configuration = jsonref_obj
    tree = {"name" : "Root", "children" :[]}
    sort = {}
    for component in configuration["components"]:
        if component["modules"][0]["type"] in sort.keys():
            sort[component["modules"][0]["type"]].append(component["modules"][0]["name"])
        else:
            sort[component["modules"][0]["type"]] = []
            sort[component["modules"][0]["type"]].append(component["modules"][0]["name"])
    

    for key,item in sorted(sort.items()): 
        # if there is only one module in the categorie, there is no category
        if len(item) == 1:
            cat = {"name" : item[0], "types" : [{"type" : corresponding_type(key)}]}  
            tree["children"].append(cat)
            continue
        cat = {"name" : key, "types" :[{"type": corresponding_type(key)}], "children" : []}

        for comp in item :
            cat["children"].append({"name":comp})
        tree["children"].append(cat)
    return tree


if __name__ == "__main__":
    if len(sys.argv) != 2 :
        print(f"USAGE : python {sys.argv[0]} <configFullPath>")
        exit(1)
    
    configPath     = sys.argv[1]                                  # full path to config
    configsDirPath = os.path.dirname(os.path.abspath(configPath)) # full path to containing dir
    configName     = os.path.basename(configPath)                 # name of the config with extension
    configDir      = os.path.join(configsDirPath,configName.replace('.json','')) # dir of specific config
    dict_obj = {
        "tree"     : "control-tree.json",
        "fsm_rules": "../fsm-rules.json",
        "config"   : f"../{configName}",
        "grafana"  : "../grafana.json"
    }

    with open(os.path.join(configsDirPath,"fsm-rules.json"), "r") as fp:
        fsm_rules = json.load(fp)

    tree_config = createTreeJSON(configPath, configsDirPath)

    try : 
        os.mkdir(configDir)
        with open(os.path.join(configDir,"control-tree.json"), "w") as fp:
            jsonref.dump(tree_config,fp, indent=4)
        with open(os.path.join(configDir,"config-dict.json"), "w") as fp:
            jsonref.dump(dict_obj,fp, indent=4)
    except FileExistsError:
        print("Directory already exists ! ")
        r = input("Do you want to overwrite it ? (y/N) ")
        if (r.upper() == "Y"):
            with open(os.path.join(configDir,"control-tree.json"), "w") as fp:
                jsonref.dump(tree_config,fp, indent=4)
            with open(os.path.join(configDir,"config-dict.json"), "w") as fp:
                jsonref.dump(dict_obj,fp, indent=4)
        else:
            print("Exiting...")
            exit(0)
