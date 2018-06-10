/******************************************************************************
Filename    :rvsdpmsgreparse.c
Description : reparse routines.

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
#include "rvsdp.h"
#include "rvsdpprsutils.h"
#include "rvsdpprivate.h"


#ifdef RV_SDP_ENABLE_REPARSE
    
#define REPARSE_END \
    else \
    {\
        if (pD.iCriticalError)\
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;\
        else\
            *stat = RV_SDPPARSER_STOP_ERROR;\
        p = NULL;\
    }\
    rvSdpMsgDestruct(&nmsg);\
    return p;

/*
 *	Performs initial actions for almost all reparse functions.
 *  Construct temp. RvSdpMsg object and set it within the parser data.
 *  Set the objects bad syntax field as the parser  input buffer.
 */ 
RvBool rvSdpReparseInit(
            RvBool isBadSyntax,     /* whether there is bad syntax input */
            char* badSyntax,        /* the bad syntax field of reparsed object */
            RvSdpParseStatus* stat, /* the reparse result status */
            RvSdpParserData* pD,    /* parser data */
            RvSdpMsg* msg,          /* the temp. message */
            RvAlloc* a)             /* allocator */
{
    if (!isBadSyntax)
    {
        *stat = RV_SDPPARSER_STOP_ERROR;
        return RV_FALSE;
    }
    if (rvSdpMsgConstruct2(msg,a) == NULL)
    {
        *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        return RV_FALSE;
    }
    *stat = RV_SDPPARSER_STOP_ZERO;
    memset(pD,0,sizeof(RvSdpParserData));
    pD->iCurrValue = badSyntax;
    pD->iCurrValueLen = (RvUint16) strlen(badSyntax);
	pD->iMsg = msg;
    return RV_TRUE;    
}

/***************************************************************************
 * rvSdpConnectionReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpConnection object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the connection field
 *      becomes valid SDP object. Use rvSdpConnectionGetBadSyntax and
 *      rvSdpConnectionSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpConnection  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the connection object is contained.
 *      c - pointer to reparse RvSdpConnection object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpConnection* rvSdpConnectionReparse(RvSdpMsg* msg, RvSdpConnection* p, 
										int* len, RvSdpParseStatus* stat,
										RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);

    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
                            &pD,&nmsg,a))
        return NULL;
        
    retV = rvSdpParseConnection(&pD,RV_FALSE,RV_FALSE);
    if (retV)
	{
        /* the parse was successful */
        RvSdpConnection* nc;
        /* get the connection object */
        nc = rvSdpMsgGetConnection(&nmsg);
        /* and copy its values to the 'p' connection */
		p = rvSdpConnectionFill(p,nc->iAddress,nc->iAddrTypeStr,
                            nc->iNetTypeStr,nc->iNumAddr,nc->iTtl);
		if (!p)
			*stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpConnectionSetBadSyntax(p,NULL);
	}
    REPARSE_END
}

/***************************************************************************
 * rvSdpBandwidthReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpBandwidth object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the bandwidth field
 *      becomes valid SDP object. Use rvSdpBandwidthGetBadSyntax and
 *      rvSdpBandwidthSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpBandwidth  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the bandwidth object is contained.
 *      c - pointer to reparse RvSdpBandwidth object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpBandwidth* rvSdpBandwidthReparse(RvSdpMsg* msg, RvSdpBandwidth* p, 
                                        int* len,RvSdpParseStatus* stat,
                                        RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);

    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
        &pD,&nmsg,a))
        return NULL;
    retV = rvSdpParseBandwidth(&pD,RV_FALSE,RV_FALSE);
    if (retV)
    {
        RvSdpBandwidth* nb;
        //nb = (RvSdpBandwidth*)rvSdpListPullTail(&nmsg.iCommonFields.iBandwidthList);
        nb = rvSdpMsgGetBandwidth(&nmsg);
        p = rvSdpBandwidthFill(p,nb->iBWType,nb->iBWValue);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpBandwidthSetBadSyntax(p,NULL);
    }
    REPARSE_END
}


/***************************************************************************
 * rvSdpOriginReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpOrigin object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the origin field
 *      becomes valid SDP object. Use rvSdpOriginGetBadSyntax and
 *      rvSdpOriginSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpOrigin  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the origin object is contained.
 *      c - pointer to reparse RvSdpOrigin object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpOrigin* rvSdpOriginReparse(RvSdpMsg* msg, RvSdpOrigin* p, 
                                int* len,RvSdpParseStatus* stat,
                                RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);

    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
        &pD,&nmsg,a))
        return NULL;
    retV = rvSdpParseOrigin(&pD,RV_FALSE,RV_FALSE);
    if (retV)
    {
        RvSdpOrigin* np;
        np = &nmsg.iOrigin;
        p = rvSdpOriginFill(p,np->iUserName,np->iSessionId,np->iVersion,
            np->iNetTypeStr,np->iAddrTypeStr,np->iAddress);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpOriginSetBadSyntax(p,NULL);
    }
    REPARSE_END
}

/***************************************************************************
 * rvSdpEmailReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpEmail object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the email field
 *      becomes valid SDP object. Use rvSdpEmailGetBadSyntax and
 *      rvSdpEmailSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpEmail  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the email object is contained.
 *      c - pointer to reparse RvSdpEmail object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpEmail* rvSdpEmailReparse(RvSdpMsg* msg, RvSdpEmail* p, 
                              int* len,RvSdpParseStatus* stat,
                              RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);

    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
        &pD,&nmsg,a))
        return NULL;
    retV = rvSdpParseEmail(&pD,RV_FALSE,RV_FALSE);
    if (retV)
    {
        RvSdpEmail* np;
        //np = (RvSdpEmail*)rvSdpListPullTail(&nmsg.iEmailList);
        np = rvSdpMsgGetEmail(&nmsg,0);
        p = rvSdpEmailFill(p,np->iAddress,np->iText,np->iSeparSymbol);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpEmailSetBadSyntax(p,NULL);
    }
    REPARSE_END
}

/***************************************************************************
 * rvSdpPhoneReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpPhone object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the phone field
 *      becomes valid SDP object. Use rvSdpPhoneGetBadSyntax and
 *      rvSdpPhoneSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpPhone  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the phone object is contained.
 *      c - pointer to reparse RvSdpPhone object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpPhone* rvSdpPhoneReparse(RvSdpMsg* msg, RvSdpPhone* p, 
                              int* len,RvSdpParseStatus* stat,
                              RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);

    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
                            &pD,&nmsg,a))
        return NULL;
    retV = rvSdpParsePhone(&pD,RV_FALSE,RV_FALSE);
    if (retV)
    {
        RvSdpPhone* np;
        //np = (RvSdpPhone*)rvSdpListPullTail(&nmsg.iPhoneList);
        np = rvSdpMsgGetPhone(&nmsg,0);
        p = rvSdpPhoneFill(p,np->iPhoneNumber,np->iText,np->iSeparSymbol);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpPhoneSetBadSyntax(p,NULL);
    }
    REPARSE_END
}


/***************************************************************************
 * rvSdpSessionTimeReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpSessionTime object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the session time field
 *      becomes valid SDP object. Use rvSdpSessionTimeGetBadSyntax and
 *      rvSdpSessionTimeSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpSessionTime  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the session time object is contained.
 *      c - pointer to reparse RvSdpSessionTime object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpSessionTime* rvSdpSessionTimeReparse(RvSdpMsg* msg, RvSdpSessionTime* p, 
                                      int* len,RvSdpParseStatus* stat,
                                      RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);
    
    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
                            &pD,&nmsg,a))
        return NULL;
    retV = rvSdpParseTime(&pD,RV_FALSE,RV_FALSE);
    if (retV)
    {
        RvSdpSessionTime* np;
        np = (RvSdpSessionTime*)rvSdpListPullTail(&nmsg.iSessionTimeList);
        //np = rvSdpMsgGetSessionTime(&nmsg,0);
        p = rvSdpSessionTimeFill(p,np->iStart,np->iEnd);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpSessionTimeSetBadSyntax(p,NULL);
        rvSdpSessionTimeDestruct(np);
    }
    REPARSE_END
}

/***************************************************************************
 * rvSdpRepeatIntReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpRepeatInterval object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the repeat interval field
 *      becomes valid SDP object. Use rvSdpRepeatIntervalGetBadSyntax and
 *      rvSdpRepeatIntervalSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpRepeatInterval  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the repeat interval object is contained.
 *      c - pointer to reparse RvSdpRepeatInterval object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpRepeatInterval* rvSdpRepeatIntReparse(RvSdpMsg* msg, RvSdpRepeatInterval* p, 
                              int* len,RvSdpParseStatus* stat,
                              RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;
    RvSdpSessionTime* t;
    
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);
    

    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
                            &pD,&nmsg,a))
        return NULL;
	t = rvSdpMsgAddSessionTime(&nmsg,0,0);
    if (!t)
    {
        *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        return  NULL;
    }
    pD.iSessionTime = t;
        
    retV = rvSdpParseTimeRepeat(&pD,RV_FALSE,RV_FALSE);
    if (retV)        
    {
        RvSdpRepeatInterval* np;
        np = rvSdpSessionTimeGetRepeatInterval(t,0);
        p = rvSdpRepeatIntervalFill(p,np);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpRepeatIntervalSetBadSyntax(p,NULL);
    }
    REPARSE_END
}

/***************************************************************************
 * rvSdpKeyReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpKey object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the key field
 *      becomes valid SDP object. Use rvSdpKeyGetBadSyntax and
 *      rvSdpKeySetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpKey  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the key object is contained.
 *      c - pointer to reparse RvSdpKey object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpKey* rvSdpKeyReparse(RvSdpMsg* msg, RvSdpKey* p, 
                          int* len,RvSdpParseStatus* stat,
                          RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);
    
    
    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
        &pD,&nmsg,a))
        return NULL;
    retV = rvSdpParseKey(&pD,RV_FALSE,RV_FALSE);
    if (retV)
    {
        RvSdpKey* np;
        np = &nmsg.iCommonFields.iKey;
        p = rvSdpKeyFill(p,np->iTypeStr,np->iData);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpKeySetBadSyntax(p,NULL);
    }
    REPARSE_END
}

/***************************************************************************
 * rvSdpMediaDescrReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpMediaDescr object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the media descriptor field
 *      becomes valid SDP object. Use rvSdpMediaDescrGetBadSyntax and
 *      rvSdpMediaDescrSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpMediaDescr  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the media descriptor object is contained.
 *      c - pointer to reparse RvSdpMediaDescr object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpMediaDescr* rvSdpMediaDescrReparse(RvSdpMsg* msg, RvSdpMediaDescr* p, 
                              int* len,RvSdpParseStatus* stat,
                              RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);
    
    
    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
							&pD,&nmsg,a))
        return NULL;
    retV = rvSdpParseMediaDescr(&pD,RV_FALSE,RV_FALSE);
    if (retV)
    {
        RvSdpMediaDescr* np;
        //np = (RvSdpMediaDescr*)rvSdpListPullTail(&nmsg.iMediaDescriptors);
        np = rvSdpMsgGetMediaDescr(&nmsg,0);
        p = rvSdpMediaDescrFill(p,np);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            rvSdpMediaDescrSetBadSyntax(p,NULL);
    }
    REPARSE_END
}

/***************************************************************************
 * rvSdpRtpMapReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of RvSdpRtpMap object. The object's bad syntax 
 *      is used for reparse. If the reparse succeeds the RTP map field
 *      becomes valid SDP object. Use rvSdpRtpMapGetBadSyntax and
 *      rvSdpRtpMapSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *          
 * Return Value: 
 *      Pointer to the RvSdpRtpMap  in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the RTP map object is contained.
 *      c - pointer to reparse RvSdpRtpMap object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RvSdpRtpMap* rvSdpRtpMapReparse(RvSdpMsg* msg, RvSdpRtpMap* p, 
                              int* len,RvSdpParseStatus* stat,
                              RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);
    
    
    if (!rvSdpReparseInit((p->iBadSyntaxField!=NULL),p->iBadSyntaxField,stat,
							&pD,&nmsg,a))
        return NULL;
    retV = rvSdpParseAttribute(&pD,RV_FALSE,RV_FALSE);
    if (retV)
    {
        RvSdpRtpMap* np;
        np = (RvSdpRtpMap*)rvSdpGetSpecialAttr(&nmsg.iCommonFields,0,SDP_FIELDTYPE_RTP_MAP);
        p = rvSdpRtpMapFill(p,np->iPayload,np->iEncName,np->iClockRate,
                            np->iEncParameters,NULL);
        if (!p)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
    }
    REPARSE_END
}

/***************************************************************************
 * rvSdpUriReparse
 * ------------------------------------------------------------------------
 * General: 
 *      Performs reparse of URI field of the message. 
 *          
 * Return Value: 
 *      Pointer to valid URI field in case of successfull reparse 
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the URI is contained.
 *      u - URI line for reparse.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
const char* rvSdpUriReparse(RvSdpMsg* msg, const char* p, 
                            int* len,RvSdpParseStatus* stat,
                            RvAlloc* a)
{
    RvSdpMsg nmsg;
    RvSdpParserData pD;
    RvBool retV;        
	const char* r;
    RV_UNUSED_ARG(len);
    RV_UNUSED_ARG(msg);
    

    if (!msg)
        return NULL;
    
    if (rvSdpMsgFillURI(msg,p,RV_TRUE) != RV_SDPSTATUS_OK)
    {
        *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        return  NULL;
    }

    if (!rvSdpReparseInit(RV_TRUE,msg->iUri.iUriTxt,stat,&pD,&nmsg,a))
        return NULL;
    
    retV = rvSdpParseURI(&pD,RV_FALSE,RV_FALSE);

    if (retV && !nmsg.iUri.iUriBadSyntax)
    {
		if (rvSdpMsgFillURI(msg,nmsg.iUri.iUriTxt,RV_FALSE) != RV_SDPSTATUS_OK)
		{
			*stat = RV_SDPPARSER_STOP_ALLOCFAIL;
            rvSdpMsgDestruct(&nmsg);
			return  NULL;
		}
		r = msg->iUri.iUriTxt;
    }
    else 
    {
        if (pD.iCriticalError)
            *stat = RV_SDPPARSER_STOP_ALLOCFAIL;
        else
            *stat = RV_SDPPARSER_STOP_ERROR;
        r = NULL;
    }
    rvSdpMsgDestruct(&nmsg);
    return r;
}

#endif /*RV_SDP_ENABLE_REPARSE*/
