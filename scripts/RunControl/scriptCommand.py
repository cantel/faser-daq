#!/usr/bin/env python3

# The only purpose of this script is to be called from a sequence to test the sequence

import sys,time
from subprocess import getstatusoutput

def error() : 
    code, output = getstatusoutput("cat test.txt")
    if code :
        print("Error",output)
        exit(1)
    else :
        print(output)
    
if sys.argv[1]  == "initCommand" : 
    print("precommand")
elif sys.argv[1] == "finalizeCommand" : 
    # error()
    print("finalizeCommand")
elif "preCommand" in sys.argv[1] :  
    print("preCommand")
elif ("postCommand" in sys.argv[1]) or ("customPostCommand" in sys.argv[1])  : 
    print("postCommand")
time.sleep(2)