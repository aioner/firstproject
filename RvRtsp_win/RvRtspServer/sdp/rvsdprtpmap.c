/******************************************************************************
Filename    :rvsdprtpmap.c
Description :RTP map manipulation routines.

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
#include "rvsdpprsutils.h"

#include <stdio.h>
#include <stdlib.h>

/*
 *	Sets the internal RvSdpRtpMap fields to the supplied values.
 *  Returns the 'rtpMap' of NULL if fails.
 */
RvSdpRtpMap* rvSdpRtpMapFill(
				RvSdpRtpMap* rtpMap,    /* the RvSdpRtpMap instance to fill */
				int payload,            /* the payload of RTP map */
				const char* encName,    /* the encoding name */ 
				int rate,               /* the rate */
				const char* encParams,  /* the encoding parameters */
                const char* badSyn)     /* the proprietary formatted syntax of RTP map
                                           or NULL */
{
    RV_UNUSED_ARG(badSyn);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&rtpMap->iBadSyntaxField,rtpMap->iObj,badSyn) 
                                                                    != RV_SDPSTATUS_OK)
            return NULL;
        return rtpMap;
    }
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    rtpMap->iPayload = payload;
    rtpMap->iClockRate = rate;
    /* allocate the strings on the msg strings buffer */
    if (rvSdpSetTextField(&rtpMap->iEncName,rtpMap->iObj,encName) != RV_SDPSTATUS_OK ||
		rvSdpSetTextField(&rtpMap->iEncParameters,rtpMap->iObj,encParams) 
                                                                != RV_SDPSTATUS_OK)
		return NULL;
	return rtpMap;
}

/***************************************************************************
 * rvSdpRtpMapConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRtpMap object.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to valid RvSdpMsg object or NULL. If the parameter is not
 *            NULL and 'rtpMap' is NULL the RTP map will be allocated from
 *            the 'msg' pool of RTP maps. If 'msg' is not NULL the constructed
 *            RTP map will be appended to 'msg' list of attributes. If the 'msg'
 *            is NULL the 'a' allocator will be used.
 *      rtpMap - a pointer to valid RvSdpRtpMap object or NULL.
 *      payload - the RTP map payload number.
 *      encoding_name - the RTP map encoding name.
 *      rate - the RTP map rate value.
 *      badSyn - the proprietary formatted RTP map field or NULL if standard 
 *               RTP map is constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 *      dontConstruct - if set to RV_TRUE the 'rtpMap' must point to valid & constructed
 *                      RvSdpRtpMap object. This parameter (with RV_TRUE value) 
 *                      is used when objects are copied without construction.
 ***************************************************************************/
RvSdpRtpMap* 
rvSdpRtpMapConstruct2(RvSdpMsg* msg, RvSdpRtpMap* rtpMap, int payload, 
                      const char* encoding_name, int rate, 
                      const char* badSyn, RvAlloc* a)
{
    if (a && RV_SDP_OBJ_IS_MESSAGE2(a))
        msg = (RvSdpMsg*) a;
    if (msg)
        /* the RvSdpMsg is provided, all allocations will be performed in 
           the RvSdpMsg context */
    {            

        if (rtpMap)
            /* the 'email' can't be set it has to be allocated from the msg emails pool */
            return NULL;
    
        rtpMap = rvSdpPoolTake(&msg->iRtpMapsPool);
        if (!rtpMap)
            /* failed to allocate from the msg emails pool */
            return NULL;

        memset(rtpMap,0,sizeof(RvSdpRtpMap));        
    
        rtpMap->iObj = msg;
    }
    else 
    {
        /* obsolete API usage:
            no msg context given, will be using allocator */
        if (!rtpMap)
            /* the RvSdpRtpMap instance has to be supplied */
            return NULL;

        memset(rtpMap,0,sizeof(RvSdpRtpMap));        
        if (!a)
            /* the dault allocator will be used */
            a = rvSdpGetDefaultAllocator();

        /* save the allocator used */
        rtpMap->iObj = a;
    }

    rtpMap->iRMAttribute.iSpecAttrData = 
        rvSdpFindSpecAttrDataByFieldType(SDP_FIELDTYPE_RTP_MAP);
    if (msg)  
    {
        int n = 0;
        n += (badSyn) ? strlen(badSyn) : 0;
        n += (encoding_name) ? strlen(encoding_name) : 0;
        rvSdpMsgPromiseBuffer(msg,n+30);/* +30 because later the attribute itself (with name "crypto" will be added) */
    }

            
	if (!rvSdpRtpMapFill(rtpMap,payload,encoding_name,rate,NULL,badSyn))
    {
        rvSdpRtpMapDestruct(rtpMap);
        return NULL;
    }

    return rtpMap;   
}

/***************************************************************************
 * rvSdpRtpMapCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the values from a source RTP map object to destination.
 *          
 * Return Value: 
 *      A pointer to the destination object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination RTP map. Must point 
 *             to constructed RvSdpRtpMap object.
 *      src - a RTP map object.
 ***************************************************************************/
RvSdpRtpMap* rvSdpRtpMapCopy(RvSdpRtpMap* dest, const RvSdpRtpMap* src)
{
    RvSdpAttribute *dattr;
    const RvSdpAttribute *sattr;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    /* allocate the strings on the msg strings buffer */
    if (rvSdpSetTextField(&dest->iBadSyntaxField,dest->iObj,
                                src->iBadSyntaxField) != RV_SDPSTATUS_OK)
        return NULL;
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    if (!rvSdpRtpMapFill(dest,src->iPayload,src->iEncName,src->iClockRate,
                        src->iEncParameters,
                        RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField)))
        return NULL;

    dattr = &dest->iRMAttribute;
    sattr = &src->iRMAttribute;

    if (rvSdpSetTextField(&dattr->iAttrName,dattr->iObj,sattr->iAttrName) 
                                                                != RV_SDPSTATUS_OK ||
        rvSdpSetTextField(&dattr->iAttrValue,dattr->iObj,sattr->iAttrValue) 
                                                                != RV_SDPSTATUS_OK)
        return NULL;        
    return dest;   
}

/***************************************************************************
 * rvSdpRtpMapCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpRtpMap from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpRtpMap object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpRtpMap object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object or
 *            allocator pointer.
 ***************************************************************************/
RvSdpRtpMap* 
rvSdpRtpMapCopy2(RvSdpRtpMap* dest, const RvSdpRtpMap* src, void* obj)
{
    dest = rvSdpRtpMapConstruct2(NULL,dest,src->iPayload,src->iEncName,
               src->iClockRate,RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField),obj);
    if (dest == NULL)
        return NULL;

    if (rvSdpSetTextField(&dest->iEncParameters,dest->iObj,src->iEncParameters) 
                                                                != RV_SDPSTATUS_OK)
        goto failure;

    if (obj)
    {
        RvSdpMsg *msg;        
        msg = (RV_SDP_OBJ_IS_MESSAGE2(obj))?(RvSdpMsg*)obj:NULL;
        rvSdpAttributeConstruct2(msg,&dest->iRMAttribute,src->iRMAttribute.iAttrName,
            src->iRMAttribute.iAttrValue,obj,RV_FALSE);
    }

    return dest;
failure:
    rvSdpRtpMapDestruct(dest);
    return NULL;
}

/*
 *	To allocate the memory for RvSdpRtpMap object (called by the pool)
 *  Return: 
 *      valid RvSdpRtpMap pointer
 *      or NULL if fails
 */
RvSdpRtpMap* rvSdpRtpMapCreateByPool(RvSdpMsg* msg)
{
    RvSdpRtpMap* rm;
    
    rm = rvSdpAllocAllocate(msg->iAllocator,sizeof(RvSdpRtpMap));
    if (!rm)
        return NULL;
    
    memset(rm,0,sizeof(RvSdpRtpMap));   
    rm->iObj = msg;
    
    return rm;
}

/*
 *	To free the memory for RvSdpRtpMap object (called by the pool)
 */
void rvSdpRtpMapDestroyByPool(RvSdpRtpMap* p)
{
    rvSdpAllocDeallocate(((RvSdpMsg*)(p->iObj))->iAllocator,sizeof(RvSdpRtpMap),p);
}

/***************************************************************************
 * rvSdpRtpMapDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destructs the RTP map object.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
***************************************************************************/
void rvSdpRtpMapDestruct(RvSdpRtpMap* rtpMap)
{
    if (!rtpMap->iObj)
        /* cannot deallocate memory */
        return;
    
    rvSdpUnsetTextField(&rtpMap->iEncName,rtpMap->iObj);
    rvSdpUnsetTextField(&rtpMap->iEncParameters,rtpMap->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&rtpMap->iBadSyntaxField,rtpMap->iObj);
#endif

    rvSdpAttributeDestructNetto(&rtpMap->iRMAttribute,RV_FALSE);

    if (RV_SDP_OBJ_IS_MESSAGE(rtpMap))
        rvSdpPoolReturn(&((RvSdpMsg*)(rtpMap->iObj))->iRtpMapsPool,rtpMap);
}

/***************************************************************************
 * rvSdpRtpMapGetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Prints the textual value of RTP map attribute  into provided 
 *      buffer.
 *          
 * Return Value: 
 *      Returns the buffer pointer on success or NULL if fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *          rtpMap - instance of RvSdpRtpMap.
 *          txt - the buffer for the value.
 ***************************************************************************/
char* rvSdpRtpMapGetValue(RvSdpRtpMap* rtpMap, char* txt)
{
    char *p = txt;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (rtpMap->iBadSyntaxField)
    {
        strcpy(txt,rtpMap->iBadSyntaxField);
        return p;
    }
#endif
    rvSdpItoa(txt,rtpMap->iPayload);
    txt += strlen(txt);
    *txt++ = ' ';
    strcpy(txt,rtpMap->iEncName);

    if (rtpMap->iClockRate == RV_SDP_INT_NOT_SET)
        return p;
    txt += strlen(txt);
    *txt++ = '/';
    rvSdpItoa(txt,rtpMap->iClockRate);
    if (!rtpMap->iEncParameters)
        return p;
    txt += strlen(txt);
    *txt++ = '/';
    strcpy(txt,rtpMap->iEncParameters);
    return p;
}

/***************************************************************************
 * rvSdpRtpMapGetChannels
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the number of channels. Used for media type audio instead of
 *      rvSdpRtpMapGetEncodingParameters().
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
int rvSdpRtpMapGetChannels(const RvSdpRtpMap* rtpMap)
{
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (rtpMap->iBadSyntaxField)
        return 0;
#endif
    
    if(rtpMap->iEncParameters && strlen(rtpMap->iEncParameters))
        return atoi(rtpMap->iEncParameters);
    return 1;
}


/***************************************************************************
 * rvSdpRtpMapSetChannels
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the number of channels. Used for media type audio instead of
 *      rvSdpRtpMapSetEncodingParameters().
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      channels - the number of channels.
 ***************************************************************************/
RvSdpStatus rvSdpRtpMapSetChannels(RvSdpRtpMap* rtpMap, int channels)
{
    char parameters[16];
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (rtpMap->iBadSyntaxField)
        return RV_SDPSTATUS_PARSEFAIL;
#endif
    
    return rvSdpSetTextField(&rtpMap->iEncParameters,rtpMap->iObj,
                                        rvSdpItoa(parameters,channels));
}

/*
 *	Called during message strings buffer reshuffle.
 *  Copies all string fields of RTP map into '*ptr' while
 *  increasing '*ptr' value by the length of copied strings.
 */
void rvSdpRtpMapReshuffle(
                RvSdpRtpMap *rtpMap,  /* the RTP map to reshuffle */
                char** ptr)           /* points inside the new buffer string chunk */
{                
    rvSdpChangeText(ptr,&rtpMap->iEncName);
    rvSdpChangeText(ptr,&rtpMap->iEncParameters);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpChangeText(ptr,&rtpMap->iBadSyntaxField);
#endif                
}


/*
 *	Parses the RTP map value. Sets the supplied RTP map fields pointer to the 
 *  values found in the 'ptr' input. 
 *  Sets the zero bytes at the end of all textual fields (encName and encParams).
 *  Returns RV_FALSE if the parsing fails.
 */
RvBool rvSdpParseRtpMap(
                RvSdpParserData* pD,    /* the parser data  instance;
                                           used to create parse warnings */
                char *ptr,              /* the input pointer */
                int* payload,           /* will be set to payload */
                char** encName,         /* will point to the start of encoding name in
                                           the input buffer */
                int* encRate,           /* will be set to encoding rate */
                char** encParams,       /* will point to the start of encoding param in
                                           the input buffer */
                REPLACING_DECL)         /* used to set zero bytes in the input buffer */
{
    char *p1;
    RvSdpIntegerParseWarning intW = RvSdpIntNonWarning;

    if (!pD)
        return RV_FALSE;
    
    *encRate = RV_SDP_INT_NOT_SET;
    *encParams = NULL;
    
    if (!rvSdpParseInt(0,0,(RvUint32*)payload,RV_TRUE,RV_FALSE,&ptr,&intW) 
                                    || !rvSdpParseSpace(&ptr))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnRtpMapPayloadTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnRtpMapNoPayload;
        }
        return RV_FALSE;
    }
    
    *encName = ptr;
    
    if (*ptr == 0 || *ptr == '/')
    {
        pD->iWarning.iWarningNum = RvSdpWarnRtpMapNoEncName;
        return RV_FALSE;
    }
    
    p1 = strchr(ptr,'/');
    if (!p1)
        return RV_TRUE;
    
    REPLACING2(p1);
    
    ptr = p1+1;
    
    if (!rvSdpParseInt(0,0,(RvUint32*)encRate,RV_TRUE,RV_FALSE,&ptr,&intW))
    {
        if (intW == RvSdpWarnNumberOverflow)
        {
            pD->iWarning.iWarningNum = RvSdpWarnRtpMapEncRateTooLong;
        }
        else
        {
            pD->iWarning.iWarningNum = RvSdpWarnRtpMapBadEncRate;
        }        
        return RV_FALSE;
    }
    
    if (*ptr == 0)
        return RV_TRUE;
    
    if (*ptr != '/')
    {
        pD->iWarning.iWarningNum = RvSdpWarnRtpMapBadEncRate;
        return RV_FALSE;
    }
    
    ptr ++;
    *encParams = ptr;
    
    return RV_TRUE;
}


/*
 *	Special attribute  parse function for the RTP map. 
 *  Parses RTP map attribute field and constructs the RvSdpRtpMap object.
 *  The constructed object is added to current context message or media 
 *  descriptor.
 *  If parsing or construction fails the correspondent status is returned.
 */
RvSdpSpecAttrParseSts rvSdpRtpMapParsing(
				const RvSdpSpecAttributeData* sad,  /* the special attribute data */
                RvBool createAsBadSyntax,           /* if set to RV_TRUE the bad 
                                                       syntax RTP map attribute will be  
                                                       created */
                RvSdpParserData* pD,    /* the parser data instance */        
                RvSdpCommonFields *cm,  /* the current context common fields instance,
                                           here the special attribute will be added */
				char* n,                /* the attribute name */
                char* v,                /* the attribute value to parse */
                REPLACING_DECL)         /* used for zero-substitutions in input buffer */
{
	RV_UNUSED_ARG(n);
	RV_UNUSED_ARG(sad);

    if (createAsBadSyntax)
    {
#ifdef RV_SDP_CHECK_BAD_SYNTAX
        if (rvSdpAddRtpMap2(pD->iMsg,cm,0,NULL,0,v)==NULL)
            return rvSdpSpAttrPrsAllocFail;
#else /*RV_SDP_CHECK_BAD_SYNTAX*/
        return rvSdpSpAttrPrsCrtRegular;
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/
    }
    else 
    {
        int  payload, encRate;
        char *encName, *encParams;
        RvSdpRtpMap *rm;
        
        if (!rvSdpParseRtpMap(pD,v,&payload,&encName,&encRate,
                                        &encParams, REPLACING_ARGS2))
        {
            pD->iLineStatus = RvSdpValueParseFailed;
            return rvSdpSpAttrPrsFail;
        }
        else
        {           
            rm = rvSdpAddRtpMap2(pD->iMsg,cm,payload,encName,encRate,NULL);
            
            if (!rm)
                return rvSdpSpAttrPrsAllocFail;
            if (encParams)
                rvSdpRtpMapSetEncodingParameters(rm,encParams);
        }
    }

	return rvSdpSpAttrPrsOK;
}



#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpRtpMapConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRtpMap object.
 *      This function is obsolete. The rvSdpMsgAddRtpMap or  
 *      rvSdpMediaDescrAddRtpMap should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to valid RvSdpRtpMap object.
 *      payload - the RTP map payload number.
 *      encoding_name - the RTP map encoding name.
 *      rate - the RTP map rate value.
 ***************************************************************************/
RvSdpRtpMap* rvSdpRtpMapConstruct(RvSdpRtpMap* rtpMap, int payload, 
                                  const char* encoding_name, int rate)
{
    return rvSdpRtpMapConstruct2(NULL,rtpMap,payload,encoding_name,rate,NULL,NULL);
}

/***************************************************************************
 * rvSdpRtpMapConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRtpMap object.
 *      This function is obsolete. The rvSdpMsgAddRtpMap or  
 *      rvSdpMediaDescrAddRtpMap should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to valid RvSdpRtpMap object.
 *      payload - the RTP map payload number.
 *      encoding_name - the RTP map encoding name.
 *      rate - the RTP map rate value.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpRtpMap* 
rvSdpRtpMapConstructA(RvSdpRtpMap* rtpMap, int payload, 
                      const char* encoding_name, int rate, RvAlloc* alloc)
{
    return rvSdpRtpMapConstruct2(NULL,rtpMap,payload,encoding_name,rate,NULL,alloc);
}

/***************************************************************************
 * rvSdpRtpMapConstructCopy
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a RTP map object and copies the values from a source 
 *      RTP map field. 
 *      This function is obsolete. The rvSdpMsgAddRtpMap or  
 *      rvSdpMediaDescrAddRtpMap should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to RTP map to be constructed. Must point 
 *             to valid memory.
 *      src - a source RTP map object.
 ***************************************************************************/
RvSdpRtpMap* rvSdpRtpMapConstructCopy(RvSdpRtpMap* dest, const RvSdpRtpMap* src)
{
    return rvSdpRtpMapCopy2(dest,src,NULL);
}

/***************************************************************************
 * rvSdpRtpMapConstructCopyA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs a RTP map object and copies the values from a source 
 *      RTP map field. 
 *      This function is obsolete. The rvSdpMsgAddRtpMap or  
 *      rvSdpMediaDescrAddRtpMap should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to RTP map to be constructed. Must point 
 *             to valid memory.
 *      src - a source RTP map object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpRtpMap* rvSdpRtpMapConstructCopyA(RvSdpRtpMap* dest, const RvSdpRtpMap* src, 
                                       RvAlloc* a)
{
    return rvSdpRtpMapCopy2(dest,src,a);
}


#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxRtpMapConstruct
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRtpMap object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxRtpMap or  
 *      rvSdpMediaDescrAddBadSyntaxRtpMap should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to valid RvSdpRtpMap object.
 *      badSyn - the proprietary format of RTP map field.
 ***************************************************************************/
RvSdpRtpMap* rvSdpBadSyntaxRtpMapConstruct(RvSdpRtpMap* rtpMap, const char* badSyn)
{
    return rvSdpRtpMapConstruct2(NULL,rtpMap,0,NULL,0,badSyn,NULL);
}

/***************************************************************************
 * rvSdpBadSyntaxRtpMapConstructA
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs the RvSdpRtpMap object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxRtpMap or  
 *      rvSdpMediaDescrAddBadSyntaxRtpMap should be used instead.
 *          
 * Return Value: 
 *      A pointer to the constructed object, or NULL if the function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to valid RvSdpRtpMap object.
 *      badSyn - the proprietary format of RTP map field.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RvSdpRtpMap* rvSdpBadSyntaxRtpMapConstructA(RvSdpRtpMap* rtpMap, const char* badSyn, 
                                            RvAlloc* alloc)
{
    return rvSdpRtpMapConstruct2(NULL,rtpMap,0,NULL,0,badSyn,alloc);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */



/***************************************************************************
 * rvSdpRtpMapGetPayload
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the payload number.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
int rvSdpRtpMapGetPayload(const RvSdpRtpMap* rtpMap)
{
    return rtpMap->iPayload;
}

/***************************************************************************
 * rvSdpRtpMapSetPayload
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the payload number.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      payload - the payload number.
 ***************************************************************************/
void rvSdpRtpMapSetPayload(RvSdpRtpMap* rtpMap, int payload)
{
    rtpMap->iPayload = payload;
}

/***************************************************************************
 * rvSdpRtpMapGetEncodingName
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the RTP map encoding name.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
const char* rvSdpRtpMapGetEncodingName(const RvSdpRtpMap* rtpMap)
{
    return RV_SDP_EMPTY_STRING(rtpMap->iEncName);
}

/***************************************************************************
 * rvSdpRtpMapGetClockRate
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the RTP map clock-rate.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
RvUint32 rvSdpRtpMapGetClockRate(const RvSdpRtpMap* rtpMap)
{
    return (rtpMap->iClockRate==RV_SDP_INT_NOT_SET) 
                    ? RV_SDP_DEFAULT_CLOCK_RATE : rtpMap->iClockRate;
}

/***************************************************************************
 * rvSdpRtpMapSetClockRate
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the RTP map clock-rate.
 *
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      rate - the RTP map clock-rate.
 ***************************************************************************/
void rvSdpRtpMapSetClockRate(RvSdpRtpMap* rtpMap, RvUint32 rate)
{
    rtpMap->iClockRate = rate;
}

/***************************************************************************
 * rvSdpRtpMapGetEncodingParameters
 * ------------------------------------------------------------------------
 * General: 
 * Return Value: 
 *      Returns the RTP map encoding parameters.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
const char* rvSdpRtpMapGetEncodingParameters(const RvSdpRtpMap* rtpMap)
{
    return RV_SDP_EMPTY_STRING(rtpMap->iEncParameters);
}

/***************************************************************************
 * rvSdpRtpMapSetEncodingName
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the RTP map emcoding name.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      name - the RTP map emcoding name.
 ***************************************************************************/
RvSdpStatus rvSdpRtpMapSetEncodingName(RvSdpRtpMap* rtpMap, const char* name)
{
    return rvSdpSetTextField(&rtpMap->iEncName,rtpMap->iObj,name);
}

/***************************************************************************
 * rvSdpRtpMapSetEncodingParameters
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the RTP map encoding parameters.
 *
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      s - the RTP map encoding parameters.
 ***************************************************************************/
RvSdpStatus rvSdpRtpMapSetEncodingParameters(RvSdpRtpMap* rtpMap, const char* s)
{
    return rvSdpSetTextField(&rtpMap->iEncParameters,rtpMap->iObj,s);
}

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpRtpMapGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted RTP map attribute value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
const char* rvSdpRtpMapGetBadSyntax(const RvSdpRtpMap* rtpMap)
{
    return RV_SDP_EMPTY_STRING(rtpMap->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpRtpMapIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the RTP map attribute field is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
RvBool rvSdpRtpMapIsBadSyntax(RvSdpRtpMap* rtpMap)
{
    return (rtpMap->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpRtpMapSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP RTP map attribute value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpRtpMap object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpRtpMapSetBadSyntax(RvSdpRtpMap* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}


#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

#endif /*#ifndef RV_SDP_USE_MACROS*/

