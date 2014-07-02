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
#include "daq_worker_list.hh"
#include "daq_writer_online.hh"
#include "daq_writer_root.hh"
#include "event_builder.hh"
#include "daq_structs.hh"

using namespace daq;

int daq::vme::device = -1;

// Anonymous namespace for "global" parameters
namespace {
  
  // simple declarations
  bool is_running = false;
  bool rc = 0;

  // std declarations
  string msg_string;
  string conf_file;

  // zmq declarations
  zmq::context_t master_ctx(1);
  zmq::socket_t master_sck(master_ctx, ZMQ_SUB);
  zmq::message_t message(10);

  // project declarations
  DaqWorkerList daq_workers;
  vector<DaqWriterBase *> daq_writers;
  EventBuilder *event_builder = nullptr;
}

// Function declarations
int LoadConfig();
int ReloadConfig();
int StartRun();
int StopRun();

// The main loop
int main(int argc, char *argv[])
{
  // If there was a command line argument, grab that file.
  if (argc > 1) {

    conf_file = string(argv[1]);

  } else {

    conf_file = string("config/.default_master.json"); // default

  }

  // Load the configuration
  LoadConfig();

  while (true) {

    // Check for a message.
    rc = master_sck.recv(&message);

    if (rc == true) {

      // Process the message.
      std::istringstream ss(static_cast<char *>(message.data()));
      std::getline(ss, msg_string, ':');

      if (msg_string == string("START") && !is_running) {
	
	string file_name("data/run_");
	std::getline(ss, msg_string, ':');
	file_name.append(msg_string);
	file_name.append(".root");

	ptree conf;
	read_json(conf_file, conf);
	conf.put("writers.root.file", file_name);
	write_json(conf_file, conf);

        ReloadConfig();
        StartRun();

      } else if (msg_string == string("STOP") && is_running) {

        StopRun();

      }
    }

    usleep(daq::long_sleep);
  }

  return 0;
}

int LoadConfig() 
{
  // Load up the configuration file
  cout << "Opening config file: " << conf_file << endl;
  ptree conf;
  read_json(conf_file, conf);

  // Connect the socket.
  master_sck.bind(conf.get<string>("master_port").c_str());
  master_sck.setsockopt(ZMQ_SUBSCRIBE, "", 0);

  // Get the fake data writers (for testing).
  BOOST_FOREACH(const ptree::value_type &v, conf.get_child("devices.fake")) {
    
    string name(v.first);
    string dev_conf_file(v.second.data());

    daq_workers.PushBack(new DaqWorkerFake(name, dev_conf_file));
  } 

  // Set up the sis3350 devices.
  BOOST_FOREACH(const ptree::value_type &v, 
		            conf.get_child("devices.sis_3350")) {

    string name(v.first);
    string dev_conf_file(v.second.data());

    daq_workers.PushBack(new DaqWorkerSis3350(name, dev_conf_file));
  }  

  // Set up the sis3302 devices.
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3302")) {

    string name(v.first);
    string dev_conf_file(v.second.data());

    daq_workers.PushBack(new DaqWorkerSis3302(name, dev_conf_file));
  }

  // Set up the sis3302 devices.
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.caen_1785")) {

    string name(v.first);
    string dev_conf_file(v.second.data());

    daq_workers.PushBack(new DaqWorkerCaen1785(name, dev_conf_file));
  }

  // Set up the writers.
  BOOST_FOREACH(const ptree::value_type &v,
                conf.get_child("writers")) {
 
    if (string(v.first) == string("root")) {
   
      daq_writers.push_back(new DaqWriterRoot(conf_file));
   
    } else if (string(v.first) == string("online")) {
   
      daq_writers.push_back(new DaqWriterOnline(conf_file));
   
    }
  }

  // Set up the event builder.
  event_builder = new EventBuilder(daq_workers, daq_writers, conf_file);

  return 0;
}

int ReloadConfig() {
  // load up the configuration.
  ptree conf;
  read_json(conf_file, conf);

  // Delete the allocated workers.
  daq_workers.FreeList();

  // Delete the allocated writers.
  for (auto &writer : daq_writers) {
    delete writer;
  }
  daq_writers.resize(0);

  delete event_builder;

  // Get the fake data writers (for testing).
  BOOST_FOREACH(const ptree::value_type &v, conf.get_child("devices.fake")) {
    
    string name(v.first);
    string dev_conf_file(v.second.data());

    daq_workers.PushBack(new DaqWorkerFake(name, dev_conf_file));
  } 

  // Set up the sis3350 devices.
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3350")) {

    string name(v.first);
    string dev_conf_file(v.second.data());

    daq_workers.PushBack(new DaqWorkerSis3350(name, dev_conf_file));
  }  

  // Set up the sis3302 devices.
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3302")) {

    string name(v.first);
    string dev_conf_file(v.second.data());

    daq_workers.PushBack(new DaqWorkerSis3302(name, dev_conf_file));
  }

  // Set up the sis3302 devices.
  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.caen_1785")) {

    string name(v.first);
    string dev_conf_file(v.second.data());

    daq_workers.PushBack(new DaqWorkerCaen1785(name, dev_conf_file));
  }

  // Set up the writers.
  BOOST_FOREACH(const ptree::value_type &v,
                conf.get_child("writers")) {
 
    if (string(v.first) == string("root")) {
   
      daq_writers.push_back(new DaqWriterRoot(conf_file));
   
    } else if (string(v.first) == string("online")) {
   
      daq_writers.push_back(new DaqWriterOnline(conf_file));
   
    }
  }

  // Set up the event builder.
  event_builder = new EventBuilder(daq_workers, daq_writers, conf_file);

  return 0;
}

// Flush the buffers and start data taking.
int StartRun() {
  cout << "Starting run." << endl;
  is_running = true;

  // Start the event builder
  event_builder->StartBuilder();

  // Start the writers
  for (auto it = daq_writers.begin(); it != daq_writers.end(); ++it) {
    (*it)->StartWriter();
  }

  // Start the data gatherers
  daq_workers.StartRun();

  return 0;
}

// Write the data file and reset workers.
int StopRun() {
  cout << "Stopping run." << endl;
  is_running = false;

  // Stop the event builder
  event_builder->StopBuilder();

  // Stop the writers
  for (auto it = daq_writers.begin(); it != daq_writers.end(); ++it) {
    (*it)->StopWriter();
  }

  // Stop the data gatherers
  daq_workers.StopRun();

  return 0;
}
