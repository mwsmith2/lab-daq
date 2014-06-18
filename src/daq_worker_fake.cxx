#include "daq_worker_fake.hh"

namespace daq {

// ctor
DaqWorkerFake::DaqWorkerFake(string conf) : DaqWorkerBase(conf) 
{ 
  LoadConfig();

  num_ch_ = SIS_3350_CH;
  len_tr_ = SIS_3350_LN;
  has_fake_event_ = false;

  work_thread_ = std::thread(&DaqWorkerFake::WorkLoop, this);
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

    usleep(1.0e6 / rate_);

    for (int i = 0; i < num_ch_; ++i){

      event_data_.timestamp[i] = clock();

      for (int j = 0; j < len_tr_; ++j){

        event_data_.trace[i][j] = sin(j * 0.1);

      }
    }

    has_fake_event_ = true;    
  } 
}

void DaqWorkerFake::GetEvent(data_struct bundle)
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

    if (HasEvent()) {

      data_struct bundle;
      GetEvent(bundle);

      queue_mutex_.lock();
      data_queue_.push(bundle);
      has_event_ = true;
      queue_mutex_.unlock();

    } else {

      usleep(100);

    }

    std::this_thread::yield();

  }
}

} // daq