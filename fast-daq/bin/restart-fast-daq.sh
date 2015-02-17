#!/bin/bash

TMPDIR=`pwd`

SCRIPTDIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
cd $SCRIPTDIR

. stop-fast-daq.sh

sleep 0.5

. start-fast-daq.sh $1

cd $TMPDIR

