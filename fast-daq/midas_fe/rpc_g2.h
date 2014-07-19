#ifndef DEFINE_RPC_LIST
#define DEFINE_RPC_LIST
/**
 * @file    frontends/master/rpc_g2.h
 * @author  Vladimir Tishchenko <tishenko@pa.uky.edu>
 * @date    Thu May 24 10:43:34 2012 (-0500)
 * @date    Last-Updated: Thu May 24 15:19:16 2012 (-0500)
 *          By : Data Acquisition
 *          Update #: 12
 * @version $Id$
 * 
 * @copyright (c) new (g-2) collaboration
 * 
 * addtogroup synchronization
 *  - \ref rpc_g2.h
 * 
 * page   rpc_g2.h
 * 
 * @brief   RPC communication
 * 
 * @details RPC communication
 * 
 * @todo Document this code 
 * 
 * @section Changelog
 * @verbatim
 * $Log$
 * @endverbatim
 */

/**
 * defgroup rpc_list RPC communication messages
 */ 

/**
 * @defgroup rpc_list RPC communication messages
 * @{ */

/**
 * This RPC message is sent from master to slaves when spill ends
 */ 
#define RPC_END_OF_FILL           2201
#define RPC_READY                 2202
#define RPC_ARM_SAMPLING_LOGIC    2203

/**
 * @}
 * endcode
 * endlink
 */


/**
 * @brief RPC communication messages 1
 */
RPC_LIST rpc_list_g2[] = { 

  // sent from master frontend to slave frontends
  {RPC_END_OF_FILL, "rpc_end_of_fill", 
   {
     {TID_DWORD, RPC_IN, 0} , // fill number
     {TID_DWORD, RPC_IN, 0} , // trigger mask
     {TID_DWORD, RPC_IN, 0} , // trigger time, seconds
     {TID_DWORD, RPC_IN, 0} , // trigger time, microseconds
     {0, 0, 0} 
   }, 
   0
  }, 

  // sent from slave frontend to master frontend
  {RPC_READY, "rpc_ready", 
   {
     {TID_INT, RPC_IN, 0},   // frontend number
     {0, 0, 0} 
   }, 
   0
  }, 

  // sent from master frontend to slave frontends
  {RPC_ARM_SAMPLING_LOGIC, "rpc_arm_sampling_logic", 
   {
     {TID_DWORD, RPC_IN, 0} , // spill number
     {0, 0, 0} 
   }, 
   0
  }, 


  {0, 0, 
   {
     {0,0,0}
   },
   0
  }
};

//#else
/**
 *   @brief RPC communication messages 2
 */
//extern RPC_LIST rpc_list_g2[];

#endif
