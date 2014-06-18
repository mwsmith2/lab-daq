#ifndef SLAC_DAQ_INCLUDE_DAQ_WORKER_BASE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WORKER_BASE_HH_

// Class that covers the basics that DaqWorkers should inherit

//--- std includes ----------------------------------------------------------//
#include <string>
#include <atomic>
using std::string;

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>

//--- project includes ------------------------------------------------------//

namespace daq {

class DaqWorkerBase {

  public:

    //ctor
    DaqWorkerBase(string conf_file);

    virtual int load_config() = 0;
    virtual int start_worker() = 0;
    virtual int stop_worker() = 0;
    virtual int loop_thread();

  protected:

    string conf_file_;
    std::atomic bool go_time_;
};

}