import json
from jsonschema import validate

with open("config.1.json") as f:
  data = json.load(f)
f.close()

with open("json-config.schema") as f:
  schema = json.load(f)
f.close()

try:
    validate(instance=data, schema=schema)
except AssertionError as error:
    print("Exception", error)
