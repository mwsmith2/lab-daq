#!/bin/bash

TMPDIR=`pwd`

SCRIPTDIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
cd $SCRIPTDIR

. stop-daqometer.sh

sleep 0.5

. start-daqometer.sh $1

cd $TMPDIR

