#
# Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

# Author : Sam Meehan
#
# Description : Used for validating schemas and configs in the FASER CI

# https://python-jsonschema.readthedocs.io/en/v1.1.0/validate.html#the-validator-interface

import argparse
import os
import json
from jsonschema import validate
from jsonschema import Draft7Validator # could also be imported for different drafts
import jsonref

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('-d', '--directory', type=str,   help= '')
  parser.add_argument('-s', '--schemas',   nargs='+',    help= '')
  parser.add_argument('-c', '--configs',   nargs='+',    help= '')
  parser.add_argument('-t', '--templates', nargs='+',    help= '')
  parser.add_argument('-e', '--extras',    nargs='+',    help= '')
  
  args = parser.parse_args()
  directory       = args.directory
  schemas         = args.schemas
  configs         = args.configs
  templates       = args.templates
  extras          = args.extras

  print("directory   ",directory )
  print("schemas     ",schemas   )
  print("configs     ",configs   )
  print("templates   ",templates )
  print("extras      ",extras    )
  

  # check all of the schemas for validity
  for path_to_schema in schemas:
    print("Checking : ",path_to_schema)
    valid = checkSchema(directory+"/"+path_to_schema)
    print(path_to_schema," -- ",valid)
    
  # check each of the templates against the schemas
  for path_to_template in templates:
    print("Checking : ",path_to_template)
    
    filepath = directory+"/"+path_to_template

    # get full template
    fin_cfg = open(filepath)
    cfg = json.load(fin_cfg)
    # go through each of the entries in the template
    for key in cfg.keys():
      
      # get config type
      cfg_type   = cfg[key]["type"]
      # form schema path
      cfg_schema = directory+"/schemas/"+cfg_type+".schema"
      print("Validating against : ",cfg_schema)                
      
      print("Checking config : ",filepath, cfg_schema, directory, [key])
      result_config = checkConfig(filepath, cfg_schema, directory, [key])
      print("Config validity : ",result_config)
      
  # check each of the top level configs
  for path_to_config in configs:
    print("Checking : ",path_to_config)
    filepath = directory+"/"+path_to_config
    cfg = getConfig(filepath, directory)

    for cfg_sub in cfg["configuration"]["components"]:
      
      # get config type
      cfg_name   = cfg_sub["name"]
      cfg_type   = cfg_sub["type"]
      # form schema path
      cfg_schema = directory+"/schemas/"+cfg_type+".schema"
      print("Validating - ",cfg_name," | ",cfg_type," - against : ",cfg_schema)                
    
      result_config = checkConfig(cfg_sub, cfg_schema, directory)
    
      print("Config validity : ",result_config)
    
  # finally, check that all files in the repository have been accounted for
  allfiles = []
  for i in schemas:
    allfiles.append(os.path.join(directory,i))
  for i in templates:
    allfiles.append(os.path.join(directory,i))
  for i in configs:
    allfiles.append(os.path.join(directory,i))
  for i in extras:
    allfiles.append(os.path.join(directory,i))
    
  print(allfiles)

  countMissing = 0
  for root, dirs, files in os.walk(directory, topdown=False):
    for name in files:  
      filepath = os.path.join(root, name)
      
      if filepath not in allfiles:
        print("Missing : ",filepath)
        countMissing+=1
        
  # if you haven't checked some file then its not acceptable
  if countMissing!=0:
    exit(1)
  
                      

# used for loading the list of multiple types
def int_or_str(value):
  try:
    return int(value)
  except:
    return value

# validate a schema against 
def checkSchema(path_to_schema):

  if type(path_to_schema) is str:
    fin_schema = open(path_to_schema,"r")
    fin_schema_str = ""
    for line in fin_schema.readlines():
      fin_schema_str += line
    schema = json.loads(fin_schema_str)
  else:
    # assuming the thing is json schema already
    schema = path_to_schema

  try:
    Draft7Validator.check_schema(schema)
    return True
  except:
    Draft7Validator.check_schema(schema)
    return False
  
# validate config against schema
def checkConfig(path_to_config, path_to_schema, directory, entry=False):

  # get the schema
  if type(path_to_schema) is str:
    fin_schema = open(path_to_schema)
    schema = json.load(fin_schema)
  else:
    # assuming the thing is json schema already
    schema = path_to_schema
 
  result_schema = checkSchema(schema)
  print("Schema validity : ",result_schema)
  
  # get the config
  if type(path_to_config) is str:    
    cfg = getConfig(path_to_config, directory)
    
  else:
    print("Direct Config File")
    # assuming the thing is json schema already
    cfg = path_to_config
    
  # tunnel down into the config if there is a subpath to test
  cfg_to_test = cfg
  if entry!=False:
    for tunnel in entry:
      cfg_to_test = cfg_to_test[tunnel]

  try:
    validate(instance=cfg_to_test, schema=schema)
    return True
  except:
    validate(instance=cfg_to_test, schema=schema)
    return False
    
    
def getConfig(path_to_config, directory):
  fin_cfg = open(path_to_config)
  cfg_init = json.load(fin_cfg)
  
  base_path = os.path.abspath(directory)
  if not base_path.endswith("/"):
    base_path = base_path + "/"
  if os.name == "nt":
    base_uri_path = "file:///" + base_path.replace('\\', '/')
  else:
    base_uri_path = "file://" + base_path

  loader = jsonref.JsonLoader(cache_results=False)

  cfg_str = json.dumps(cfg_init)
  cfg = jsonref.loads(cfg_str, base_uri=base_uri_path, loader=loader, jsonschema=True)

  return cfg

if __name__=="__main__":
  main()

