#!/bin/python

import slow_control as sc
import sys, json, os

# Get the curren bias configuration.
if (len(sys.argv) > 1):
    config_file = sys.argv[1]

else:
    config_file = 'bias_voltages.json'
    
# Open the json file.
conf = json.load(open(config_file))

# Get the voltages into an array
voltages = []

for x in conf['voltages']:
	for val in x:
	    voltages.append(val)

# Turn on and set the voltages.
hv = sc.UvaBias('http://192.168.1.50')

hv.bias_on()

for i in range(hv.num_channels):
	hv.set_channel(i + 1, voltages[i])

# Read out the voltages to make sure they were set
hv.read_all_channels()
