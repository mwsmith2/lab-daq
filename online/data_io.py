import numpy as np
from time import sleep
import threading

data = []
e = threading.Event()
lock = threading.Lock()

def clear_data():
    global data
    data = []
#    data = np.empty(0)


def pulse_shape(t):
    return -1.0*np.exp(-(t)/5.0)*(1-np.exp(-(t)/1.0))
    
def fill_trace():
    trace = np.array(xrange(50))
    return pulse_shape(trace)

def generate_data(e, data):
    while not e.isSet():
        global lock
        lock.acquire()
        newValue = np.random.standard_normal(1)[0]
        data.append(newValue)
        lock.release()
        sleep(0.1)
    
def begin_run():
    global e
    e.clear()
    clear_data()
    t = threading.Thread(name='data-generator', target=generate_data, args=(e,data))
    t.start()

def end_run():
    global e
    e.set()
    
