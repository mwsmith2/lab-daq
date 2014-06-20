#include "daq_worker_sis3350.hh"

namespace daq {

DaqWorkerSis3350::DaqWorkerSis3350(string name, string conf) : DaqWorkerBase<event_struct>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3350_CH;
  len_tr_ = SIS_3350_LN;

  work_thread_ = std::thread(&DaqWorkerSis3350::WorkLoop, this);
}

void DaqWorkerSis3350::LoadConfig()
{
  //stub
}

void DaqWorkerSis3350::WorkLoop()
{
  //stub
}

event_struct DaqWorkerSis3350::PopEvent()
{
  event_struct sis;
  return sis;
}

} // ::daq