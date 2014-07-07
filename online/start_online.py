#Aaron Fienberg
#Fienberg@uw.edu
#online display for SLAC test beam run

from gevent import monkey
monkey.patch_all()

from flask import Flask, render_template, send_from_directory, redirect, url_for
from flask import g, request, session
from flask.ext.socketio import SocketIO, emit
from werkzeug.utils import secure_filename
from uuid import uuid4
import couchdb
from couchdb.design import ViewDefinition
import os, glob, datetime
import threading

import data_io
import zmq, json
from time import sleep, time

import numpy as np
from matplotlib import pyplot as plt

# global parameters
# flag denoting whether or not a run is in progress
running = False
# current working directory
cwd = os.path.dirname(os.path.realpath(__file__))

#setup flask app
app = Flask(__name__)
app.config.update(dict(
    UPLOAD_FOLDER=cwd+'/uploads',
    IMAGE_UPLOADS=['.jpg', '.jpe', '.jpeg', '.png', '.gif', '.svg', '.bmp'],
    DEBUG=True))
app.config['SECRET_KEY'] = '\xf5\x1a#qx%`Q\x88\xd1h4\xc3\xba1~\x16\x11\x81\t\x8a?\xadF'
socketio = SocketIO(app)


#Define attributes of a run
run_info = {}
run_info['db_name'] = 'run_db'
run_info['attr'] = ['Description', 'Table x', 'Table y', 'Beam Energy [GeV]']
run_info['log_info'] = ['Events', 'Date', 'Time']
run_info['runlog'] = 'runlog.csv'

@app.errorhandler(404)
def page_not_found(e):
    return render_template('error404.html', in_progress=running), 404

@app.route('/')
def home():
    """display home page of online DAQ"""
    return render_template('home.html', in_progress=running)

@app.route('/new')
def new_run():
    """User wants to start a new run. Load the data from 
    the previous run and prompt user for summary information"""
    
    last_data = {}
    if run_info['last_run'] != 0:
        db = connect_db(run_info['db_name'])
        last_data = db[db['toc'][str(run_info['last_run'])]]
        last_data['Description'] = ''

    return render_template('new_run.html', info=run_info, data=last_data, 
                           new=True, in_progress=running)

@app.route('/start', methods=['POST'])
def start_run():
    """Check user form for completion. If it is complete, start the run.
    This entails saving the user configuration in couchdb and 
    launching the data getter and emitter"""
    global running

    data = copy_form_data(run_info, request)
    complete = check_form_data(run_info, data)
    if not complete:
        error = "All fields in the form must be filled."
        return render_template('new_run.html', info=run_info, 
                               data=data, error=error, new=True,
                               in_progress=running)

    # Send a start run signal to fe_master.
    context = zmq.Context()

    handshake_sck = context.socket(zmq.REQ)
    handshake_sck.setsockopt(zmq.LINGER, 0);
    start_sck = context.socket(zmq.PUB)
    start_sck.setsockopt(zmq.LINGER, 0)

    conf = json.load(open(os.path.join(cwd, '../config/.default_master.json')))
    start_sck.connect(conf['trigger_port'])
    handshake_sck.connect(conf['handshake_port'])

    msg = "CONNECT"
    handshake_sck.send(msg)
    if (handshake_sck.recv() == msg):
        # Connection established.
        start_sck.send("START:%05i:" % (run_info['last_run'] + 1))

    handshake_sck.close()
    start_sck.close()
    context.destroy()           
    
    #save the run info
    db = get_db(run_info['db_name'])
    run_info['last_run'] += 1
    data['run_number'] = run_info['last_run']
    now = datetime.datetime.now()
    data['Date'] = "%02i/%02i" % (now.month, now.day)
    data['Time'] = "%02i:%02i" % (now.hour, now.minute)
    save_db_data(db, data)

    #start the run and launch the data emitter
    running = True
    data_io.begin_run()

    t = threading.Thread(name='emitter', target=send_events)
    t.start()
    
    broadcast_refresh()
    
    return redirect(url_for('home'))

@app.route('/end')
def end_run():
    """Ends the run, broadcasts a refresh message to all clients"""

    # Send a stop run signal to fe_master.
    context = zmq.Context()
    handshake_sck = context.socket(zmq.REQ)
    handshake_sck.setsockopt(zmq.LINGER, 0);
    stop_sck = context.socket(zmq.PUB)
    stop_sck.setsockopt(zmq.LINGER, 0)

    conf = json.load(open(os.path.join(cwd, '../config/.default_master.json')))
    stop_sck.connect(conf['trigger_port'])
    handshake_sck.connect(conf['handshake_port'])

    msg = "CONNECT"
    handshake_sck.send(msg)
    if (handshake_sck.recv() == msg):
        # Connection established.
        stop_sck.send("STOP:")

    handshake_sck.close()
    stop_sck.close()
    context.destroy()

    global running
    if running:
        running = False
        data_io.end_run()

    db = connect_db(run_info['db_name'])
    data = db[db['toc'][str(run_info['last_run'])]]
    data['Events'] = data_io.eventCount
    print "%i events" % data_io.eventCount
    db.save(data)

    sleep(0.1)
    broadcast_refresh()

    context.destroy()
    generate_runlog()

    return redirect(url_for('running_hist'))

@app.route('/hist')
def running_hist():
    """displays an online histogram"""
    if len(data_io.hists)==0:
        return render_template('no_data.html', in_progress=running)

    if 'device' not in session:
        session['device'] = 'sis_fast_0'
    if 'channel' not in session:
        session['channel'] = 0

    if 'refresh_rate' not in session:
        session['refresh_rate'] = 1.

    devices = []
    for device in data_io.data:
        try:
            num_chans = len(data_io.data[device]['trace'])
            for i in xrange(num_chans):
                devices.append(device + ' channel ' + str(i))
        except:
            continue

    current_selection = session['device'] + ' channel ' + str(session['channel'])

    return render_template('hist.html', in_progress=running, 
                           r_rate=session['refresh_rate'], options=devices, 
                           selected=current_selection)

@app.route('/traces')
def traces():
    """Displays online traces"""
    if len(data_io.hists)==0:
        return render_template('no_data.html', in_progress=running)
    
    if 'device' not in session:
        session['device'] = 'sis_fast_0'
    if 'channel' not in session:
        session['channel'] = 0

    devices = []
    for device in data_io.data:
        try:
            num_chans = len(data_io.data[device]['trace'])
            for i in xrange(num_chans):
                devices.append(device + ' channel ' + str(i))
        except:
            continue

    current_selection = session['device'] + ' channel ' + str(session['channel'])

    return render_template('traces.html', options=devices, 
                           selected=current_selection, in_progress=running)

@app.route('/nodata')
def no_data():
    """Displays a message if someone tries to draw a histogram when no data
    is available"""
    return render_template('no_data.html', in_progress=running)

@app.route('/revision_select')
def revision_select():
    """display page where user can select a run to revise"""
    if run_info['last_run']<10:
        end = 0
    else:
        end = run_info['last_run']-10
    
    return render_template("revision_select.html", 
                           in_progress=running,
                           last_ten=range(run_info['last_run'],end,-1))

@app.route('/revise/<string:run_num>')
def revise(run_num):
    """display page for revising the given run"""
    try:
        db = connect_db(run_info['db_name'])
        old_data = db[db['toc'][run_num]]

        session['revision_number']=run_num
        return render_template('revise.html', num=run_num, 
                               info=run_info, data=old_data,
                               in_progress=running)
    except:
         return render_template('no_run.html')
    

@app.route('/save_revision', methods=['POST'])
def save_revision():
    """save run revision information"""
    new_data = copy_form_data(run_info, request)
    complete = check_form_data(run_info, new_data)
    if not complete:
        error = "All fields in the form must be filled."
        return render_template('revise.html',num=session['revision_number'], 
                               info=run_info, data=new_data,error=error, 
                               in_progress=running)
        
    #save the revision    
    db = connect_db(run_info['db_name'])
    data = db[db['toc'][str(session['revision_number'])]]
    for attr in run_info['attr']:
        data[attr] = new_data[attr]

    save_db_data(db, data)
    return render_template('revision_success.html', in_progress=running)

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
    while not data_io.runOver.isSet():
        sleep(0.1)
        socketio.emit('event info', {"count" : data_io.eventCount, "rate" : data_io.rate},
                      namespace='/online')
        

@socketio.on('update histogram', namespace='/online')
def update_hist(msg):
    """update the histogram upon request from client and then
    respond when it's ready"""

    selection = msg['selected'].split(' ')
    session['device'] = selection[0]
    session['channel'] = int(selection[-1])

    name, path = generate_hist()

    emit('histogram ready', {"path" : path});

@socketio.on('update trace', namespace='/online')
def update_trace(msg):
    """update the histogram upon request from client and then
    respond when it's ready"""
    try: 
        xmin = int(msg['x_min'])
        xmax = int(msg['x_max'])
        if xmin < 0:
            xmin = 0
        if xmax > 1024:
            xmax = 1024
        if xmin >= xmax:
            xmin = 0
            xmax = 1024
    except:
        xmin = 0
        xmax = 1024

    selection = msg['selected'].split(' ')
    session['device'] = selection[0]
    session['channel'] = int(selection[-1])
                           
    name, path = generate_trace(xmin, xmax)

    emit('trace ready', {"path" : path, "x_min" : xmin, "x_max" : xmax});

@socketio.on('start continual update', namespace='/online')
def start_continual():
    """begins the chain of continual histogram updates"""
    session['updating_hist'] = True
    continue_updating()
    
@socketio.on('continue updating', namespace='/online')
def continue_updating():
    """updates the histogram and then informs the client"""
    sleep(1.0/session['refresh_rate'])

    if session['updating_hist']:
        update_hist()
        emit('updated')

@socketio.on('refresh rate', namespace='/online')
def set_refresh(msg):
    """sets the histogram refresh rate"""
    try:
        new_rate = float(msg)
        if 0 < new_rate <= 5.0:
            session['refresh_rate'] = new_rate
    except ValueError:
        pass
        
    emit('current rate', str(session['refresh_rate']))

@socketio.on('stop continual update', namespace='/online')
def stop_continual():
    """ends the chain of continual histogram updates"""
    session['updating_hist'] = False


@socketio.on('generate runlog', namespace='/online')
def generate_upon_request():
    """generates runlog upon request from client"""
    generate_runlog();
    emit('runlog ready')

def generate_runlog():
    '''generates the runlog from db'''
    #generate column headers
    start = time()
    runlog_headers = ''
    for attr in run_info['attr']:
        runlog_headers += ', ' + attr
    for info in run_info['log_info']:
        runlog_headers += ', ' + info
        
    #fill runlog lines with database info
    runlog_lines = []
    db = connect_db(run_info['db_name'])
    n_runs = int(db['toc']['n_runs'])
    counter = 0
    for doc in db.view('_design/all/_view/all'):
        data = doc['value']
    
        runlog_lines.append('')
        if 'run_number' in data:
            runlog_lines[-1] += str(data['run_number'])
        else:
            runlog_lines[-1] += 'N/A'
        for attr in run_info['attr']:
                if attr in data:
                    runlog_lines[-1] += ', ' + str(data[attr])
                else: 
                    runlog_lines[-1] += (', N/A')
        for info in run_info['log_info']:
            if info in data:
                runlog_lines[-1] += ', ' + str(data[info])
            else:
                runlog_lines[-1] += (', N/A')

        counter+=1
        progress = 100*float(counter)/n_runs

        emit('progress', "%02i%s Generated" % 
             (progress, "%"))
    
    #write the file
    with open(app.config['UPLOAD_FOLDER']+'/'+run_info['runlog'], 'w') as runlog:
        runlog.write(runlog_headers)
        for line in runlog_lines:
            runlog.write('\n' + line)
    print '%i seconds' % (time() - start)
    


@socketio.on('refreshed', namespace='/online')
def on_refresh():
    """when a client refreshes his page, this ds him a fresh batch of data"""
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
        this_dev = session['device'] + ' channel ' + str(session['channel'])
        plt.hist(data_io.hists[this_dev], np.sqrt(data_io.eventCount))
        plt.title('Run %i Event %i, %s channel %i' % 
                  (run_info['last_run'], data_io.eventCount,
                   session['device'], session['channel']))
    except IndexError:
        return 'failed', 'failed'

    for temp_file in glob.glob(app.config['UPLOAD_FOLDER'] + '/temp_hist*'):
        os.remove(temp_file)
    filename = unique_filename('temp_hist.png')
    filepath = upload_path(filename)
    plt.savefig(filepath)
    
    return filename, filepath
 
def generate_trace(xmin, xmax):
    """generate the trace plot"""
    plt.clf()
    plt.plot(data_io.data[session['device']]['trace'][session['channel']])
    plt.title('Run %i Event %i, %s channel %i' % 
              (run_info['last_run'], data_io.eventCount,
              session['device'], session['channel']))
    plt.xlim([xmin,xmax])

    for tempFile in glob.glob(app.config['UPLOAD_FOLDER'] + '/temp_trace*'):
        os.remove(tempFile)
    filename = unique_filename('temp_trace.png')
    filepath = upload_path(filename)
    plt.savefig(filepath)
    
    return filename, filepath
    
   
def unique_filename(upload_file):
     """Take a base filename, add characters to make it more unique, and ensure that it is a secure filename."""
     filename = os.path.basename(upload_file).split('.')[0]
     filename += '-' + str(uuid4().hex[-12:])
     filename += os.path.splitext(upload_file)[1]
     return secure_filename(filename)

def upload_path(filename):
    """Construct the full path of the uploaded file."""
    basename = os.path.basename(filename)
    return os.path.join(cwd+
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
    if str(run_info['last_run']) not in toc:
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

    #create permanent view to all if one doesn't exist
    if '_design/all' not in db:
        view_def = ViewDefinition('all', 'all',''' 
				  function(doc) { 
				      if( doc.run_number )
					  emit(parseInt(doc.run_number), doc);
				  }''')
        view_def.sync(db)

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
