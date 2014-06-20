#include "daq_worker_sis3350.hh"

namespace daq {

DaqWorkerSis3350::DaqWorkerSis3350(string name, string conf) : DaqWorkerBase<event_struct>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3350_CH;
  len_tr_ = SIS_3350_LN;

  work_thread_ = std::thread(&DaqWorkerSis3350::WorkLoop, this);
}

int DaqWorkerSis3350::Read(int addr, int &msg)
{
  int ret;
  int stat;

  stat = (ret = vme_A32D32_read(device_, base_address_ + addr, &msg));
  if (stat != 0) {
    cerr << "Address not found." << endl;
  }

  return ret;
}

int DaqWorkerSis3350::Write(int addr, int &msg)
{
  
}

void DaqWorkerSis3350::LoadConfig()
{ 
  boost::property_tree:ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  if ((device_ = open(conf.get<string>("device"), O_RDWR, 0) < 0) {
      cerr << "Open vme device." << endl;
      return -1;
  }
  
  base_address_ = conf.get<int>("base_address");

  int ret;
  unsigned int msg = 0;

  ret = Read(0x0, msg);
  cout << "sis3350 found at 0x%08x\n" << base_address_ << endl;

  //reset
  status = 1;
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x400, status)) != 0) {
    printf("error writing sis3350 reset register\n");
  }

  //get ID
  status = 0;
  if ((ret = vme_A32D32_read(vme_dev, sis3350_base_address + 0x4, &status)) != 0) {
    printf("error reading sis3350 ID\n");
  }
  printf("sis3350 ID: %04x, maj rev: %02x, min rev: %02x\n",
	 status >> 16, (status >> 8) & 0xff, status & 0xff);

  //control/status register
  status = 0;
  //status |= 0x10; // invert EXT TRIG
  status |= 0x1; // LED on
  status = ((~status & 0xffff) << 16) | status; // j/k
  status &= 0x00110011; // zero reserved bits

  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address, status)) != 0) {
    printf("error writing sis3350 status register\n");
  }

  status = 0;
  if ((ret = vme_A32D32_read(vme_dev, sis3350_base_address, &status)) != 0) {
    printf("error reading sis3350 status register\n");
  }

  printf("EXT LEMO set to %s\n", ((status & 0x10) == 0x10)?"NIM":"TTL");
  printf("user LED turned %s\n", (status & 0x1)?"ON":"OFF");

  //acquisition register
  status = 0;
  status |= 0x1; //sync ring buffer mode
  //status |= 0x1 << 5; //enable multi mode
  //status |= 0x1 << 6; //enable internal (channel) triggers
  status |= 0x1 << 8; //enable EXT LEMO
  //status |= 0x1 << 9; //enable EXT LVDS
  //status |= 0x0 << 12; // clock source: synthesizer

  status = ((~status & 0xffff) << 16) | status; // j/k
  status &= ~0xcc98cc98; //reserved bits

  printf("sis 3350 setting acq reg to 0x%08x\n", status);

  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x10, status)) != 0) {
    printf("error writing sis3350 acq register\n");
  }

  status = 0;
  if ((ret = vme_A32D32_read(vme_dev, sis3350_base_address + 0x10, &status)) != 0) {
    printf("error reading sis3350 acq register\n");
  }
  printf("sis 3350 acq reg reads 0x%08x\n", status);

  //synthesizer reg
  status = 0x14; //500 MHz
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x1c, status)) != 0) {
    printf("error writing sis3350 freq synthesizer register\n");
  }

  //memory page
  status = 0; //first 8MB chunk
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x34, status)) != 0) {
    printf("error writing sis3350 freq synthesizer register\n");
  }

  //trigger output
  status = 0;
  status |= 0x1; //LEMO IN -> LEMO OUT
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x38, status)) != 0) {
    printf("error writing sis3350 freq synthesizer register\n");
  }

  //ext trigger threshold
  //first load data, then clock in, then ramp
  status = 37500; // +1.45V TTL
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x54, status)) != 0) {
    printf("error writing sis3350 ext trg data register\n");
  }

  status = 0;
  status |= 0x1 << 4; //TRG threshold
  status |= 0x1; //load shift register DAC
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x50, status)) != 0) {
    printf("error writing sis3350 ext trg ctrl register\n");
  }

  unsigned int timeout_max = 1000;
  unsigned int timeout_cnt = 0;
  do {
    status = 0;
    if ((ret = vme_A32D32_read(vme_dev, sis3350_base_address + 0x50, &status)) != 0) {
      printf("error reading sis3350 ext trg ctrl register\n");
    }
    timeout_cnt++;
  } while (((status & 0x8000) == 0x8000) && (timeout_cnt < timeout_max));

  if (timeout_cnt == timeout_max) {
    printf("error loading ext trg shift reg\n");
  }

  status = 0;
  status |= 0x1 << 4; //TRG threshold
  status |= 0x2; //load DAC
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x50, status)) != 0) {
    printf("error writing sis3350 ext trg ctrl register\n");
  }

  timeout_cnt = 0;
  do {
    status = 0;
    if ((ret = vme_A32D32_read(vme_dev, sis3350_base_address + 0x50, &status)) != 0) {
      printf("error reading sis3350 ext trg ctrl register\n");
    }
    timeout_cnt++;
  } while (((status & 0x8000) == 0x8000) && (timeout_cnt < timeout_max));

  if (timeout_cnt == timeout_max) {
    printf("error loading ext trg dac\n");
  }

  //board temperature
  status = 0;
  if ((ret = vme_A32D32_read(vme_dev, sis3350_base_address + 0x70, &status)) != 0) {
    printf("error reading sis3350 temperature register\n");
  }
  printf("sis3350 board temperature: %.2f degC\n", (float)status / 4.0);

  //ring buffer sample length
  status = sis3350_trace_length;
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x01000020, status)) != 0) {
    printf("error writing sis3350 ringbuffer sample length register\n");
  }

  //ring buffer pre-trigger sample length
  status = 0x100;
  if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x01000024, status)) != 0) {
    printf("error writing sis3350 ringbuffer sample length register\n");
  }

  //range -1.5 to +0.3 V
  unsigned int ch;
  //DAC offsets
  for (ch = 0; ch < 4; ch++) {
    int offset = 0x02000050;
    offset |= (ch >> 1) << 24;

    status = 39000; // ??? V
    if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + offset | 0x4, status)) != 0) {
      printf("error writing sis3350 ADC DAC data register\n");
    }

    status = 0;
    status |= (ch % 2) << 4;
    status |= 0x1; //load shift register DAC
    if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + offset, status)) != 0) {
      printf("error writing sis3350 adc dac ctrl register\n");
    }

    timeout_max = 1000;
    timeout_cnt = 0;
    do {
      status = 0;
      if ((ret = vme_A32D32_read(vme_dev, sis3350_base_address + offset, &status)) != 0) {
	printf("error reading sis3350 adc dac ctrl register\n");
      }
      timeout_cnt++;
    } while (((status & 0x8000) == 0x8000) && (timeout_cnt < timeout_max));

    if (timeout_cnt == timeout_max) {
      printf("error loading ext trg shift reg\n");
    }

    status = 0;
    status |= (ch % 2) << 4;
    status |= 0x2; //load DAC
    if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + offset, status)) != 0) {
      printf("error writing sis3350 ext trg ctrl register\n");
    }

    timeout_cnt = 0;
    do {
      status = 0;
      if ((ret = vme_A32D32_read(vme_dev, sis3350_base_address + offset, &status)) != 0) {
	printf("error reading sis3350 ext trg ctrl register\n");
      }
      timeout_cnt++;
    } while (((status & 0x8000) == 0x8000) && (timeout_cnt < timeout_max));

    if (timeout_cnt == timeout_max) {
      printf("error loading adc dac\n");
    }
  }

  //gain
  //factory default 18 -> 5V
  for (ch = 0; ch < 4; ch++) {
    status = 45;
    int offset = 0x02000048;
    offset |= (ch >> 1) << 24;
    offset |= (ch % 2) << 2;
    if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + offset, status)) != 0) {
      printf("error reading sis3350 adc gain register\n");
    }
    printf("adc %d gain %d\n", ch, status);
  }

  return SUCCESS;
}

  /*-- Frontend Exit -------------------------------------------------*/
  INT frontend_exit()
  {
    //disarm sampling logic
    unsigned int armit = 0;
    int ret = 0;
    if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x414, armit)) != 0) {
      printf("error sis3350 disarming sampling logic\n");
    }

    return SUCCESS;
  }

  /*-- Begin of Run --------------------------------------------------*/
  INT begin_of_run(INT run_number, char *error)
  {
    //HW part

    unsigned int armit = 1;
    int ret = 0;
    if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x410, armit)) != 0) {
      printf("error sis3350 arming sampling logic\n");
    }

    sis_que.clear();
    time_to_finish = false;
    has_event = false;
    sis_thread = std::thread(sis_worker);

    //DATA part

    HNDLE hDB, hkey;
    INT status;
    char str[256], filename[256];
    int size;

    cm_get_experiment_database(&hDB, NULL);
    db_find_key(hDB, 0, "/Logger/Data dir", &hkey);

    if (hkey) {
      size = sizeof(str);
      db_get_data(hDB, hkey, str, &size, TID_STRING);
      if (str[strlen(str) - 1] != DIR_SEPARATOR) {
	strcat(str, DIR_SEPARATOR_STR);
      }
    }

    db_find_key(hDB, 0, "/Runinfo", &hkey);
    if (db_open_record(hDB, hkey, &runinfo, sizeof(runinfo), MODE_READ,
		       NULL, NULL) != DB_SUCCESS) {
      cm_msg(MERROR, "analyzer_init", "Cannot open \"/Runinfo\" tree in ODB");
      return 0;
    }

    strcpy(filename, str);
    sprintf(str, "run_%05d.root", runinfo.run_number);
    strcat(filename, str);

    root_file = new TFile(filename, "recreate");
    t = new TTree("t", "t");
    char br_str[32];
    sprintf(br_str, "timestamp[4]/l:trace[4][%d]/s", sis3350_trace_length);
    t->Branch("sis", &br_sis, br_str);

    t->SetAutoSave(0);
    t->SetAutoFlush(0);
    //t->Print();

    run_in_progress = true;

    return SUCCESS;
  }

  /*-- End of Run ----------------------------------------------------*/
  INT end_of_run(INT run_number, char *error)
  {
    time_to_finish = true;
    sis_thread.join();

    //disarm sampling logic
    unsigned int armit = 0;
    int ret = 0;
    if ((ret = vme_A32D32_write(vme_dev, sis3350_base_address + 0x414, armit)) != 0) {
      printf("error sis3350 disarming sampling logic\n");
    }

    if (run_in_progress) {
      t->Write();
      root_file->Close();

      delete root_file;
      run_in_progress = false;
    }

    return SUCCESS;
  }

  /*-- Pause Run -----------------------------------------------------*/
  INT pause_run(INT run_number, char *error)
  {
    return SUCCESS;
  }

  /*-- Resuem Run ----------------------------------------------------*/
  INT resume_run(INT run_number, char *error)
  {
    return SUCCESS;
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
