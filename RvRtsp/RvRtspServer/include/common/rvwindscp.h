#ifndef _rvwindscp_h
#define _rvwindscp_h

#include "rvlog.h"

#define RV_WIN_DSCP_NONE 0
#define RV_WIN_DSCP_TC 1
#define RV_WIN_DSCP_GQOS 2
#define RV_WIN_DSCP_IPTOS 3

#if !defined(RV_WIN_DSCP)
#  if defined(_WINDOWS)
#    define RV_WIN_DSCP RV_WIN_DSCP_TC 
#  else
#    define RV_WIN_DSCP RV_WIN_DSCP_NONE 
#  endif /* _WINDOWS */
#endif   /*RV_WIN_DSCP */

#if RV_WIN_DSCP == RV_WIN_DSCP_NONE 

#define RvWinDscpGetTos(sock, tos, logMgr) (RV_OK)
#define RvWinDscpSetTos(sock, tos, logMgr) (RV_OK)
#define RvWinDscpRemoveSocket(sock, logMgr) (RV_OK)
#define RvWinDscpCheckTos(sock, logMgr) (RV_OK)
#define RvWinDscpEnd() (RV_OK)
void CheckTosSupport(void);

#elif RV_WIN_DSCP == RV_WIN_DSCP_IPTOS

#define RvWinDscpEnd() (RV_OK)
#define RvWinDscpRemoveSocket(sock, logMgr) (RV_OK)
#define RvWinDscpCheckTos(sock, logMgr) (RV_OK)


#else

#include <Winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif

RvStatus RvWinDscpGetTos(SOCKET sock, int *tos, RvLogMgr *logMgr);
RvStatus RvWinDscpSetTos(SOCKET sock, int tos, RvLogMgr *logMgr);
RvStatus RvWinDscpRemoveSocket(SOCKET sock, RvLogMgr *logMgr);
RvStatus RvWinDscpCheckTos(SOCKET sock, RvLogMgr *logMgr);
void     CheckTosSupport(void);
RvStatus RvWinDscpEnd();

#ifdef __cplusplus
}
#endif

#endif

#endif
