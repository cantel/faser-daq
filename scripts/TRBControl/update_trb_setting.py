#!/usr/bin/env python3

#
#  Copyright (C) 2019-2022 CERN for the benefit of the FASER collaboration
#

import json
import time
import copy

# CHECK PATH
TRBJSON_IN = "/home/cantel/faser-daq/configs/Templates/TRB.json"
TRBSJSON_OUT = "/home/cantel/faser-daq/configs/Templates/TRB.json"

class TRBSettings:
    def __init__(self, json_in, json_out):
        self.TRBjson_str = json_in
        self.TRBjson_out_str = json_out
        with open(self.TRBjson_str, 'r') as TRBjson_file:
            data=TRBjson_file.read()
        TRBjson_file.close()
        self.trb_objs = json.loads(data)
    
    def update(self,trb_name, names,values, val_indices=-1):

        if trb_name == "":
          print("ERROR no TRB name given. Provide name of TRB receiver, e.g. --trb TRBReceiver00")
          return
        total_updates = len(names)
        if not total_updates == len(values):
            print(f"ERROR Number of field names ({total_updates}) does not match number of values ({len(values)}). Provide a matching list, e.g. --name FinePhaseClk0,FinePhaseClk1 --value 16,32")
            return

        trb_obj = copy.deepcopy(self.trb_objs[trb_name])
        #print(trb_obj)
        trb_id = int(trb_name[-2:])
        
        
        for idx in range(total_updates):
            name = names[idx]
            value = values[idx]
            if isinstance(val_indices,list):
                try:
                   val_idx = val_indices[idx]
                except: 
                   print("ERROR Not enough value indices provided. Number of value indices ({len(val_indices)}) must equal values ({len(values)}). Will not update property.")
                   return
            else:
                val_idx = val_indices
            if val_idx >= 0:
                if not isinstance(trb_obj["modules"][0]["settings"][name],list):
                    print("ERROR a value index is given, but property is not a list type. Will not update property.")
                    return
                #print(f"INFO Updating TRB {trb_name}: Setting {name}, index {val_idx} to value {value}")
                trb_obj["modules"][0]["settings"][name][val_idx] = value
            else:
                if isinstance(trb_obj["modules"][0]["settings"][name],list):
                    print("ERROR This property is a list type, but no index for the value to be updated given.  Will not update property.")
                    return
                #print(f"INFO Updating TRB {trb_name}: Setting {name} to value {value}")
                trb_obj["modules"][0]["settings"][name] = value
            if "FinePhaseClk" in name:  # if Clk fine phase is updated, LED fine phase is to be updated accordingly
                ledphase_name = "FinePhaseLed"+name[-1]
                old_val = self.trb_objs[trb_name]["modules"][0]["settings"][name]
                old_ledphase = trb_obj["modules"][0]["settings"][ledphase_name]
                ledphase = (old_ledphase+value-old_val)%64
                print(f"INFO [automatic update] Updating TRB {trb_name}: Setting {ledphase_name} to value {ledphase}")
                trb_obj["modules"][0]["settings"][ledphase_name] = ledphase
            time.sleep(0.1)
        self.trb_objs[trb_name] = trb_obj
        
        with open(self.TRBjson_out_str,'w') as trb_json_out_file:
            json.dump(self.trb_objs,trb_json_out_file, indent = 4, separators=(',', ': '))
    
def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--trb', type=str, help='Name of TRB in TRB.json')
    parser.add_argument('-n', '--name', type=str, help='Property name(s) to be set. Can be comma-separated list')
    parser.add_argument('-v', '--value', type=str, help='Property value(s) to be set. Can be comma-separated list')
    parser.add_argument('--idx', type=int, default=-1, help='Index of value in case property is an array. Can be comma-separated list')
    args=parser.parse_args()

    trb_name = args.trb
    names = (args.name).split(',')
    values = (args.value).split(',')
    val_indices = (args.idx).split(',')

    trb_settings = TRBSettings(TRBJSON_IN,TRBSJSON_OUT)
    trb_settings.update(trb_name,names,values,val_indices)

if __name__ == "__main__":
	main()
