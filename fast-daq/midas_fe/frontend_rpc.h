/**
 * @file    frontends/common/frontend_rpc.h
 * @author  Vladimir Tishchenko <tishenko@pa.uky.edu>
 * @date    Fri Feb 17 10:31:39 2012
 * @date    Last-Updated: Wed Jul 25 12:37:49 2012 (-0500)
 *          By : Data Acquisition
 *          Update #: 12 
 * @version $Id$
 * 
 * @brief   Declaration of frontend RPC interface
 * 
 * @section Changelog
 * @verbatim
 * $Log$
 * @endverbatim
 */
#ifndef frontend_rpc_h
#define frontend_rpc_h

#ifdef __cplusplus
extern "C" {
#endif

  extern INT frontend_rpc_init(void);
  extern INT frontend_rpc_bor(const int id);
  extern INT frontend_rpc_eor(void);
  extern INT rpc_g2_ready(const int id);

#ifdef __cplusplus
}
#endif


#endif /* frontend_rpc_h defined */
