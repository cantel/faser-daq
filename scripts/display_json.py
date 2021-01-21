#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser(description='Output plots contained in given json file.')
parser.add_argument('-i', '--input', type=str, help= 'path_to_input_json.json')

args = parser.parse_args()
file_name = args.input 
print("opening file ", file_name)

import json
with open(file_name) as infile:
        data = json.load(infile)

for histobj in data:
    print(histobj)
    print("drawing ", histobj["name"])

    x = histobj["x"]
    y = histobj["y"]
    n = [ i for i in range(len(y))] 
   
    fig, ax = plt.subplots() 
    if histobj["type"] == "category":
        ax.bar(n, y, color = 'b' )
        ax.set_xticks( [ i+0.5 for i in range(len(x))] )
        ax.set_xticklabels(x,rotation=45)
    elif histobj["type"] == "graph":
        ax.plot( x,y )  
        plt.xlim(x[0], x[len(x)-1])
    else:
        ax.bar(n, y, color = 'b' )
        plt.xlim(x[0], x[len(x)-1])
    
    hist_title = histobj["title"]
    plt.title(hist_title)    
    plt.xlabel(histobj["xlabel"])
    plt.ylabel(histobj["ylabel"])
    
    plt.show()
    
    input("ok")
