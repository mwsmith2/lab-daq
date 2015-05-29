#ifndef SLAC_DAC_SLOW_CONTROL_INCLUDE_ASYNC_ROOT_WRITER_HH_
#define SLAC_DAC_SLOW_CONTROL_INCLUDE_ASYNC_ROOT_WRITER_HH_

//--- std includes ----------------------------------------------------------//
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <ctime>
#include <vector>
using std::string;
using std::cout;
using std::endl;
using std::vector;

//--- other includes --------------------------------------------------------//
#include "TFile.h"
#include "TTree.h"

//--- project includes ------------------------------------------------------//

struct tm_abr {
  int sec;
  int min;
  int hour;
  int mday;
  int mon;
  int year;
  int wday;
  int yday;
};

namespace sc {

class AsyncRootWriter {

  public:

    // ctor
    AsyncRootWriter();
    AsyncRootWriter(string filename);

    // dtor
    ~AsyncRootWriter();

    // Member functions
    int PushData(const string &data);
    void CreateTree(const string &setup);
    void WriteFile();

  private:

    tm_abr time_info_; // time struct

    // data structure variables
    TFile *pf_;
    vector<TTree *> pt_vec_;
    std::map<string, int> name_map_;
    std::map<string, int> key_map_;
    vector<std::map<string, double *>> data_map_vec_;

    void UpdateTime();
};

} // ::sc

#endif


