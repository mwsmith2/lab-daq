#include "daq_worker_sis3302.hh"

namespace daq {

DaqWorkerSis3302::DaqWorkerSis3302(string name, string conf) : DaqWorkerVme<sis_3302>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3302_CH;
  len_tr_ = SIS_3302_LN;

  work_thread_ = std::thread(&DaqWorkerSis3302::WorkLoop, this);
}

void DaqWorkerSis3302::LoadConfig()
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

  // Read the base register.
  Read(0x0, msg);
  printf("sis3302 found at 0x%08x\n", base_address_);

  // Reset the device.
  msg = 1;
  if ((ret = Write(0x400, msg)) != 0) {
    printf("error writing sis3302 reset register\n");
  }

  // Get device ID.
  msg = 0;
  Read(0x4, msg);
  printf("sis3302 ID: %04x, maj rev: %02x, min rev: %02x\n",
   msg >> 16, (msg >> 8) & 0xff, msg & 0xff);

  // Read control/status register.
  msg = 0;

  if (conf.get<bool>("user_led_on", true))
    msg |= 0x1; // LED on

  msg |= 0xfffe0000; // zero reserved bits

  Write(0x0, msg);

  // Set and check the control/status register.
  msg = 0;

  if (conf.get<bool>("invert_ext_lemo")) {
    msg |= 0x10; // invert EXT TRIG
  }

  if (conf.get<bool>("user_led_on")) {
    msg |= 0x1; // LED on
  }

  Write(0x0, msg);
  
  msg = 0;
  Read(0x0, msg);
  printf("user LED turned %s\n", (msg & 0x1) ? "ON" : "OFF");

  // Set Acquisition register.
  msg = 0;
  if (conf.get<bool>("enable_int_stop", true))
    msg |= 0x1 << 6; //enable internal stop trigger

  if (conf.get<bool>("enable_ext_lemo", true))
    msg |= 0x1 << 8; //enable EXT LEMO

  // Set the clock source: 0x0 = Internal, 100MHz
  msg |= conf.get<int>("clock_settings", 0x0) << 12;
  msg &= 0x00007df0; // zero reserved bits / disable bits

  printf("sis 3302 setting acq reg to 0x%08x\n", msg);
  Write(0x10, msg);

  // Set the start delay.
  msg = conf.get<int>("start_delay", 0);
  Write(0x14, msg);

  // Set the stop delay.
  msg = conf.get<int>("stop_delay", 0);
  Write(0x18, msg);

  // Read event configure register.
  msg = 0;
  Read(0x02000000, msg);

  // Set event configure register with changes

  if (conf.get<bool>("enable_event_length_stop", true))
    msg = 0x1 << 5; // enable event length stop trigger

  Write(0x01000000, msg);
  
  // Event length register - odd setting method (see manual).
  msg = (SIS_3302_LN - 4) & 0xfffffc;
  Write(0x01000004, msg);

  // Memory page
  msg = 0; //first 8MB chunk
  Write(0x34, msg);

} // LoadConfig

void DaqWorkerSis3302::WorkLoop()
{
  while (true) {

    t0_ = high_resolution_clock::now();

    while (go_time_) {

      if (EventAvailable()) {

        static sis_3302 bundle;
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

sis_3302 DaqWorkerSis3302::PopEvent()
{
  // Copy the data.
  sis_3302 data = data_queue_.front();

  queue_mutex_.lock();
  data_queue_.pop();
  queue_mutex_.unlock();

  // Check if this is that last event.
  if (data_queue_.size() == 0) has_event_ = false;

  return data;
}


bool DaqWorkerSis3302::EventAvailable()
{
  // Check acq reg.
  static uint msg = 0;
  static bool is_event;

  Read(0x10, msg);
  is_event = !(msg & 0x10000);

  return is_event;
}

void DaqWorkerSis3302::GetEvent(sis_3302 &bundle)
{
  int ch, offset, ret = 0;

  // Check how long the event is.
  //expected SIS_3302_LN + 8
  
  uint next_sample_address[SIS_3302_CH];

  for (ch = 0; ch < SIS_3302_CH; ch++) {

    next_sample_address[ch] = 0;

    offset = 0x02000010;
    offset |= (ch >> 1) << 24;
    offset |= (ch & 0x1) << 2;

    Read(offset, next_sample_address[ch]);
  }

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<nanoseconds>(dtn).count();  

  //todo: check it has the expected length
  uint trace[SIS_3302_CH][SIS_3302_LN / 2 + 4];

  for (ch = 0; ch < SIS_3302_CH; ch++) {
    offset = (0x8 + ch) << 23;
    Read(0x10000, trace[ch][0]);
    Read(0x10001, trace[ch][1]);
    ReadTrace(offset, trace[ch] + 4);
  }

  //arm the logic
  uint armit = 1;
  Write(0x410, armit);

  //decode the event (little endian arch)
  for (ch = 0; ch < SIS_3302_CH; ch++) {

    bundle.device_clock[ch] = 0;
    bundle.device_clock[ch] = trace[ch][1] & 0xfff;
    bundle.device_clock[ch] |= (trace[ch][1] & 0xfff0000) >> 4;
    bundle.device_clock[ch] |= (trace[ch][0] & 0xfffULL) << 24;
    bundle.device_clock[ch] |= (trace[ch][0] & 0xfff0000ULL) << 20;

    uint idx;
    for (idx = 0; idx < SIS_3302_LN / 2; idx++) {
      bundle.trace[ch][2 * idx] = trace[ch][idx + 4] & 0xfff;
      bundle.trace[ch][2 * idx + 1] = (trace[ch][idx + 4] >> 16) & 0xfff;
    }
  }
}
//   int sis_idx = 0;
//   bundle.is_bad_event = 0;

//   if (sis_idx > 0) {
//     unsigned int status = 0;
  //   high_resolution_clock::time_point t1 = high_resolution_clock::now();
  //   do {
  //     int ret = 0;
  //     if ((ret = Read(0x10, &status)) != 0) {
  // printf("error reading sis3302 acq register\n");
  //     }

  //     high_resolution_clock::time_point t2 = high_resolution_clock::now();
  //     duration<double> time_span = duration_cast<duration<double>>(t2-t1);
  //     if (time_span.count() > 3e-3) {
  // printf("sis3302 readout timeout address: 0x%08x\n",  base_address_);
  // printf("time span: %lf\n", time_span.count());
  // break;
  //     }
  
  //   } while((status & 0x10000));
  
  //   // MWS - setting the high res system clock here
  //   high_resolution_clock::duration dtn = t1.time_since_epoch();
  //   mSystemTime = duration_cast<nanoseconds>(dtn).count();  

//     if (status & 0x10000) {
//       bundle.is_bad_event++;
//     }
//   }

//   int ch, offset, ret = 0;
//   //pull the event

//   //check how long the event is
//   //expected mTraceLength + 8
//   unsigned int next_sample_address[4] = {0, 0, 0, 0};
//   for (ch = 0; ch < 4; ch++) {
//     offset = 0x02000010;
//     offset |= (ch >> 1) << 24;
//     offset |= (ch & 0x1) << 2;

//     if ((ret = Read(offset, next_sample_address + ch)) != 0) {
//       printf("error sis3302 reading trace length\n");
//     }
//   }

//   //todo: check it has the expected length

//   unsigned int trace[4][mTraceLength / 2 + 4];

//   for (ch = 0; ch < 4; ch++) {
//     //offset = (0x4 + ch) << 24;
//     offset = 0x04000000 + 0x00800000 * ch;
//     unsigned int got_num_words;
//     //if ((ret = vme_A32MBLT64_read(mDevice,
//     if ((ret = ReadBlock(offset,
//        (unsigned int*)(trace[ch]),
//        mTraceLength / 2 + 4,
//        &got_num_words)) != 0)
//       {
//   printf("error sis3302 reading trace length\n");
//       }

//     /*
//       printf("adc%d: ", ch);
//       for(unsigned int sample = 0; sample < 8; sample++) {
//       printf("%08x ", trace[ch][sample]);
//       }
//       printf("\n");
//     */
//   }

//   //arm the logic
//   unsigned int armit = 1;
//   if ((ret = Write(0x410, armit)) != 0) {
//     printf("error sis3302 arming sampling logic sis %d\n", sis_idx);
//   }

//   //decode the event (little endian arch)
//   for (ch = 0; ch < 4; ch++) {
//     bundle.timestamp[sis_idx*4+ch] = 0;
//     bundle.timestamp[sis_idx*4+ch] = trace[ch][1] & 0xfff;
//     bundle.timestamp[sis_idx*4+ch] |= (trace[ch][1] & 0xfff0000) >> 4;
//     bundle.timestamp[sis_idx*4+ch] |= (trace[ch][0] & 0xfffULL) << 24;
//     bundle.timestamp[sis_idx*4+ch] |= (trace[ch][0] & 0xfff0000ULL) << 20;

//     unsigned int idx;
//     for (idx = 0; idx < mTraceLength / 2; idx++) {
//       bundle.trace[sis_idx*4+ch][2*idx] = trace[ch][idx + 4] & 0xffff;
//       bundle.trace[sis_idx*4+ch][2*idx + 1] = (trace[ch][idx + 4] >> 16) & 0xffff;
//     }
//   }

//   sis_idx++;
// }

} // ::daq
