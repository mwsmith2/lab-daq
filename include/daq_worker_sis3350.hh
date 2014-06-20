#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_SIS3350_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_SIS3350_HH_

//--- std includes ----------------------------------------------------------//
#include <ctime>
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

//--- other includes --------------------------------------------------------//
#include "vme/sis3100_vme_calls.h"

//--- project includes ------------------------------------------------------//
#include "daq_worker_base.hh"
#include "daq_structs.hh"

// This class pulls data from a sis_3350 device.
namespace daq {

typedef sis_3350 event_struct;

class DaqWorkerSis3350 : public DaqWorkerBase<event_struct> {
    
public:
  
  // ctor
  DaqWorkerSis3350(string name, string conf);
  
  void LoadConfig();
  void WorkLoop();
  event_struct PopEvent();
  
private:
  
  int num_ch_;
  int len_tr_;

  int device_;
  int base_address_;
  
  bool EventAvailable();
  void GetEvent(event_struct &bundle);
  
  int Read(int addr, uint &msg);
  int Write(int addr, uint msg);
  int ReadTrace(int addr, uint *trace);
};

} // ::daq

#endif
