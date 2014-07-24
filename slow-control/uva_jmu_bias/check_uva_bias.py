#!/bin/python

import slow_control as sc
# Read out the voltages in the high voltage supply.

hv = sc.UvaBias()
hv.read_all_channels()
