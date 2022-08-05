#!/usr/bin/env python3

#
#  Copyright (C) 2019-2022 CERN for the benefit of the FASER collaboration
#

from update_trb_setting import TRBSettings
import argparse

# CHECK PATH
TRBJSON_IN  = "/home/shifter/software/faser-daq/configs/Templates/TRB.json"
TRBJSON_OUT = TRBJSON_IN #"/home/cantel/faser-daq/configs/Templates/TRBNEW.json"

# base fine time setting per TRB "TRBReceiver<N>": [<HW delay>, <fine time delay>] 
TRB_reference_settings = {
                          "TRBReceiver11": [1,27],
                          "TRBReceiver12": [1,27],
                          "TRBReceiver13": [1,27],
                          "TRBReceiver00": [1,41],
                          "TRBReceiver01": [1,41],
                          "TRBReceiver02": [1,41],
                          "TRBReceiver03": [2,51],
                          "TRBReceiver04": [2,51],
                          "TRBReceiver05": [2,51],
                          "TRBReceiver06": [2,0],
                          "TRBReceiver07": [2,0],
                          "TRBReceiver08": [2,0]
                         }

def main():
    
    parser = argparse.ArgumentParser(description='Adjust TRB SCT timing settings w.r.t. to a reference given a fine time adjustment')
    parser.add_argument('--trb', type=str, help="Name of TRB in TRB.json. Specify 'all' if want to modify all TRBs in all 4 stations.")
    parser.add_argument('--adjust', type=int, help='Clock fine time adjustment from reference.')
    args=parser.parse_args()
    
    if not args.trb:
        print("ERROR TRB argument must be provided. Provide name of TRB as given in json config file or specify 'all'")
        exit()
    trb_list = (args.trb).split(',')
    fine_adjustment = args.adjust
    
    if len(trb_list)==1 and trb_list[0]=="all":
        all_trbs=[]
        for boardID in list(range(9))+list(range(11,14)):
            trb_name_i = f"TRBReceiver{str(0)*(boardID<10)}{boardID}"
            all_trbs.append(trb_name_i)
        trb_list = all_trbs
    
    ##
    # Calculate and set new Clk coarse and fine time setting:
    ##
    if fine_adjustment is None:
        print(f"ERROR value set for fine adjustment is invalid: {fine_adjustment}. Can't continue.")
        exit()
    print(f"\nINFO Applying fine time adjustment {fine_adjustment}")
    for trb_name in trb_list:
        if trb_name == "":
            print("ERROR no TRB name given. Provide name of TRB receiver, e.g. --trb TRBReceiver00")
            exit() 
        if not trb_name in TRB_reference_settings:
            print("ERROR TRB name not recognised!")
            exit()
    
        print(f"INFO: Updating TRB {trb_name}")
        ref_coarse_time, ref_fine_time = TRB_reference_settings[trb_name]
        coarse_adjust, clk_fine_time = divmod(ref_fine_time+fine_adjustment, 64)
        coarse_jump_adjust = clk_fine_time//47
        hw_delay = ref_coarse_time + coarse_adjust + coarse_jump_adjust
        hw_delay = min(2, max(0,hw_delay)) # hw_delay must be between 0 and 2 incl.
        #print(f"TRB: {trb_name}, ref_coarse: {ref_coarse}, ref_fine: {ref_fine}")
        #print(f"clk_fine_time: {clk_fine_time}")
        #print(f"hw_adjust: {hw_adjust}")
        print(f"INFO Computed new values:  HW delay={hw_delay}, Clk fine phase={clk_fine_time}")
        trb_settings = TRBSettings(TRBJSON_IN,TRBJSON_OUT)
        trb_settings.update(trb_name, ["HWDelayClk0","HWDelayClk1","FinePhaseClk0","FinePhaseClk1"],[hw_delay,hw_delay,clk_fine_time,clk_fine_time])

if __name__ == "__main__":
	main()
