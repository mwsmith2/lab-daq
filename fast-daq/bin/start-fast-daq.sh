#!/bin/bash

# Start the lab daq

TMPDIR=`pwd`

. /usr/local/opt/root/bin/thisroot.sh

SCRIPTDIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
cd $SCRIPTDIR/..

if [[ $EUID -ne 0 ]]; then

    ./fe_master $1 &> data/log &

else

    ./fe_master $1 &> /var/log/fast-daq.log &
fi

cd $TMPDIR

#end script
