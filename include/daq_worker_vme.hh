#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_VME_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_VME_HH_

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

// This class pulls data from a vme device.
namespace daq {

template<typename T>
class DaqWorkerVme : public DaqWorkerBase<T> {

public:
  
  // ctor
  DaqWorkerVme(string name, string conf) : DaqWorkerBase<T>(name, conf), num_ch_(SIS_3302_CH), len_tr_(SIS_3302_LN) {};

protected:

  int num_ch_;
  int len_tr_;

  int base_address_;
  std::mutex mutex_;
  
  virtual bool EventAvailable() = 0;
  
  int Read(int addr, uint &msg);
  int Write(int addr, uint msg);
  int Read16(int addr, ushort &msg);
  int Write16(int addr, ushort msg);
  int ReadTrace(int addr, uint *trace);

};

template<typename T>
int DaqWorkerVme<T>::Read(int addr, uint &msg)
{
  static int retval;
  static int status;

  mutex_.lock();
  status = (retval = vme_A32D32_read(vme::device, base_address_ + addr, &msg));
  mutex_.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Address 0x%08x not readable.\n", base_address_ + addr);
    perror(str);
  }

  return retval;
}

template<typename T>
int DaqWorkerVme<T>::Write(int addr, uint msg)
{
  static int retval;
  static int status;

  mutex_.lock();
  status = (retval = vme_A32D32_write(vme::device, base_address_ + addr, msg));
  mutex_.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Address 0x%08x not writeable.\n", base_address_ + addr);
    perror(str);
  }

  return retval;
}

template<typename T>
int DaqWorkerVme<T>::Read16(int addr, ushort &msg)
{
  static int retval;
  static int status;

  mutex_.lock();
  status = (retval = vme_A32D16_read(vme::device, base_address_ + addr, &msg));
  mutex_.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Address 0x%08x not readable.\n", base_address_ + addr);
    perror(str);
  }

  return retval;
}

template<typename T>
int DaqWorkerVme<T>::Write16(int addr, ushort msg)
{
  static int retval;
  static int status;

  mutex_.lock();
  status = (retval = vme_A32D16_write(vme::device, base_address_ + addr, msg));
  mutex_.unlock();

  if (status != 0) {
    char str[100];
    sprintf(str, "Address 0x%08x not writeable.\n", base_address_ + addr);
    perror(str);
  }

  return retval;
}

template<typename T>
int DaqWorkerVme<T>::ReadTrace(int addr, uint *trace)
{
  static int retval;
  static int status;
  static uint num_got;

  status = (retval = vme_A32_2EVME_read(vme::device,
                                        base_address_ + addr,
                                        trace,
                                        len_tr_ / 2 + 4,
                                        &num_got));

  if (status != 0) {
    char str[100];
    sprintf(str, "Error reading trace at 0x%08x.\n", base_address_ + addr);
    perror(str);
    cout << "asked for: " << len_tr_ / 2 + 4 << ", got: " << num_got << endl;
  }

  return retval;
}

} // ::daq

#endif
