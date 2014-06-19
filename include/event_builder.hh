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

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//--- projects includes -----------------------------------------------------//
#include "daq_structs.hh"
#include "daq_worker_base.hh"
#include "daq_worker_fake.hh"
#include "data_writer_base.hh"
#include "data_writer_root.hh"

namespace daq {

typedef boost::variant<DaqWorkerBase<sis_3350> *, DaqWorkerBase<sis_3302> *> worker_ptr_types;

// This class pulls data form all the workers.
class EventBuilder {

  public:

    //ctors
    EventBuilder() {};
    EventBuilder(string conf_file, 
      const vector<worker_ptr_types>& daq_workers);

    // inline functions
    void StartBuilder() { go_time_ = true; };
    void StopBuilder() { go_time_ = false; };

    // member functions
    void LoadConfig();
    void Init(string conf_file,const vector<worker_ptr_types>& daq_workers);

  private:

    // Simple variable declarations
    const int max_queue_length_ = 10;
    string conf_file_;
    std::atomic<bool> go_time_;
    std::atomic<bool> push_new_data_;

    // Data accumulation variables
    vector<worker_ptr_types> daq_workers_;
    vector<event_data> push_data_vec_;
    std::queue<event_data> pull_data_que_;

    // Concurrency variables
    std::mutex queue_mutex_;
    std::thread builder_thread_;
    std::thread push_data_thread_;

    // Private member functions
    bool WorkersHaveEvents();
    void GetEventData(event_data& bundle);
    void BuilderLoop();
    void PushDataLoop();

};

} // ::daq

#endif