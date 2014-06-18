#include "daq_worker_fake.hh"

#include <iostream>
using std::cout;
using std::endl;

namespace daq {

// ctor
DaqWorkerFake::DaqWorkerFake(string name, string conf) : DaqWorkerBase<event_struct>(name, conf)
{ 
  LoadConfig();

  num_ch_ = SIS_3350_CH;
  len_tr_ = SIS_3350_LN;
  has_fake_event_ = false;

  work_thread_ = std::thread(&DaqWorkerFake::WorkLoop, this);
  event_thread_ = std::thread(&DaqWorkerFake::GenerateEvent, this);
}

void DaqWorkerFake::LoadConfig() 
{
  // Load the config file
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  rate_ = conf.get<double>("rate");
  jitter_ = conf.get<double>("jitter");
  drop_rate_ = conf.get<double>("drop_rate");
}

void DaqWorkerFake::GenerateEvent()
{
  // Make fake events.
  while (true) {

    while (go_time_) {

      usleep(1.0e6 / rate_);

      cout << name_ << " generated an event." << endl;

      for (int i = 0; i < num_ch_; ++i){

        event_data_.timestamp[i] = clock();

        for (int j = 0; j < len_tr_; ++j){

          event_data_.trace[i][j] = sin(j * 0.1);

        }
      }

      has_fake_event_ = true;    
    }
  } 
}

void DaqWorkerFake::GetEvent(event_struct bundle)
{
  // Copy the data.
  for (int i = 0; i < num_ch_; ++i){
    bundle.timestamp[i] = event_data_.timestamp[i];
    memcpy(bundle.trace[i], event_data_.trace[i], len_tr_);
  }

  has_fake_event_ = false;
}

void DaqWorkerFake::WorkLoop() 
{
  while (true) {

    while(go_time_) {

      if (EventAvailable()) {

        event_struct bundle;
        GetEvent(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        has_event_ = true;
        queue_mutex_.unlock();

        cout << name_ << " pushed an event into the queue." << endl;

      } else {

        usleep(100);

      }

      std::this_thread::yield();

    }
  }
}

event_struct DaqWorkerFake::PopEvent()
{
  event_struct data(data_queue_.front());
  data_queue_.pop();

  // Check if this is that last event.
  if (data_queue_.size() == 0) has_event_ = false;

  return data;
}

} // daq