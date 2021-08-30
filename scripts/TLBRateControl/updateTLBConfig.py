#!/usr/bin/env python3

import json

TLBjson_str = "/home/cantel/faser-daq/configs/Templates/TLB.json"
TLBjson_out_str = "/home/cantel/faser-daq/configs/Templates/TLB.json"

def main():
  
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', "-n", type=str, help='Name of TLB configs')
    parser.add_argument('--random', "-r", default=-1, type=int, help='Set random trigger rate')
    parser.add_argument('--deadtime', "-d", default=-1, type=int, help='Set deadtime setting')
    args=parser.parse_args()
    
    tlb_name = args.name
    if tlb_name == "":
        print("ERROR no TRB name given. Provide name of TRB receiver, e.g. --trb TRBReceiver00")
        exit()
    rnd_rate = args.random
    deadtime = args.deadtime 
    
    with open(TLBjson_str, 'r') as TLBjson_file:
        data=TLBjson_file.read()
    TLBjson_file.close()
    
    tlb_objs = json.loads(data)
    
    tlb_obj = tlb_objs[tlb_name]["modules"][0]
    
    
    #import time
    #time.sleep(2)
    
    if rnd_rate>=0:
      print(f"Adjusting random rate to {rnd_rate}")
      tlb_obj["settings"]["RandomTriggerRate"] = rnd_rate
      tlb_objs[tlb_name]["modules"][0] = tlb_obj
    if deadtime>=0:
      print(f"Adjusting deadtime to {deadtime} BC")
      tlb_obj["settings"]["Deadtime"] = deadtime
      tlb_objs[tlb_name]["modules"][0] = tlb_obj
    
    with open(TLBjson_out_str,'w') as tlb_json_out_file:
        json.dump(tlb_objs,tlb_json_out_file, indent = 4, separators=(',', ': '))

if __name__ == "__main__":
        main()
