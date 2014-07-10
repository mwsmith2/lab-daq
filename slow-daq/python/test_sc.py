#!/bin/python

import zmq, json, time, uuid

conf = json.load(open('config/.default_sc.json'))

name = "fake_sc"
key = str(uuid.uuid4())
branch_vars = "val_0:val_1:val_2"
branch_vals = "val_0:7:val_1:21:val_2:23"

context = zmq.Context()
data_sck = context.socket(zmq.PUB)
tree_sck = context.socket(zmq.PUB)
msg_sck = context.socket(zmq.SUB)

data_sck.connect(conf['data_port'])
tree_sck.connect(conf['tree_port'])
msg_sck.connect(conf['msg_port'])
msg_sck.setsockopt(zmq.SUBSCRIBE, key)

time.sleep(0.005)

# Ask to create a tree
tree_sck.send(':'.join(['TREE', key, name, branch_vars, '__EOM__\0']))
time_last = time.time()

while (True):
	
	if (msg_sck.poll(timeout=100) != 0):
		
		# This happens if the device pushes data before creating a tree.
		msg = msg_sck.recv()
		tree_sck.send(':'.join(['TREE', key, name, branch_vars, '__EOM__\0']))


	if (time.time() - time_last >= 5):

		# Send more fake data.
		data_sck.send(':'.join(['DATA', key, branch_vals, '__EOM__\0']))
		time_last = time.time()

	time.sleep(1.0)
