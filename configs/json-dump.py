#!/usr/bin/env python3

# small tool to check loading of multi-file json loads fine

import os
import sys

import json
import jsonref
from pathlib import Path
from urllib.parse import urljoin

from daqcontrol import jsonref_to_json

base_dir_url = Path(os.path.realpath(os.getcwd())).as_uri() + '/'
base_file_url = urljoin(base_dir_url, sys.argv[1])

jsonref_obj = jsonref.load(open(sys.argv[1],"r"),base_uri=base_file_url)
conv=jsonref_to_json(jsonref_obj)
print(json.dumps(conv, sort_keys=True, indent=4))

