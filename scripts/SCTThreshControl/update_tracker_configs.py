#!/usr/bin/env python3

import json

# CHECK THESE FIELDS
TRBjson_str = "/home/shifter/software/faser-daq/configs/Templates/TRB.json"
TRBjson_out_str = "/home/shifter/software/faser-daq/configs/Templates/TRB.json"

mod_cfg_field_name = "moduleConfigFiles"

import argparse
parser = argparse.ArgumentParser()
parser.add_argument('--trb', type=str, help='Name of TRB in TRB.json')
parser.add_argument('-i', '--input', type=str, help='Directory where Layer0, Layer1 and Layer2 calibration folders live')
parser.add_argument('-t', '--tag', type=str, default="", help='Tag to identify module config files')
args=parser.parse_args()

trb_name = args.trb
if trb_name == "":
  print("ERROR no TRB name given. Provide name of TRB receiver, e.g. --trb TRBReceiver00")
  exit() 
base_input_dir_name = args.input
if base_input_dir_name.endswith("/"): base_input_dir_name = base_input_dir_name[:-1]
tag = args.tag


with open(TRBjson_str, 'r') as TRBjson_file:
    data=TRBjson_file.read()
TRBjson_file.close()

trb_objs = json.loads(data)

trb_obj = trb_objs[trb_name]

#print(trb_obj)


trb_id = int(trb_name[-2:])
#layer = "Layer"+str(int(trb_id%2))
layer=""

import os
input_dir_name = base_input_dir_name+"/"+layer
input_dir_name = os.path.abspath(input_dir_name)
print("INFO Updating to calibration files in %s with tag %s for TRB %s"%(input_dir_name, tag, trb_name))
import time
time.sleep(2)


modcfgs = {}

for modcfg in os.listdir(input_dir_name):
  if not modcfg.startswith("Module"): continue
  if not modcfg.endswith(tag+".json"): continue
  print(" .. found modcfg = ",modcfg)
  mod_idx = modcfg[6:7]
  print(" .. mod_idx: ",mod_idx)
  full_modfcg_name = input_dir_name+"/"+modcfg
  if mod_idx in modcfgs:
    print("ERROR: File for module %s already found. FIX ME!"%(mod_idx))
    exit()
  modcfgs[mod_idx] = full_modfcg_name

if len(modcfgs) != 8:
  print("ERROR: Should have found exactly 8 new module configurations but %i found. FIX ME!"%(len(modcfgs)))
  exit()
#print(modcfgs)

trb_obj["modules"][0]["settings"][mod_cfg_field_name] = modcfgs
trb_objs[trb_name] = trb_obj

with open(TRBjson_out_str,'w') as trb_json_out_file:
    json.dump(trb_objs,trb_json_out_file, indent = 4, separators=(',', ': '))


