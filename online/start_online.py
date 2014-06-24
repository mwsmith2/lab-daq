#Aaron Fienberg
#Fienberg@uw.edu
#online display for SLAC test beam run

from flask import Flask, render_template, send_from_directory, redirect, url_for
from flask import session
from werkzeug.utils import secure_filename
from uuid import uuid4
import os, glob

import data_io
from time import sleep

import numpy as np
from matplotlib import pyplot as plt

app = Flask(__name__)
app.config.update(dict(
    UPLOAD_FOLDER='uploads',
    IMAGE_UPLOADS=['.jpg', '.jpe', '.jpeg', '.png', '.gif', '.svg', '.bmp'],
    DEBUG=True))

@app.route('/')
def home():
    if 'running' not in session:
        session['running'] = False
    return render_template('layout.html')

@app.route('/start')
def start_run():
    session['running'] = True
    session['events'] = 0
    print 'attempting to begin run'
    data_io.begin_run()
    sleep(.5)
    return redirect(url_for('running_hist'))

@app.route('/end')
def end_run():
    session['running'] = False
    data_io.end_run()
    return redirect(url_for('home'))

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
    except IndexError:
        data_io.lock.release()
        return 'failed'
    session['events'] = len(data_io.data)
    data_io.lock.release()

    for temp_file in glob.glob(app.config['UPLOAD_FOLDER'] + '/temp_hist*'):
        os.remove(temp_file)
    filename = unique_filename('temp_hist.png')
    filepath = upload_path(filename)
    plt.savefig(filepath)
    
    return filepath

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
    app.secret_key = '\xf5\x1a#qx%`Q\x88\xd1h4\xc3\xba1~\x16\x11\x81\t\x8a?\xadF'
    app.run();
