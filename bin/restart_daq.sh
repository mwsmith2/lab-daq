#!/bin/bash

killall fe_master

./fe_master $1 &> log &
