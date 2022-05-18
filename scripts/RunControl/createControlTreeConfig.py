from copy import deepcopy
import os,sys
from pathlib import Path
from os import environ as env
import jsonref

lt = {
    "ReadoutInterface" : "RI",
    "EventBuilder" : "EB",
    "FileWriter" : "FW", 

    "EventBuilderFaser": "EBF", #
    "FileWriterFaser" : "FWF", #

    "TriggerReceiver" :"TR", # hardware
    "TriggerGenerator" : "TG", # emulation
    "FrontEndReceiver" : "FR", # hardware
    "FrontEndMonitor" : "FM",
    "FrontEndEmulator" : "FE",
    "EmulatorMonitor": "EM"
}

# configPath = sys.argv[1]
# configPath = "demo-tree/config.json" # path to config file


def createJSON(configPath):
    configDirPath = os.path.join(env['DAQ_CONFIG_DIR'])
    try :
        with open(os.path.join(configDirPath, configPath), "r") as fp :
            base_dir_uri = f"{Path(configDirPath).as_uri()}/"
            jsonref_obj = jsonref.load(fp,base_uri=base_dir_uri,loader=jsonref.JsonLoader())
    except FileNotFoundError:
        print("Error")
        exit(1)

    if "configuration" in jsonref_obj: # faser ones have "configuration" but not demo-tree
        # schema with references (version >= 10)
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

    for key,item in sort.items():
        cat = {"name" : key, "types" :[{"type": lt[key]}], "children" : []}
        for comp in item :
            cat["children"].append({"name":comp})
        tree["children"].append(cat)


    return tree



if __name__ == "__main__":
    
    if len(sys.argv) != 3 :
        print(f"USAGE : python {sys.argv[0]} <configPath> <destinationPath>")
        exit(1)
    
    configPath = sys.argv[1]
    savePath = sys.argv[2] 

    treeConfig = createJSON(configPath)
     
    with open(savePath, "w") as fp:
        jsonref.dump(treeConfig,fp, indent=4)




    
    



