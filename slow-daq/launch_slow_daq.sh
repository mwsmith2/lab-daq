#!/bin/bash

killall sc_monitor
killall sc_worker

./sc_monitor &> data/sc_log &

python python/bk_precision_sm.py /dev/ttyUSB0 &> data/bk_log_0 &
python python/bk_precision_sm.py /dev/ttyUSB1 &> data/bk_log_1 &
python python/uva_jmu_bias_sm.py &> data/bias_log_0 &
