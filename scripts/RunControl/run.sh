#export FLASK_APP=./daqControl/ControlGUIServer.py
#export FLASK_ENV=development
# python3 -m flask run --host=localhost
#export PYTHONPATH=/home/egalanta/latest/faser-daq/scripts/RunControl/Control:$PYTHONPATH
python3 daqControl/ControlGUIServer.py $1
