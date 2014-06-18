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


int main(int argc, char *argv[])
{
  // Set up the configuration.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json("config/.default_master.json", conf);  

  // Initialize some parameters.
  bool is_running = false;
  bool rc;
  //vector<* DaqWorker> daq_workers;

  // Set up a zmq socket.
  zmq::context_t master_ctx(1);
  zmq::socket_t master_sck(master_ctx, ZMQ_PULL);
  zmq::message_t message(10);

  // Connect the socket.
  master_sck.bind(conf.get<std::string>("master_port").c_str());

  while (true) {

    // Check for a message.
    rc = master_sck.recv(&message);
    cout << "rc: " << rc << endl;

    // Process the message.
    std::string msg_string;
    std::istringstream ss(static_cast<char*>(message.data()));
    std::getline(ss, msg_string, ':');

    if (rc == true) {

      cout << msg_string << endl;

    } else {

      cout << "No message." << endl;

    }

    if (is_running) {

    }

  }

  return 0;
}