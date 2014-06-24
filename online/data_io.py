import numpy as np
from time import sleep, time
import threading
from flask.ext.socketio import emit

data = []
e = threading.Event()
lock = threading.Lock()
start = 0
rate = 0 
eventCount = 0

def clear_data():
    global data
    global eventCount
    eventCount=0
    data = []

def pulse_shape(t):
    return -1.0*np.exp(-(t)/5.0)*(1-np.exp(-(t)/1.0))
    
def fill_trace():
    trace = np.array(xrange(50))
    return pulse_shape(trace)

def generate_data(e, data):
    while not e.isSet():
        global lock
        global rate
        global eventCount
        lock.acquire()
        newValue = np.random.standard_normal(1)[0]
        data.append(newValue)
        rate = len(data)/(time() - start)
        eventCount+=1
        lock.release()
        sleep(0.1)
    
def begin_run():
    global e
    global start
    e.clear()
    clear_data()
    t = threading.Thread(name='data-generator', target=generate_data, args=(e,data))
    start = time()
    t.start()

def end_run():
    global e
    e.set()
    
