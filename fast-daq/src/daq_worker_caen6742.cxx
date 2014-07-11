#include "daq_worker_caen6742.hh"

namespace daq {

DaqWorkerCaen6742::DaqWorkerCaen6742(string name, string conf) : DaqWorkerBase<caen_6742>(name, conf)
{
  buffer_ = nullptr;
  event_ = nullptr;

  LoadConfig();
}

DaqWorkerCaen6742::~DaqWorkerCaen6742()
{
  thread_live_ = false;
  if (work_thread_.joinable()) {
    work_thread_.join();
  }

  CAEN_DGTZ_FreeReadoutBuffer(&buffer_);
  CAEN_DGTZ_CloseDigitizer(device_);
}

void DaqWorkerCaen6742::LoadConfig()
{ 
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  int ret;
  uint msg = 0;

  int device_id = conf.get<int>("device_id");

  // Open the device.
  ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, device_id, 0, 0, &device_);
  cout << "device: " << device_ << endl;

  // Reset the device.
  ret = CAEN_DGTZ_Reset(device_);

  // Get the board info.
  ret = CAEN_DGTZ_GetInfo(device_, &board_info_);
  printf("\nFound caen %s.\n", board_info_.ModelName);
  printf("\tROC FPGA Release is %s\n", board_info_.ROC_FirmwareRel);
  printf("\tAMC FPGA Release is %s\n", board_info_.AMC_FirmwareRel);
  
  // Set the trace length.
  ret = CAEN_DGTZ_SetRecordLength(device_, CAEN_6742_LN);

  // Set "pretrigger".
  ret = CAEN_DGTZ_SetPostTriggerSize(device_, 50);

  // Set the sampling rate.
  ret = CAEN_DGTZ_SetDRS4SamplingFrequency(device_, CAEN_DGTZ_DRS4_1GHz);

  if (conf.get<bool>("use_drs4_corrections")) {
    // Load and enable DRS4 corrections.
    ret = CAEN_DGTZ_LoadDRS4CorrectionData(device_, CAEN_DGTZ_DRS4_1GHz);
    ret = CAEN_DGTZ_EnableDRS4Correction(device_);
  }

  // Set the channel enable mask.
  ret = CAEN_DGTZ_SetGroupEnableMask(device_, 0x3); // all on

  uint ch = 0;
  //DAC offsets
  for (auto &val : conf.get_child("channel_offset")) {

    float volts = val.second.get_value<float>();
    int dac = (int)((volts / vpp_) * 0xffff + 0x8000);

    if (dac < 0x0) dac = 0;
    if (dac > 0xffff) dac = 0xffff;
    
    CAEN_DGTZ_SetChannelDCOffset(device_, ch++, dac);
  }    

  // Enable external trigger.
  ret = CAEN_DGTZ_SetExtTriggerInputMode(device_, CAEN_DGTZ_TRGMODE_ACQ_ONLY);

  // Set the acquisition mode.
  ret = CAEN_DGTZ_SetAcquisitionMode(device_, CAEN_DGTZ_SW_CONTROLLED);

  // Set max events to 1 our purposes.
  ret = CAEN_DGTZ_SetMaxNumEventsBLT(device_, 1);
  
  // Allocated the readout buffer.
  ret = CAEN_DGTZ_MallocReadoutBuffer(device_, &buffer_, &size_);
  
} // LoadConfig

void DaqWorkerCaen6742::WorkLoop()
{
  int ret = CAEN_DGTZ_AllocateEvent(device_, (void **)&event_);
  ret = CAEN_DGTZ_SWStartAcquisition(device_);

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

  ret = CAEN_DGTZ_SWStopAcquisition(device_);
  ret = CAEN_DGTZ_FreeEvent(device_, (void **)&event_);
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

  // 
  ret = CAEN_DGTZ_GetEventInfo(device_, buffer_, bsize_, 0, &event_info_, &evtptr);

  ret = CAEN_DGTZ_DecodeEvent(device_, evtptr, (void **)&event_);

  int gr, idx, ch_idx;
  for (gr = 0; gr < CAEN_6742_GR; ++gr) {
    for (ch = 0; ch < CAEN_6742_CH / CAEN_6742_GR; ++ch) {
      for (idx = 0; idx < event_->DataGroup[gr].ChSize[ch]; ++idx) {
	ch_idx = ch + gr * (CAEN_6742_CH / CAEN_6742_GR);
	bundle.trace[ch_idx][idx] = 
	  event_->DataGroup[gr].DataChannel[ch][idx];
      }
    }
  }
}
  
} // ::daq
