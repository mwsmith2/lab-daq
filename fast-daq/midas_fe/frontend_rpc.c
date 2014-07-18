/**
 * @file    frontends/common/frontend_rpc.c
 * @author  Vladimir Tishchenko <tishenko@pa.uky.edu>
 * @date    Fri Feb 17 10:04:03 2012
 * @date    Last-Updated: Fri May 30 14:25:52 2014 (-0400)
 *          By : Data Acquisition
 *          Update #: 56 
 * @version $Id$
 * 
 * @copyright (c) new (g-2) collaboration
 * 
 * @brief   RPC interface of the frontend
 * 
 * @details Functions common to UDP-based and VME-based frontends
 * 
 * @section Changelog
 * @verbatim
 * $Log$
 * @endverbatim
 */


#include <stdio.h>
#include <stdlib.h>
#include <midas.h>

#include "frontend_rpc.h"

// rpc commumication
//#define DEFINE_RPC_LIST
#include "rpc_g2.h"
static HNDLE rpc_master_hndle;
static char *rpc_master_name = "MasterSLAC";
//#undef DEFINE_RPC_LIST

/*-- globals ---------------------------------------------------------*/
extern INT frontend_index; ///< frontend index from command line argument -i

// these functions must be defined in frondendXXX.cpp
extern "C" {
extern INT rpc_g2_end_of_fill(INT index, void *prpc_param[]);
extern INT rpc_g2_arm_sampling_logic(INT index, void *prpc_param[]);
}

/** 
 * Setup RPC interface to the master crate
 * 
 * @return SUCCESS on success
 */
INT frontend_rpc_init()
{
  INT status;
  
  /************************************************************************
   **                    setup RPC communication                         **
   ************************************************************************/
  
  // register rpc functions for a slave
  status = rpc_register_functions(rpc_list_g2, NULL);
  if ( status != RPC_SUCCESS )
    {
      cm_msg(MERROR, "frontend_init", "Cannot register RPC functions, err = %d", status);
      return status;
    }
  
  status = rpc_register_function(RPC_END_OF_FILL, rpc_g2_end_of_fill); 
  if ( status != RPC_SUCCESS )
    {
      cm_msg(MERROR, "rpc_init", "Cannot register function rpc_g2_end_of_fill, err = %d",status);
      return status;
    }
  
  status = rpc_register_function(RPC_ARM_SAMPLING_LOGIC, rpc_g2_arm_sampling_logic);
  if ( status != RPC_SUCCESS )
    {
      cm_msg(MERROR, "rpc_init", "Cannot register function rpc_g2_arm_sampling_logic, err = %d",status);
      return status;
    }
  
  
  return SUCCESS;

}

/** 
 * Called when run starts. Connects to the master frontend
 * 
 * @return CM_SUCCESS on success
 */
INT frontend_rpc_bor( const int id )
{

  INT status;

  /************************************************************************
   **         establish RPC communication with master FE                 **
   ************************************************************************/
  status = cm_connect_client(rpc_master_name, &rpc_master_hndle);
  
  switch(status)
    {
    case CM_SUCCESS :
      printf("\n Successfully connected to [%s]\n",rpc_master_name);
      break;
      
    case CM_NO_CLIENT :
      cm_msg(MERROR, "rpc_connect", "Client [%s] not found",rpc_master_name);
      break;
      
    case RPC_NET_ERROR:
      cm_msg(MERROR, "rpc_connect", "Network error");
      break;
      
    case RPC_NO_CONNECTION:
      cm_msg(MERROR, "rpc_connect", "Maximum number of connections exceeded");
      break;
      
    case RPC_NOT_REGISTERED:
      cm_msg(MERROR, "rpc_connect", "cm_connect_experiment() has not been registered"); 
      break;
      
    default:
      cm_msg(MERROR, "rpc_connect", "Unknown error");
      break;
    } 
  
  if ( status != CM_SUCCESS ) 
    {
      cm_msg(MERROR, "rpc_connect", "Cannot connect to master [%s]",rpc_master_name);
      return status;
    }

  rpc_set_option(rpc_master_hndle, RPC_OTRANSPORT, RPC_FTCP);
  rpc_set_option(rpc_master_hndle, RPC_NODELAY, TRUE);

  // Inform the master that we are ready to go!
  if ( id > 0  )
    status = rpc_client_call(rpc_master_hndle, RPC_READY, id);
  else
    status = rpc_client_call(rpc_master_hndle, RPC_READY, frontend_index);
  if(status != RPC_SUCCESS ) 
    {
      cm_msg(MERROR, "rpc_call", "No RPC to master");
    }



  return status;

}



/** 
 * Called when run stops. Disconnects from the master frontend,
 * 
 * @return RPC_SUCCESS on success
 */
INT frontend_rpc_eor()
{

  INT status;

  /************************************************************************
   **         establish RPC communication with master FE                 **
   ************************************************************************/
  status = cm_disconnect_client(rpc_master_hndle, 0);
  if ( status != RPC_SUCCESS )
    {
      cm_msg(MERROR, "rpc_disconnect", "Cannot disconnect from master [%s]",rpc_master_name);
    }
  else
    {
      printf("Disconnecting from master frontend ....    success\n");
    }
  
  return status;

}


INT rpc_g2_ready(const int id)
{

  int status;
  if ( id > 0 ){
    printf("rpc_g2_ready from frontend_index %d\n",id);
    status = rpc_client_call(rpc_master_hndle, RPC_READY, id);
  }
  else
    status = rpc_client_call(rpc_master_hndle, RPC_READY, frontend_index);
  if(status != RPC_SUCCESS ) 
    {
      cm_msg(MERROR, "rpc_call", "No RPC to master");
    }
  
  return status;
}
