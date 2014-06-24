#ifndef SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_
#define SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_

#define SIS_3350_CH 4
#define SIS_3350_LN 1024

#define SIS_3302_CH 8 
#define SIS_3302_LN 1024

#define CAEN_1785_CH 8

//--- std includes ----------------------------------------------------------//
#include <vector>
using std::vector;

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>


//--- projects includes -----------------------------------------------------//
#include "daq_worker_base.hh"

namespace daq {

// Basic structs
struct sis_3350 {
  unsigned long long system_clock;
  unsigned long long device_clock[SIS_3350_CH];
  ushort trace[SIS_3350_CH][SIS_3350_LN];
};

struct sis_3302 {
  unsigned long long system_clock;
  unsigned long long device_clock[SIS_3302_CH];
  ushort trace[SIS_3302_CH][SIS_3302_LN];
};

struct caen_1785 {
  unsigned long long system_clock;
  unsigned long long device_clock[CAEN_1785_CH];
  ushort value[CAEN_1785_CH];
};

// Built from basic structs 
struct event_data {
  vector<sis_3350> fake;
  vector<sis_3350> sis_fast;
  vector<sis_3302> sis_slow;
  vector<caen_1785> caen_adc;
};

// typedef for all workers
typedef boost::variant<DaqWorkerBase<sis_3350> *, DaqWorkerBase<sis_3302> *, DaqWorkerBase<caen_1785> *> worker_ptr_types;

namespace vme {

extern int device;

} // ::vme

} // ::daq

#endif