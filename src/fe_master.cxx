// Master frontend


//--- std includes ----------------------------------------------------------//
#include <cassert>
#include <ctime>
#include <iostream>
#include <vector>
using std::cout;
using std::endl;
using std::vector;
using std::string;

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <zmq.hpp>

//--- project includes -----------------------------------------------------//

// Anonymous namespace for "global" paramaters
namespace {
  
  // simple declarations
  bool is_running = false;
  bool rc = 0;

  // std declarations
  string msg_string;
  std::istringstream ss;

  // zmq declarations
  zmq::context_t master_ctx(1);
  zmq::socket_t master_sck(master_ctx, ZMQ_PULL);
  zmq::message_t message(10);

  // project declarations
  // vector<* DaqWorkerBase> daq_workers;
}

// Function declarations
int LoadConfig();
int StartWorkers();
int StopWorkers();

// The main loop
int main(int argc, char *argv[])
{
  LoadConfig();

  while (true) {

    // Check for a message.
    rc = master_sck.recv(&message, ZMQ_NOBLOCK);

    if (rc == true) {

      // Process the message.
      ss = std::istringstream(static_cast<char *>(message.data()));
      std::getline(ss, msg_string, ':');

      if (msg_string == string("START") && !is_running) {

        StartWorkers();

      } else if (msg_string == string("STOP") && is_running) {

        StopWorkers();

      }
    }
  }

  return 0;
}

int LoadConfig(){
  // load up the configuration.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json("config/.default_master.json", conf);  

  // Connect the socket.
  master_sck.bind(conf.get<string>("master_port").c_str());

  return 0;
}

// Flush the buffers and start data taking.
int StartWorkers(){
  cout << "Starting run." << endl;
  is_running = true;

  return 0;
}

// Write the data file and reset workers.
int StopWorkers(){
  cout << "Stopping run." << endl;
  is_running = false;

  return 0;
}