#ifndef _rvsocketshadow_h
#define _rvsocketshadow_h

#include "rvtypes.h"
#include "rverror.h"
#include "rvsocket.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef const void* RvSocketShadowID;
typedef void* RvSocketShadow;

typedef void (*RvSocketShadowDestruct)(RvSocketShadow shadow);

#ifndef RV_SOCKET_USE_SHADOWS
#define RV_SOCKET_USE_SHADOWS 0
#endif


#if RV_SOCKET_USE_SHADOWS


RVCOREAPI
RvStatus RvSocketShadowInit();

RVCOREAPI
void RvSocketShadowEnd();

RVCOREAPI
RvSocketShadow RvSocketShadowFind(RvSocket *sock, RvSocketShadowID id);

RVCOREAPI
RvStatus RvSocketShadowAdd(RvSocket *sock, RvSocketShadowID id, RvSocketShadow shadow, RvSocketShadowDestruct destruct);

RVCOREAPI
RvSocketShadow RvSocketShadowRemove(RvSocket *sock, RvSocketShadowID id, RvBool bDestruct);

RVCOREAPI
void RvSocketShadowRemoveAll(RvSocket *sock);

#else /* RV_SOCKET_USE_SHADOWS */

#define RvSocketShadowInit() (RV_OK)
#define RvSocketShadowEnd()
#define RvSocketShadowFind(s, id) ((void)(s), (void)id, 0)
#define RvSocketShadowAdd(s, id, shadow, destructor) ((s),(id),(shadow),(destructor),(RV_ERROR_NOTSUPPORTED))
#define RvSocketShadowRemove(s, id, bDestruct) ((s),(id),(bDestruct),0)
#define RvSocketShadowRemoveAll(s) ((void)(s))

#endif

#ifdef __cplusplus
}
#endif

#endif
