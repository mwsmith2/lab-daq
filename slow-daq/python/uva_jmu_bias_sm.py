#!/usr/bin/python

import zmq, json, time, uuid, sys
import slow_control as sc
from setproctitle import *

# Get the config file
conf = json.load(open('config/.default_sc.json'))

name = "uva_jmu_bias"
key = str(uuid.uuid4())
branch_vars = "temp1:temp2"
push_interval = conf['push_interval']

# Change the name to reflect device
dev_path = 'http://192.168.1.50'
hv = sc.UvaBias(dev_path)

context = zmq.Context()
data_sck = context.socket(zmq.PUB)
tree_sck = context.socket(zmq.PUB)
msg_sck = context.socket(zmq.SUB)

data_sck.connect(conf['data_port'])
tree_sck.connect(conf['tree_port'])
msg_sck.connect(conf['msg_port'])
msg_sck.setsockopt(zmq.SUBSCRIBE, key)

# Set the process name
setproctitle(conf['worker_name'])

time.sleep(0.005)

# Ask to create a tree
tree_sck.send(':'.join(['TREE', key, name, branch_vars, '__EOM__\0']))
time_last = time.time()

while (True):
    
    if (msg_sck.poll(timeout=100) != 0):

        print "Asking to create tree."

        # This happens if the device pushes data before creating a tree.
        msg = msg_sck.recv()
        tree_sck.send(':'.join(['TREE', key, name, branch_vars, '__EOM__\0']))


    if (time.time() - time_last >= push_interval):

        # Get data
        t = hv.read_temperature()
        
        # Join data
        data_str = ['DATA', key]
        for i in range(len(t)):
            data_str.append(branch_vars.split(':')[i])
            data_str.append(str(t[i]))

        data_str.append('__EOM__\0')

        # Send data
        data_sck.send(':'.join(data_str))
        time_last = time.time()

	time.sleep(0.5)

