#include "daq_writer_root.hh"

namespace daq {

DaqWriterRoot::DaqWriterRoot(string conf_file) : DaqWriterBase(conf_file)
{
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

    static int count = 0;

    root_data_.fake.resize(count + 1);

    sprintf(br_vars, "timestamp[%i]/l:trace[%i][%i]/s", 
      SIS_3350_CH, SIS_3350_CH, SIS_3350_LN);

    pt_->Branch(br_name.c_str(), &root_data_.fake[count++], br_vars);

  }    

}

void DaqWriterRoot::StopWriter()
{
  pf_->Write();
  pf_->Close();
  delete pf_;
}

void DaqWriterRoot::PullData(vector<event_data> data_buffer)
{
  for (auto it = data_buffer.begin(); it != data_buffer.end(); ++it) {

    memcpy(&root_data_, &(*it), sizeof(*it));

    pt_->Fill();

  }
}

} // ::daq