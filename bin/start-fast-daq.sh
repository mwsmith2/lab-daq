#!/bin/bash

# Start the lab daq

TMPDIR=`pwd`

. /usr/local/opt/root/bin/thisroot.sh

SCRIPTDIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
cd $SCRIPTDIR/../fast

if [[ $EUID -ne 0 ]]; then

    ./bin/fe_master $1 &> data/log &

else

    ./bin/fe_master $1 &> /var/log/lab-daq/fast-frontend.log &
fi

cd $TMPDIR

#end script
