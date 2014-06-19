#ifndef SLAC_DAQ_INCLUDE_DATA_WRITER_ROOT_HH_
#define SLAC_DAQ_INCLUDE_DATA_WRITER_ROOT_HH_

//--- std includes ----------------------------------------------------------//

//--- other includes --------------------------------------------------------//
#include "TFile.h"
#include "TTree.h"

//--- project includes ------------------------------------------------------//
#include "daq_structs.hh"

namespace daq {

// A class that interfaces with the an EventBuilder and writes a root file.

class DataWriterRoot : DataWriterBase {

  public:

    //ctor
    DataWriterRoot(string conf_file);

    // Member Functions
    void LoadConfig();
    void StartWriter();
    void StopWriter();

    void PullData(vector<event_data> data_buffer);

  private:


};

} // ::daq