#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_

//--- std includes ----------------------------------------------------------//
#include <cmath>

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "daq_worker_base.hh"
#include "daq_structs.hh"

typedef sis_3350 data_struct;

// This class produces fake data to test functionality
namespace daq {

class DaqWorkerFake : public DaqWorkerBase {

  public:

    // ctor
    DaqWorkerFake(string conf);

    void LoadConfig();
    void WorkLoop();

  private:

    // Fake data variables
    double rate_;
    double jitter_;
    double drop_rate_;

    // Data queue
    std::queue<struct data_struct> data_queue_;

    bool HasData() { return has_data_; };
    void GetData(data_struct);

    void GenerateData();
};

} // daq

#endif