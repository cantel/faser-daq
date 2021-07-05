#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
from numpy import array
import redis
import json

def get_stored_histogram(r_db: redis.client.Redis, ID: str, timestamp:str):
    hist =""
    if "old" in timestamp:
        hist = json.loads(r_db.hget(f"old:{ID}", timestamp))
    else:   
        hist = json.loads(r_db.hget(ID, timestamp))
    return hist

    