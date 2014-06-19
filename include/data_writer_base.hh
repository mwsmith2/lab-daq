#ifndef SLAC_DAQ_INCLUDE_DATA_WRITER_BASE_HH_
#define SLAC_DAQ_INCLUDE_DATA_WRITER_BASE_HH_

//--- std includes ----------------------------------------------------------//
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>
using std::vector;
using std::string;

//--- other includes --------------------------------------------------------//


//--- project includes ------------------------------------------------------//


namespace daq {

// This class defines an abstract base class for data writers to inherit form.

class DataWriterBase {

  public:

    DataWriterBase(string conf_file) : conf_file_(conf_file) {};

    // Basic functions
    virtual void LoadConfig() = 0;
    virtual void StartWriter() = 0;
    virtual void StopWriter() = 0;

    virtual void PullData(vector<event_data> data_buffer) = 0;

  protected:

    // Simple variables
    conf_file_;
    std::atomic<bool> go_time_;

    // Concurrency variables
    std::thread writer_thread_;
    std::mutex writer_mutex_;

};

} // ::daq