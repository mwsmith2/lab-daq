#!/bin/bash

killall sc_monitor
killall sc_worker

./sc_monitor &> sc_log &

python python/bk_precision_sm.py /dev/ttyUSB0 &> bk_log_0 &
python python/bk_precision_sm.py /dev/ttyUSB1 &> bk_log_1 &
