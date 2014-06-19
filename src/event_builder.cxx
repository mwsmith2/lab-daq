#include "event_builder.hh"

namespace daq {

EventBuilder::EventBuilder(const vector<worker_ptr_types>& daq_workers, 
                           const vector<DaqWriterBase *> daq_writers)
{
  daq_workers_ = daq_workers;
  daq_writers_ = daq_writers;
  go_time_ = false;
  push_new_data_ = false;

  LoadConfig();

  builder_thread_ = std::thread(&EventBuilder::BuilderLoop, this);
  push_data_thread_ = std::thread(&EventBuilder::PushDataLoop, this);
}

void EventBuilder::LoadConfig()
{
//  boost::property_tree::ptree conf;
//  boost::property_tree::read_json(conf_file_, conf);
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
      bundle.sis_slow.push_back(ptr->PopEvent());

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
        pull_data_que_.push(bundle);
        queue_mutex_.unlock();

        cout << "Data queue is now size: ";
        cout << pull_data_que_.size() << endl;

        if (pull_data_que_.size() > max_queue_length_) {
          push_new_data_ = true;
        }

      } 

      std::this_thread::yield();
      usleep(100);

    }
  }
}

void EventBuilder::PushDataLoop()
{
  while (true) {

    while (go_time_) {

      if (push_new_data_) {

        cout << "Pushing data." << endl;

        push_data_vec_.resize(0);

        for (int i = 0; i < max_queue_length_; ++i) {

          cout << "Size of pull queue is: " << pull_data_que_.size() << endl;

          queue_mutex_.lock();
          push_data_vec_.push_back(pull_data_que_.front());
          pull_data_que_.pop();
          queue_mutex_.unlock();

        }

        push_new_data_ = false;

        

        for (auto it = daq_writers_.begin(); it != daq_writers_.end(); ++it) {

          (*it)->PushData(push_data_vec_);

        }
      }

      std::this_thread::yield();
      usleep(100);

    }
  }
}







} // ::daq