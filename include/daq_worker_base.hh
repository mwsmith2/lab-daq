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

template <typename T>
class DaqWorkerBase {

  public:

    //ctor
    DaqWorkerBase(string name, string conf_file) : name_(name),
                                                   conf_file_(conf_file),
                                                   go_time_(false), 
                                                   has_event_(false) {};

    // flow control functions                                  
    void StartWorker() { go_time_ = true; };
    void StopWorker() { go_time_ = false; };
    bool HasEvent() { return has_event_; };

    // Need to be implented by descendants.
    virtual void LoadConfig() = 0;
    virtual T PopEvent() = 0;

  protected:

    string name_;
    string conf_file_;
    std::atomic<bool> has_event_;
    std::atomic<bool> go_time_;

    std::queue<T> data_queue_;
    std::mutex queue_mutex_;
    std::thread work_thread_;

    virtual void WorkLoop() = 0;

};

} // daq

#endif