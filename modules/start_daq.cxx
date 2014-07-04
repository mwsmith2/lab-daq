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
  zmq::socket_t handshake_sck(ctx, ZMQ_REQ);

  start_sck.connect(conf.get<std::string>("trigger_port").c_str());
  handshake_sck.connect(conf.get<std::string>("handshake_port").c_str());

  std::string connect("CONNECTED");
  zmq::message_t handshake_msg(connect.size());
  memcpy(handshake_msg.data(), connect.c_str(), connect.size());

  // Establish a connection.
  handshake_sck.send(handshake_msg);
  bool rc = handshake_sck.recv(&handshake_msg);

  if (rc == true) {
    // Create the start message
    std::string trigger("START:test:");
    zmq::message_t start_msg(trigger.size());
    memcpy(start_msg.data(), trigger.c_str(), trigger.size());
    
    start_sck.send(start_msg);
    cout << "Connection established.  Sending start trigger." << endl;
    return 0;

  } else {
    
    cout << "Connection not established.  No trigger sent." << endl;
    return 0;
  }
}
