#!/bin/python

import slow_control as sc
import sys, json, os

# Turn off and set the voltages.
hv = sc.UvaBias('http://192.168.1.50')

hv.bias_off()

# Read out the voltages to make sure they were set
hv.read_all_channels()
