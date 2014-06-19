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

namespace daq {

typedef boost::variant<DaqWorkerBase<sis_3350> *, DaqWorkerBase<sis_3302> *> worker_ptr_types;

struct event_data {
  vector<sis_3350> fake;
  vector<sis_3350> sis_fast;
  vector<sis_3302> sis_slow;
};

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
    string conf_file_;
    std::atomic<bool> go_time_;

    // 
    vector<worker_ptr_types> daq_workers_;
    std::queue<event_data> data_queue_;

    std::mutex queue_mutex_;
    std::thread builder_thread_;

    bool WorkersHaveEvents();
    void GetEventData(event_data& bundle);
    void BuilderLoop();


};

} // ::daq

#endif