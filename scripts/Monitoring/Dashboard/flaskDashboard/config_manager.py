#  Copyright (C) 2019-2022 CERN for the benefit of the FASER collaboration
#

import json
import sys

def loadConfig(path):
    try :
        with open(path,"r") as fp:
            data = json.load(fp)
    except FileNotFoundError :
        print("ERROR: Didn't load configuration file --> not found")
        exit(1)
    else:
        return data

def writeDefaultConfig(path):
    data = {}
    data["redis"] = {
        "port":6379,
        "host": "localhost",
        "pub_histo_db": 1,
        "run_info_db": 2,
        "tags_db" : 5,
        "old_histo_db": 7
    }
    data["general"] = {
        "update_refresh_rate": 5,
        "short_save_histo_rate" : 60,
        "long_save_histo_rate" : 3600,
        "influxDB_related_stuff" : None       # TODO
    }
    if path.endswith("/"):
        path = path[:-1]
    with open(f"{path}/config.json","w") as fp:
        json.dump(data,fp, indent=2)


if __name__ == "__main__":
    """
    If no configuration file exists, one can be created using directly this script :
     --> python3 config_manager <dirPath>
    """
    if len(sys.argv) != 2:
        print(f"USAGE: python {sys.argv[0]} <dirPath>")
        exit(1)
    writeDefaultConfig(path=sys.argv[1])





