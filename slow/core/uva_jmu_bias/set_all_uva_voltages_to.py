#!/bin/python

import slow_control as sc
import sys, json, os

# Get the curren bias configuration.
if (len(sys.argv) > 1):
    voltage = float(sys.argv[1])

else:
    voltage = 66.5

# Turn on and set the voltages.
hv = sc.UvaBias('http://192.168.1.50')

hv.bias_on()

for i in range(hv.num_channels):
    hv.set_channel(i + 1, voltage)

# Read out the voltages to make sure they were set
hv.read_all_channels()
