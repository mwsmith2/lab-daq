#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_

//--- std includes ----------------------------------------------------------//
#include <ctime>

//--- other includes --------------------------------------------------------//
//#include "vme/sis3100_vme_calls.h"

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

    bool EventAvailable();
    void GetEvent(event_struct &bundle);

};

} // ::daq

#endif