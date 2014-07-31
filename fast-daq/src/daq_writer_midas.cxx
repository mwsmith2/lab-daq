#include "daq_writer_midas.hh"

namespace daq {

DaqWriterMidas::DaqWriterMidas(string conf_file) : DaqWriterBase(conf_file), midas_ctx_(1), midas_sck_(midas_ctx_, ZMQ_PUSH)
{
  thread_live_ = true;
  go_time_ = false;
  end_of_batch_ = false;
  queue_has_data_ = false;
  LoadConfig();

  writer_thread_ = std::thread(&DaqWriterMidas::SendMessageLoop, this);
}

void DaqWriterMidas::LoadConfig()
{
  ptree conf;
  read_json(conf_file_, conf);

  int hwm = conf.get<int>("writers.midas.high_water_mark", 10);
  midas_sck_.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
  int linger = 0;
  midas_sck_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger)); 
  midas_sck_.connect(conf.get<string>("writers.midas.port").c_str());
}

void DaqWriterMidas::PushData(const vector<event_data> &data_buffer)
{
  number_of_events_ += data_buffer.size();

  auto it = data_buffer.begin();
  writer_mutex_.lock();
  while (it != data_buffer.end()) {

    data_queue_.push(*it);
    ++it;

  }
  writer_mutex_.unlock();

  cout << "Midas writer was pushed some data." << endl;
  queue_has_data_ = true;
}

void DaqWriterMidas::EndOfBatch(bool bad_data)
{
  writer_mutex_.lock();
  data_queue_.empty();
  writer_mutex_.unlock();

  zmq::message_t msg(10);
  memcpy(msg.data(), string("__EOB__").c_str(), 10);

  int count = 0;
  while (count < 10) {

    midas_sck_.send(msg, ZMQ_DONTWAIT);
    usleep(100);

    count++;
  }
}

void DaqWriterMidas::SendMessageLoop()
{
  ptree conf;
  read_json(conf_file_, conf);

  zmq::context_t send_ctx_(1);
  zmq::socket_t send_sck_(send_ctx_, ZMQ_PUSH);

  int hwm = conf.get<int>("writers.midas.high_water_mark", 100);
  send_sck_.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
  int linger = 0;
  send_sck_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger)); 
  send_sck_.connect(conf.get<string>("writers.midas.port").c_str());

  while (thread_live_) {

    while (go_time_ && queue_has_data_) {
      
      bool rc = SendDataMessage();
      
      if (rc == true) {
	
	cout << "Midas writer sent message successfully." << endl;
	message_ready_ = false;
	
      }
      
      usleep(daq::short_sleep);
      std::this_thread::yield();
     }
    
    usleep(daq::long_sleep);
    std::this_thread::yield();
  }
}

bool DaqWriterMidas::SendDataMessage()
{
  using boost::uint64_t;

  cout << "Packing message." << endl;

  int count = 0;
  char str[50];
  string data;

  // Copy the first event
  writer_mutex_.lock();
  if (data_queue_.empty()) return;

  event_data data = data_queue_.front();
  data_queue_.pop();
  if (data_queue_.size() == 0) queue_has_data_ = false;
  writer_mutex_.unlock();

  // For each device send "<dev_name>:<dev_type>:<binary_data>".
  
  int count = 0;
  for (auto &sis : data.sis_fast) {
    
    sprintf(str, "sis_fast_%i", count++);
    data.append(string(str));
    data.append("sis_3350:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(sis));
    
    memcpy(&data_msg, &data, sizeof(data));
    memcpy(&data_msg + sizeof(data), &sis, sizeof(sis));

    midas_skc_.send(data_msg, ZMQ_SENDMORE);
  }

  count = 0;
  for (auto &sis : data.sis_slow) {

  }

  count = 0;
  for (auto &caen : data.caen_adc) {

  }

  count = 0;
  for (auto &caen : data.caen_drs) {

  }

  cout << "Midas writer sent." << endl;
}

} // ::daq
