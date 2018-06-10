/***********************************************************************
Filename   : rvnotifyinternL.h
Description: nitifications handling
************************************************************************
Copyright (c) 2011 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/

#include "rvnotify.h" 
#include "rvlog.h"
#include "rvlock.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
    RvNotifierSrcId       srcId;
    RvNotifierSource      srcPublicId;
    RvChar*             srcTxt;    
} RvNotifierSrc;

#define RV_NOTIFY_MAX_SRC_PER_LISTENER  20


typedef struct {
    RvChar*             name;
    RvNotificationLvl   minLvl;

    RvBool              bOnlyRegisteredSrc;

    struct {
            RvNotifierSrcId     srcId;
            RvNotificationLvl   lvl;
    } regSrc[RV_NOTIFY_MAX_SRC_PER_LISTENER];
    RvInt   regSrcCnt;   

    RvNotifierFunc    ntfFunc;
    void*           ntfAppCtx;
} RvNtfListener;


#define RV_NOTIFIER_MAX_SOURCES         64
#define RV_NOTIFIER_MAX_LISTENERS       64

typedef struct {
    RvLock          lock;
    RvLogMgr*       pLogMgr;
    RvLogSource     logSrc;
    RvLogSource*    pLogSrc;

    RvNotifierSrc*  srcArr[RV_NOTIFIER_MAX_SOURCES];
    RvInt           sourceCnt;

    RvNtfListener*  listenersArr[RV_NOTIFIER_MAX_LISTENERS];
    RvInt           listenersCnt;
} RvNotifierMgr;


#include "rvmemory.h"

static RvNotifierMgr gNtMgr;

#define NT_LOG_ENTER(f) \
    RvLogEnter(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f))
#define NT_LOG_ENTER_1(f,p1) \
    RvLogEnter(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1))
#define NT_LOG_ENTER_2(f,p1,p2) \
    RvLogEnter(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2))
#define NT_LOG_ENTER_3(f,p1,p2,p3) \
    RvLogEnter(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2,p3))
#define NT_LOG_ENTER_4(f,p1,p2,p3,p4) \
    RvLogEnter(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2,p3,p4))
#define NT_LOG_ENTER_5(f,p1,p2,p3,p4,p5) \
    RvLogEnter(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2,p3,p4,p5))
#define NT_LOG_ENTER_6(f,p1,p2,p3,p4,p5,p6) \
    RvLogEnter(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2,p3,p4,p5,p6))
#define NT_LOG_ENTER_7(f,p1,p2,p3,p4,p5,p6,p7) \
    RvLogEnter(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2,p3,p4,p5,p6,p7))

#define NT_LOG_LEAVE(f) \
    RvLogLeave(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f))
#define NT_LOG_LEAVE_1(f,p1) \
    RvLogLeave(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1))
#define NT_LOG_LEAVE_2(f,p1,p2) \
    RvLogLeave(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2))


#define NT_LOG_ERR(f) \
    RvLogError(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f))
#define NT_LOG_ERR_1(f,p1) \
    RvLogError(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1))
#define NT_LOG_ERR_2(f,p1,p2) \
    RvLogError(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2))
#define NT_LOG_ERR_3(f,p1,p2,p3) \
    RvLogError(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2,p3))

#define NT_LOG_DBG(f) \
    RvLogDebug(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f))
#define NT_LOG_DBG_1(f,p1) \
    RvLogDebug(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1))
#define NT_LOG_DBG_2(f,p1,p2) \
    RvLogDebug(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2))
#define NT_LOG_DBG_3(f,p1,p2,p3) \
    RvLogDebug(gNtMgr.pLogSrc, (gNtMgr.pLogSrc, FUNCTION_NAME ": " f,p1,p2,p3))


RVCOREAPI
RvStatus RVCALLCONV RvNotifierRegisterSource(
    IN  const RvChar*           srcName,
    IN  RvNotifierSource        srcNum,
    OUT RvNotifierSrcId*        pSrcId)
{
#define FUNCTION_NAME "RvNotifierRegisterSource"
    RvInt cnt;
    RvStatus retV;
    RvNotifierSrc* pNtSrc = NULL;

    RvLockGet(&gNtMgr.lock, gNtMgr.pLogMgr);

    NT_LOG_ENTER_2("Called: num %d name %s",
        srcNum,(srcName)?srcName:"");

    for (cnt = 0; cnt < gNtMgr.sourceCnt; cnt ++)
    {
        if (gNtMgr.srcArr[cnt] == NULL)
            break;
    }

    if (cnt == gNtMgr.sourceCnt)
    {

        if (cnt == RV_NOTIFIER_MAX_SOURCES)
        {
            NT_LOG_ERR("Too many sources");
            retV = RV_ERROR_OUTOFRESOURCES;
            goto finish;
        }
        gNtMgr.sourceCnt++;
    }

    pNtSrc = gNtMgr.srcArr[cnt];

    retV = RvMemoryAlloc(NULL,sizeof(RvNotifierSrc),gNtMgr.pLogMgr,(void**)&pNtSrc);
    if (retV != RV_OK)
    {
        NT_LOG_ERR("Failed in RvMemoryAlloc for RvNotifierSrc");
        retV = RV_ERROR_OUTOFRESOURCES;
        goto finish;
    }

    memset(pNtSrc,0,sizeof(RvNotifierSrc));
    pNtSrc->srcId = cnt+1;
    pNtSrc->srcPublicId = srcNum;
    if (srcName && *srcName)
    {
        RvSize_t len = strlen(srcName)+1;

        retV = RvMemoryAlloc(NULL,len,gNtMgr.pLogMgr,(void**)&pNtSrc->srcTxt);
        if (retV != RV_OK)
        {
            NT_LOG_ERR("Failed in RvMemoryAlloc for name");
            RvMemoryFree(pNtSrc,gNtMgr.pLogMgr);
            goto finish;
        }
        memcpy(pNtSrc->srcTxt,srcName,len);
    }

    pNtSrc->srcId = cnt + 1;
    *pSrcId = cnt + 1;

    gNtMgr.srcArr[cnt] = pNtSrc;

    NT_LOG_DBG_3("Added source num %d name %s id %d",
        srcNum,(srcName)?srcName:"",cnt+1);


finish:
    NT_LOG_LEAVE_1("Leaving with %d",retV);

    RvLockRelease(&gNtMgr.lock, gNtMgr.pLogMgr);

    return retV;

#undef FUNCTION_NAME
}


RVCOREAPI
RvStatus RVCALLCONV RvNotifierUnregisterSource(
    IN  RvNotifierSrcId       srcId)
{
#define FUNCTION_NAME "RvNotifierUnregisterSource"
    RvStatus retV = RV_OK;
    RvInt cnt;

    RvLockGet(&gNtMgr.lock, gNtMgr.pLogMgr);

    NT_LOG_ENTER_1("Called: srcId %d ",srcId);

    if (srcId < 0 || srcId > gNtMgr.sourceCnt)
    {
        NT_LOG_ERR_1("Wrong srcId %d",srcId);
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }
    
    if (gNtMgr.srcArr[srcId - 1] == NULL)
    {
        NT_LOG_ERR_1("Not existing srcId %d",srcId);
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }

    if (gNtMgr.srcArr[srcId-1]->srcTxt)
        RvMemoryFree(gNtMgr.srcArr[srcId-1]->srcTxt,gNtMgr.pLogMgr);
    
    RvMemoryFree(gNtMgr.srcArr[srcId-1],gNtMgr.pLogMgr);
    gNtMgr.srcArr[srcId-1] = NULL;

    if (srcId == gNtMgr.sourceCnt)
    {
        for (;;)
        {
            gNtMgr.sourceCnt--;
            if (gNtMgr.sourceCnt == 0)
                break;

            if (gNtMgr.srcArr[gNtMgr.sourceCnt-1])
                break;
        }
    }

    /* now clean this srcId from the listeners */
    for (cnt = 0; cnt < gNtMgr.listenersCnt; cnt ++)
    {
        RvNtfListener* pNtLstnr;
        RvInt cnt1 = 0;
        
        pNtLstnr = gNtMgr.listenersArr[cnt];
        if (!pNtLstnr || !pNtLstnr->regSrcCnt)
            continue;

        while (cnt1 < pNtLstnr->regSrcCnt)
        {
            if (pNtLstnr->regSrc[cnt1].srcId == srcId)
            {
                /* move the last to the current position, only
                   if the last differs from the current */
                pNtLstnr->regSrcCnt--;
                if (cnt1 != pNtLstnr->regSrcCnt)
                {
                    pNtLstnr->regSrc[cnt1] = pNtLstnr->regSrc[pNtLstnr->regSrcCnt];
                }
                memset(pNtLstnr->regSrc+pNtLstnr->regSrcCnt,0,sizeof(pNtLstnr->regSrc[pNtLstnr->regSrcCnt]));
            }
            else
                cnt1++;
        }
    }

    NT_LOG_DBG_1("Removed srcId %d",srcId);


finish:
    NT_LOG_LEAVE_1("Leaving with %d",retV);

    RvLockRelease(&gNtMgr.lock, gNtMgr.pLogMgr);

    return retV;

#undef FUNCTION_NAME
}

RVCOREAPI
RvStatus RVCALLCONV RvNotifierGetSourceByName(
    IN  const RvChar*       name,
    OUT RvNotifierSrcId*    pSrcId,
    OUT RvNotifierSource*   pSrcPubNum)
{
    RvInt cnt = 0;
    RvStatus retV = RV_ERROR_BADPARAM;

    if (pSrcId)
        *pSrcId = RV_NOTIFIER_SRC_ID_UNDEFINED;

    if (pSrcPubNum)
        *pSrcPubNum = 0;

    RvLockGet(&gNtMgr.lock, gNtMgr.pLogMgr);

    for (cnt = 0; cnt < gNtMgr.sourceCnt; cnt++)
    {
        if (gNtMgr.srcArr[cnt] && gNtMgr.srcArr[cnt]->srcTxt && strcmp(gNtMgr.srcArr[cnt]->srcTxt,name) == 0)
        {
            if (pSrcId)
                *pSrcId = gNtMgr.srcArr[cnt]->srcId;

            if (pSrcPubNum)
                *pSrcPubNum = gNtMgr.srcArr[cnt]->srcPublicId;

            retV = RV_OK;
            break;
        }
    }

    RvLockRelease(&gNtMgr.lock, gNtMgr.pLogMgr);
    return retV;
}

RVCOREAPI
RvStatus RVCALLCONV RvNotifierRegisterListener(
    IN  const RvChar*       name,
    IN  RvNotifierFunc      cbFunc,
    IN  void*               cbCtx,
    IN  RvNotificationLvl   lvl,
    IN  RvBool              onlyRegisteredSrc)
{
#define FUNCTION_NAME "RvNotifierRegisterListener"
    RvInt cnt;
    RvStatus retV;
    RvNtfListener* pNtLstnr;

    RvLockGet(&gNtMgr.lock, gNtMgr.pLogMgr);

    NT_LOG_ENTER_1("Called: name %s",(name)?name:"");

    for (cnt = 0; cnt < gNtMgr.listenersCnt; cnt ++)
    {
        if (gNtMgr.listenersArr[cnt] == NULL)
            break;
    }

    if (cnt == gNtMgr.listenersCnt)
    {

        if (cnt == RV_NOTIFIER_MAX_LISTENERS)
        {
            NT_LOG_ERR("Too many listeners");
            retV = RV_ERROR_OUTOFRESOURCES;
            goto finish;
        }
        gNtMgr.listenersCnt++;
    }


    retV = RvMemoryAlloc(NULL,sizeof(RvNtfListener),gNtMgr.pLogMgr,(void**)&pNtLstnr);
    if (retV != RV_OK)
    {
        NT_LOG_ERR("Failed in RvMemoryAlloc for RvNtfListener");
        retV = RV_ERROR_OUTOFRESOURCES;
        goto finish;
    }

    memset(pNtLstnr,0,sizeof(RvNtfListener));
    pNtLstnr->ntfFunc = cbFunc;
    pNtLstnr->ntfAppCtx = cbCtx;
    pNtLstnr->bOnlyRegisteredSrc = onlyRegisteredSrc;
    pNtLstnr->minLvl = lvl;

    if (name && *name)
    {
        RvSize_t len = strlen(name)+1;

        retV = RvMemoryAlloc(NULL,len,gNtMgr.pLogMgr,(void**)&pNtLstnr->name);
        if (retV != RV_OK)
        {
            NT_LOG_ERR("Failed in RvMemoryAlloc for name");
            RvMemoryFree(pNtLstnr,gNtMgr.pLogMgr);
            goto finish;
        }
        memcpy(pNtLstnr->name,name,len);
    }

    gNtMgr.listenersArr[cnt] = pNtLstnr;


finish:
    NT_LOG_LEAVE_1("Leaving with %d",retV);

    RvLockRelease(&gNtMgr.lock, gNtMgr.pLogMgr);

    return retV;

#undef FUNCTION_NAME
}


RVCOREAPI
RvStatus RVCALLCONV RvNotifierUnregisterListener(
    IN  const RvChar*       name,
    IN  RvNotifierFunc      cbFunc,
    IN  void*               cbCtx)
{
#define FUNCTION_NAME "RvNotifierUnregisterListener"
    RvStatus retV = RV_OK;
    RvInt cnt;
    RvNtfListener* pNtLstnr;
    RvBool found = RV_FALSE;

    RvLockGet(&gNtMgr.lock, gNtMgr.pLogMgr);

    NT_LOG_ENTER_3("Called: name %s func %p ctx %p",
        (name)?name:"",cbFunc,cbCtx);


    if ((!name || !*name) && !cbFunc && !cbCtx)
    {
        NT_LOG_ERR("Name, cbFunc and cnCtx are all NULLs");
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }

    for (cnt = 0; cnt < gNtMgr.listenersCnt; cnt ++)
    {
        pNtLstnr = gNtMgr.listenersArr[cnt];
        if (pNtLstnr == NULL)
            continue;
        if (name && *name && pNtLstnr->name && strcmp(name,pNtLstnr->name))
            continue;

        if (cbCtx && pNtLstnr->ntfAppCtx && cbCtx != pNtLstnr->ntfAppCtx)
            continue;

        if (cbFunc && pNtLstnr->ntfFunc != cbFunc)
            continue;

        found = RV_TRUE;
        break;
    }

    if (!found)
    {
        NT_LOG_ERR_3("Could not find for name %s func %p ctx %p",
            (name)?name:"",cbFunc,cbCtx);
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }

    if (gNtMgr.listenersArr[cnt]->name)
        RvMemoryFree(gNtMgr.listenersArr[cnt]->name,gNtMgr.pLogMgr);

    RvMemoryFree(gNtMgr.listenersArr[cnt],gNtMgr.pLogMgr);
    gNtMgr.listenersArr[cnt] = NULL;

    if (cnt == gNtMgr.listenersCnt-1)
    {
        for (;;)
        {
            gNtMgr.listenersCnt--;
            if (gNtMgr.listenersCnt == 0)
                break;

            if (gNtMgr.listenersArr[gNtMgr.listenersCnt-1])
                break;
        }
    }

finish:
    NT_LOG_LEAVE_1("Leaving with %d",retV);
    RvLockRelease(&gNtMgr.lock, gNtMgr.pLogMgr);
    return retV;
#undef FUNCTION_NAME
}

RVCOREAPI
RvStatus RVCALLCONV RvNotifierListenerAddSrc(
    IN  const RvChar*           name,
    IN  RvNotifierFunc          cbFunc,
    IN  void*                   cbCtx,
    IN  RvNotifierSrcId         srcId,
    IN  RvChar*                 srcName,
    IN  RvNotificationLvl       lvl)
{

#define FUNCTION_NAME "RvNotifierListenerAddSrc"
    RvStatus retV = RV_OK;
    RvInt cnt;
    RvNtfListener* pNtLstnr = NULL;
    RvNotifierSrc* pNtSrc = NULL;
    RvBool found = RV_FALSE;

    RvLockGet(&gNtMgr.lock, gNtMgr.pLogMgr);

    NT_LOG_ENTER_6("Called: name %s func %p ctx %p, srcId %d, srcName %s,  lvl %d",
        (name)?name:"",cbFunc,cbCtx,
        srcId,(srcName)?srcName:"",lvl);


    if ((!name || !*name) && !cbFunc && !cbCtx)
    {
        NT_LOG_ERR("Name, cbFunc and cnCtx are all NULLs");
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }

    if ((srcId == RV_NOTIFIER_SRC_ID_UNDEFINED || srcId < 0 || srcId > gNtMgr.sourceCnt) && (!srcName || !*srcName))
    {
        NT_LOG_ERR("valid srcId or srcName must be given");
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }


    for (cnt = 0; cnt < gNtMgr.listenersCnt; cnt ++)
    {
        pNtLstnr = gNtMgr.listenersArr[cnt];
        if (pNtLstnr == NULL)
            continue;
        if (name && *name && pNtLstnr->name && strcmp(name,pNtLstnr->name))
            continue;

        if (cbCtx && pNtLstnr->ntfAppCtx && cbCtx != pNtLstnr->ntfAppCtx)
            continue;

        if (cbFunc && pNtLstnr->ntfFunc != cbFunc)
            continue;

        found = RV_TRUE;
        break;
    }

    if (!found)
    {
        NT_LOG_ERR_3("Could not find for name %s func %p ctx %p",
            (name)?name:"",cbFunc,cbCtx);
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }

    /* now find the source */
    if (srcId != RV_NOTIFIER_SRC_ID_UNDEFINED)
    {
        pNtSrc = gNtMgr.srcArr[srcId-1];
        if (pNtSrc)
        {
            NT_LOG_ERR_1("Bad srcId %d",srcId);
            retV = RV_ERROR_BADPARAM;
            goto finish;
        }
    }

    if (pNtSrc == 0)
    {
        for (cnt = 0; cnt < gNtMgr.sourceCnt; cnt++)
        {
            if (gNtMgr.srcArr[cnt] && gNtMgr.srcArr[cnt]->srcTxt && strcmp(gNtMgr.srcArr[cnt]->srcTxt,srcName) == 0)
            {
                pNtSrc = gNtMgr.srcArr[cnt];
                break;
            }
        }
    }

    if (pNtSrc == NULL)
    {
        NT_LOG_ERR_2("Could not find the source for srcId %d, srcName %s",
            srcId,(srcName)?srcName:"");
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }

    if (pNtLstnr->regSrcCnt == RV_NOTIFY_MAX_SRC_PER_LISTENER)
    {
        NT_LOG_ERR("Too many sources for the listener");
        retV = RV_ERROR_BADPARAM;
        goto finish;
    }

    pNtLstnr->regSrc[pNtLstnr->regSrcCnt].lvl = lvl;
    pNtLstnr->regSrc[pNtLstnr->regSrcCnt].srcId = pNtSrc->srcId;
    pNtLstnr->regSrcCnt ++;
    
finish:
    NT_LOG_LEAVE_1("Leaving with %d",retV);
    RvLockRelease(&gNtMgr.lock, gNtMgr.pLogMgr);
    return retV;
#undef FUNCTION_NAME
}




RVCOREAPI
RvStatus RVCALLCONV RvNotify(
    IN RvNotifierSrcId      ntSrcId,
    IN RvNotificationLvl    ntLvl,
    IN RvNotificationId     ntId,
    IN void*                ntCtx)
{
    RvNotificationData ntData;

    memset(&ntData,0,sizeof(ntData));

    ntData.ntSrcId = ntSrcId;
    ntData.ntLvl = ntLvl;
    ntData.ntId = ntId;
    ntData.ntCtx = ntCtx;

    return RvNotifyEx(&ntData);
}

RVCOREAPI
RvStatus RVCALLCONV RvNotifyEx(
    IN RvNotificationData*  pNtData)
{
#define FUNCTION_NAME   "RvNotifyEx"
    RvInt cnt;
    RvStatus retV = RV_OK;
    RvNotifierSrc* pNtSrc;
    RvNtfListener* pNtLstnr;
    RvInt ntfCnt = 0;


    RvLockGet(&gNtMgr.lock, gNtMgr.pLogMgr);

    NT_LOG_ENTER_5("Called: srcId %d srcName %s lvl %d id %d reas %s",
        pNtData->ntSrcId,
        (pNtData->ntSrcName)?pNtData->ntSrcName:"",
        pNtData->ntLvl,pNtData->ntId,(pNtData->strReason)?pNtData->strReason:"");

    if (pNtData->ntSrcId == RV_NOTIFIER_SRC_ID_UNDEFINED && (pNtData->ntSrcName == NULL || *pNtData->ntSrcName == 0))
    {
        NT_LOG_ERR("No src id and no src name");
        retV = RV_ERROR_UNKNOWN;
        goto finish;
    }

    if (pNtData->ntSrcId && (pNtData->ntSrcId < 0 || pNtData->ntSrcId > gNtMgr.sourceCnt))
    {
        NT_LOG_ERR_1("Illegal value of notifier source (%d)", pNtData->ntSrcId);
        retV = RV_ERROR_UNKNOWN;
        goto finish;
    }

    pNtSrc = NULL;

    if (pNtData->ntSrcId)
    {
        pNtSrc = gNtMgr.srcArr[pNtData->ntSrcId-1];
    }   
    else
    {
        for (cnt = 0; cnt < gNtMgr.sourceCnt; cnt++)
        {
            if (gNtMgr.srcArr[cnt] && gNtMgr.srcArr[cnt]->srcTxt && strcmp(pNtData->ntSrcName,gNtMgr.srcArr[cnt]->srcTxt) == 0)
            {
                pNtSrc = gNtMgr.srcArr[cnt];
                break;
            }       
        }
    }

    if (pNtSrc == NULL) 
    {
        NT_LOG_ERR_2("Cannot find source for id %d, txt %s",
            pNtData->ntSrcId, (pNtData->ntSrcName)?pNtData->ntSrcName:"");
        retV = RV_ERROR_UNKNOWN;
        goto finish;
    }

    pNtData->ntSourceNum = pNtSrc->srcPublicId;
    if (pNtData->ntSrcName == NULL)
        pNtData->ntSrcName = pNtSrc->srcTxt;

    for (cnt = 0; cnt < gNtMgr.listenersCnt; cnt ++)
    {
        RvBool found;

        pNtLstnr = gNtMgr.listenersArr[cnt];
        if (pNtLstnr == NULL)
            continue;

        found = RV_FALSE;

        if (pNtLstnr->bOnlyRegisteredSrc)
        {
            /* need to find among selected */
            RvInt cnt1;

            for (cnt1 = 0; cnt1 < pNtLstnr->regSrcCnt; cnt1 ++)
            {
                if (pNtLstnr->regSrc[cnt1].srcId == pNtData->ntSrcId && 
                    pNtLstnr->regSrc[cnt1].lvl <= pNtData->ntLvl)
                {
                    found = RV_TRUE;
                    break;
                } 
            }
            
            if (!found)
                continue;
        }

        if (pNtLstnr->minLvl > pNtData->ntLvl)
            continue;

        ntfCnt ++;
        NT_LOG_DBG_1("Before calling notify callback for %s",
            (pNtLstnr->name)?pNtLstnr->name:"");

        (pNtLstnr->ntfFunc)(pNtLstnr->ntfAppCtx,pNtLstnr->name,pNtData);

        NT_LOG_DBG_1("After calling notify callback for %s",
            (pNtLstnr->name)?pNtLstnr->name:"");
    }


finish:

    NT_LOG_LEAVE_2("retV %d, notifyCnt %d",retV,ntfCnt);


    RvLockRelease(&gNtMgr.lock, gNtMgr.pLogMgr);

    return retV;
#undef FUNCTION_NAME
}

RvStatus RvNotifierInit(void)
{
    RvStatus retv;
    memset(&gNtMgr,0,sizeof(gNtMgr));
    retv = RvLockConstruct(NULL,&gNtMgr.lock);
    return retv;
}

RvStatus RvNotifierEnd(void)
{
    RvInt cnt;
    RvNtfListener* pNtLstnr;
    RvNotifierSrc* pNtSrc;

    for (cnt = 0; cnt < gNtMgr.listenersCnt; cnt++)
    {
        pNtLstnr = gNtMgr.listenersArr[cnt];
        if (pNtLstnr == NULL)
            break;


        if (pNtLstnr->name)
            RvMemoryFree(pNtLstnr->name,NULL);
        RvMemoryFree(pNtLstnr,NULL);
    }

    for (cnt = 0; cnt < gNtMgr.sourceCnt; cnt++)
    {
        pNtSrc = gNtMgr.srcArr[cnt];
        if (pNtSrc == NULL)
            break;

        if (pNtSrc->srcTxt)
            RvMemoryFree(pNtSrc->srcTxt,NULL);
        RvMemoryFree(pNtSrc,NULL);
    }


    RvLockDestruct(&gNtMgr.lock,NULL);
    return RV_OK;
}

RVCOREAPI
RvStatus RVCALLCONV RvNotifierSetLogMgr(
    RvLogMgr* pLogMgr)
{
    if (gNtMgr.pLogSrc)
    {
        RvLogSourceDestruct(gNtMgr.pLogSrc);
        gNtMgr.pLogSrc = NULL;
        gNtMgr.pLogMgr = NULL;
    }

    if (pLogMgr)
    {
        RvStatus retv;

        retv = RvLogSourceConstruct(pLogMgr,&gNtMgr.logSrc,"NOTIFY","Rv Notifier");
        if (retv != RV_OK)
            return RV_ERROR_UNKNOWN;

        gNtMgr.pLogSrc = &gNtMgr.logSrc;
        gNtMgr.pLogMgr = pLogMgr;
    }

    return RV_OK;
}

RVCOREAPI
const RvChar* RVCALLCONV RvNotifierLvl2Text(
    IN  RvNotificationLvl       lvl)
{
    static char txt[100];
    switch (lvl)
    {
    case RvNtfLvlInfo:
        return "Info";
    case RvNtfLvlWarning:
        return "Warning";
    case RvNtfLvlError:
        return "Error";
    case RvNtfLvlException:
        return "Exception";
    default:
        sprintf(txt,"Ntf:%d",lvl);
        return txt;
    }
}


#if defined(__cplusplus)
}
#endif
