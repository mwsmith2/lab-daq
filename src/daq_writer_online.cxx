#include "daq_writer_online.hh"

namespace daq {

DaqWriterOnline::DaqWriterOnline(string conf_file) : DaqWriterBase(conf_file), online_ctx_(1), online_sck_(online_ctx_, ZMQ_PUSH)
{
  thread_live_ = true;
  go_time_ = false;
  end_of_batch_ = false;
  queue_has_data_ = false;
  LoadConfig();

  writer_thread_ = std::thread(&DaqWriterOnline::SendMessageLoop, this);
}

void DaqWriterOnline::LoadConfig()
{
  ptree conf;
  read_json(conf_file_, conf);

  online_sck_.connect(conf.get<string>("writers.online.port").c_str());

  message_size_ = conf.get<int>("writers.online.message_size");
}

void DaqWriterOnline::PushData(const vector<event_data> &data_buffer)
{
  auto it = data_buffer.begin();

  while (data_queue_.size() < kMaxQueueSize && it != data_buffer.end()) {

    data_queue_.push(*it);
    ++it;

  }

  cout << "Online writer was pushed some data." << endl;
  queue_has_data_ = true;
}

void DaqWriterOnline::EndOfBatch(bool bad_data)
{
  data_queue_.empty();
}

void DaqWriterOnline::SendMessageLoop()
{
  while (thread_live_) {

    while (go_time_ && queue_has_data_) {

      if (!message_ready_) {
        PackMessage();
      }

      while (message_ready_) {

        writer_mutex_.lock();
        cout << "Online writer is sending message." << endl;
        bool rc = online_sck_.send(message_);
        writer_mutex_.unlock();

        if (rc == true) {

          cout << "Message sent successfully." << endl;
          message_ready_ = false;

        }

        usleep(100);
        std::this_thread::yield();
      }

    usleep(100);
    std::this_thread::yield();

    }

    usleep(100);
    std::this_thread::yield();
  }
}

void DaqWriterOnline::PackMessage()
{
  using boost::uint64_t;

  cout << "Packing message." << endl;

  int count = 0;
  char str[50];

  json_spirit::Object json_map;

  writer_mutex_.lock();
  event_data data = data_queue_.front();
  data_queue_.pop();
  if (data_queue_.size() == 0) queue_has_data_ = false;
  writer_mutex_.unlock();

  json_map.push_back(json_spirit::Pair("run_number", 0));
  json_map.push_back(json_spirit::Pair("event_number", 1));

  for (auto &sis : data.sis_fast) {

    json_spirit::Object sis_map;

    sprintf(str, "system_clock");
    sis_map.push_back(json_spirit::Pair(str, (uint64_t)sis.system_clock));

    sprintf(str, "device_clock");
    sis_map.push_back(json_spirit::Pair(str, 
      json_spirit::Array(
        &sis.device_clock[0], 
        &sis.device_clock[SIS_3350_CH])));

    json_spirit::Array arr;
    for (int ch = 0; ch < SIS_3350_CH; ++ch) {
      arr.push_back(json_spirit::Array(
        &sis.trace[ch][0], &sis.trace[ch][SIS_3350_LN]));

    }

    sprintf(str, "trace");
    sis_map.push_back(json_spirit::Pair(str, arr));

    sprintf(str, "sis_fast_%i", count++);
    json_map.push_back(json_spirit::Pair(str, sis_map));
  }

  for (auto &sis : data.sis_slow) {

    json_spirit::Object sis_map;

    sprintf(str, "system_clock");
    sis_map.push_back(json_spirit::Pair(str, (uint64_t)sis.system_clock));

    sprintf(str, "device_clock");
    sis_map.push_back(json_spirit::Pair(str, 
      json_spirit::Array(
        &sis.device_clock[0], 
        &sis.device_clock[SIS_3302_CH])));

    json_spirit::Array arr;
    for (int ch = 0; ch < SIS_3302_CH; ++ch) {
      arr.push_back(json_spirit::Array(
        &sis.trace[ch][0], &sis.trace[ch][SIS_3302_LN]));

    }

    sprintf(str, "trace");
    sis_map.push_back(json_spirit::Pair(str, arr));

    sprintf(str, "sis_slow_%i", count++);
    json_map.push_back(json_spirit::Pair(str, sis_map));
  }

  count = 0;
  for (auto &caen : data.caen_adc) {

    json_spirit::Object caen_map;

    sprintf(str, "system_clock");
    caen_map.push_back(json_spirit::Pair(str, (uint64_t)caen.system_clock));

    sprintf(str, "device_clock");
    caen_map.push_back(json_spirit::Pair(str, 
      json_spirit::Array(
        &caen.device_clock[0], 
        &caen.device_clock[CAEN_1785_CH])));

    sprintf(str, "value");
    caen_map.push_back(json_spirit::Pair(str, 
      json_spirit::Array(
        &caen.value[0], 
        &caen.value[CAEN_1785_CH])));

    sprintf(str, "caen_adc_%i", count++);
    json_map.push_back(json_spirit::Pair(str, caen_map));
  }

  string buffer = json_spirit::write(json_map);
  buffer.append("__EOM__");

  message_ = zmq::message_t(message_size_);
  memcpy(message_.data(), buffer.c_str(), buffer.size());

  cout << "Online writer message ready." << endl;
  message_ready_ = true;
}









} // ::daq