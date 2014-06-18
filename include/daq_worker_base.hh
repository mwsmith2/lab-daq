#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_BASE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_BASE_HH_

// Class that covers the basics that DaqWorkers should inherit

//--- std includes ----------------------------------------------------------//
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>
using std::string;

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//--- project includes ------------------------------------------------------//

namespace daq {

class DaqWorkerBase {

  public:

    //ctor
    DaqWorkerBase(string conf_file) : conf_file_(conf_file),
                                      go_time_(false), 
                                      has_data_(false) {};

    // flow control functions                                  
    void StartWorker() { go_time_ = true; };
    void StopWorker() { go_time_ = false; };

    // Need to be implented by descendants.
    virtual void LoadConfig() = 0;

  protected:

    string conf_file_;
    std::atomic<bool> go_time_;
    std::atomic<bool> has_data_;
    std::mutex queue_mutex_;
    std::thread work_thread_;

    virtual bool HasEvent() = 0;
    virtual void WorkLoop() = 0;

};

} // daq

#endif