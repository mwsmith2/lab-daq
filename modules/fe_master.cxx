// Master Frontend

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
#include <boost/foreach.hpp>
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;

#include <zmq.hpp>

//--- project includes -----------------------------------------------------//
#include "daq_worker_fake.hh"
#include "daq_worker_sis3350.hh"
#include "daq_worker_sis3302.hh"
#include "event_builder.hh"
using namespace daq;

typedef boost::variant<DaqWorkerBase<sis_3350> *, DaqWorkerBase<sis_3302> *> worker_ptr_types;

int daq::vme::device = -1;

// Anonymous namespace for "global" parameters
namespace {
  
  // simple declarations
  bool is_running = false;
  bool rc = 0;

  // std declarations
  string msg_string;

  // zmq declarations
  zmq::context_t master_ctx(1);
  zmq::socket_t master_sck(master_ctx, ZMQ_PULL);
  zmq::message_t message(10);

  // project declarations
  vector<worker_ptr_types> daq_workers;
  vector<DaqWriterBase *> daq_writers;
  EventBuilder *event_builder;
}

// Function declarations
int LoadConfig();
int StartRun();
int StopRun();

// The main loop
int main(int argc, char *argv[])
{
  LoadConfig();

  while (true) {

    // Check for a message.
    rc = master_sck.recv(&message, ZMQ_NOBLOCK);

    if (rc == true) {

      // Process the message.
      std::istringstream ss(static_cast<char *>(message.data()));
      std::getline(ss, msg_string, ':');

      if (msg_string == string("START") && !is_running) {

        StartRun();

      } else if (msg_string == string("STOP") && is_running) {

        StopRun();

      }
    }
  }

  return 0;
}

int LoadConfig(){
  // load up the configuration.
  string conf_file("config/.default_master.json");
  ptree conf;
  read_json(conf_file, conf);  

  // Connect the socket.
  master_sck.bind(conf.get<string>("master_port").c_str());

  // Get the fake data writers (for testing).
  BOOST_FOREACH(const ptree::value_type &v, conf.get_child("devices.fake")) {

    string name(v.first);
    string conf_file(v.second.data());

    daq_workers.push_back(new DaqWorkerFake(name, conf_file));
  } 

  // Set up the sis3350 devices.
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3350")) {

    string name(v.first);
    string conf_file(v.second.data());

    daq_workers.push_back(new DaqWorkerSis3350(name, conf_file));
  }  

  // Set up the sis3302 devices.
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3302")) {

    string name(v.first);
    string conf_file(v.second.data());

    daq_workers.push_back(new DaqWorkerSis3302(name, conf_file));
  }

  // Set up the data writers.
  daq_writers.push_back(new DaqWriterRoot(conf_file));

  // Set up the event builder.
  event_builder = new EventBuilder(daq_workers, daq_writers);

  return 0;
}

// Flush the buffers and start data taking.
int StartRun(){
  cout << "Starting run." << endl;
  is_running = true;

  // Start the event builder
  event_builder->StartBuilder();

  // Start the writers
  for (auto it = daq_writers.begin(); it != daq_writers.end(); ++it) {
    (*it)->StartWriter();
  }

  // Start the data gatherers
  for (auto it = daq_workers.begin(); it != daq_workers.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StartWorker();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StartWorker();

    }
  }

  return 0;
}

// Write the data file and reset workers.
int StopRun(){
  cout << "Stopping run." << endl;
  is_running = false;

  // Stop the event builder
  event_builder->StopBuilder();

  // Stop the writers
  for (auto it = daq_writers.begin(); it != daq_writers.end(); ++it) {
    (*it)->StopWriter();
  }

  // Stop the data gatherers
  for (auto it = daq_workers.begin(); it != daq_workers.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StopWorker();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StopWorker();

    }
  }

  return 0;
}
