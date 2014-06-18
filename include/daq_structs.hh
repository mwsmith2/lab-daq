#ifndef SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_
#define SLAC_DAQ_INCLUDE_DAQ_STRUCTS_HH_

namespace daq {

#define SIS_3350_CH
#define SIS_3350_LN

#define SIS_3302_CH
#define SIS_3302_LN

struct sis_3350 {
    unsigned long long timestamp[SIS_3350_CH];
    ushort trace[SIS_3350_CH][SIS_3350_LN];
};

struct sis_3302 {
  unsigned long long timestamp[SIS_3302_CH];
  ushort trace[SIS_3302_CH][SIS_3302_LN]
};

}

#endif