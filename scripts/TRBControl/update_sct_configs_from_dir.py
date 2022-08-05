#!/usr/bin/env python3
#
#  Copyright (C) 2019-2022 CERN for the benefit of the FASER collaboration
#

import json
import argparse
import os
import time
import re
from update_trb_setting import TRBSettings

# CHECK THESE FIELDS
TRBjson_base_dir = "/home/cantel/faser-daq/configs/Templates"
mod_cfg_field_name = "moduleConfigFiles"
kLAYERS=3
kIFT_BOARDID_LAYER0 = 11 # TRB board ID of IFT layer 0 to caulcate board IDs of all layers assuming they stay sequential.

def main():
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    parser.add_argument('--cfg', type=str, default="TRB.json", help='Name of TRB josn cfg in base directory set in script')
    parser.add_argument('--new', action='store_true', help='Create new TRB config file (TRB cfg name + NEW)')
    group.add_argument('--trb', type=str, help='Name of TRB in TRB json cfg')
    group.add_argument('--layer', type=int, help='Layer number to update, in which case station ID also needs to be provided.')
    parser.add_argument('--station', type=int, help='Station ID for layer ')
    parser.add_argument('-i', '--input', type=str, help='Directory where configuration json files live')
    parser.add_argument('-t', '--tag', type=str, default="", help='Tag to identify module config files')
    parser.add_argument('--prefix', type=str, default="", help='Prefix to identify module config files. Usually nothing but sometimes files just have to be special.')
    args=parser.parse_args()
   
    trb_cfg_file = args.cfg 
    if not trb_cfg_file.endswith('.json'):
        raise ValueError(f"ERROR invalid TRB config file name provided: {trb_cfg_file}. File must be a json file! Will not continue.")
    create_new = args.new
    trb_name = args.trb
    layer = args.layer
    station_id = args.station
    get_trb_name = False
    if not layer is None:
        get_trb_name = True
        if station_id is None:
            raise ValueError("ERROR Layer  number given, but no station Id provided? Which station should I update? Try e.g. --station 0")
    base_input_dir_name = args.input
    if base_input_dir_name.endswith("/"): base_input_dir_name = base_input_dir_name[:-1]
    tag = args.tag
    prefix = args.prefix
    if not prefix == "" and not prefix.endswith('_'): prefix+='_'

    if get_trb_name:
        trb_id = (station_id-1)*kLAYERS+layer if station_id > 0 else kIFT_BOARDID_LAYER0+layer
        trb_name = f"TRBReceiver{str(0)*(trb_id<10)}{trb_id}"
        print(f"INFO Deduced TRB name {trb_name} from station ID {station_id} and layer {layer}")
    else: # get layer number instead
        trb_id = int(trb_name[-2:])
        layer = int(trb_id%3) if trb_id < 10 else int((trb_id-2)%3)
    
    TRBJSON_IN = f"{TRBjson_base_dir}/{trb_cfg_file}"
    with open(TRBJSON_IN, 'r') as TRBjson_file:
        data=TRBjson_file.read()
    TRBjson_file.close()
    
    trb_objs = json.loads(data)
    trb_obj = trb_objs[trb_name]
    
    #print(trb_obj)
    
    if os.path.exists(f"{base_input_dir_name}/Layer{str(layer)}"):
        input_dir_name = f"{base_input_dir_name}/Layer{str(layer)}"
        print(f"INFO Found directory Layer{layer}, assuming config files live here.")
    else:
        print(f"INFO Did not find directory Layer{layer}, assuming config files live in top directory.")
        input_dir_name = base_input_dir_name
    input_dir_name = os.path.abspath(input_dir_name)
    print("INFO Updating to calibration files in %s with tag %s for TRB %s"%(input_dir_name, tag, trb_name))
    time.sleep(0.5)
    
    
    modcfgs = {}
    
    pattern = re.compile(r"Module(\d)")
    
    for modcfg in os.listdir(input_dir_name):
      if not modcfg.startswith(prefix+"Module"): continue
      if not tag in modcfg: continue
      print("modcfg = ",modcfg)
      matched = pattern.search(modcfg)
      if matched:
          mod_idx = matched.group(1)
      else:
          print("ERROR mod idx could not be found in config file name.")
      print("mod_idx: ",mod_idx)
      full_modfcg_name = input_dir_name+"/"+modcfg
      if mod_idx in modcfgs:
        print("ERROR: File for module %s already found. FIX ME!"%(mod_idx))
        exit()
      modcfgs[mod_idx] = full_modfcg_name
    
    if len(modcfgs) != 8:
      print("ERROR: Should have found exactly 8 new module configurations but %i found. FIX ME!"%(len(modcfgs)))
      exit()
    #print(modcfgs)
    if create_new: 
        TRBJSON_OUT = TRBJSON_IN[:-5]+'_NEW.json'
    else:
        TRBJSON_OUT = TRBJSON_IN
    trb_settings = TRBSettings(TRBJSON_IN,TRBJSON_OUT)
    trb_settings.update(trb_name, ["moduleConfigFiles"],[modcfgs])

if __name__ == "__main__":
	main()
