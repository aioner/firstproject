/******************************************************************************
Filename    :rvsdpsesstime.c
Description :session time manipulation routines.

  ******************************************************************************
  Copyright (c) 2005 RADVision Inc.
  ************************************************************************
  NOTICE:
  This document contains information that is proprietary to RADVision LTD.
  No part of this publication may be reproduced in any form whatsoever
  without written prior approval by RADVision LTD..
  
    RADVision LTD. reserves the right to revise this publication and make
    changes without obligation to notify any person of such revisions or
    changes.
    ******************************************************************************
Author:Rafi Kiel
******************************************************************************/
#include <string.h>
#include "rvsdpprivate.h"

/*
 *	Sets the internal RvSdpSessionTime fields to the supplied values.
 *  Returns the 'sessTime' of NULL if fails.
 */
RvSdpSessionTime*
rvSdpSessionTimeFill(RvSdpSessionTime* sessTime, RvUint32 start, RvUint32 end)
{
    sessTime->iStart = start;
    sessTime->iEnd = end;
    return sessTime;

}

/***************************************************************************
 * rvSdpSessionTimeConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpSessionTime object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'sessTime' is NULL the sesion time will be allocated from
 *            the 'msg' pool of session time objectss. If 'msg' is not NULL 
 *            the constructed session time  will be appended to 'msg' list of  
 *            session times.
 *            If the 'msg' is NULL the 'a' allocator will be used.
 *      sessTime - a pointer to valid RvSdpSessionTime object or NULL.
 *      start - the start time of the session.
 *      end - the end time of the session.
 *      badSyn - the proprietary formatted session time field or NULL if standard 
 *               session time is constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'sessTime' must point to valid & constructed
 *                      RvSdpSessionTime object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpSessionTime* 
rvSdpSessionTimeConstruct2(RvSdpMsg* msg, RvSdpSessionTime* sessTime, 
                           RvUint32 start, RvUint32 end, 
                           const char* badSyn, RvAlloc* a, RvBool dontConstruct)
{
    if (!dontConstruct)
    {
        if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
            msg = (RvSdpMsg*)a;

        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed 
               in the RvSdpMsg context */
        {            
            if (sessTime)
                /* the 'sessTime' can't be set it has to be allocated from the msg 
                   session times pool */
                return NULL;
        
            sessTime = rvSdpPoolTake(&msg->iSessTimesPool);
            if (!sessTime)
                /* failed to allocate from the msg emails pool */
                return NULL;
            memset(sessTime,0,sizeof(RvSdpSessionTime));        
            sessTime->iObj = msg;
        }
        else 
        {
        
            if (!sessTime)
                /* the RvSdpSessionTime instance has to be supplied */
                return NULL;
            memset(sessTime,0,sizeof(RvSdpSessionTime));

            /* obsolete API usage:
                no msg context given, will be using allocator */

            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();

            /* save the allocator used */
            sessTime->iObj = a;
        }
    }

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&sessTime->iBadSyntaxField,sessTime->iObj,badSyn) 
                                                                    != RV_SDPSTATUS_OK)
        {
            if (msg && !dontConstruct)
                rvSdpPoolReturn(&msg->iSessTimesPool,sessTime);
            return NULL;
        }
        goto cont;
    }
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    if (badSyn)
        return NULL;
        
    rvSdpSessionTimeFill(sessTime,start,end);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
cont:
#endif

    if (!dontConstruct)
    {
        rvSdpListInitialize(&sessTime->iRepeatList,
            RV_OFFSETOF(RvSdpRepeatInterval,iNextRepeatInterval), 
            (rvListNodeDestructFunc)rvSdpRepeatIntervalDestruct);

        if (msg)
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_TIME,&sessTime->iLineObj,
                        RV_OFFSETOF(RvSdpSessionTime,iLineObj));                
    }
    
    return sessTime;   
}

/***************************************************************************
 * rvSdpSessionTimeConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a session time  object and copies the values from a source 
 *      session time. 
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used 
 *      instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to session time to be constructed. Must point 
 *             to valid memory.
 *      src - a source session time.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpSessionTime* rvSdpSessionTimeConstructCopyA(RvSdpSessionTime* dest, 
                                                 const RvSdpSessionTime* src, 
                                                 RvAlloc* a)
{
    return rvSdpSessionTimeCopy2(dest,src,a,RV_FALSE,RV_TRUE);
}


/*
 *	To allocate the memory for RvSdpSessionTime object (called by the pool)
 *  Return: 
 *      valid RvSdpSessionTime pointer
 *      or NULL if fails
 */
RvSdpSessionTime* rvSdpSessTimeCreateByPool(RvSdpMsg* msg)
{
    RvSdpSessionTime* st;
    
    st = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpSessionTime));
    if (!st)
        return NULL;
    
    memset(st,0,sizeof(RvSdpSessionTime));   
    st->iObj = msg;
    
    return st;
}

/***************************************************************************
 * rvSdpSessionTimeCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpSessionTime from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpSessionTime object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpSessionTime object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 *      copyRepeatIn - if set to true the repeat intervals owned by 'src' will
 *                     be copied as well.
 ***************************************************************************/
RvSdpSessionTime* 
rvSdpSessionTimeCopy2(RvSdpSessionTime* dst, const RvSdpSessionTime* src, 
                      void* obj, RvBool dontConstruct,RvBool copyRepeatIn)
{
    RvSdpSessionTime* t;

    if (!obj && dst && dontConstruct)
        obj = dst->iObj;

    t = rvSdpSessionTimeConstruct2(NULL,dst,src->iStart,src->iEnd,
                RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),obj,dontConstruct);
    if (!t)
        return NULL;
    if (copyRepeatIn)
    {
        rvSdpListClear(&t->iRepeatList);
        if (rvSdpListCopy(&t->iRepeatList,(RvSdpList*)&src->iRepeatList,
                (rvSdpListCopyFunc)rvSdpRepeatIntervalCopyForSessTime,obj) 
                                                                != RV_SDPSTATUS_OK)
        {
            rvSdpSessionTimeDestruct(t);
            return NULL;
        }
    }
    return t;
}

/*
 *	To free the memory for RvSdpSessionTime object (called by the pool)
 */
void rvSdpSessTimeDestroyByPool(RvSdpSessionTime* p)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(p->iObj))->iAllocator,sizeof(RvSdpSessionTime),p);
}

/***************************************************************************
 * rvSdpSessionTimeDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the session time object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
void rvSdpSessionTimeDestruct(RvSdpSessionTime* sessTime)
{
    if (!sessTime->iObj)
        /* cannot deallocate memory */
        return;
    
    rvSdpListClear(&sessTime->iRepeatList);
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&sessTime->iBadSyntaxField,sessTime->iObj);
#endif

    rvSdpLineObjsListRemove(sessTime->iObj,&sessTime->iLineObj);
    
    if (RV_SDP_OBJ_IS_MESSAGE(sessTime))
        rvSdpPoolReturn(&((RvSdpMsg*)(sessTime->iObj))->iSessTimesPool,sessTime);
}

 /***************************************************************************
 * rvSdpSessionTimeAddRepeatInterval2
 * ------------------------------------------------------------------------
 * General: 
 *      Adds new repeat interval to the session time.
 *          
 * Return Value: 
 *      The pointer to added RvSdpRepeatInterval object or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 *      time - the time length of the repeat times.
 *      t_units - the time units of the repeat times.
 *      duration - the length of the active duration.
 *      d_units - the time units of the active duration.
 *      badSyn - the proprietary formatted repeat interval field or NULL if standard 
 *               repeat interval is added.
 **************************************************************************/
RvSdpRepeatInterval* 
rvSdpSessionTimeAddRepeatInterval2(RvSdpRepeatInterval* interv,
                                  RvSdpSessionTime* session,
                                  RvUint32 time,
                                  RvSdpTimeUnit t_units,
                                  RvUint32 duration ,
                                  RvSdpTimeUnit d_units,
                                  const char *badSyn)
{        
    RvSdpMsg* msg;
    msg = RV_SDP_OBJ_IS_MESSAGE(session) ? (RvSdpMsg*)session->iObj:NULL;

    if (!interv)
    {
    
        interv = rvSdpRepeatIntervalConstruct2(msg,NULL,time,t_units,duration,
                                d_units,badSyn,NULL,RV_FALSE);
        if (!interv)
            return NULL;
    }
    rvSdpListTailAdd(&session->iRepeatList,interv);

    return interv;
}

/***************************************************************************
 * rvSdpTimeZoneAdjustConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpTimeZoneAdjust object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'timeZone' is NULL the time zone will be allocated from
 *            the 'msg' pool. If 'msg' is not NULL the constructed
 *            object will be appended to 'msg' list of time zones. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      timeZone - a pointer to valid RvSdpTimeZoneAdjust object or NULL.
 *      t - the time of time shift
 *      offsetTime - the lehgth of the time shift
 *      offsetUnits - the units  of 'offsetTime'
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'timeZone' must point to valid & constructed
 *                      RvSdpTimeZoneAdjust object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpTimeZoneAdjust* 
rvSdpTimeZoneAdjustConstruct2(RvSdpMsg* msg,RvSdpTimeZoneAdjust* timeZone, RvUint32 t,
                              RvInt32 offsetTime, RvSdpTimeUnit offsetUnits, RvAlloc* a,
                              RvBool dontConstruct)
{       
    if (!dontConstruct)
    {
        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed 
               in the RvSdpMsg context */
        {            
            if (timeZone)
                /* the 'timeZone' can't be set it has to be allocated 
                   from the msg time zone adjust pool */
                return NULL;
        
            timeZone = rvSdpPoolTake(&msg->iTimeZonesPool);
            if (!timeZone)
                /* failed to allocate from the msg emails pool */
                return NULL;
            memset(timeZone,0,sizeof(RvSdpTimeZoneAdjust));        
            timeZone->iObj = msg;
        }
        else 
        {
            if (!timeZone)
                /* the RvSdpSessionTime instance has to be supplied */
                return NULL;
            memset(timeZone,0,sizeof(RvSdpTimeZoneAdjust));

            /* obsolete API usage:
                no msg context given, will be using allocator */

            if (!a)
                /* the dault allocator will be used */
                a = rvSdpGetDefaultAllocator();

            /* save the allocator used */
            timeZone->iObj = a;
        }
    }
    rvSdpTimeZoneAdjustSetTime(timeZone,t);
    rvSdpTimeZoneAdjustSetOffsetTime(timeZone,offsetTime);
    rvSdpTimeZoneAdjustSetOffsetUnits(timeZone,offsetUnits);

    if (msg && !msg->iTZA.iLineObj.iUsed)
    {
        /* the first tza inserted */
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_TIME_ZONE_ADJUST,&msg->iTZA.iLineObj,
            RV_OFFSETOF(RvSdpTZA,iLineObj));
    }

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (msg && msg->iTZA.iBadSyntaxField)
        rvSdpUnsetTextField(&msg->iTZA.iBadSyntaxField,msg);
#endif
        
    return timeZone;   
}

/***************************************************************************
 * rvSdpTimeZoneAdjustCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpTimeZoneAdjust from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpTimeZoneAdjust object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpTimeZoneAdjust object or NULL. 
 *      src - a pointer to the source object
 *      msg - the RvSdpMsg instance that will own the destination object.
 ***************************************************************************/
RvSdpTimeZoneAdjust* 
rvSdpTimeZoneAdjustCopy2(RvSdpTimeZoneAdjust* dst, const RvSdpTimeZoneAdjust* src, 
                         RvSdpMsg* dstMsg)
{
    return rvSdpTimeZoneAdjustConstruct2(dstMsg,dst,src->iAdjustmentTime,
                        src->iOffsetTime,src->iOffsetUnits,NULL,RV_FALSE);
}


#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMsgSetBadSyntaxZoneAdjustment
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP time zone adjustment field with a proprietary formatted value.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpMsgSetBadSyntaxZoneAdjustment(RvSdpMsg* msg, const char* badSyn)
{
    if (rvSdpSetTextField(&msg->iTZA.iBadSyntaxField,msg,badSyn) != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;
    
    if (rvSdpMsgTZAIsUsed(msg))
    {
        if (!msg->iTZA.iLineObj.iUsed)
            rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_TIME_ZONE_ADJUST,
                        &msg->iTZA.iLineObj,RV_OFFSETOF(RvSdpTZA,iLineObj));
    }
    else
    {
        if (msg->iTZA.iLineObj.iUsed)
            rvSdpLineObjsListRemove(msg,&msg->iTZA.iLineObj);
    }

    if (msg->iTZA.iBadSyntaxField)
        rvSdpListClear(&msg->iTZA.iTimeZoneList);

    return RV_SDPSTATUS_OK;
}

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/*
 *	To allocate the memory for RvSdpTimeZoneAdjust object (called by the pool)
 *  Return: 
 *      valid RvSdpTimeZoneAdjust pointer
 *      or NULL if fails
 */
RvSdpTimeZoneAdjust* rvSdpTimeZoneCreateByPool(RvSdpMsg* msg)
{
    RvSdpTimeZoneAdjust* tz;
    
    tz = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpTimeZoneAdjust));
    if (!tz)
        return NULL;
    
    memset(tz,0,sizeof(RvSdpTimeZoneAdjust));   
    tz->iObj = msg;
    
    return tz;
}

/***************************************************************************
 * rvSdpTimeZoneAdjustDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the time zone adjustment object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 ***************************************************************************/
void rvSdpTimeZoneAdjustDestruct(RvSdpTimeZoneAdjust* timeZone)
{    
    if (timeZone->iObj && RV_SDP_OBJ_IS_MESSAGE(timeZone))
    {
        RvSdpMsg *msg = (RvSdpMsg*) timeZone->iObj;
        if (!rvSdpMsgTZAIsUsed(msg))
            rvSdpLineObjsListRemove(msg,&msg->iTZA.iLineObj);
        
        /* RvSdpMsg context was used */
        rvSdpPoolReturn(&msg->iTimeZonesPool,timeZone);
    }
}

/*
 *	To free the memory for RvSdpTimeZoneAdjust object (called by the pool)
 */
void rvSdpTimeZoneAdjustDestroyByPool(RvSdpTimeZoneAdjust* p)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(p->iObj))->iAllocator,sizeof(RvSdpTimeZoneAdjust),p);
}

/***************************************************************************
 * rvSdpRepeatIntervalConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRepeatInterval object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'interv' is NULL the repeat interval will be allocated from
 *            the 'msg' pool of repeat intervals. If 'msg' is not NULL the constructed
 *            repeat interv will be appended to 'msg' list of repeat interval. If the 
 *            'msg' is NULL the 'a' allocator will be used.
 *      interv - a pointer to valid RvSdpRepeatInterval object or NULL.
 *      time - the length of repeat interval.
 *      t_units - the units of repeat interval length.
 *      duration - the length of session active duration.
 *      d_units - the units of session active duration length.
 *      badSyn - the proprietary formatted repeat interval field or NULL if 
 *               standard repeat interval is constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'interv' must point to valid & constructed
 *                      RvSdpRepeatInterval object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpRepeatIntervalConstruct2(RvSdpMsg* msg,
                              RvSdpRepeatInterval* interv,                                                                   
                              RvUint32 time,
                              RvSdpTimeUnit t_units,
                              RvUint32 duration,
                              RvSdpTimeUnit d_units,
                              const char* badSyn,
                              RvAlloc* a,
                              RvBool dontConstruct)
{
    if (!dontConstruct)
    {
        if (msg)
            /* the RvSdpMsg is provided, all allocations will be performed 
               in the RvSdpMsg context */
        {            
            if (interv)
                /* the 'interv' can't be set it has to be allocated from the msg 
                   repeat intervals pool */
                return NULL;
        
            {
                interv = rvSdpPoolTake(&msg->iRepeatIntervalPool);
                if (!interv)
                    /* failed to allocate from the msg emails pool */
                    return NULL;
                memset(interv,0,sizeof(RvSdpRepeatInterval));        
                interv->iObj = msg;
            }
        }
        else 
        {        
            if (!a)
                a = rvSdpGetDefaultAllocator();
            if (!interv)
            {
                interv = rvSdpAllocAllocate(a,sizeof(RvSdpRepeatInterval));
                if (!interv)
                    return NULL;
                memset(interv,0,sizeof(RvSdpRepeatInterval));
                interv->iPrivateAllocation = RV_TRUE;
            }
            else
                memset(interv,0,sizeof(RvSdpRepeatInterval));

            /* obsolete API usage:
                no msg context given, will be using allocator */

            /* save the allocator used */
            interv->iObj = a;
        }
    }

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        rvSdpSetTextField(&interv->iBadSyntaxField,interv->iObj,badSyn);
        if (!interv->iBadSyntaxField)
        {
            if (msg && !dontConstruct)
                rvSdpPoolReturn(&msg->iRepeatIntervalPool,interv);
            return NULL;
        }

        goto cont;
    }
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    if (badSyn)
        return NULL;

    interv->iInterval.iTimeValue = time;
    interv->iInterval.iTimeType = t_units;
    interv->iDuration.iTimeValue = duration;
    interv->iDuration.iTimeType = d_units;                    

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
cont:
#endif

    if (!dontConstruct)
    {
        rvSdpListInitialize(&interv->iOffsetsList,
            RV_OFFSETOF(RvSdpTypedTime,iNextTypedTime),
            (rvListNodeDestructFunc)rvSdpTypedTimeDestruct);
    }
    
    if (msg && !dontConstruct)
        rvSdpLineObjsListInsert(msg,SDP_FIELDTYPE_REPEAT,&interv->iLineObj,
                RV_OFFSETOF(RvSdpRepeatInterval,iLineObj));

    return interv;
}

/***************************************************************************
 * rvSdpTypedTimeConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpTypedTime object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL  the typed time will be allocated from
 *            the 'msg' pool of typed times.  If the 
 *            'msg' is NULL the 'a' allocator will be used.
 *      tt - a pointer to valid RvSdpTypedTime object or NULL.
 *      time - the time interval length.
 *      units - the units of 'time'.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpTypedTime*
rvSdpTypedTimeConstruct2(RvSdpMsg* msg,
                         RvSdpTypedTime* tt,                                                                   
                         RvUint32 time,
                         RvSdpTimeUnit units,
                         RvAlloc* a)
{        
    if (msg)
        /* the RvSdpMsg is provided, all allocations will be performed in the 
                RvSdpMsg context */
    {            
        if (tt)
            /* the 'tt' can't be set it has to be allocated from the msg 
               typed times pool */
            return NULL;
        
        tt = rvSdpPoolTake(&msg->iTypedTimePool);
        if (!tt)
            /* failed to allocate from the msg emails pool */
            return NULL;
        memset(tt,0,sizeof(RvSdpTypedTime));        
        tt->iObj = msg;
    }
    else 
    {        
        if (!a)
            /* the dault allocator will be used */
            a = rvSdpGetDefaultAllocator();

        if (!tt)
        {
            tt = rvSdpAllocAllocate(a,sizeof(RvSdpTypedTime));
            if (!tt)
                return NULL;
            memset(tt,0,sizeof(RvSdpTypedTime));
            tt->iPrivateAllocation = RV_TRUE;
        }
        else
            memset(tt,0,sizeof(RvSdpTypedTime));


        /* save the allocator used */
        tt->iObj = a;
    }

    tt->iTimeValue = time;
    tt->iTimeType = units;
    return tt;    
}
/*
 *	Copies 'src' typed time  object to 'dst'.
 */
RvSdpTypedTime*
rvSdpTypedTimeCopy2(RvSdpTypedTime* dst, const RvSdpTypedTime* src, RvSdpMsg* msg)
{
    return rvSdpTypedTimeConstruct2(msg,dst,src->iTimeValue,src->iTimeType,NULL);
}

/*
 *	Sets the internal RvSdpRepeatInterval fields to the supplied values.
 *  Returns the 'dst' of NULL if fails.
 */
RvSdpRepeatInterval* 
rvSdpRepeatIntervalFill(RvSdpRepeatInterval* dst, const RvSdpRepeatInterval* src)
{
	dst->iDuration.iTimeValue = src->iDuration.iTimeValue;
	dst->iDuration.iTimeType = src->iDuration.iTimeType;

	dst->iInterval.iTimeValue = src->iInterval.iTimeValue;
	dst->iInterval.iTimeType = src->iInterval.iTimeType;

    if (rvSdpListCopy(&dst->iOffsetsList,(RvSdpList*)&src->iOffsetsList,
                (rvSdpListCopyFunc)rvSdpTypedTimeCopy2,dst->iObj) != RV_SDPSTATUS_OK)
        return NULL;

	return dst;
}	

/***************************************************************************
 * rvSdpRepeatIntervalCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpRepeatInterval from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *      The destination object will or willl not be constructed depending
 *      on 'dontConstruct' value. If 'dontConstruct' is true the 'dest'
 *      must point to valid & constructed object.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpRepeatInterval object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpRepeatInterval object or NULL. 
 *      src - a pointer to the source object
 *      msg - the RvSdpMsg instance that will own the destination object.
 *      dontConstruct - whether the destination object has to be constructed
 *                      prior to copy.
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpRepeatIntervalCopy2(RvSdpRepeatInterval* dst, const RvSdpRepeatInterval* src, 
                         RvSdpMsg* msg, RvBool dontConstruct)
{
    if (dontConstruct && !msg && dst && RV_SDP_OBJ_IS_MESSAGE(dst))
        msg = dst->iObj;

    dst = rvSdpRepeatIntervalConstruct2(msg,dst,src->iInterval.iTimeValue,
                    src->iInterval.iTimeType,
                    src->iDuration.iTimeValue,src->iDuration.iTimeType,
                    RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),NULL,dontConstruct);
    if (!dst)
        return NULL;

    rvSdpListClear(&dst->iOffsetsList);

    if (rvSdpListCopy(&dst->iOffsetsList,(RvSdpList*)&src->iOffsetsList,
                        (rvSdpListCopyFunc)rvSdpTypedTimeCopy2,msg) != RV_SDPSTATUS_OK)
        goto failure;

    return dst;
    
failure:
    rvSdpRepeatIntervalDestruct(dst);
    return NULL;
    
}

/* copies the 'src' RvSdpRepeatInterval to the 'dst' RvSdpRepeatInterval */
RvSdpRepeatInterval* 
rvSdpRepeatIntervalCopyForSessTime(RvSdpRepeatInterval* dst,
                        const RvSdpRepeatInterval* src, RvSdpMsg* msg)
{
    return rvSdpRepeatIntervalCopy2(dst,src,msg,RV_FALSE);
}


/***************************************************************************
 * rvSdpRepeatIntervalDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the repeat interval object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
void rvSdpRepeatIntervalDestruct(RvSdpRepeatInterval* interv)
{
    if (!interv->iObj)
        /* cannot deallocate memory */
        return;
    
    rvSdpRepeatIntervalClearOffset(interv);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&interv->iBadSyntaxField,interv->iObj);
#endif
    

    rvSdpLineObjsListRemove(interv->iObj,&interv->iLineObj);
        
    if (RV_SDP_OBJ_IS_MESSAGE(interv))
    {
        /* RvSdpMsg context was used */
        rvSdpPoolReturn(&((RvSdpMsg*)(interv->iObj))->iRepeatIntervalPool,interv);
    }
    else if (interv->iPrivateAllocation)
        rvSdpAllocDeallocate(interv->iObj,sizeof(RvSdpRepeatInterval),interv);        
}

/***************************************************************************
 * rvSdpRepeatIntervalAddOffset
 * ------------------------------------------------------------------------
 * General: 
 *      Adds another session start offset time to the repeat interval object.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      time - the session start offset time length.
 *      units - the session start offset time units.
 ***************************************************************************/
RvSdpStatus rvSdpRepeatIntervalAddOffset(RvSdpRepeatInterval* repeat, 
                                         RvUint32 time,
                                         RvSdpTimeUnit units)
{
    RvSdpTypedTime* tt;
    RvSdpMsg* msg;
    
    msg = RV_SDP_OBJ_IS_MESSAGE(repeat) ? (RvSdpMsg*)repeat->iObj:NULL;
    
    tt = rvSdpTypedTimeConstruct2(msg,NULL,time,units,NULL);
    if (!tt)
        return RV_SDPSTATUS_ALLOCFAIL;
    rvSdpListTailAdd(&repeat->iOffsetsList,tt);
    return RV_SDPSTATUS_OK;    
}

/***************************************************************************
 * rvSdpRepeatIntervalGetOffsetTime
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the session start offset time by index.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      i - the index of the offset.
 ***************************************************************************/
RvUint32 rvSdpRepeatIntervalGetOffsetTime(RvSdpRepeatInterval* repeat, RvSize_t i)
{
    RvSdpTypedTime *tt;
    
    tt = rvSdpListGetByIndex(&repeat->iOffsetsList,i);
    if (!tt)
        return 0;
    return tt->iTimeValue;
}

/***************************************************************************
 * rvSdpRepeatIntervalGetOffsetUnits
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the session start offset time units by index.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      i - the index of the offset.
 ***************************************************************************/
RvSdpTimeUnit rvSdpRepeatIntervalGetOffsetUnits(RvSdpRepeatInterval* repeat, RvSize_t i)
{
    RvSdpTypedTime *tt;
    
    tt = rvSdpListGetByIndex(&repeat->iOffsetsList,i);
    if (!tt)
        return RV_SDPTIMETYPE_NOT_SET;
    return tt->iTimeType;
}

/*
 *	returns the time and units of RvSdpTypedTime instance
 */
RvBool rvSdpBreakTT(RvSdpTypedTime *tt, RvUint32* time, RvSdpTimeUnit* t_unit)
{
    if (!tt)
        return RV_FALSE;
    *time = tt->iTimeValue;
    *t_unit = tt->iTimeType;
    return RV_TRUE;
}


#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpBadSyntaxRepeatIntervalConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRepeatInterval object with proprietary format.
 *      This function is obsolete. The rvSdpSessionTimeAddRepeatInterval should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpRepeatInterval object.
 *      badSyn - the proprietary format of repeat interval.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpRepeatIntervalBadSyntaxConstructA(RvSdpRepeatInterval* interv, 
                                       char *badSyntax,
                                       RvAlloc* a)
{
    return rvSdpRepeatIntervalConstruct2(NULL,interv,0,RV_SDPTIMETYPE_NOT_SET,0,
        RV_SDPTIMETYPE_NOT_SET,badSyntax,a,RV_FALSE);
}
#endif /*#if defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/*
 *	destructs (frees the memory or returns to pool) the RvSdpTypedTime instance
 */
void rvSdpTypedTimeDestruct(RvSdpTypedTime *tt)
{
    if (!tt->iObj)
        /* cannot deallocate memory */
        return;
    
    if (RV_SDP_OBJ_IS_MESSAGE(tt))
    {
        /* RvSdpMsg context was used */
        rvSdpPoolReturn(&((RvSdpMsg*)(tt->iObj))->iTypedTimePool,tt);
    }
    else if (tt->iPrivateAllocation)
        rvSdpAllocDeallocate(tt->iObj,sizeof(RvSdpTypedTime),tt);        
}

/*
 *	To allocate the memory for RvSdpRepeatInterval object (called by the pool)
 *  Return: 
 *      valid RvSdpRepeatInterval pointer
 *      or NULL if fails
 */
RvSdpRepeatInterval* rvSdpRepeatIntervalCreateByPool(RvSdpMsg* msg)
{
    RvSdpRepeatInterval* interv;
    
    interv = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpRepeatInterval));
    if (!interv)
        return NULL;
    
    memset(interv,0,sizeof(RvSdpRepeatInterval));   
    interv->iObj = msg;
    
    return interv;
}

/*
 *	To free the memory for RvSdpRepeatInterval object (called by the pool)
 */
void rvSdpRepeatIntervalDestroyByPool(RvSdpRepeatInterval* interv)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(interv->iObj))->iAllocator,
        sizeof(RvSdpRepeatInterval),interv);
}

/*
 *	To allocate the memory for RvSdpTypedTime object (called by the pool)
 *  Return: 
 *      valid RvSdpTypedTime pointer
 *      or NULL if fails
 */
RvSdpTypedTime* rvSdpTypedTimeCreateByPool(RvSdpMsg* msg)
{
    RvSdpTypedTime* tt;
    
    tt = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpTypedTime));
    if (!tt)
        return NULL;
    
    memset(tt,0,sizeof(RvSdpTypedTime));   
    tt->iObj = msg;
    
    return tt;
}

/*
 *	To free the memory for RvSdpTypedTime object (called by the pool)
 */
void rvSdpTypedTimeDestroyByPool(RvSdpTypedTime* tt)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(tt->iObj))->iAllocator,sizeof(RvSdpTypedTime),tt);
}

/***************************************************************************
 * rvSdpMsgTZACopy
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the time zone adjustment field of 'dest' SDP message as the time
 *      zone adjustments of 'src' SDP message. 
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to the destination RvSdpMsg object.
 *      src - a pointer to the source RvSdpMsg object.
 ***************************************************************************/
RvSdpStatus rvSdpMsgTZACopy(RvSdpMsg* dest, const RvSdpMsg* src)
{
    rvSdpMsgTZADestroy(dest);
    if (rvSdpListCopy(&dest->iTZA.iTimeZoneList,(RvSdpList*)&src->iTZA.iTimeZoneList,
                            (rvSdpListCopyFunc)rvSdpTimeZoneAdjustCopy2,dest) 
                                                    != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (rvSdpMsgSetBadSyntaxZoneAdjustment(dest,src->iTZA.iBadSyntaxField) 
                                                            != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpMsgTZADestroy
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the time zone adjustment field of SDP message.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to RvSdpMsg object.
 ***************************************************************************/
void rvSdpMsgTZADestroy(RvSdpMsg* msg)
{
    rvSdpListClear(&msg->iTZA.iTimeZoneList);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpMsgSetBadSyntaxZoneAdjustment(msg,NULL);
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/
}

/***************************************************************************
 * rvSdpMsgTZAIsUsed
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the time zone adjustment field ('z=') of SDP message is set.
 *          
 * Return Value: 
 *      RV_TRUE if there are time zone adjustments set or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to RvSdpMsg object.
 ***************************************************************************/
RvBool rvSdpMsgTZAIsUsed(RvSdpMsg* msg)
{
    if (msg->iTZA.iTimeZoneList.iListSize)
        return RV_TRUE;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    else if (msg->iTZA.iBadSyntaxField)
        return RV_TRUE;
#endif    
    else 
        return RV_FALSE;
}


#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpRepeatIntervalGetFirstOffset
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first offset data defined in the repeat interval object. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value:
 *      RV_TRUE if there is at least one offset in the repeat interval object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      i - pointer to RvSdpListIter to be used for subsequent 
 *          rvSdpRepeatIntervalGetNextOffset calls.
 *      time - will be filled with offset time interval.
 *      t_unit - will be filled with offset's time units.
 ***************************************************************************/
RvBool rvSdpRepeatIntervalGetFirstOffset(RvSdpRepeatInterval* repeat, RvSdpListIter* i, 
                                         RvUint32* time, RvSdpTimeUnit* t_unit)
{
    return rvSdpBreakTT((RvSdpTypedTime*)rvSdpListGetFirst(&repeat->iOffsetsList,i),
                    time,t_unit);
}

/***************************************************************************
 * rvSdpRepeatIntervalGetNextOffset
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next offset data defined in the repeat interval object. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      RV_TRUE if the next offset exists, RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpRepeatIntervalGetFirstOffset function. 
 *      time - will be filled with offset time interval.
 *      t_unit - will be filled with offset's time units.
 ***************************************************************************/
RvBool rvSdpRepeatIntervalGetNextOffset(RvSdpListIter* i, RvUint32* time, 
                                        RvSdpTimeUnit* t_unit)
{
    return rvSdpBreakTT((RvSdpTypedTime*)rvSdpListGetNext(i),time,t_unit);
}

/***************************************************************************
 * rvSdpRepeatIntervalRemoveCurrentOffset
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the repeat interval offset pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpRepeatIntervalRemoveCurrentOffset(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}


/***************************************************************************
 * rvSdpSessionTimeConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpSessionTime object.
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used 
 *      instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to valid RvSdpSessionTime object.
 *      start - the start time of the session.
 *      end - the end time of the session.
 ***************************************************************************/
RvSdpSessionTime* 
rvSdpSessionTimeConstruct(RvSdpSessionTime* sessTime, RvUint32 start, RvUint32 end)
{
    return rvSdpSessionTimeConstruct2(NULL,sessTime,start,end,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpSessionTimeConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpSessionTime object.
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used 
 *      instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to valid RvSdpSessionTime object.
 *      start - the start time of the session.
 *      end - the end time of the session.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpSessionTime* 
rvSdpSessionTimeConstructA(RvSdpSessionTime* sessTime, RvUint32 start, 
                           RvUint32 end, RvAlloc* a)
{
    return rvSdpSessionTimeConstruct2(NULL,sessTime,start,end,NULL,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpSessionTimeConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a session time  object and copies the values from a source 
 *      session time. 
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used 
 *      instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to session time to be constructed. Must point 
 *             to valid memory.
 *      src - a source session time.
 ***************************************************************************/
RvSdpSessionTime* 
rvSdpSessionTimeConstructCopy(RvSdpSessionTime* dest, const RvSdpSessionTime* src)
{
    return rvSdpSessionTimeCopy2(dest,src,NULL,RV_FALSE,RV_TRUE);
}


/***************************************************************************
 * rvSdpSessionTimeConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a session time  object and copies the values from a source 
 *      session time. 
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used 
 *      instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to session time to be constructed. Must point 
 *             to valid memory.
 *      src - a source session time.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpSessionTime* 
rvSdpSessionTimeCopy(RvSdpSessionTime* dest, const RvSdpSessionTime* src)
{
    return rvSdpSessionTimeCopy2(dest,src,NULL,RV_TRUE,RV_TRUE);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxSessionTimeConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpSessionTime object with proprietary format using 
 *      default allocator.
 *      This function is obsolete. The 'rvSdpMsgAddBadSyntaxSessionTime' should 
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpSessionTime object.
 *      badSyn - the proprietary format of session time.
 ***************************************************************************/
RvSdpSessionTime* 
rvSdpBadSyntaxSessionTimeConstruct(RvSdpSessionTime* sessTime, const char* badSyn)
{
    return rvSdpSessionTimeConstruct2(NULL,sessTime,0,0,badSyn,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpBadSyntaxSessionTimeConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpSessionTime object with proprietary format using 
 *      provided allocator.
 *      This function is obsolete. The 'rvSdpMsgAddBadSyntaxSessionTime' should 
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpSessionTime object.
 *      badSyn - the proprietary format of session time.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpSessionTime* 
rvSdpBadSyntaxSessionTimeConstructA(RvSdpSessionTime* sessTime, 
                                    const char* badSyn, RvAlloc* a)
{
    return rvSdpSessionTimeConstruct2(NULL,sessTime,0,0,badSyn,a,RV_FALSE);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpSessionTimeGetStart
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the session start time.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to RvSdpSessionTime object. 
 ***************************************************************************/
RvUint32 rvSdpSessionTimeGetStart(RvSdpSessionTime* sessTime)
{
    return sessTime->iStart;
}

/***************************************************************************
 * rvSdpSessionTimeSetStart
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the session start time.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to RvSdpSessionTime object. 
 *      start - the new session start time.
 ***************************************************************************/
void rvSdpSessionTimeSetStart(RvSdpSessionTime* sessTime, RvUint32 start)
{
    sessTime->iStart = start;
}

/***************************************************************************
 * rvSdpSessionTimeGetEnd
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the session end time.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to RvSdpSessionTime object. 
 ***************************************************************************/
RvUint32 rvSdpSessionTimeGetEnd(RvSdpSessionTime* sessTime)
{
    return sessTime->iEnd;
}

/***************************************************************************
 * rvSdpSessionTimeSetEnd
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the session end time.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to RvSdpSessionTime object. 
 *      end - the new session end time.
 ***************************************************************************/
void rvSdpSessionTimeSetEnd(RvSdpSessionTime* sessTime, RvUint32 end)
{
    sessTime->iEnd = end;
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpSessionTimeIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the session time field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
RvBool rvSdpSessionTimeIsBadSyntax(RvSdpSessionTime* sessTime)
{
    return (sessTime->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpSessionTimeGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted session time field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
const char* rvSdpSessionTimeGetBadSyntax(const RvSdpSessionTime* sessTime)
{
    return RV_SDP_EMPTY_STRING(sessTime->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpSessionTimeAddBadSyntaxRepeatInterval
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the new proprietary formatted RvSdpRepeatInterval object  
 *      to the session time.
 *          
 * Return Value: 
 *      Returns the pointer to the newly created RvSdpRepeatInterval object if the 
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 *      badSyn - the proprietary value of session time field
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpSessionTimeAddBadSyntaxRepeatInterval(RvSdpSessionTime* session, const char* badSyn)
{
    return rvSdpSessionTimeAddRepeatInterval2(NULL,session,0,RV_SDPTIMETYPE_NOT_SET,
        0,RV_SDPTIMETYPE_NOT_SET,badSyn);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */


/***************************************************************************
 * rvSdpSessionTimeGetNumOfRepeatInterval
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of repeat intervals of session time object.
 *          
 * Return Value: 
 *      Number of repeat intervals defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
RvSize_t rvSdpSessionTimeGetNumOfRepeatInterval(const RvSdpSessionTime* session)
{
    return session->iRepeatList.iListSize;
}

/***************************************************************************
* rvSdpSessionTimeAddRepeatInterval
* ------------------------------------------------------------------------
* General: 
*      Adds new repeat interval to the session time.
*          
* Return Value: 
*      The pointer to added RvSdpRepeatInterval object or NULL if the function fails.
* ------------------------------------------------------------------------
* Arguments:
*      session - a pointer to the RvSdpSessionTime object.
*      time - the time length of the repeat times.
*      t_units - the time units of the repeat times.
*      duration - the length of the active duration.
*      d_units - the time units of the active duration.
**************************************************************************/
RvSdpRepeatInterval* 
rvSdpSessionTimeAddRepeatInterval(RvSdpSessionTime* session,
                                  RvUint32 time,
                                  RvSdpTimeUnit t_units,
                                  RvUint32 duration ,
                                  RvSdpTimeUnit d_units)
{
    return rvSdpSessionTimeAddRepeatInterval2(NULL,session,time,t_units,
        duration,d_units,NULL);
} 

/***************************************************************************
* rvSdpSessionTimeGetRepeatInterval
* ------------------------------------------------------------------------
* General: 
*      Gets a repeat interval object by index (in session time context). 
*          
* Return Value: 
*      The requested RvSdpRepeatInterval pointer.
* ------------------------------------------------------------------------
* Arguments:
*      session - a pointer to the RvSdpSessionTime object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by correspondent 
*              rvSdpSessionTimeGetNumOfRepeatInterval() call. 
***************************************************************************/
RvSdpRepeatInterval* 
rvSdpSessionTimeGetRepeatInterval(const RvSdpSessionTime* session,RvSize_t i)
{
    return rvSdpListGetByIndex(&session->iRepeatList,i);
}

/***************************************************************************
 * rvSdpSessionTimeClearRepeatIntervals
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all repeat intervals set in session time object.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
void rvSdpSessionTimeClearRepeatIntervals(RvSdpSessionTime* session)
{
    rvSdpListClear(&session->iRepeatList);
}

/***************************************************************************
 * rvSdpSessionTimeGetFirstRepeatInterval
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the first repeat interval object defined in the session time. 
 *      Also sets the list iterator for the further use.
 *          
 * Return Value: 
 *      Pointer to the RvSdpRepeatInterval  object or the NULL pointer if there are no
 *      repeat intervals defined in the session time object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 *      iter - pointer to RvSdpListIter to be used for subsequent 
 *             rvSdpSessionTimeGetNextRepeatInterval calls
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpSessionTimeGetFirstRepeatInterval(RvSdpSessionTime* session, RvSdpListIter* iter)
{
    return (RvSdpRepeatInterval*)rvSdpListGetFirst(&session->iRepeatList,iter);
}

/***************************************************************************
 * rvSdpSessionTimeGetNextRepeatInterval
 * ------------------------------------------------------------------------
 * General: 
 *      Returns the next repeat interval object defined in the session time. 
 *      The 'next' object is defined based on the list iterator state.
 *          
 * Return Value: 
 *      Pointer to the RvSdpRepeatInterval object or the NULL pointer if there is no
 *      more repeat intervals defined in the session time.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpSessionTime(GetFirst/Next)RepeatInterval function. 
 ***************************************************************************/
RvSdpRepeatInterval* rvSdpSessionTimeGetNextRepeatInterval(RvSdpListIter* iter)
{
    return (RvSdpRepeatInterval*)rvSdpListGetNext(iter);
}

/***************************************************************************
 * rvSdpSessionTimeRemoveCurrentRepeatInterval
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the repeat interval object pointed by list iterator. 
 *      The value of iterator becomes undefined after the function call.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function. 
 ***************************************************************************/
void rvSdpSessionTimeRemoveCurrentRepeatInterval(RvSdpListIter* iter)
{
    rvSdpListRemoveCurrent(iter);
}

/***************************************************************************
 * rvSdpSessionTimeRemoveRepeatInterval
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the repeat interval object by index in the
 *      context of session time.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpSessionTimeGetNumOfRepeatInterval call. 
 ***************************************************************************/
void rvSdpSessionTimeRemoveRepeatInterval(RvSdpSessionTime* session, RvSize_t index)
{
    rvSdpListRemoveByIndex(&session->iRepeatList,index);
}
         
/***************************************************************************
 * rvSdpTimeZoneAdjustConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpTimeZoneAdjust object.
 *      This function is obsolete. The 'rvSdpMsgTimeAddZoneAdjustment' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to valid RvSdpTimeZoneAdjust object.
 *      t - when the time shift happens
 *      offsetTime - the offset time.
 *      offsetUnits  - the units of the offset.
 ***************************************************************************/
RvSdpTimeZoneAdjust* 
rvSdpTimeZoneAdjustConstruct(RvSdpTimeZoneAdjust* timeZone, RvUint32 t, 
                             RvInt32 offsetTime, RvSdpTimeUnit offsetUnits)
{
    return rvSdpTimeZoneAdjustConstruct2(NULL,timeZone,t,offsetTime,offsetUnits,
                NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpTimeZoneAdjustConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpTimeZoneAdjust object.
 *      This function is obsolete. The 'rvSdpMsgTimeAddZoneAdjustment' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to valid RvSdpTimeZoneAdjust object.
 *      t - when the time shift happens
 *      offsetTime - the offset time.
 *      offsetUnits  - the units of the offset.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpTimeZoneAdjust* 
rvSdpTimeZoneAdjustConstructA(RvSdpTimeZoneAdjust* timeZone, RvUint32 t, 
                              RvInt32 offsetTime, RvSdpTimeUnit offsetUnits, RvAlloc* a)
{
    return rvSdpTimeZoneAdjustConstruct2(NULL,timeZone,t,offsetTime,offsetUnits,
                                         a,RV_FALSE);
}

/***************************************************************************
 * rvSdpTimeZoneAdjustConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a time zone adjustment object and copies the values from
 *      a source time zone adjustment. 
 *      This function is obsolete. The 'rvSdpMsgTimeAddZoneAdjustment' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to time zone adjustment to be constructed. Must point 
 *             to valid memory.
 *      src - a source time zone adjustment.
 ***************************************************************************/
RvSdpTimeZoneAdjust* 
rvSdpTimeZoneAdjustConstructCopy(RvSdpTimeZoneAdjust* dest, 
                                 const RvSdpTimeZoneAdjust* src)
{
    return rvSdpTimeZoneAdjustConstruct2(NULL,dest,src->iAdjustmentTime,
                    src->iOffsetTime,src->iOffsetUnits,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpTimeZoneAdjustConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a time zone adjustment object and copies the values from
 *      a source time zone adjustment. 
 *      This function is obsolete. The 'rvSdpMsgTimeAddZoneAdjustment' should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to time zone adjustment to be constructed. Must point 
 *             to valid memory.
 *      src - a source time zone adjustment.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpTimeZoneAdjust* 
rvSdpTimeZoneAdjustConstructCopyA(RvSdpTimeZoneAdjust* dest, 
                                  const RvSdpTimeZoneAdjust* src, RvAlloc* a)
{
    return rvSdpTimeZoneAdjustConstruct2(NULL,dest,src->iAdjustmentTime,
                    src->iOffsetTime,src->iOffsetUnits,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpTimeZoneAdjustCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source time zone adjustment object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination time zone adjustment. Must point 
 *             to constructed RvSdpTimeZoneAdjust object.
 *      src - a time zone adjustment object.
 ***************************************************************************/
RvSdpTimeZoneAdjust* rvSdpTimeZoneAdjustCopy(RvSdpTimeZoneAdjust* dest, 
                                             const RvSdpTimeZoneAdjust* src)
{
    return rvSdpTimeZoneAdjustConstruct2(NULL,dest,src->iAdjustmentTime,
        src->iOffsetTime,src->iOffsetUnits,NULL,RV_TRUE);
}

/***************************************************************************
 * rvSdpTimeZoneAdjustGetTime
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the time of time-shift event.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 ***************************************************************************/
RvUint32 rvSdpTimeZoneAdjustGetTime(const RvSdpTimeZoneAdjust* timeZone)
{
    return timeZone->iAdjustmentTime;
}

/***************************************************************************
 * rvSdpTimeZoneAdjustSetTime
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the time of time-shift event.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 *      t - the time-shift event time.
 ***************************************************************************/
void rvSdpTimeZoneAdjustSetTime(RvSdpTimeZoneAdjust* timeZone, RvUint32 t)
{
    timeZone->iAdjustmentTime = t;
}

/***************************************************************************
 * rvSdpTimeZoneAdjustGetOffsetTime
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the length of time-shift.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 ***************************************************************************/
RvInt32 rvSdpTimeZoneAdjustGetOffsetTime(const RvSdpTimeZoneAdjust* timeZone)
{
    return timeZone->iOffsetTime;
}

/***************************************************************************
 * rvSdpTimeZoneAdjustSetOffsetTime
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the length of time-shift.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 *      offsetTime - the new offset length.
 ***************************************************************************/
void rvSdpTimeZoneAdjustSetOffsetTime(RvSdpTimeZoneAdjust* timeZone, RvInt32 offsetTime)
{
    timeZone->iOffsetTime = offsetTime;
}

/***************************************************************************
 * rvSdpTimeZoneAdjustGetOffsetUnits
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the units of time-shift length.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 ***************************************************************************/
RvSdpTimeUnit rvSdpTimeZoneAdjustGetOffsetUnits(const RvSdpTimeZoneAdjust* timeZone)
{
    return timeZone->iOffsetUnits;
}

/***************************************************************************
 * rvSdpTimeZoneAdjustSetOffsetUnits
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the units of time-shift length.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 *      offsetUnits - the new offset length units.
 ***************************************************************************/
void rvSdpTimeZoneAdjustSetOffsetUnits(RvSdpTimeZoneAdjust* timeZone, 
                                       RvSdpTimeUnit offsetUnits)
{
    timeZone->iOffsetUnits = offsetUnits;
}

/***************************************************************************
 * rvSdpRepeatIntervalConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRepeatInterval object.
 *      This function is obsolete. The rvSdpSessionTimeAddRepeatInterval should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to valid RvSdpRepeatInterval object.
 *      time - the length of repeat interval.
 *      t_units - the units of repeat interval length.
 *      duration - the length of session active duration.
 *      d_units - the units of session active duration length.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpRepeatIntervalConstructA(RvSdpRepeatInterval* interv,                                                                   
                              RvUint32 time,
                              RvSdpTimeUnit t_units,
                              RvUint32 duration,
                              RvSdpTimeUnit d_units,
                              RvAlloc* a)
{
    return rvSdpRepeatIntervalConstruct2(NULL,interv,time,t_units,duration,
        d_units,NULL,a,RV_FALSE);
}

/***************************************************************************
 * rvSdpRepeatIntervalConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRepeatInterval object.
 *      This function is obsolete. The rvSdpSessionTimeAddRepeatInterval should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to valid RvSdpRepeatInterval object.
 *      time - the length of repeat interval.
 *      t_units - the units of repeat interval length.
 *      duration - the length of session active duration.
 *      d_units - the units of session active duration length.
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpRepeatIntervalConstruct(RvSdpRepeatInterval* interv,
                             RvUint32 time,
                             RvSdpTimeUnit t_units,
                             RvUint32 duration,
                             RvSdpTimeUnit d_units)
{
    return rvSdpRepeatIntervalConstruct2(NULL,interv,time,t_units,duration,
        d_units,NULL,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpRepeatIntervalConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a repeat interval object and copies the values from a source 
 *      repeat interval. 
 *      This function is obsolete. The rvSdpSessionTimeAddRepeatInterval should
 *      be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to repeat interval to be constructed. Must point 
 *             to valid memory.
 *      src - a source repeat interval.
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpRepeatIntervalConstructCopy(RvSdpRepeatInterval *d,
                                 const RvSdpRepeatInterval *s,
                                 RvAlloc* alloc)
{
    RV_UNUSED_ARG(alloc);
    return rvSdpRepeatIntervalCopy2(d,s,NULL,RV_FALSE);
}

/***************************************************************************
 * rvSdpRepeatIntervalCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source repeat interval object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination repeat interval. Must point 
 *             to constructed RvSdpRepeatInterval object.
 *      src - a repeat interval object.
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpRepeatIntervalCopy(RvSdpRepeatInterval *d, const RvSdpRepeatInterval *s)
{
    return rvSdpRepeatIntervalCopy2(d,s,NULL,RV_TRUE);
}

/***************************************************************************
 * rvSdpRepeatIntervalGetDurationUnits
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the session active duration units.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
RvSdpTimeUnit rvSdpRepeatIntervalGetDurationUnits(const RvSdpRepeatInterval* interv)
{
    return interv->iDuration.iTimeType;
}

/***************************************************************************
 * rvSdpRepeatIntervalSetDurationUnits
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the session active duration units.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 *      unit - the session active duration units.
 ***************************************************************************/
void rvSdpRepeatIntervalSetDurationUnits(RvSdpRepeatInterval* interv, RvSdpTimeUnit unit)
{
    interv->iDuration.iTimeType = unit;
}

/***************************************************************************
 * rvSdpRepeatIntervalGetDurationTime
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the session active duration time length.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
RvUint32 rvSdpRepeatIntervalGetDurationTime(const RvSdpRepeatInterval* interv)
{
    return interv->iDuration.iTimeValue;
}

/***************************************************************************
 * rvSdpRepeatIntervalSetDurationTime
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the session active duration time length.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 *      time - the session active duration time length.
 ***************************************************************************/
void rvSdpRepeatIntervalSetDurationTime(RvSdpRepeatInterval* interv, RvUint32 time)
{
    interv->iDuration.iTimeValue = time;    
}

/***************************************************************************
 * rvSdpRepeatIntervalGetIntervalUnits
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the session repeat interval time units.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
RvSdpTimeUnit rvSdpRepeatIntervalGetIntervalUnits(const RvSdpRepeatInterval* interv)
{
    return interv->iInterval.iTimeType;
}

/***************************************************************************
 * rvSdpRepeatIntervalSetIntervalUnits
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the session repeat interval length units.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 *      unit - the session repeat interval length units.
 ***************************************************************************/
void rvSdpRepeatIntervalSetIntervalUnits(RvSdpRepeatInterval* interv, RvSdpTimeUnit unit)
{
    interv->iInterval.iTimeType = unit;
}

/***************************************************************************
 * rvSdpRepeatIntervalGetIntervalTime
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the session repeat interval time length.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
RvUint32 rvSdpRepeatIntervalGetIntervalTime(const RvSdpRepeatInterval* interv)
{
    return interv->iInterval.iTimeValue;
}

/***************************************************************************
 * rvSdpRepeatIntervalSetIntervalTime
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the session repeat interval time length.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 *      time - the session repeat interval time length.
 ***************************************************************************/
void rvSdpRepeatIntervalSetIntervalTime(RvSdpRepeatInterval* interv, RvUint32 time)
{
    interv->iInterval.iTimeValue = time;
}

/***************************************************************************
 * rvSdpRepeatIntervalRemoveOffset
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the repeat interval offset by index.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpRepeatIntervalGetNumOfOffset call. 
 ***************************************************************************/
void rvSdpRepeatIntervalRemoveOffset(RvSdpRepeatInterval* repeat, RvSize_t index)
{
    rvSdpListRemoveByIndex(&repeat->iOffsetsList,index);
}

/***************************************************************************
 * rvSdpRepeatIntervalClearOffset
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all offsets set in repeat interval object.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
void rvSdpRepeatIntervalClearOffset(RvSdpRepeatInterval* repeat)
{
    rvSdpListClear(&repeat->iOffsetsList);
}

/***************************************************************************
 * rvSdpRepeatIntervalGetNumOfOffset
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of offsets in repeat interval object.
 *          
 * Return Value: 
 *      Number of offsets defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
RvSize_t rvSdpRepeatIntervalGetNumOfOffset(RvSdpRepeatInterval* repeat)
{
    return repeat->iOffsetsList.iListSize;
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxRepeatIntervalConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRepeatInterval object with proprietary format.
 *      This function is obsolete. The rvSdpSessionTimeAddBadSyntaxRepeatInterval 
 *      should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpRepeatInterval object.
 *      badSyn - the proprietary format of repeat interval.
 ***************************************************************************/
RvSdpRepeatInterval* 
rvSdpRepeatIntervalBadSyntaxConstruct(RvSdpRepeatInterval* interv,
                                      char *badSyntax)
{
    return rvSdpRepeatIntervalBadSyntaxConstructA(interv,badSyntax,NULL);
}

/***************************************************************************
 * rvSdpRepeatIntervalIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the session repeat interval field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
RvBool rvSdpRepeatIntervalIsBadSyntax(RvSdpRepeatInterval* interv)
{
    return (interv->iBadSyntaxField) ? RV_TRUE : RV_FALSE;
}

/***************************************************************************
 * rvSdpRepeatIntervalGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted repeat interval field value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
const char* rvSdpRepeatIntervalGetBadSyntax(RvSdpRepeatInterval* interv)
{
    return RV_SDP_EMPTY_STRING(interv->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpMsgGetBadSyntaxZoneAdjustment
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted time zone adjustment field of SDP message
 *      or empty string ("") if the value is either legal or is not set. 
 *          
 * Return Value: 
 *      Gets the proprietary formatted time zone adjustment field of SDP message
 *      or empty string ("") if the value is either legal or is not set. 
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
const char* rvSdpMsgGetBadSyntaxZoneAdjustment(RvSdpMsg* msg)
{
    return RV_SDP_EMPTY_STRING(msg->iTZA.iBadSyntaxField);
}

/***************************************************************************
 * rvSdpMsgIsBadSyntaxZoneAdjustment
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the time zone adjustment field of SDP message
 *      is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RvBool rvSdpMsgIsBadSyntaxZoneAdjustment(RvSdpMsg* msg)
{
    return (msg->iTZA.iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpSessionTimeSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP session time field value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpSessionTime object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpSessionTimeSetBadSyntax(RvSdpSessionTime* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

/***************************************************************************
 * rvSdpRepeatIntervalSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP repeat interval field value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpRepeatInterval object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpRepeatIntervalSetBadSyntax(RvSdpRepeatInterval* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /* RV_SDP_CHECK_BAD_SYNTAX*/



#endif /*#ifndef RV_SDP_USE_MACROS*/

