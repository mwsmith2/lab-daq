#ifndef SLAC_DAQ_INCLUDE_DAQ_WRITER_BASE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WRITER_BASE_HH_

//--- std includes ----------------------------------------------------------//
#include <thread>
#include <mutex>
#include <string>
#include <vector>
using std::vector;
using std::string;

//--- other includes --------------------------------------------------------//


//--- project includes ------------------------------------------------------//
#include "daq_structs.hh"

namespace daq {

// This class defines an abstract base class for data writers to inherit form.

class DaqWriterBase {

  public:

    DaqWriterBase(string conf_file) : conf_file_(conf_file) {};

    // Basic functions
    virtual void LoadConfig() = 0;
    virtual void StartWriter() = 0;
    virtual void StopWriter() = 0;

    virtual void PullData(vector<event_data> data_buffer) = 0;

  protected:

    // Simple variables
    string conf_file_;

    // Concurrency variables
    std::thread writer_thread_;
    std::mutex writer_mutex_;

};

} // ::daq

#endif