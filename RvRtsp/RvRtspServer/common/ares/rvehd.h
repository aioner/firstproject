#ifndef _RV_EHD_H
#define _RV_EHD_H

#include "rvlog.h"
#include "rvselect.h"

#ifdef __cplusplus
extern "C" {
#endif

#if RV_NET_TYPE != RV_NET_NONE

#define RV_EHD_SEARCH_HOSTS_PLATFORM -1
#define RV_EHD_SEARCH_HOSTS_FIRST 0
#define RV_EHD_SEARCH_HOSTS_LAST  1

typedef struct _RvEHD RvEHD;

RvStatus RvEHDNew(RvEHD **self, RvLogMgr *logMgr);

RvStatus RvEHDDelete(RvEHD *self);

RvStatus RvEHDFind(RvEHD *self, RvChar *name, RvUint16 qtype, RvUint8 *pBuf, RvSize_t *pSize, RvInt *pNfound);

/* RvEHDGetSearchOrder() 
 *
 * Returns hosts file search order:
 * RV_EHD_SEARCH_HOSTS_FIRST - search hosts file before DNS servers
 * RV_EHD_SEARCH_HOSTS_LAST  - search hosts file after  DNS servers
 */
RvInt RvEHDGetSearchOrder(void);

#endif

#ifdef __cplusplus
}
#endif

#endif

