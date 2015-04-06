#!/bin/bash

killall fe_master
killall daqometer

sleep 1

cd fast-daq
./fe_master &> data/log &

cd ../online
python start_online.py &> log &

cd ..
