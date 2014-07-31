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
  quitting_time_ = false;
  finished_run_ = false;

  batch_size_ = conf.get<int>("batch_size", 10);
  max_event_time_ = conf.get<int>("max_event_time_", 2000);

  live_time_ = conf.get<int>("trigger_control.live_time");
  dead_time_ = conf.get<int>("trigger_control.dead_time");
  live_ticks_ = live_time_ * CLOCKS_PER_SEC;
}

void EventBuilder::BuilderLoop()
{
  while (thread_live_) {

    batch_start_ = clock();

    while (go_time_ && !got_last_event_) {

      if (daq_workers_.AnyWorkersHaveEvent()) {
	
	usleep(max_event_time_);

	event_data bundle;
	daq_workers_.GetEventData(bundle);
	daq_workers_.FlushEventData();

	queue_mutex_.lock();
	pull_data_que_.push(bundle);
	queue_mutex_.unlock();

	if (pull_data_que_.size() >= batch_size_) {
	  push_new_data_ = true;
	}

	if (flush_time_) {
	  got_last_event_ = true;
	  StopWorkers();
	}
      }
    //   if (daq_workers_.AllWorkersHaveEvent()){
        if (pull_data_que_.size() >= batch_size_) {
          push_new_data_ = true;
        }

    // 	event_data bundle;
    //     daq_workers_.GetEventData(bundle);

    //     queue_mutex_.lock();
    //     pull_data_que_.push(bundle);
    //     queue_mutex_.unlock();

    //     cout << "Data queue is now size: ";
    //     cout << pull_data_que_.size() << endl;

    //     if (pull_data_que_.size() >= batch_size_) {
    //       push_new_data_ = true;
    //     }

    //     if (flush_time_) {

    // 	  StopWorkers();

    // 	  while (daq_workers_.AllWorkersHaveEvent()) {

    // 	    event_data bundle;
    // 	    daq_workers_.GetEventData(bundle);
	    
    // 	    queue_mutex_.lock();
    // 	    pull_data_que_.push(bundle);
    // 	    queue_mutex_.unlock();

    // 	    std::this_thread::yield();
    // 	    usleep(10);
    // 	  }

    //       got_last_event_ = true;
    //     }
    //   } 

	flush_time_ = (clock() - batch_start_) > live_ticks_;

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

        for (int i = 0; i < batch_size_; ++i) {

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

	  if (pull_data_que_.size() > 5 * batch_size_) {

	    for (auto it = daq_writers_.begin(); it != daq_writers_.end(); ++it) {

	      (*it)->PushData(push_data_vec_);

	    }

	    push_data_vec_.resize(0);
	  }
        }

        queue_mutex_.unlock();

        push_new_data_ = false;

        // Check to make sure there are no straggling events.
        bool bad_data = daq_workers_.AnyWorkersHaveEvent();

	// Start them back up so that we don't get a backlog of events.
	daq_workers_.FlushEventData();
        daq_workers_.StartWorkers();

        for (auto it = daq_writers_.begin(); it != daq_writers_.end(); ++it) {

          (*it)->PushData(push_data_vec_);
          (*it)->EndOfBatch(bad_data);

        }
        push_data_mutex_.unlock();

        flush_time_ = false;
        got_last_event_ = false;

	  if (quitting_time_ == true) {
	    go_time_ = false;
	  }

        sleep(dead_time_);
	
	daq_workers_.FlushEventData();
        // Start the workers and make sure they start in sync.
        while (!daq_workers_ .AnyWorkersHaveEvent()) {
          daq_workers_.FlushEventData();
        }
	
	batch_start_ = clock();
      }

      if (quitting_time_) {
	
	// First flush all the remaining events.
	flush_time_ = true;

	// Wait for the last event
	while (!got_last_event_);
	
        push_data_mutex_.lock();
        push_data_vec_.resize(0);

        queue_mutex_.lock();
        while (pull_data_que_.size() != 0) {

          cout << "Size of pull queue is: " << pull_data_que_.size() << endl;

          push_data_vec_.push_back(pull_data_que_.front());
          pull_data_que_.pop();

	  if (pull_data_que_.size() > 5 * batch_size_) {

	    for (auto it = daq_writers_.begin(); it != daq_writers_.end(); ++it) {
	      (*it)->PushData(push_data_vec_);

	    }
	    push_data_vec_.resize(0);
	  }
        }

        queue_mutex_.unlock();

        push_new_data_ = false;

        // Check to make sure there are no straggling events.
        bool bad_data = daq_workers_.AnyWorkersHaveEvent();
        daq_workers_.FlushEventData();

	cout << "Sending end of batch/run to the writers." << endl;
        for (auto it = daq_writers_.begin(); it != daq_writers_.end(); ++it) {

          (*it)->PushData(push_data_vec_);
	  (*it)->EndOfBatch(bad_data);

        }
        push_data_mutex_.unlock();
	
	go_time_ = false;
	thread_live_ = false;
	finished_run_ = true;
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
