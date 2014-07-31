slac-daq
========

The project contains data acquisition code for our upcoming run at SLAC as well as an online monitoring app.

fast-daq
-------

The fast DAQ folder contains the code for the program that pull data from several waveform digitizers and analog-to-digital converters including:

* 2 SIS 3350 units
* 1 SIS 3302 unit
* 2 CAEN 6742 units
* 3 CAEN 1785 units

Compiling generates 3 binaries: fe_master which runs runs the workers and event builder and start_daq/stop_daq for manual tests.

online
-----

A python web app developed using the Flask microframework.

slow-daq
------

Monitors for temperature and voltage.
