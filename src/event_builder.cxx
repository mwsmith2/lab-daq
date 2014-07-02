#include "event_builder.hh"

namespace daq {

EventBuilder::EventBuilder(const DaqWorkerList &daq_workers, 
                           const vector<DaqWriterBase *> daq_writers,
                           string conf_file)
{
  daq_workers_ = daq_workers;
  daq_writers_ = daq_writers;
  conf_file_ = conf_file;

  LoadConfig();

  builder_thread_ = std::thread(&EventBuilder::BuilderLoop, this);
  push_data_thread_ = std::thread(&EventBuilder::PushDataLoop, this);
}

void EventBuilder::LoadConfig()
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  thread_live_ = true;
  go_time_ = false;
  flush_time_ = false;
  push_new_data_ = false; 
  got_last_event_ = false;

  live_time = conf.get<int>("trigger_control.live_time");
  dead_time = conf.get<int>("trigger_control.dead_time");

  live_ticks = live_time * CLOCKS_PER_SEC;
  batch_start = clock();
}

void EventBuilder::BuilderLoop()
{
  while (thread_live_) {

    batch_start = clock();

    while (go_time_) {

      flush_time_ = (clock() - batch_start) > live_ticks;

      if (daq_workers_.AllWorkersHaveEvent()){

        event_data bundle;
        daq_workers_.GetEventData(bundle);

        queue_mutex_.lock();
        pull_data_que_.push(bundle);
        queue_mutex_.unlock();

        cout << "Data queue is now size: ";
        cout << pull_data_que_.size() << endl;

        if (pull_data_que_.size() >= kMaxQueueLength) {
          push_new_data_ = true;
        }

        if (flush_time_) {

          got_last_event_ = true;
          StopWorkers();
        }
      } 

      std::this_thread::yield();

      usleep(daq::short_sleep);
    }

    std::this_thread::yield();

    usleep(daq::long_sleep);
  }
}

void EventBuilder::PushDataLoop()
{
  while (thread_live_) {

    while (go_time_) {

      if (push_new_data_) {

        cout << "Pushing data." << endl;

        push_data_mutex_.lock();
        push_data_vec_.resize(0);

        for (int i = 0; i < kMaxQueueLength; ++i) {

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
        push_data_mutex_.unlock();
      }

      if (flush_time_ && got_last_event_) {

        cout << "Pushing end of batch." << endl;

        push_data_mutex_.lock();
        push_data_vec_.resize(0);

        queue_mutex_.lock();
        while (pull_data_que_.size() != 0) {

          cout << "Size of pull queue is: " << pull_data_que_.size() << endl;

          push_data_vec_.push_back(pull_data_que_.front());
          pull_data_que_.pop();

        }
        queue_mutex_.unlock();

        push_new_data_ = false;

        // Check to make sure there are no straggling events.
        bool bad_data = daq_workers_.AnyWorkersHaveEvent();
        daq_workers_.FlushEventData();

        for (auto it = daq_writers_.begin(); it != daq_writers_.end(); ++it) {

          (*it)->PushData(push_data_vec_);
          (*it)->EndOfBatch(bad_data);

        }
        push_data_mutex_.unlock();

        flush_time_ = false;
        got_last_event_ = false;

        sleep(dead_time);

        // Start thw workers and make sure they start in sync.
        StartWorkers();
        while (!daq_workers_ .AnyWorkersHaveEvent()) {
          daq_workers_.FlushEventData();
        }
      }

      std::this_thread::yield();

      usleep(daq::short_sleep);
    }

    std::this_thread::yield();

    usleep(daq::long_sleep);
  }
}

// Start the workers taking data.
void EventBuilder::StartWorkers() {
  cout << "Event Builder is starting workers." << endl;
  daq_workers_.StartWorkers();
}

// Write the data file and reset workers.
void EventBuilder::StopWorkers() {
  cout << "Event Builder is stopping workers." << endl;
  daq_workers_.StopWorkers();
}

} // ::daq
