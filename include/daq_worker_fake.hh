#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_FAKE_HH_

//--- std includes ----------------------------------------------------------//
#include <string>
#include <cmath>
using std::string;

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "daq_worker_base.hh"


// This class produces fake data to test functionality
namespace daq {

class DaqWorkerFake : public DaqWorkerBase {

  public:

    // ctor
    DaqWorkerFake(string conf) : DaqWorkerBase(conf) { load_config(); };
    


  private:


};

}