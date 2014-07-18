// Matthias Smith - MIDAS Bridge for my frontend

#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <midas.h>
#include <msystem.h>
using std::cout;
using std::endl;

#include "frontend_rpc.h"

#include "daq_structs.hh"
#include <zmq.hpp>

/** 
 * Trigger information 
 */
static struct s_trigger_info
{
  DWORD trigger_nr;    ///< trigger number (from RPC message from masterMT)
  DWORD trigger_mask;  ///< trigger mask (from RPC message from masterMT)
  DWORD time_s;        ///< trigger time (from RPC message from masterMT), sec
  DWORD time_us;       ///< trigger time (from RPC message from master MT), usec
  DWORD time_recv_s;   ///< recepience time of RPC message, seconds
  DWORD time_recv_us;  ///< recepience time of RPC message, microseconds
  DWORD time_done_s;   ///< end time of event readout, seconds
  DWORD time_done_us;  ///< end time of event readout, microseconds
} trigger_info;

/* make frontend functions callable from the C framework */
extern "C" {
  
/*-- Globals -------------------------------------------------------*/

/** The frontend name (client name) as seen by other MIDAS clients   */
char *frontend_name = "vme_test";
/** The frontend file name, don't change it */
char *frontend_file_name = __FILE__;

/** frontend_loop is called periodically if this variable is TRUE    */
BOOL frontend_call_loop = TRUE;

/** a frontend status page is displayed with this frequency in ms */
INT display_period = 0;
  
/** maximum event size produced by this frontend */
INT max_event_size = 0xffff;

/* maximum event size for fragmented events (EQ_FRAGMENTED) */
INT max_event_size_frag = 0;

/** buffer size to hold events */
  INT event_buffer_size = 0xfffff;
  
extern INT run_state;      ///< run state (STATE_RUNNING, ...)
extern INT frontend_index; ///< frontend index from command line argument -i

/*-- Local variables -----------------------------------------------*/
static int block_nr; /**< acquisition block number */
static BOOL data_avail; ///< True if data is available for readout

/*-- Function declarations -----------------------------------------*/
  INT frontend_init();
  INT frontend_exit();
  INT begin_of_run(INT run_number, char *error);
  INT end_of_run(INT run_number, char *error);
  INT pause_run(INT run_number, char *error);
  INT resume_run(INT run_number, char *error);
  INT frontend_loop();
  
  INT interrupt_configure(INT cmd, INT source, POINTER_T adr);
  INT poll_event(INT source, INT count, BOOL test);
  INT read_trigger_event(char *pevent, INT off);

  INT rpc_g2_end_of_fill(INT index, void *prpc_param[]);
  INT rpc_g2_arm_sampling_logic(INT index, void *prpc_param[]);

  extern int send_event(INT idx, BOOL manual_trig);
  
/*-- Equipment list ------------------------------------------------*/

/**
 *  @brief vme_test
 *  @details MIDAS equipment for vme_test with SIS3350 boards
 *  @ingroup group_equipment
 */
EQUIPMENT equipment[] = { 
  {//"vme_test%02d",                   /* equipment name */
   "vme_test",                       /* equipment name */
   {1, TRIGGER_ALL,                  /* event ID, trigger mask */
     "BUF02",                        /* event buffer */
     EQ_POLLED | EQ_EB,              /* equipment type */
     LAM_SOURCE(0, 0xFFFFFF),        /* event source crate 0, all stations */
     "MIDAS",                        /* format */
     TRUE,                           /* enabled */
     RO_RUNNING,                     /* read only when running */
     10,                             /* poll for 1ms */
     0,                              /* stop run after this event limit */
     0,                              /* number of sub events */
     0,                              /* don't log history */
     "", "", "",},
    read_trigger_event,              /* readout routine */
    },

   {""}
};
} // extern cpp

std::string trigger_port("tcp://127.0.0.1:42040");
std::string handshake_port("tcp://127.0.0.1:42041");
std::string midas_port("tcp://127.0.0.1:42044");

zmq::context_t ctx(1);
zmq::socket_t midas_sck(ctx, ZMQ_PULL);


/*-- Frontend Init -------------------------------------------------*/

INT frontend_init()
{
  // Initialize the data socket.
  int timeout = 10;
  midas_sck.setsockopt(ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
  midas_sck.connect(midas_port.c_str());


  // Initialization done elsewhere. This is a control wrapper.
  // May want to ensure that it is running.
  int status = frontend_rpc_init();
  
  if (status != SUCCESS) {
    cout << "Failed to make rpc connection." << endl;
  }

  return SUCCESS;
}

/*-- Frontend Exit -------------------------------------------------*/

INT frontend_exit()
{
  return SUCCESS;
}

/*-- Begin of Run --------------------------------------------------*/

// This function sends a ZMQ start run message to the vme frontend.
INT begin_of_run(INT run_number, char *error)
{
  // Handshake call via rpc
  int status = frontend_rpc_bor(1);
  if (status != SUCCESS) {
    cout << "Failed rpc bor" << endl;
  }

  // Set up the zmq context
  zmq::socket_t start_sck(ctx, ZMQ_PUB);
  zmq::socket_t handshake_sck(ctx, ZMQ_REQ);

  start_sck.connect(trigger_port.c_str());
  handshake_sck.connect(handshake_port.c_str());

  std::string connect("CONNECTED");
  zmq::message_t handshake_msg(connect.size());
  memcpy(handshake_msg.data(), connect.c_str(), connect.size());

  // Establish a connection.
  handshake_sck.send(handshake_msg);
  bool rc = handshake_sck.recv(&handshake_msg);

  if (rc == true) {
    // Create the start message
    // Get the run number.
    
    RUNINFO runinfo;
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
    sprintf(str, "midas_%05d:", runinfo.run_number);
    
    std::string trigger("START:");
    trigger.append(std::string(str));
    zmq::message_t start_msg(trigger.size());
    memcpy(start_msg.data(), trigger.c_str(), trigger.size());

    start_sck.send(start_msg);
    cout << "Connection established.  Sending start trigger." << endl;
    return CM_SUCCESS;

  } else {
    
    cout << "Connection not established.  No trigger sent." << endl;
    return CM_SUCCESS;
  }

  return CM_SUCCESS;
}

/*-- End of Run ----------------------------------------------------*/

INT end_of_run(INT run_number, char *error)
{
  zmq::socket_t stop_sck(ctx, ZMQ_PUB);
  zmq::socket_t handshake_sck(ctx, ZMQ_REQ);

  stop_sck.connect(trigger_port.c_str());
  handshake_sck.connect(handshake_port.c_str());

  std::string connect("CONNECTED");
  zmq::message_t handshake_msg(connect.size());
  memcpy(handshake_msg.data(), connect.c_str(), connect.size());

  // Establish a connection.
  handshake_sck.send(handshake_msg);
  bool rc = handshake_sck.recv(&handshake_msg);

  if (rc == true) {
    // Create the stop message
    std::string trigger("STOP:");
    zmq::message_t stop_msg(trigger.size());
    memcpy(stop_msg.data(), trigger.c_str(), trigger.size());
    
    stop_sck.send(stop_msg);
    cout << "Connection established.  Sending stop trigger." << endl;
    return 0;

  } else {
    
    cout << "Connection not established.  No trigger sent." << endl;
    return 0;
  }

  // Handshake call via rpc
  int status = frontend_rpc_eor();
  if (status != SUCCESS) {
    cout << "Failed rpc eor" << endl;
  }
  return SUCCESS;
}

/*-- Pause Run -----------------------------------------------------*/

INT pause_run(INT run_number, char *error)
{
  cm_msg(MERROR, "pause_run", "This functionality is not implemented");
  return CM_INVALID_TRANSITION;
}

/*-- Resuem Run ----------------------------------------------------*/

INT resume_run(INT run_number, char *error)
{
  cm_msg(MERROR, "resume_run", "This functionality is not implemented");
  return CM_INVALID_TRANSITION;
}


/*-- Frontend Loop -------------------------------------------------*/

INT frontend_loop()
{
  return SUCCESS;
}

/*------------------------------------------------------------------*/

/********************************************************************\

  Readout routines for different events

\********************************************************************/

/*-- Trigger event routines ----------------------------------------*/

INT poll_event(INT source, INT count, BOOL test)
{
  if (test) {
    for (int i = 0; i < count; i++) {
      usleep(10);
    }
    return 0;
  }

  static zmq::message_t start_msg(10);
  bool rc = midas_sck.recv(&start_msg, ZMQ_NOBLOCK);
  if (rc) {
    cout << "got event." << endl;
    return 1;
  }

  if (run_state == STATE_RUNNING) {

    if (data_avail) {  
      return 1;

    } else {
      return 0;

    }
  } 
  return 0;
}

/*-- Interrupt configuration ---------------------------------------*/

INT interrupt_configure(INT cmd, INT source, POINTER_T adr)
{
  return SUCCESS;
}

/*-- Event readout -------------------------------------------------*/

INT read_trigger_event(char *pevent, INT off)
{
  cout << "Beginning read trigger event." << endl;
  BYTE *pdata;
  char bk_name[8];
  int status;
  
  data_avail = FALSE;

  static zmq::message_t data_msg(max_event_size);

  //  static bool set_socket = true;
  //  if (set_socket) {
  //    midas_sck.connect(midas_port.c_str());
  //    set_socket = false;
  //  }
  
  // Useful parameter for multipart messages.
  bool rc = false;
  int64_t more = 1;
  size_t more_size = sizeof(more);
  int count;

  // Check the socket
  midas_sck.getsockopt(ZMQ_RCVMORE, &more, &more_size);

  // Loop until we have no more messages.
  count = 0;
  while (more != 0) {
    
    // Get the message
    rc = midas_sck.recv(&data_msg);

    if (rc == true) {

      std::string bank_name;
      std::string bank_type;

      std::istringstream ss(static_cast<char *>(data_msg.data()));

      std::getline(ss, bank_name, ':');
      std::getline(ss, bank_type, ':');


      // Use a switch to copy the data structure.
      if (bank_type == "sis_3350") {
	
        sprintf(bk_name, "SF%02i", count++);
      	sis_3350 sis;
      	int offset = bank_name.size() + bank_type.size();
      	memcpy(&sis, data_msg.data() + offset, sizeof(sis));
      	
      	DWORD *traces;
      	bk_create(pevent, bank_name.c_str(), TID_DWORD, traces);

      	memcpy(traces, sis.trace, sizeof(sis.trace));
      	traces += sizeof(sis.trace) / sizeof(TID_DWORD);

      	bk_close(pevent, traces);

      } else if (bank_type == "sis_3302") {
  
        sprintf(bk_name, "SS%02i", count++);
        sis_3302 sis;
        int offset = bank_name.size() + bank_type.size();
        memcpy(&sis, data_msg.data() + offset, sizeof(sis));
        
        DWORD *traces;
        bk_create(pevent, bank_name.c_str(), TID_DWORD, traces);

        memcpy(traces, sis.trace, sizeof(sis.trace));
        traces += sizeof(sis.trace) / sizeof(TID_DWORD);

        bk_close(pevent, traces);

      } else if (bank_type == "caen_1785") {
  
        sprintf(bk_name, "CA%02i", count++);
        caen_1785 caen;
        int offset = bank_name.size() + bank_type.size();
        memcpy(&caen, data_msg.data() + offset, sizeof(caen));
        
        DWORD *traces;
        bk_create(pevent, bank_name.c_str(), TID_DWORD, traces);

        memcpy(traces, caen.value, sizeof(caen.value));
        traces += sizeof(caen.value) / sizeof(TID_DWORD);

        bk_close(pevent, traces);
      } else if (bank_type == "caen_6742") {

        sprintf(bk_name, "CD%02i", count++);  
        caen_6742 sis;
        int offset = bank_name.size() + bank_type.size();
        memcpy(&caen, data_msg.data() + offset, sizeof(caen));
        
        DWORD *traces;
        bk_create(pevent, bank_name.c_str(), TID_DWORD, traces);

        memcpy(traces, caen.trace, sizeof(caen.trace));
        traces += sizeof(caen.trace) / sizeof(TID_DWORD);

        bk_close(pevent, traces);
      }
    }

    midas_sck.getsockopt(ZMQ_RCVMORE, &more, &more_size);
  }
  
  struct timeval t;
  DWORD *tr_data;
  
  sprintf(bk_name,"SR%02i",frontend_index);
  bk_create(pevent, bk_name, TID_DWORD, &tr_data);
  
  // status = gettimeofday( &t, NULL);
  // if ( status != 0)
  //   {
  //     printf("ERROR! gettimeofday() failed\n");
  //     t.tv_sec = 0;
  //     t.tv_usec = 0;
  //   }
  
  // *tr_data++ = t.tv_sec;
  // *tr_data++ = t.tv_usec;
  
  // bk_close(pevent, tr_data);
  
  rpc_g2_ready(1);
  data_avail = false;
  
  return bk_size(pevent);
}


INT rpc_g2_end_of_fill(INT index, void *prpc_param[])
{
  char *p;
  int i;

  data_avail = true;
  cout << "Got rpc_g2_end_of_fill call." << endl;
  return SUCCESS;
}

INT rpc_g2_arm_sampling_logic(INT index, void *prpc_param[])
{
  return SUCCESS;
}
