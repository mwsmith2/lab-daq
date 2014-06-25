#include "daq_writer_root.hh"

namespace daq {

DaqWriterRoot::DaqWriterRoot(string conf_file) : DaqWriterBase(conf_file)
{
  end_of_batch_ = false;
  LoadConfig();
}

void DaqWriterRoot::LoadConfig()
{
  ptree conf;
  read_json(conf_file_, conf);

  outfile_ = conf.get<string>("writers.root.file", "default.root");
  tree_name_ = conf.get<string>("writers.root.tree", "t");
}

void DaqWriterRoot::StartWriter()
{
  // Allocate ROOT files
  pf_ = new TFile(outfile_.c_str(), "RECREATE");
  pt_ = new TTree(tree_name_.c_str(), tree_name_.c_str());

  // Need to get tree names out of the config file
  ptree conf;
  read_json(conf_file_, conf);

  // For each different device we need to loop and assign branches.
  string br_name;
  char br_vars[100];

  BOOST_FOREACH(const ptree::value_type &v, conf.get_child("devices.fake")) {

    int count = 0;
    root_data_.fake.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s", 
      SIS_3350_CH, SIS_3350_CH, SIS_3350_LN);

    pt_->Branch(br_name.c_str(), &root_data_.fake[count++], br_vars);

  }

  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3350")) {

    int count = 0;
    root_data_.sis_fast.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s", 
      SIS_3350_CH, SIS_3350_CH, SIS_3350_LN);

    pt_->Branch(br_name.c_str(), &root_data_.sis_fast[count++], br_vars);

  }

  BOOST_FOREACH(const ptree::value_type &v, 
                conf.get_child("devices.sis_3302")) {

    int count = 0;
    root_data_.sis_slow.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s", 
      SIS_3302_CH, SIS_3302_CH, SIS_3302_LN);

    pt_->Branch(br_name.c_str(), &root_data_.sis_slow[count++], br_vars);

  }

    BOOST_FOREACH(const ptree::value_type &v, 
                  conf.get_child("devices.caen_1785")) {

    int count = 0;
    root_data_.caen_adc.resize(count + 1);

    br_name = string(v.first);
    sprintf(br_vars, "system_clock/l:device_clock[%i]/l:values[%i]/s", 
      CAEN_1785_CH, CAEN_1785_CH);

    pt_->Branch(br_name.c_str(), &root_data_.caen_adc[count++], br_vars);

  }
}

void DaqWriterRoot::StopWriter()
{
  pf_->Write();
  pf_->Close();
  delete pf_;
}

void DaqWriterRoot::PushData(const vector<event_data> &data_buffer)
{
  for (auto it = data_buffer.begin(); it != data_buffer.end(); ++it) {

    root_data_ = *it;

    pt_->Fill();

  }
}

void DaqWriterRoot::EndOfBatch(bool bad_data)
{
  
}

} // ::daq
