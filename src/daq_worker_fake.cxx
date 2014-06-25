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

      // Get the system time
      auto t1 = high_resolution_clock::now();
      auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();     
      event_data_.system_clock = duration_cast<nanoseconds>(dtn).count();

      for (int i = 0; i < num_ch_; ++i){

        event_data_.device_clock[i] = clock();

        for (int j = 0; j < len_tr_; ++j){

          event_data_.trace[i][j] = 1000 + 100 * sin(j * 0.01);

        }
      }

      has_fake_event_ = true;
  
      std::this_thread::yield();
    }

    std::this_thread::yield();

    usleep(100);
  }
}

void DaqWorkerFake::GetEvent(event_struct &bundle)
{
  // Copy the data.
  bundle = event_data_;

  has_fake_event_ = false;
}

void DaqWorkerFake::WorkLoop() 
{
  while (true) {

    t0_ = high_resolution_clock::now();

    while(go_time_) {

      if (EventAvailable()) {

        event_struct bundle;
        GetEvent(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        has_event_ = true;
        queue_mutex_.unlock();

        cout << name_ << " pushed an event into the queue." << endl;

      }

      std::this_thread::yield();

      usleep(100);
    }

    std::this_thread::yield();

    usleep(100);
  }
}

event_struct DaqWorkerFake::PopEvent()
{
  // Copy the data.
  event_struct data = data_queue_.front();
  data_queue_.pop();

  // Check if this is that last event.
  if (data_queue_.size() == 0) has_event_ = false;

  return data;
}

} // daq