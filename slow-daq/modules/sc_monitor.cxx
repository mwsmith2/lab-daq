//--- std includes ----------------------------------------------------------//
#include <thread>
#include <ctime>
#include <iostream>
#include <atomic>
using std::cout;
using std::endl;

//--- other includes --------------------------------------------------------//
#include <zmq.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;

//--- project includes ------------------------------------------------------//
#include "async_root_writer.hh"

// Anonymous namespace for "global" variables.
namespace {

  // The config container.
  ptree conf;

  // Set up the sockets.
  zmq::context_t sc_ctx(1);
  zmq::socket_t data_sck(sc_ctx, ZMQ_SUB);
  zmq::socket_t tree_sck(sc_ctx, ZMQ_SUB);
  zmq::socket_t msg_sck(sc_ctx, ZMQ_PUB);

  long long ticks_to_write; // The timeout before writing the root file.
}

// Helper Functions
void MessageLoop();

// The main program.
int main(int argc, char *argv[])
{
  // Load the config file.
  read_json("config/.default_sc.json", conf);

  // Connect the socket and subscribe to everything.
  msg_sck.bind(conf.get<string>("msg_port").c_str());
  data_sck.bind(conf.get<string>("data_port").c_str());
  data_sck.setsockopt(ZMQ_SUBSCRIBE, "", 0);
  tree_sck.bind(conf.get<string>("tree_port").c_str());
  tree_sck.setsockopt(ZMQ_SUBSCRIBE, "", 0);

  ticks_to_write = conf.get<int>("run_time", 60); // in minutes
  ticks_to_write *= 60; // to seconds
  ticks_to_write *= CLOCKS_PER_SEC; // to system ticks.

  while (true) {
    MessageLoop();
  }

  return 0;
}

void MessageLoop()
{
  zmq::message_t message_in(1024);
  std::string str;
  bool time_to_write = false;

  read_json("data/.sc_runs.json", conf);
  str = conf.get<string>("prefix");
  char tmp[10];
  sprintf(tmp, "%04i", conf.get<int>("run_number"));
  str.append(tmp);
  str.append(conf.get<string>("suffix"));

  conf.put("run_number", conf.get<int>("run_number") + 1);
  write_json("data/.sc_runs.json", conf);

  sc::AsyncRootWriter sc_root_writer(str); // run name

  long long start_time = clock();
  cout << "Starting a new run at time " << start_time << endl;
  while (!time_to_write) {

    // Check for new tree creation messages.
    if (tree_sck.recv(&message_in, ZMQ_DONTWAIT)) {

      std::istringstream ss(static_cast<char *>(message_in.data()));
      std::getline(ss, str, ':');

      if (str == "TREE") {
	
        std::getline(ss, str);
        sc_root_writer.CreateTree(str);

      } else {

        cout << "Message received with an unknown request." << endl;

      }
    }

    // Check for data messages.
    if (data_sck.recv(&message_in, ZMQ_DONTWAIT)) {

      std::istringstream ss(static_cast<char *>(message_in.data()));
      std::getline(ss, str, ':');

      if (str == "DATA") {

        std::getline(ss, str);
        int rc = sc_root_writer.PushData(str);
        cout << "Got some data." << endl;

        if (rc != 0) {

          cout << "Tree not found in slow control data." << endl;
          cout << "Requesting structure of new tree." << endl;

          std::getline(ss, str, ':'); // Should be the name of the device.
          zmq::message_t msg(str.size());
          std::copy(str.begin(), str.end(), static_cast<char *>(msg.data()));

          int count = 0;
          while (!msg_sck.send(msg, ZMQ_NOBLOCK) && (count < 50)) {

            msg_sck.send(msg, ZMQ_NOBLOCK);
            usleep(100);

            count++;
          }
        }

      } else {

        cout << "Message received with an unknown request." << endl;

      }
    }
    time_to_write = (clock() - start_time) > ticks_to_write;
  }

  sc_root_writer.WriteFile();
}






