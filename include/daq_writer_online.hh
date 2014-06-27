#ifndef SLAC_DAQ_INCLUDE_DAQ_WRITER_ONLINE_HH_
#define SLAC_DAQ_INCLUDE_DAQ_WRITER_ONLINE_HH_

//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <fstream>
#include <queue>
using std::cout;
using std::endl;

//--- other includes --------------------------------------------------------//
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;

#include <zmq.hpp>
#include <json_spirit.h>

//--- project includes ------------------------------------------------------//
#include "daq_writer_base.hh"
#include "daq_structs.hh"

namespace daq {

// A class that interfaces with the an EventBuilder and writes a root file.

class DaqWriterOnline : public DaqWriterBase {

  public:

    //ctor
    DaqWriterOnline(string conf_file);

    // Member Functions
    void LoadConfig();
    void StartWriter() { go_time_ = true; };
    void StopWriter() { go_time_ = false; };

    void PushData(const vector<event_data> &data_buffer);
    void EndOfBatch(bool bad_data);

  private:

    const int kMaxQueueSize = 20;
    int message_size_;
    std::atomic<bool> message_ready_;
    std::atomic<bool> go_time_;
    std::atomic<bool> queue_has_data_;
    std::queue<event_data> data_queue_;

    // zmq stuff
    zmq::context_t online_ctx_;
    zmq::socket_t online_sck_;
    zmq::message_t message_;

    void PackMessage();
    void SendMessageLoop();

};

} // ::daq

#endif