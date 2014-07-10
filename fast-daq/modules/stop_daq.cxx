// Generate a stop trigger for the DAQ.

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
  zmq::socket_t stop_sck(ctx, ZMQ_PUB);
  zmq::socket_t handshake_sck(ctx, ZMQ_REQ);

  stop_sck.connect(conf.get<std::string>("trigger_port").c_str());
  handshake_sck.connect(conf.get<std::string>("handshake_port").c_str());

  std::string connect("CONNECTED");
  zmq::message_t handshake_msg(connect.size());
  memcpy(handshake_msg.data(), connect.c_str(), connect.size());

  // Establish a connection.
  handshake_sck.send(handshake_msg);
  bool rc = handshake_sck.recv(&handshake_msg);

  if (rc == true) {
    // Create the stop message
    std::string trigger("STOP:");
    zmq::message_t stop_msg(trigger.size());
    memcpy(stop_msg.data(), trigger.c_str(), trigger.size());
    
    stop_sck.send(stop_msg);
    cout << "Connection established.  Sending stop trigger." << endl;
    return 0;

  } else {
    
    cout << "Connection not established.  No trigger sent." << endl;
    return 0;
  }
}