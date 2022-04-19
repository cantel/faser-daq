#  Copyright (C) 2019-2022 CERN for the benefit of the FASER collaboration
#

import json
import sys, os

def loadConfig(path):
    try :
        with open(path,"r") as fp:
            data = json.load(fp)
    except FileNotFoundError :
        print("ERROR: Didn't load configuration file : File not found")
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
        "max_hist_per_page" : 30
    }

    with open(os.path.join(data,"config.json"),"w") as fp:
        json.dump(data,fp, indent=2)


if __name__ == "__main__":
    """
    If no configuration file exists, one can be created using directly this script :
    python3 config_manager <dirPath>.
    The new created config file contains the following parameters:

     - port : Port used by Redis
     - host : Adress used by Redis
     - pub_histo_db : Redis database where the histogramms are published
     - run_info_db : Redis database where the current run informations are stored
     - tags_db : Redis database where the tags of the histograms are be stored
     - old_histo_db : Redis database where the "old" histograms are stored
     - update_refresh_rate : time between the update of the histograms (seconds)
     - short_save_histo_rate : time between each histogram backup (max 30 histograms saved) (seconds)
     - long_save_histo_rate : time between each old histogram backup (max 168 old histograms saved) (seconds)
     - max_hist_per_page : number of histogram per page on the web interface
    """

    if len(sys.argv) != 2:
        print(f"USAGE: python {sys.argv[0]} <dirPath>")
        exit(1)
    writeDefaultConfig(path=sys.argv[1])





