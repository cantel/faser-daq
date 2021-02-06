#!/usr/bin/env python3
#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

import os
import sys

import json
from jsonschema import validate

if len(sys.argv)<2 or len(sys.argv)>3:
  print(f"Usage: {sys.argv[0]} <jsonFile> [ModuleName]")
  sys.exit(1)

inputJSON=sys.argv[1]
moduleName=None
if len(sys.argv)>2:
  moduleName=sys.argv[2]
  

with open(inputJSON) as f:
  data = json.load(f)
  f.close()

with open("json-config.schema") as f:
  schema = json.load(f)
  f.close()

try:
  validate(instance=data, schema=schema)
except AssertionError as error:
  print("Failed to validate json file against top-level schema")
  print("Exception", error)
  sys.exit(1)

for comp in data['components']:
  if not moduleName or comp['type']==moduleName:
    print(f"Validating: {comp['name']} (type={comp['type']})")
    schemaFile=f"schemas/{comp['type']}.schema"
    if os.access(schemaFile,os.R_OK):
      with open(schemaFile) as f:
        compschema = json.load(f)
      f.close()
      try:
        myschema=schema['properties']['components']['items']
        myschema['name']=compschema['properties']['name']
# Brian: not sure why we can't check that type is specified correctly
#        myschema['type']=compschema['properties']['type']
        myschema['settings']=compschema['properties']['settings']
        validate(instance=comp, schema=myschema)
      except AssertionError as error:
        print("Failed to validate against component schema")
        print("Exception", error)
        sys.exit(1)
    else:
      print("No schema file found")
      
  
