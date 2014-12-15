#!/bin/bash

killall fe_master

sleep 2

./fe_master $1 &> data/log &

