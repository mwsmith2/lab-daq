/**
 * @file    rpc_MT.c
 * @author  Vladimir Tishchenko <tishenko@pa.uky.edu>
 * @date    Thu May 24 10:45:51 2012 (-0500)
 * @date    Last-Updated: Thu May 24 10:46:07 2012 (-0500)
 *          By : Data Acquisition
 *          Update #: 3
 * @version $Id$
 * 
 * @copyright (c) new (g-2) collaboration
 * 
 * @brief   RPC synchronization
 *
 * @details initialization of variables for RPC-based 
 *          synchronization between the frontends
 * 
 * @todo Document this code 
 * 
 * @section Changelog
 * @verbatim
 * $Log$
 * @endverbatim
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <midas.h>

// header file for rpc commumication
#define DEFINE_RPC_LIST
#include "rpc_g2.h"
#undef  DEFINE_RPC_LIST

