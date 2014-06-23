import numpy as np

data = np.empty(0)

def update_data():
    global data 
    data = np.append(data, np.random.standard_normal(1000))

def clear_data():
    global data
    data = np.empty(0)

def pulse_shape(t):
    return -1.0*np.exp(-(t)/5.0)*(1-np.exp(-(t)/1.0))
    

def fill_trace():
    trace = np.array(xrange(50))
    return pulse_shape(trace)
