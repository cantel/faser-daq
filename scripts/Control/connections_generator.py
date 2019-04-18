import json

with open("config.2.json") as f:
  data = json.load(f)
f.close()

# sorts the processes based on their stream_level
data.sort(key = lambda x: x["stream_level"])

data_port = 8001
connections = [{"sources": {}, "destinations": {}} for x in range(len(data))]
ch_count = [1 for x in range(len(data))]

for idx, p in enumerate(data):
  print(p)
  lvl = p['stream_level']
  for idx1, p1 in enumerate(data):
    if lvl == p1['stream_level'] - 1:
      dsts = len(connections[idx]["destinations"])
      print("dsts",dsts)
      connections[idx]["destinations"].update({str(dsts+1): {"chid": ch_count[idx], "host": p1['host'], "port": data_port}})
      ch_count[idx] = ch_count[idx] + 1

      srcs = len(connections[idx1]["sources"])
      print("srcs",srcs)
      connections[idx1]["sources"].update({str(srcs+1): {"chid": ch_count[idx1], "host": "*", "port": data_port}})
      ch_count[idx1] = ch_count[idx1] + 1

      data_port = data_port + 1
      
print("===================")
print(connections)


