#!/bin/bash

echo "Get run number"
runno=`curl -s -u FASER:HelloThere -d '{"type":"calibration", "version":"v1.2.0", "configuration": {"a":"b","c":[1,2,3]}, "valid":false}' -H "Content-Type: application/json" -X POST http://faser-daq-001:5002/NewRunNumber`
echo "Got: $runno"
echo "End run and set some run data"
curl -u FASER:HelloThere -d '{"runinfo": {"d": [3,45]}}' -H "Content-Type: application/json" -X POST http://faser-daq-001:5002/AddRunInfo/$runno
echo "Get run information"
curl http://faser-daq-001:5002/RunList
curl http://faser-daq-001:5002/RunInfo/$runno
