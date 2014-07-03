#!/bin/bash

killall fe_master

sleep 1

./fe_master $1 &> log &

