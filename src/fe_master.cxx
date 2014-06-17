// Master frontend


//--- std includes ----------------------------------------------------------//
#include <cassert>
#include <ctime>
#include <iostream>
#include <vector>
using std::cout;
using std::endl;
using std::vector;

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <zmq.hpp>

//--- project includes -----------------------------------------------------//


int main(int argc, char **argv)
{
  // Initialize some parameters.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json("config/.default_master.json", conf);  
  //vector<* DaqWorker> daq_workers;
  bool is_running = false;

  // Set up a zmq socket.
  zmq::context_t master_ctx(1);
  zmq::socket_t master_sck(master_ctx, ZMQ_SUB);
  zmq::message_t message;

  // Connect the socket.
  master_sck.bind(conf.get<std::string>("master_port").c_str());

  while (true) {

    if (master_sck.recv(&message, ZMQ_NOBLOCK) == 0) {

      cout << message.data() << endl;;

    }

    if (is_running) {

    }

  }

}