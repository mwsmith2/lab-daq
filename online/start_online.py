#Aaron Fienberg
#Fienberg@uw.edu
#online display for SLAC test beam run

from gevent import monkey
monkey.patch_all()

from flask import Flask, render_template, send_from_directory, redirect, url_for
from flask import g, request
from flask.ext.socketio import SocketIO, emit
from werkzeug.utils import secure_filename
from uuid import uuid4
import couchdb
import os, glob
import threading

import data_io
from time import sleep

import numpy as np
from matplotlib import pyplot as plt

#setup flask app
app = Flask(__name__)
app.config.update(dict(
    UPLOAD_FOLDER=os.path.dirname(os.path.realpath(__file__))+'/uploads',
    IMAGE_UPLOADS=['.jpg', '.jpe', '.jpeg', '.png', '.gif', '.svg', '.bmp'],
    DEBUG=True))
app.config['SECRET_KEY'] = '\xf5\x1a#qx%`Q\x88\xd1h4\xc3\xba1~\x16\x11\x81\t\x8a?\xadF'
socketio = SocketIO(app)


#Define attributes of a run
run_info = {}
run_info['db_name'] = 'run_db'
run_info['attr'] = ['Description', 'Table x', 'Table y', 'Beam Energy [GeV]']
run_info['runlog'] = 'runlog.csv'

#global flag denoting whether or not a run is in progress
running = False

@app.route('/')
def home():
    """display home page of online DAQ"""
    return render_template('home.html', in_progress=running)

@app.route('/new')
def new_run():
    """a new run is about to begin. Load the data from 
    the previous run and prompt user"""
    
    last_data = {}
    if run_info['last_run'] != 0:
        db = connect_db(run_info['db_name'])
        last_data = db[db['toc'][str(run_info['last_run'])]]
        last_data['Description'] = ''

    return render_template('new_run.html', info=run_info, data=last_data, new=True)

@app.route('/start', methods=['POST'])
def start_run():
    """Check user form for completion. If it is complete, start the run.
    This entails saving the user configuration in couchdb and 
    launching the data getter and emitter"""
    data = copy_form_data(run_info, request)
    complete = check_form_data(run_info, data)
    if not complete:
        error = "All fields in the form must be filled."
        return render_template('new_run.html', info=run_info, 
                                   data=data, error=error, new=True )

    #save the run info
    db = get_db(run_info['db_name'])
    run_info['last_run'] += 1
    data['run_number'] = run_info['last_run']
    save_db_data(db, data)

    #start the run and launch the data emitter
    global running
    running = True
    data_io.begin_run()
    
    t = threading.Thread(name='emitter', target=send_events)
    t.start()
    
    sleep(0.1)
    broadcast_refresh()
    return redirect(url_for('running_hist'))

@app.route('/end')
def end_run():
    """Ends the run, broadcasts a refresh message to all clients"""

    global running
    if running:
        running = False
        data_io.end_run()
    
    broadcast_refresh()
    return redirect(url_for('running_hist'))

@app.route('/hist')
def running_hist():
    """displays an online histogram"""
    filename, filepath = generate_hist()

    if filepath == 'failed':
        return render_template('no_data.html')
    return render_template('hist.html', path=filepath, in_progress=running)

@app.route('/traces')
def traces():
    """Displays online traces"""
    filepath = generate_traces()
    return render_template('traces.html', path=filepath, in_progress=running)

@app.route('/nodata')
def no_data():
    """Displays a message if someone tries to draw a histogram when no data
    is available"""
    return render_template('no_data.html', in_progress=running)

@app.route('/runlog')
def runlog():
    """renders page from which user can request the runlog"""

    runlog_path = upload_path(run_info['runlog'])
    return render_template('runlog.html', in_progress=running,
                           path=runlog_path)

@app.route('/<path:filename>')
def get_upload(filename):
    """Return the requested file from the server."""
    filename = os.path.basename(filename)
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename)

def send_events():
    """sends data to the clients while a run is going"""
    while not data_io.e.isSet():
        socketio.emit('event info', {"count" : data_io.eventCount, "rate" : data_io.rate},
                      namespace='/online')
        sleep(0.1)

@socketio.on('update histogram', namespace='/online')
def update_hist():
    """update the histogram upon request from client and then
    respond when it's ready"""
    print 'generating hist'
    name, path = generate_hist()
    send_from_directory(app.config['UPLOAD_FOLDER'], name)
    emit('histogram ready', {"path" : path});

@socketio.on('generate runlog', namespace='/online')
def generate_runlog():
    """generates runlog upon request from client"""
    print 'clicked'
    
    with open(app.config['UPLOAD_FOLDER']+'/'+run_info['runlog'], 'w') as runlog:
        db = connect_db(run_info['db_name'])
        
        #write column headers
        runlog.write('run number')
        for attr in run_info['attr']:
            runlog.write(', ' + attr)
        
        #fill runlog data from database
        for run_idx in xrange(int(db['toc']['n_runs'])):
            runlog.write('\n')
            run_num = str(run_idx+1)
            runlog.write(run_num)
            
            data = db[db['toc'][run_num]]
            for attr in run_info['attr']:
                runlog.write(', ' + data[attr])

    emit('runlog ready')

@socketio.on('refreshed', namespace='/online')
def on_refresh():
    """when a client refreshes his page, this sends him a fresh batch of data"""
    if running:
        emit('event info', {"count" : data_io.eventCount, "rate" : data_io.rate},
                      namespace='/online')

@socketio.on('connect', namespace='/online')
def test_connect():
    """communicates upon connection with a client"""
    emit('my response', {'data': 'Connected'})

def broadcast_refresh():
    """tells all clients to refresh their pages. This is called when a run begins
    or ends"""
    socketio.emit('refresh', {'data': ''}, namespace='/online')

def generate_hist():
    """updates the online histogram"""
    plt.clf()
  
    try:
        plt.hist(data_io.data, np.sqrt(data_io.eventCount))
        plt.title('Event ' + str(data_io.eventCount))
    except IndexError:
        return 'failed'

    for temp_file in glob.glob(app.config['UPLOAD_FOLDER'] + '/temp_hist*'):
        os.remove(temp_file)
    filename = unique_filename('temp_hist.png')
    filepath = upload_path(filename)
    plt.savefig(filepath)
    
    return filename, filepath

def generate_traces():
    """generates the online traces"""
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
    return os.path.join(os.path.dirname(os.path.realpath(__file__))+
                        '/uploads', basename)

def copy_form_data(info, req):
    """Copy the data in the form that is filled out."""
    data = {}
    for key in info['attr']:
        data[key] = req.form[key]

    return data

def check_form_data(info, data):
    """Check each spot in the form data."""
    for key in info['attr']:
        if data[key] == '':
            return False

    return True

def save_db_data(db, data):
    """Save the entry to the db and enter it into the table of contents."""
    # Form a map to the most recent entry.
    toc = db.get('toc')
        
    # Create if null.
    if (toc == None):
        toc = {}
        toc['n_runs'] = 0
        toc['_id'] = 'toc'

    # Increment n_entries if necessary.
    if run_info['last_run'] not in toc:
        toc['n_runs'] += 1

    # Save the data.
    idx, ver = db.save(data)
    
    toc[run_info['last_run']] = idx
    db.save(toc)

def connect_db(db_name):
    """Connect to the database if open, and start database if not running."""
    try:
        client = couchdb.Server()

    except:
        subprocess.call(['couchdb', '-b'])
        time.sleep(2)
        client = couchdb.Server()

    try:
        db = client[db_name]

    except:
        client.create(db_name)
        db = client[db_name]
        toc = {}
        toc['n_runs'] = 0

        toc['_id'] = 'toc'
        db.save(toc)

    return db


def get_db(db_name):
    """Place a handle to the database in the global variables."""
    if not hasattr(g, 'client'):
        g.client = {}

    if not hasattr(g.client, db_name):
        g.client[db_name] = connect_db(db_name)

    return g.client[db_name]

def last_run_number():
    """determines the last run number by looking in the database"""
    db = connect_db(run_info['db_name'])
    return db['toc']['n_runs']

if __name__ == '__main__':
    run_info['last_run'] = last_run_number()
    socketio.run(app, host='0.0.0.0')
