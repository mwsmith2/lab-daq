#include "daq_worker_list.hh"

namespace daq {

DaqWorkerList::DaqWorkerList()
{
  // stub  
}

DaqWorkerList::~DaqWorkerList()
{
  ClearList();
}

void DaqWorkerList::StartRun()
{
  StartWorkers();
}

void DaqWorkerList::StopRun()
{
  StopWorkers();
}

void DaqWorkerList::StartWorkers()
{
  // Start the data gatherers
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StartWorker();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StartWorker();

    } else if ((*it).which() == 2) {

      boost::get<DaqWorkerBase<caen_1785> *>(*it)->StartWorker();

    }
  }
}

void DaqWorkerList::StopWorkers()
{
  // Stop the data gathering
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      boost::get<DaqWorkerBase<sis_3350> *>(*it)->StopWorker();

    } else if ((*it).which() == 1) {

      boost::get<DaqWorkerBase<sis_3302> *>(*it)->StopWorker();

    } else if ((*it).which() == 2) {

      boost::get<DaqWorkerBase<caen_1785> *>(*it)->StopWorker();

    }
  }
}

void DaqWorkerList::ClearList()
{
  // Delete the allocated workers.
  for (auto it = daq_workers_.begin(); it != daq_workers_.end(); ++it) {

    if ((*it).which() == 0) {

      delete boost::get<DaqWorkerBase<sis_3350> *>(*it);

    } else if ((*it).which() == 1) {

      delete boost::get<DaqWorkerBase<sis_3302> *>(*it);

    } else if ((*it).which() == 2) {

      delete boost::get<DaqWorkerBase<caen_1785> *>(*it);

    }
  }
  daq_workers_.resize(0);
}

} // ::daq