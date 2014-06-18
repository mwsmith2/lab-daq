#include "event_builder.hh"

namespace daq {

EventBuilder::EventBuilder(string conf_file, const vector<worker_ptr_types>& daq_workers)
{
  Init(conf_file, daq_workers);
}

void EventBuilder::Init(string conf_file, const vector<worker_ptr_types>& daq_workers)
{
  conf_file_ = conf_file;
  daq_workers_ = daq_workers;
  go_time_ = false;

  LoadConfig();

  builder_thread_ = std::thread(&EventBuilder::BuilderLoop, this);
}

void EventBuilder::LoadConfig()
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);


}

bool EventBuilder::WorkersHaveEvents()
{
  bool all_have_events = true;

  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      auto ptr = boost::get<DaqWorkerBase<sis_3350> *>(*it);
      all_have_events &= ptr->HasEvent();

    } else if ((*it).which() == 1) {

      auto ptr = boost::get<DaqWorkerBase<sis_3302> *>(*it);
      all_have_events &= ptr->HasEvent();

    }
  }

  return all_have_events;
}

void EventBuilder::GetEventData(event_data& bundle)
{
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      auto ptr = boost::get<DaqWorkerBase<sis_3350> *>(*it);
      bundle.fake.push_back(ptr->PopEvent());

    } else if ((*it).which() == 1) {

      auto ptr = boost::get<DaqWorkerBase<sis_3302> *>(*it);
      bundle.sis_s.push_back(ptr->PopEvent());

    }
  }
}

void EventBuilder::BuilderLoop()
{
  while (true) {

    while (go_time_) {

      if (WorkersHaveEvents()){

        event_data bundle;
        GetEventData(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        queue_mutex_.unlock();

        std::cout << "Data queue is now size: ";
        std::cout << data_queue_.size() << std::endl;

      } else {

        usleep(100);

      }

      std::this_thread::yield();

    }
  }
}

} // ::daq