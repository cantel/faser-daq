import json
import jsonref
from pathlib import Path
import requests
from os import environ as env

def local(path):
  with open(path) as f:
    base_dir_uri = Path(path).resolve().parent.as_uri() + '/'
    jsonref_obj = jsonref.load(f, base_uri=base_dir_uri, loader=jsonref.JsonLoader())
    f.close()
  return jsonref_obj

def printListAndExit(host, port):
  try:
    r = requests.get('http://'+host+':'+str(port)+'/listConfigs')
  except requests.exceptions.RequestException as e:
    print("Request failed, check that service is running.")
    print("Error message:")
    raise SystemExit(e)
  return r.json()['configs']

def remote(name, host, port):
  try:
    r = requests.get('http://'+host+':'+str(port)+'/retrieveLast?name='+name)
  except requests.exceptions.RequestException as e:  # This is the correct syntax
    print("Request failed, check that service is running.")
    print("Error message:")
    raise SystemExit(e)
  if(r.status_code==200):
    
    response_json = json.loads(r.json())
    deps_dir = env['DAQ_CONFIG_DIR']+name
    # base_dir_uri = Path(deps_dir).as_uri() + '/'

    deps_dir = deps_dir + "_v" + str(response_json['version']) +'/'

    #make sure not to override existing files without permission.
    while(Path(deps_dir).exists()):
      print("Path: \""+deps_dir+"\" Already exists.")
      key_input=input("Enter:\n \"f\" to force into dir (might overwride some files).\n \"q\" to exit.\n \"n\" to enter a new target directory name.\nInput: ")
      if(key_input=="q"):
        exit()
      elif(key_input=="f"):
        if Path(deps_dir).is_file():
          os.remove(deps_dir)
        break
      elif(key_input=="n"):
        new_dir=input("Enter name of new directory.(just folder name, not full path.)\nInput: ")
        deps_dir=env['DAQ_CONFIG_DIR']+new_dir+'/'
      else:
        print("Invalid input, try again!\n")
    Path(deps_dir).mkdir(parents=True, exist_ok=True)

    for f in response_json["files"]:
      # open file with w+
      name = response_json["files"][f]['name']
      file_content = json.loads(response_json["files"][f]['file_content'])
      # if name=="config":
      #   configs_json=response_json["deps"][dep]['file_content']
      with open(deps_dir+name+'.json','w+') as template_file:
        json.dump(file_content, template_file, indent=2)
    # jsonref_obj = jsonref.loads(configs_json, base_uri=base_dir_uri, loader=jsonref.JsonLoader())
    return deps_dir
  else:
    print("Unexpected http status code: "+r.status_code)
    exit()