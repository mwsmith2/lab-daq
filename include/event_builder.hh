#ifndef SLAC_DAQ_INCLUDE_EVENT_BUILDER_HH_
#define SLAC_DAQ_INCLUDE_EVENT_BUILDER_HH_

//--- std includes ----------------------------------------------------------//
#include <ctime>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
using std::string;
using std::vector;
using std::cout;
using std::endl;

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//--- projects includes -----------------------------------------------------//
#include "daq_structs.hh"
#include "daq_worker_list.hh"
#include "daq_worker_base.hh"
#include "daq_worker_fake.hh"
#include "daq_writer_base.hh"
#include "daq_writer_root.hh"

namespace daq {

// This class pulls data form all the workers.
class EventBuilder {

  public:

    // ctor
    EventBuilder(const DaqWorkerList &daq_workers, 
                 const vector<DaqWriterBase *> daq_writers,
                 string conf_file);

    // dtor
    ~EventBuilder() {
        thread_live_ = false;
        builder_thread_.join();
        push_data_thread_.join();
    }

    // member functions
    void StartBuilder() { go_time_ = true; };
    void StopBuilder() { go_time_ = false; };
    void LoadConfig();

  private:

    // Simple variable declarations
    const int kMaxQueueLength = 10;
    string conf_file_;
    int live_time;
    int dead_time;
    int live_ticks;
    int batch_start;

    std::atomic<bool> thread_live_;
    std::atomic<bool> go_time_;
    std::atomic<bool> push_new_data_;
    std::atomic<bool> flush_time_;
    std::atomic<bool> got_last_event_;

    // Data accumulation variables
    DaqWorkerList daq_workers_;
    vector<DaqWriterBase *> daq_writers_;
    vector<event_data> push_data_vec_;
    std::queue<event_data> pull_data_que_;

    // Concurrency variables
    std::mutex queue_mutex_;
    std::mutex push_data_mutex_;
    std::thread builder_thread_;
    std::thread push_data_thread_;

    // Private member functions
    void BuilderLoop();
    void PushDataLoop();
    void StopWorkers();
    void StartWorkers();
};

} // ::daq

#endif