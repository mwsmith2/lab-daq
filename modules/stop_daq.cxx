// Generate a start trigger for the DAQ.

//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <string>
#include <ctime>
using std::cout;
using std::endl;

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <zmq.hpp>


int main(int argc, char *argv[])
{
  // Get the system configuration
  boost::property_tree::ptree conf;
  boost::property_tree::read_json("config/.default_master.json", conf);

  zmq::context_t ctx(1);
  zmq::socket_t start_sck(ctx, ZMQ_PUB);

  start_sck.connect(conf.get<std::string>("master_port").c_str());
  usleep(10000);

  // Create the start message
  std::string trigger("STOP:");
  zmq::message_t start_msg(trigger.size());

  memcpy(start_msg.data(), trigger.c_str(), trigger.size());

  start_sck.send(start_msg);

  return 0;
}
