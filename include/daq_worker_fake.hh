#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_

//--- std includes ----------------------------------------------------------//
#include <cmath>
#include <ctime>

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "daq_worker_base.hh"
#include "daq_structs.hh"


// This class produces fake data to test functionality
namespace daq {

typedef sis_3350 data_struct;

class DaqWorkerFake : public DaqWorkerBase {

  public:

    // ctor
    DaqWorkerFake(string conf);

    void LoadConfig();
    void WorkLoop();

  private:

    // Fake data variables
    int num_ch_;
    int len_tr_;
    bool has_event_;
    bool has_fake_event_;
    double rate_;
    double jitter_;
    double drop_rate_;
    data_struct event_data_;

    // Data queue
    std::queue<data_struct> data_queue_;

    bool HasEvent() { return has_fake_event_; };
    void GetEvent(data_struct);

    // The function generates fake data.
    void GenerateEvent();
};

} // daq

#endif