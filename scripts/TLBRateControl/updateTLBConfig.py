#!/usr/bin/env python3

import json

TLBjson_str = "/home/cantel/daq/configs/Templates/TLB.json"
TLBjson_out_str = "/home/cantel/daq/configs/Templates/TLB.json"

def main():
  
  import argparse
  parser = argparse.ArgumentParser()
  parser.add_argument('--name', "-n", type=str, help='Name of TLB configs')
  parser.add_argument('--random', "-r", default=-1, type=int, help='Set random trigger rate')
  args=parser.parse_args()
  
  tlb_name = args.name
  rnd_rate = args.random
  
  
  with open(TLBjson_str, 'r') as TLBjson_file:
      data=TLBjson_file.read()
  TLBjson_file.close()
  
  tlb_objs = json.loads(data)
  
  tlb_obj = tlb_objs[tlb_name]
  
  
  #import time
  #time.sleep(2)
  
  if rnd_rate>=0:
    print("Adjusting random rate to %i"%(rnd_rate))
    tlb_obj["settings"]["RandomTriggerRate"] = rnd_rate
    tlb_objs[tlb_name] = tlb_obj
  
  with open(TLBjson_out_str,'w') as tlb_json_out_file:
      json.dump(tlb_objs,tlb_json_out_file, indent = 4, separators=(',', ': '))

if __name__ == "__main__":
        main()
