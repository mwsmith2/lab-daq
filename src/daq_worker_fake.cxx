#include "daq_worker_fake.hh"

namespace daq {

// ctor
DaqWorkerFake(string conf) : DaqWorkerBase(conf) 
{ 
  load_config();

  work_thread = std::thread(work_loop);
}

void DaqWorkerFake::LoadConfig() 
{
  // Load the config file
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file, conf);

  rate_ = conf.get<double>("rate");
  jitter_ = conf.get<double>("jitter");
  drop_rate_ = conf.get<double>("drop_rate");
}

bool DaqWorkerFake::HasData(){

}

void DaqWorkerFake::WorkLoop() 
{
  while (true) {

    if (HasData()) {

      struct data_struct bundle;
      GetData(bundle);

      que_mutex.lock();
      data_queue_.push_back(bundle);
      has_event = true;
      que_mutex.unlock();
   
    }

    std::this_thread::yield();

  }

}

} // daq