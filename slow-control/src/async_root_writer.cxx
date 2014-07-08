#include "async_root_writer.hh"

namespace sc {

AsyncRootWriter::AsyncRootWriter()
{
  pf_ = new TFile("test.root", "recreate");

  UpdateTime();
}

AsyncRootWriter::~AsyncRootWriter()
{
  // Take care of root stuff.
  pf_->Close();

  // Take are of data.
  for (auto it = data_map_vec_.begin(); it != data_map_vec_.end(); ++it) {
    for (auto it_2 = (*it).begin(); it_2 != (*it).end(); ++it_2) {
      delete it_2->second;
    }
  }

  delete time_info_;
}

void AsyncRootWriter::CreateTree(const string &setup)
{
  string str; // used for utility

  // Turn the setup string into an easily parseable stream.
  std::istringstream ss(setup);

  // Now get the first line which is the device name.
  std::getline(ss, str, ':');

  // Check if it is already in use.
  if (name_map_.find(str) != name_map_.end()) {

    // The device name is taken.
    cout << "Device name " << str << " is already in use." << endl;

    // Find a new name.
    int count = 0;
    string dev_name(str);

    while (name_map_.find(dev_name) != name_map_.end()) {
      dev_name = str;
      dev_name.append("_");
      dev_name.append(std::to_string(count++));
    }

    str = dev_name;
    cout << "Using device name " << str << " instead." << endl;
  }

  // Add the tree to the tree name map.
  string tree_name = str;
  name_map_[tree_name] = pt_vec_.size();

  // Create the tree and grab a handy pointer to it.
  pt_vec_.push_back(new TTree(tree_name.c_str(), tree_name.c_str()));
  TTree *pt = pt_vec_[name_map_[tree_name]];

  std::map<string, double *> data_map;
  // Now create the branches.
  while (str != string("__EOM__")) {
    
    // Get the next variable.
    std::getline(ss, str, ':');

    // Allocate as a double (low data volumes should be fine).
    double *pdata = new double;
    data_map[str] = pdata;

    // Create the branch
    string branch_name(str);
    string branch_vars(str);
    branch_vars.append("/D");

    pt->Branch(branch_name.c_str(), 
               data_map[branch_name], 
               branch_vars.c_str());
  }

  string time_str("tm_sec/I:tm_min/I:tm_hour/I:tm_mday/I:");
  time_str.append("tm_mon/I:tm_year/I:tm_wday/I:tm_yday/I:tm_isdst/I");
  pt->Branch("time", &time_info_, time_str.c_str());

  data_map_vec_.push_back(data_map);  
  cout << "Device " << tree_name << " tree created successfully." << endl;
}

int AsyncRootWriter::PushData(const string &data)
{
  // Convert to a stringstream
  std::istringstream ss(data);
  string str, val;

  // Get the tree name and index.
  std::getline(ss, str, ':');

  // Make sure the tree exists.
  if (name_map_.find(str) == name_map_.end()) return 1;

  // Else grab the tree's data map.
  int tree_idx = name_map_[str];
  std::map<string, double *> data_map = data_map_vec_[tree_idx];

  while (str != string("__EOM__")) {

    // Get the variable/branch name.
    std::getline(ss, str, ':');

    // Get the data.
    std::getline(ss, val, ':');

    *data_map[str] = std::stod(val);
  }

  UpdateTime();
  pt_vec_[tree_idx]->Fill();

  return 0;
}

void AsyncRootWriter::WriteFile()
{
  pf_->Write();
}

void AsyncRootWriter::UpdateTime()
{
  time_t now;
  time(&now);
  time_info_ = localtime(&now);
}

} // ::sc