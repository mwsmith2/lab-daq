#!/bin/bash

# A cron wrapper for sc_reviver.py since it needs root env variables.

. ~/.bashrc
python ~/Workspace/slac-test-ii/daq/slow-daq/sc_reviver.py
