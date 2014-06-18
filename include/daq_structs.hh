#ifndef SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_
#define SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_

#define SIS_3350_CH 8
#define SIS_3350_LN 1024

#define SIS_3302_CH 8 
#define SIS_3302_LN 1024

namespace daq {

struct base_event {};

struct sis_3350 : base_event {
  unsigned long long timestamp[SIS_3350_CH];
  ushort trace[SIS_3350_CH][SIS_3350_LN];
};

struct sis_3302 : base_event{
  unsigned long long timestamp[SIS_3302_CH];
  ushort trace[SIS_3302_CH][SIS_3302_LN];
};

}

#endif