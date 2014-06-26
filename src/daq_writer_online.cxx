#include "daq_writer_online.hh"

namespace daq {

DaqWriterOnline::DaqWriterOnline(string conf_file) : DaqWriterBase(conf_file), online_ctx_(1), online_sck_(online_ctx_, ZMQ_PUSH)
{
  thread_live_ = true;
  go_time_ = false;
  end_of_batch_ = false;
  LoadConfig();

  writer_thread_ = std::thread(&DaqWriterOnline::SendMessageLoop, this);
}

void DaqWriterOnline::LoadConfig()
{
  ptree conf;
  read_json(conf_file_, conf);

  //  online_sck_ = zmq::socket_t(online_ctx, ZMQ_PUSH);
  online_sck_.connect(conf.get<string>("writers.online.port").c_str());

  message_size_ = conf.get<int>("writers.online.message_size");
  message_ = zmq::message_t(message_size_);
}

void DaqWriterOnline::PushData(const vector<event_data> &data_buffer)
{
  auto it = data_buffer.begin();

  while (data_queue_.size() < kMaxQueueSize && it != data_buffer.end()) {

    data_queue_.push(*it);
    ++it;

  }
}

void DaqWriterOnline::EndOfBatch(bool bad_data)
{
  data_queue_.empty();
}

void DaqWriterOnline::SendMessageLoop()
{
  while (thread_live_) {

    while (go_time_ && (data_queue_.size() > 0)) {

      PackMessage();

      while (message_ready_) {

        bool rc = online_sck_.send(message_, ZMQ_NOBLOCK);

        if (rc == true) {

          message_ready_ = false;

        }
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
  string buffer;
  buffer.reserve(message_size_);
  int count = 0;
  char str[50];

  json_spirit::Object json_map;

  json_map.push_back(json_spirit::Pair("run_number", 0));
  json_map.push_back(json_spirit::Pair("event_number", 1));

  for (auto &sis : data_queue_.front().sis_fast) {

    json_spirit::Object sis_map;

    sprintf(str, "system_clock");
    sis_map.push_back(json_spirit::Pair(str, sis.system_clock));

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

  for (auto &sis : data_queue_.front().sis_slow) {

    json_spirit::Object sis_map;

    sprintf(str, "system_clock");
    sis_map.push_back(json_spirit::Pair(str, sis.system_clock));

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
  for (auto &caen : data_queue_.front().caen_adc) {

    json_spirit::Object caen_map;

    sprintf(str, "system_clock");
    caen_map.push_back(json_spirit::Pair(str, caen.system_clock));

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

  buffer = json_spirit::write(json_map);
  buffer.append(":EOF");

  memcpy(message_.data(), buffer.c_str(), sizeof(buffer));

  // Code for testing output.
//  std::ofstream out;
//  out.open("test.json");
//  json_spirit::write(json_map, out);
//  out.close();

  message_ready_ = true;
}









} // ::daq