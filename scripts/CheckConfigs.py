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
  
  if directory==None:
    print("You must give a base directory")
    exit(3)
  

  # check all of the schemas for validity
  print("\n\n >>>>>> CHECKING SCHEMAS <<<<<< \n\n")
  if schemas==None:
    print("None entered to check")
  else:
    for path_to_schema in schemas:
      print("Checking : ",path_to_schema)
      valid = checkSchema(directory+"/"+path_to_schema)
      print(path_to_schema," -- ",valid)
    
  # check each of the templates against the schemas
  print("\n\n >>>>>> CHECKING TEMPLATES <<<<<< \n\n")
  if templates==None:
    print("None entered to check")
  else:
    for path_to_template in templates:
      print("Checking : ",path_to_template)
    
      filepath = directory+"/"+path_to_template

      # get full template
      fin_cfg = open(filepath)
      cfg = json.load(fin_cfg)
      # go through each of the entries in the template
      for key in cfg.keys():
      
        # Not a valid component, so can't validate.
        if "modules" not in cfg[key]:
          break

        i = 0
        for modules in cfg[key]["modules"]:
          # get config type
          cfg_type   = modules["type"]
          cfg_name   = modules["name"]
          # form schema path
          cfg_schema = directory+"/schemas/"+cfg_type+".schema"
          print("Validating against : ",cfg_schema)                

          print("Checking config : ",filepath, cfg_schema, directory, [key],cfg_name)
          result_config = checkConfig(filepath, cfg_schema, directory, [key],[i])
          print("Config validity : ",result_config)
          i+=1
      
  # check each of the top level configs
  print("\n\n >>>>>> CHECKING CONFIGS <<<<<< \n\n")
  if configs==None:
    print("None entered to check")
  else:
    for path_to_config in configs:
      print("Checking : ",path_to_config)
      filepath = directory+"/"+path_to_config
      cfg = getConfig(filepath, directory)

      cfg_schema = directory+"/schemas/validation-schema.json"
      print("Validating - ",path_to_config," | ","Components"," - against : ",cfg_schema)
      result_config = checkConfig(cfg["configuration"],cfg_schema,directory)
      print("Config validity : ",result_config)

      for cfg_sub in cfg["configuration"]["components"]:
        for module_cfg in cfg_sub["modules"]:
          # get config type
          cfg_name   = module_cfg["name"]
          cfg_type   = module_cfg["type"]
          # form schema path
          cfg_schema = directory+"/schemas/"+cfg_type+".schema"
          print("Validating - ",cfg_name," | ",cfg_type," - against : ",cfg_schema)                
      
          result_config = checkConfig(module_cfg, cfg_schema, directory)
      
          print("Config validity : ",result_config)
    
  # finally, check that all files in the repository have been accounted for
  print("\n\n >>>>>> CHECKING EXTRAS <<<<<< \n\n")
  allfiles = []
  if schemas!=None:
    for i in schemas:
      allfiles.append(os.path.join(directory,i))
  if templates!=None:    
    for i in templates:
      allfiles.append(os.path.join(directory,i))
  if configs!=None:
    for i in configs:
      allfiles.append(os.path.join(directory,i))
  if extras!=None:
    for i in extras:
      allfiles.append(os.path.join(directory,i))

  countMissing = 0
  for root, dirs, files in os.walk(directory, topdown=False):
    for name in files:  
      filepath = os.path.join(root, name)
      if filepath not in allfiles:
        print("Missing : ",filepath)
        countMissing+=1
  
  if countMissing==0:
    print("Great job, you checked all the files successfully!")
        
  # if you haven't checked some file then its not acceptable
  if countMissing!=0:
    print("There are some files in the config directory of the repo that are not accounted for")
    print("They need to be categorized as either : ")
    print("schemas   : These are the files that dictate the format a config should take.")
    print("templates : These are the files that contain common configuration settings used for various subsystem processes.")
    print("configs   : These are the top level configs that often make reference to templates.")
    print("extras    : These are any extra files that exist in the directory. For example, they may be little json bits that are referenced by templates or just extra files (that should perhaps be deleted.)")
    print("Go check out scripts/runConfigCheck.sh and it should be obvious what to change")
    exit(1)
  
                      

# used for loading the list of multiple types
def int_or_str(value):
  try:
    return int(value)
  except:
    return value

# validate a schema against 
def checkSchema(path_to_schema):
  #print("Checking schema : ",path_to_schema)
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
def checkConfig(path_to_config, path_to_schema, directory, entry=False,modules_entry=False):
  #print("Checking config : ",path_to_config," at ",directory," tunelling to location [",entry,"] with schema ", path_to_schema)
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
    if modules_entry !=False:
      for modules_tunnel in modules_entry:
        cfg_to_test = cfg_to_test["modules"][modules_tunnel]

  try:
    validate(instance=cfg_to_test, schema=schema)
    return True
  except:
    validate(instance=cfg_to_test, schema=schema)
    return False
    
    
def getConfig(path_to_config, directory):
  print("Getting config [",path_to_config,"] from [",directory,"]")
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

