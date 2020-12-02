#!/usr/bin/env python3

import requests
import sys

print("Requesting run number")
r = requests.post('http://faser-daq-001:5002/NewRunNumber',
                  auth=("FASER","HelloThere"),
                  json = {'version':'123',
                          'type':'physics',
                          'configName':'ScintillatorOnly',
                          'configuration':
                          { "ch1": True,
                            "ch2": False
                          }
                  })
if r.status_code!=201:
    print("Failed to get run number:",r.text)
    sys.exit(-1)
try:
    runno=int(r.text)
except ValueError:
    print("Failed with",r.text)
    sys.exit(-1)
print(f"Got run number {runno}")
print("Mark run stop and add run info")
r = requests.post(f'http://faser-daq-001:5002/AddRunInfo/{runno}',
                  auth=("FASER","HelloThere"),
                  json = {"runinfo":
                          { "numberOfEvents": 100,
                            "Errors": 200}
                  })
if r.status_code!=200:
    print("Failed to get run number:",r.text)
    sys.exit(-1)
if not "success" in r.json():
    print("Failed with:",r.text)
    sys.exit(-1)
print("Get run information")
r = requests.get(f'http://faser-daq-001:5002/RunInfo/{runno}')
if r.status_code==200:
    print(r.json())
else:
    print("Failed to get run information",r.text)

