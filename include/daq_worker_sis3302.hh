#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_SIS3302_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_SIS3302_HH_

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

// This class pulls data from a sis_3302 device.
namespace daq {

class DaqWorkerSis3302 : public DaqWorkerBase<sis_3302> {

public:
  
  // ctor
  DaqWorkerSis3302(string name, string conf);
  
  void LoadConfig();
  void WorkLoop();
  sis_3302 PopEvent();
  
private:
  
  int num_ch_;
  int len_tr_;

  int device_;
  int base_address_;
  
  bool EventAvailable();
  void GetEvent(sis_3302 &bundle);
  
  int Read(int addr, uint &msg);
  int Write(int addr, uint msg);
  int ReadTrace(int addr, uint *trace);
};

} // ::daq

#endif
