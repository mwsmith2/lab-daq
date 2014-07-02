#include "daq_worker_caen6742.hh"

namespace daq {

DaqWorkerCaen6742::DaqWorkerCaen6742(string name, string conf) : DaqWorkerBase<caen_6742>(name, conf)
{
  LoadConfig();

  work_thread_ = std::thread(&DaqWorkerCaen6742::WorkLoop, this);
}

void DaqWorkerCaen6742::LoadConfig()
{ 
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  int ret;
  uint msg = 0;

  // Open the device.
  ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, 0, 0, 0, &device_);
  cout << "device: " << device_ << endl;

  // Get the board info.
  ret = CAEN_DGTZ_GetInfo(device_, &board_info_);
  printf("\nFound caen %s.",board_info_.ModelName);
  printf("\tROC FPGA Release is %s\n", board_info_.ROC_FirmwareRel);
  printf("\tAMC FPGA Release is %s\n", board_info_.AMC_FirmwareRel);

  // Reset the device.
  ret = CAEN_DGTZ_Reset(device_);

  // Set the trace length.
  ret = CAEN_DGTZ_SetRecordLength(device_, CAEN_6742_LN);
  
  // Set the channel enable mask.
  ret = CAEN_DGTZ_SetChannelEnableMask(device_, 0xFFFF); // all on
 
  // Enable external trigger.
  ret = CAEN_DGTZ_SetExtTriggerInputMode(device_, CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT); // ext in/out

  ret = CAEN_DGTZ_SetMaxNumEventsBLT(device_, 1);
  
  ret = CAEN_DGTZ_MallocReadoutBuffer(device_, &buffer_, &size_);
  
  // Start acquisition.
  ret = CAEN_DGTZ_SWStartAcquisition(device_);
  
} // LoadConfig

void DaqWorkerCaen6742::WorkLoop()
{
  while (thread_live_) {

    t0_ = high_resolution_clock::now();

    while (go_time_) {

      if (EventAvailable()) {

        static caen_6742 bundle;
        GetEvent(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        has_event_ = true;
        queue_mutex_.unlock();
      }

      std::this_thread::yield();

      usleep(daq::short_sleep);
    }

    std::this_thread::yield();

    usleep(daq::long_sleep);
  }
}

caen_6742 DaqWorkerCaen6742::PopEvent()
{
  queue_mutex_.lock();

  // Copy the data.
  caen_6742 data = data_queue_.front();
  data_queue_.pop();

  // Check if this is that last event.
  if (data_queue_.size() == 0) has_event_ = false;

  queue_mutex_.unlock();
  return data;
}


bool DaqWorkerCaen6742::EventAvailable()
{
  // Check acq reg.
  uint num_events = 0;

  CAEN_DGTZ_ReadData(device_, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer_, &bsize_);

  CAEN_DGTZ_GetNumEvents(device_, buffer_, bsize_, &num_events);

  if (num_events > 0) {
    return true;

  } else {
    return false;

  }
}

void DaqWorkerCaen6742::GetEvent(caen_6742 &bundle)
{
  int ch, offset, ret = 0;
  char *evtptr = nullptr;

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();  
  
  ret = CAEN_DGTZ_GetEventInfo(device_, buffer_, bsize_, 0, &event_info_, &evtptr);
  ret = CAEN_DGTZ_DecodeEvent(device_, evtptr, (void **)&event_);

  for (uint ch = 0; ch < CAEN_6742_CH; ++ch) {
    for (uint idx = 0; idx < event_->ChSize[ch]; ++idx) {
      bundle.trace[ch][idx] = event_->DataChannel[ch][idx];
    }
  }

  ret = CAEN_DGTZ_FreeEvent(device_, (void **)&event_);
}

} // ::daq
