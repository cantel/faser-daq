#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#
from flask import Flask
from flask_apscheduler import APScheduler

app = Flask(__name__)
scheduler = APScheduler()

app.config.from_object(__name__)

app.config['SECRET_KEY'] = '829a5192351b4463577b9f8b'

from flaskDashboard import routes


