// Generate a start trigger for the DAQ.

//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <string>
using std::cout;
using std::endl;

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <zmq.hpp>

#include "daq_structs.hh"

// Needs to be defined.
int daq::vme::device = -1;

int main(int argc, char *argv[])
{
  // Get the system configuration
  boost::property_tree::ptree conf;
  boost::property_tree::read_json("config/.default_master.json", conf);

  zmq::context_t ctx(1);
  zmq::socket_t start_sck(ctx, ZMQ_PUSH);

  start_sck.connect(conf.get<std::string>("master_port").c_str());

  // Create the start message
  std::string trigger("STOP:");
  zmq::message_t start_msg(trigger.size());

  memcpy(start_msg.data(), trigger.c_str(), trigger.size());

  start_sck.send(start_msg);

  return 0;
}