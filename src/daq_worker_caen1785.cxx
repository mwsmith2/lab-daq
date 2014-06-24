#include "daq_worker_caen1785.hh"

namespace daq {

DaqWorkerCaen1785::DaqWorkerCaen1785(string name, string conf) : DaqWorkerVme<caen_1785>(name, conf)
{
  LoadConfig();

  num_ch_ = CAEN_1785_CH;
  len_tr_ = 1;

  work_thread_ = std::thread(&DaqWorkerCaen1785::WorkLoop, this);
}

void DaqWorkerCaen1785::LoadConfig()
{ 
  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  // Get the device filestream
  queue_mutex_.lock();
  if (vme::device == -1) {

    string dev_path = conf.get<string>("device");
    if ((vme::device = open(dev_path.c_str(), O_RDWR | O_NONBLOCK, 0)) < 0) {
      cerr << "Open vme device." << endl;
    }
  }
  cout << "device: " << vme::device << endl;
  queue_mutex_.unlock();

  // Get the base address for the device.  Convert from hex.
  base_address_ = std::stoi(conf.get<string>("base_address"), nullptr, 0);
  
  int ret;
  uint msg = 0;

  Read(base_address_, msg);
  printf("caen1785 found at 0x%08x\n", base_address_);

  // Reset the data on the device.
  ClearData();
  printf("Device data cleared.\n");

} // LoadConfig

void DaqWorkerCaen1785::WorkLoop()
{
  while (true) {

    while (go_time_) {

      if (EventAvailable()) {

        static caen_1785 bundle;
        GetEvent(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        has_event_ = true;
        queue_mutex_.unlock();
      }

      std::this_thread::yield();

      usleep(100);
    }

    std::this_thread::yield();

    usleep(100);
  }
}

caen_1785 DaqWorkerCaen1785::PopEvent()
{
  // Copy the data.
  caen_1785 data = data_queue_.front();

  queue_mutex_.lock();
  data_queue_.pop();
  queue_mutex_.unlock();

  // Check if this is that last event.
  if (data_queue_.size() == 0) has_event_ = false;

  return data;
}


bool DaqWorkerCaen1785::EventAvailable()
{
  // Check acq reg.
  static ushort msg_ushort= 0;
  static uint msg;
  static bool is_event;

  // Check if the device is busy.
  Read16(0x100E, msg_ushort);
  is_event = !(msg_ushort & 0x4);

  // Check to make sure the buffer isn't empty.
  Read(0x0, msg);
  is_event &= ((msg & 0x07000000) == 0x06000000);

  return is_event;
}

void DaqWorkerCaen1785::GetEvent(caen_1785 &bundle)
{
  int offset;
  uint ch, data;

  // Get the event header for each high or low value
  for (ch = 0; ch < CAEN_1785_CH; ++ch) {
    
    offset = 0x0 + 4 * ch;
    Read(offset, data);

    bundle.timestamp[ch] = 21;
    bundle.value[ch] = (data & 0x7ff);
  }
}

} // ::daq
