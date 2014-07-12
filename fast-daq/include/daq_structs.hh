#ifndef SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_
#define SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_

#define SIS_3350_CH 4
#define SIS_3350_LN 1024

#define SIS_3302_CH 8 
#define SIS_3302_LN 1024

#define CAEN_1785_CH 8

#define CAEN_6742_GR 2
#define CAEN_6742_CH 16
#define CAEN_6742_LN 1024

//--- std includes ----------------------------------------------------------//
#include <vector>
using std::vector;

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>
#include "TFile.h"

//--- projects includes -----------------------------------------------------//
#include "daq_worker_base.hh"

namespace daq {

// Basic structs
struct sis_3350 {
  ULong64_t system_clock;
  ULong64_t device_clock[SIS_3350_CH];
  UShort_t trace[SIS_3350_CH][SIS_3350_LN];
};

struct sis_3302 {
  ULong64_t system_clock;
  ULong64_t device_clock[SIS_3302_CH];
  UShort_t trace[SIS_3302_CH][SIS_3302_LN];
};

struct caen_1785 {
  ULong64_t system_clock;
  ULong64_t device_clock[CAEN_1785_CH];
  UShort_t value[CAEN_1785_CH];
};

struct caen_6742 {
  ULong64_t system_clock;
  ULong64_t device_clock[CAEN_6742_CH];
  UShort_t trace[CAEN_6742_CH][CAEN_6742_LN];
};

// Built from basic structs 
struct event_data {
  vector<sis_3350> sis_fast;
  vector<sis_3302> sis_slow;
  vector<caen_1785> caen_adc;
  vector<caen_6742> caen_drs;
};

// typedef for all workers
typedef boost::variant<DaqWorkerBase<sis_3350> *, DaqWorkerBase<sis_3302> *, DaqWorkerBase<caen_1785> *, DaqWorkerBase<caen_6742> *> worker_ptr_types;

namespace vme {

extern int device;

} // ::vme

static int short_sleep = 0;
static int long_sleep = 100;

} // ::daq

#endif
