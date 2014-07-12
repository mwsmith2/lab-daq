#!/usr/bin/python

import zmq, json, time, uuid, sys
import slow_control as sc
from setproctitle import *

# Get the config file
conf = json.load(open('config/.default_sc.json'))

name = "bk_precision"
key = str(uuid.uuid4())
branch_vars = "volt"
push_interval = conf['push_interval']

if (len(sys.argv) > 1):
    bk_dev = sc.BKPrecision(sys.argv[1])

else:
    bk_dev = sc.BKPrecision('/dev/ttyUSB0')

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
       
        # This happens if the device pushes data before creating a tree.
        msg = msg_sck.recv()
        tree_sck.send(':'.join(['TREE', key, name, branch_vars, '__EOM__\0']))


    if (time.time() - time_last >= push_interval):

        volt = bk_dev.meas_volt()
        # Send data
        data_sck.send(':'.join(['DATA', key, branch_vars, volt, '__EOM__\0']))
        time_last = time.time()

	time.sleep(0.5)

