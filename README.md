lab-daq
===

The project contains data acquisition code for the muon g-2 lab at UW.  It is adapted from the slac-daq used last summer.

fast
---

The fast folder contains the code for the program that pull data from several waveform digitizers and analog-to-digital converters in a synchronous way.

**Current Hardware**
* 2 SIS 3350 units
* 1 SIS 3302 unit
* 2 CAEN 6742 units
* 3 CAEN 1785 units
* 1 CAEN 1742 unit

Compiling generates 3 binaries: fe_master which runs runs the workers and event builder and start_daq/stop_daq for manual tests.

**Adding frontends**

It's pretty tedious right now, but here it is.

1. Define data structure in common.hh
1. constants
1. struct
1. Add to worker_ptr_types
1. Add to worker_list class
1. This is a pain
1. Should be redesigned if possible
1. Add src/<front-end>.cxx and include/<frontend-end>.hh
1. Add to fe_master worker allocations
1. Add include to worker_list.hh

online
---

A python web app developed using the Flask microframework, a.k.a., the Daqometer.

slow
---

Monitors for temperature and voltage. Additionally any slower asynchronous can be poured into this channel.

system locations
---

1. The log files go to /var/log/lab-daq/ as fast-daq.log, daqometer.log, fast-frontend.log
2. The config directory should be symlinked to /usr/local/opt/lab-daq/config
