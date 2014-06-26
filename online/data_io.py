import numpy as np
from time import sleep, time
import threading
import Queue
from flask.ext.socketio import emit

data = []
e = threading.Event()

trace = np.zeros(1024)

rate = 0 
eventCount = 0

def clear_data():
    global data
    global eventCount
    eventCount=0
    data = []
    generate_data.counter = 0

def pulse_shape(t):
    return -1.0*np.exp(-(t)/5.0)*(1-np.exp(-(t)/1.0))
    
def generate_data(e, data):
    while not e.isSet():
        if generate_data.counter != generate_data.maxsize:
            generate_data.counter += 1
            
        now = time()
        past = generate_data.times.get()
        generate_data.times.put(now)
        

        global rate
        global eventCount
        global trace
        eventCount+=1

        trace = np.random.standard_normal(len(trace))
        data.append(trace.max())
        
        rate = float(generate_data.counter)/(now-past)

        sleep(0.1)
generate_data.maxsize = 10
generate_data.counter = 0
generate_data.times = Queue.Queue(maxsize=generate_data.maxsize)


def begin_run():
    global e
    print 'starting'
    e.clear()
    clear_data()
    start = time()
    for unused_i in xrange(10):
        generate_data.times.put(start)

    t = threading.Thread(name='data-generator', target=generate_data, args=(e,data))
    start = time()
    print 'starting'
    t.start()

def end_run():
    global e
    e.set()
    for i in xrange(10):
        generate_data.times.get()
         
    
