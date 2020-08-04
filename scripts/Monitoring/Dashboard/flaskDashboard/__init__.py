from flask import Flask
from flask_session import Session

app = Flask(__name__)
SESSION_TYPE = 'filesystem'
app.config.from_object(__name__)
Session(app)
# app.config['SECRET_KEY'] = '5791628bb0b13ce0c676dfde280ba245'

from flaskDashboard import routes
