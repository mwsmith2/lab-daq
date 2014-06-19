#ifndef SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_
#define SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_

#define SIS_3350_CH 4
#define SIS_3350_LN 1024

#define SIS_3302_CH 8 
#define SIS_3302_LN 1024

//--- std includes ----------------------------------------------------------//
#include <vector>
using std::vector;

//--- other includes --------------------------------------------------------//
//--- projects includes -----------------------------------------------------//

namespace daq {

// Basic structs
struct sis_3350 {
  unsigned long long timestamp[SIS_3350_CH];
  ushort trace[SIS_3350_CH][SIS_3350_LN];
};

struct sis_3302 {
  unsigned long long timestamp[SIS_3302_CH];
  ushort trace[SIS_3302_CH][SIS_3302_LN];
};

// Built from basic structs 
struct event_data {
  vector<sis_3350> fake;
  vector<sis_3350> sis_fast;
  vector<sis_3302> sis_slow;
};

}

#endif