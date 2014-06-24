#Aaron Fienberg
#Fienberg@uw.edu
#online display for SLAC test beam run
from gevent import monkey
monkey.patch_all()

from flask import Flask, render_template, send_from_directory, redirect, url_for
from flask import session
from flask.ext.socketio import SocketIO, emit
from werkzeug.utils import secure_filename
from uuid import uuid4
import os, glob
import threading

import data_io
from time import sleep

import numpy as np
from matplotlib import pyplot as plt

app = Flask(__name__)
app.config.update(dict(
    UPLOAD_FOLDER='uploads',
    IMAGE_UPLOADS=['.jpg', '.jpe', '.jpeg', '.png', '.gif', '.svg', '.bmp'],
    DEBUG=True))
app.config['SECRET_KEY'] = '\xf5\x1a#qx%`Q\x88\xd1h4\xc3\xba1~\x16\x11\x81\t\x8a?\xadF'
socketio = SocketIO(app)

@app.route('/')
def home():
    if 'running' not in session:
        session['running'] = False
    return render_template('layout.html')

def send_events():
    while not data_io.e.isSet():
        data_io.lock.acquire()
        socketio.emit('event info', {"count" : data_io.eventCount, "rate" : data_io.rate},
                      namespace='/test')
        data_io.lock.release()
        sleep(0.1)

@socketio.on('refreshed', namespace='/test')
def on_refresh():
    if session['running']:
        data_io.lock.acquire()
        socketio.emit('event info', {"count" : data_io.eventCount, "rate" : data_io.rate},
                      namespace='/test')
        data_io.lock.release()

@app.route('/start')
def start_run():
    session['running'] = True
    print 'attempting to begin run'
    data_io.begin_run()
    
    t = threading.Thread(name='emitter', target=send_events)
    t.start()
    
    sleep(0.2)
    return redirect(url_for('running_hist'))

@app.route('/end')
def end_run():
    session['running'] = False
    data_io.end_run()
    return redirect(url_for('running_hist'))

@app.route('/hist')
def running_hist():
    filepath = update_hist()
    if filepath == 'failed':
        return render_template('no_data.html')
    return render_template('hist.html', path=filepath)

@app.route('/traces')
def traces():
    filepath = generate_traces()
    return render_template('traces.html', path=filepath)

@app.route('/nodata')
def no_data():
    return render_template('no_data.html')

@app.route('/reset')
def reset():
    data_io.clear_data()
    return redirect(url_for('home'))

def update_hist():
    plt.clf()
  
    data_io.lock.acquire()
    try:
        plt.hist(data_io.data)
        plt.title('Event ' + str(data_io.eventCount))
        data_io.lock.release()
    except IndexError:
        data_io.lock.release()
        return 'failed'

    for temp_file in glob.glob(app.config['UPLOAD_FOLDER'] + '/temp_hist*'):
        os.remove(temp_file)
    filename = unique_filename('temp_hist.png')
    filepath = upload_path(filename)
    plt.savefig(filepath)
    
    return filepath

@socketio.on('connect', namespace='/test')
def test_connect():
    emit('my response', {'data': 'Connected'})

def generate_traces():
    trace = data_io.fill_trace()
    
    plt.clf()
    for i in xrange(4):
        for j in xrange(7):
            plt.subplot(4, 7, i*7+j+1)
            plt.axis('off')
            plt.plot(trace)

    for tempFile in glob.glob(app.config['UPLOAD_FOLDER'] + '/temp_traces*'):
        os.remove(tempFile)
    filename = unique_filename('temp_traces.png')
    filepath = upload_path(filename)
    plt.savefig(filepath)
    
    return filepath
    
    
def unique_filename(upload_file):
     """Take a base filename, add characters to make it more unique, and ensure that it is a secure filename."""
     filename = os.path.basename(upload_file).split('.')[0]
     filename += '-' + str(uuid4().hex[-12:])
     filename += os.path.splitext(upload_file)[1]
     return secure_filename(filename)

def upload_path(filename):
    """Construct the full path of the uploaded file."""
    basename = os.path.basename(filename)
    return os.path.join('uploads', basename)

@app.route('/uploads/<path:filename>')
def get_upload(filename):
    """Return the requested file from the server."""
    filename = os.path.basename(filename)
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename)

if __name__ == '__main__':
    socketio.run(app)
