#!/usr/bin/env python3
import json
import platform
import redis
from flask import Flask, request, jsonify, render_template
#import metricsHandler
import histogramHandler
from routes.histogram import histogram_blueprint

app  = Flask(__name__) 
app.register_blueprint(histogram_blueprint)
app.debug = True

r1 = redis.Redis(host="localhost", port= 6379, db=0, charset="utf-8", decode_responses=True)

@app.route("/")
def home():
    return render_template('monitoringHome.html')

@app.route("/histograms/<component>")
def histograms(component):
    return render_template(component+'_dashboard.html')
     
     

if __name__ == '__main__':
    port=8000
    histo=histogramHandler.Histo(app.logger)
    print(" http://%s:%d" % (platform.node(),port))
    from werkzeug.serving import run_simple
    run_simple("0.0.0.0",port,app,threaded=True)
    histo.stop()
