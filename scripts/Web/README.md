Dependencies:
```
pip3 install Flask
pip3 install Flask-Restful
```

Run test server: 
```
python3 rest.py
```

Go to URL:
```
http://rd51pro:5000/metrics
```

Post data with curl with the available metrics (datarate and cpuutil):
```
curl -d '{"value":5}' -H "Content-Type: application/json" -X POST http://rd51pro:5000/add/datarate
```

Should be extended to register/unregister metrics and adding highcharts for real-time data visualization.
(And proper storage backend... we can start with an in-memory one.)

