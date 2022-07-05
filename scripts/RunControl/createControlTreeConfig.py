from copy import deepcopy
import os,sys
from pathlib import Path
import jsonref

lt = {
    "ReadoutInterface" : "RI",
    "EventBuilder" : "EB",
    "FileWriter" : "FW", 

    "EventBuilderFaser": "EBF", #
    "FileWriterFaser" : "FWF", #

    "TriggerReceiver" :"TR", # hardware
    "TriggerGenerator" : "TG", # emulation
    "TriggerMonitor": "TM",
    "FrontEndReceiver" : "FR", # hardware
    "FrontEndMonitor" : "FM",
    "FrontEndEmulator" : "FE",
    "EmulatorMonitor": "EM",
    "DigitizerReceiver" : "DR",
    "SCTDataMonitor":"SDM",
    "TrackerReceiver": "TKR",
    "TriggerRateMonitor": "TRM",
}

def createTreeJSON(configPath):
    try :
        with open(os.path.join(configDirPath, configPath), "r") as fp :
            base_dir_uri = f"{Path(configDirPath).as_uri()}/"
            jsonref_obj = jsonref.load(fp,base_uri=base_dir_uri,loader=jsonref.JsonLoader())
    except FileNotFoundError:
        print("Error, the file doesn't exist")
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
        # if there is only one module in the categorie
        if len(item) == 1:
            cat = {"name" : item[0], "types" : [{"type" : lt[key]}]}  
            tree["children"].append(cat)
            continue
            
        cat = {"name" : key, "types" :[{"type": lt[key]}], "children" : []}
        for comp in item :
            cat["children"].append({"name":comp})
        tree["children"].append(cat)
    return tree


def create_config_dict(configName):
    # TODO check if file exists 

    linkDict = {
        "tree": "control-tree.json",
        "fsm_rules": "../fsm-rules.json",
        "config": f"../{configName}",
        "grafana": "../grafana.json"
    }
    return linkDict
    


if __name__ == "__main__":
    # configDirPath = os.path.join("/Users/edward/Documents/PROG/FASER/faser-daq/configs")
    configDirPath = os.path.join(os.environ['DAQ_CONFIG_DIR'])

    if len(sys.argv) != 2 :
        print(f"USAGE : python {sys.argv[0]} <configName>")
        exit(1)
    
    configName= sys.argv[1]

    tree_config = createTreeJSON(configName)
    dict_config = create_config_dict(configName)

    dirPath = os.path.join(configDirPath, configName.replace(".json",""))
    os.mkdir(dirPath)

    with open(os.path.join(dirPath,"control-tree.json"), "w") as fp:
        jsonref.dump(tree_config,fp, indent=4)

    with open(os.path.join(dirPath,"config-dict.json"), "w") as fp:
        jsonref.dump(dict_config,fp, indent=4)


