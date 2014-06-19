#ifndef SLAC_DAQ_INCLUDE_ROOT_WRITER_HH_
#define SLAC_DAQ_INCLUDE_ROOT_WRITER_HH_

//--- std includes ----------------------------------------------------------//

//--- other includes --------------------------------------------------------//
#include "TFile.h"
#include "TTree.h"

//--- project includes ------------------------------------------------------//
#include "daq_structs.hh"

namespace daq {

// A class that interfaces with the an EventBuilder and writes a root file.

class RootWriter {

  public:

    //ctor
    RootWriter(string conf_file);

    // Member Functions
    void LoadConfig();

    
  private:


};

} // ::daq