/******************************************************************************
Filename    :rvsdpkeymgmt.c
Description : key management manipulation routines.

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
#include <stdio.h>
#include <stdlib.h>
#include "rvsdpprivate.h"
#include "rvstrutils.h"
#include "rvbase64.h"

#ifdef RV_SDP_KEY_MGMT_ATTR

/*
 *	Allocates and constructs the object using RvAlloc allocator.
 *  Returns the constructed object or NULL if fails.
 */
RvSdpKeyMgmtAttr* rvSdpKeyMgmtConstruct2(
            void* obj,              /* points to RvSdpMsg or RvAlloc instance */
            const char* prtclId,    /* the protocol ID */
            const char* keyData,    /* the key data */
            const char* badSyn)     /* the proprietary formatted crypto attribute value
                                       or NULL */
{
    RvSdpKeyMgmtAttr* km;
    RvAlloc *a;

    RV_UNUSED_ARG(badSyn);

    /*
     *	unlike other SDP data structures this one does not allocate itself
     *  through dedicated objects pool but directly
     */

    if (RV_SDP_OBJ_IS_MESSAGE2(obj))
        a = ((RvSdpMsg*)obj)->iAllocator;
    else
        a = (RvAlloc*) obj;

    if (!a)
        a = rvSdpGetDefaultAllocator();
    
    km = rvSdpAllocAllocate(a,sizeof(RvSdpKeyMgmtAttr));
    if (!km)
        return NULL;
    
    memset(km,0,sizeof(RvSdpKeyMgmtAttr));
    
    km->iAlloc = a;
    if (!obj)
        obj = a;
    km->iObj = obj;

    if (RV_SDP_OBJ_IS_MESSAGE2(obj))
    {
        int n;
        RvSdpMsg *msg;
        msg = ((RvSdpMsg*)obj);
        n = (prtclId)?strlen(prtclId):0;
        n += (keyData)?strlen(keyData):0;
        n += (badSyn)?strlen(badSyn):0;    
        rvSdpMsgPromiseBuffer(msg,n+30);/* +30 because later the attribute itself (with name "crypto" will be added) */
    }
    
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&km->iBadSyntaxField,obj,badSyn) != RV_SDPSTATUS_OK)
            goto failed;
    }
    else
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    {
        if (rvSdpSetTextField(&km->iPrtclId,obj,prtclId) != RV_SDPSTATUS_OK ||
            rvSdpSetTextField(&km->iKeyData,obj,keyData) != RV_SDPSTATUS_OK)
            goto failed;
    }    
    km->iKMAttribute.iSpecAttrData = rvSdpFindSpecAttrDataByFieldType(SDP_FIELDTYPE_KEY_MGMT);
    return km;
    
failed:
    if (km)
        rvSdpKeyMgmtDestruct(km);
    return NULL;
}



/***************************************************************************
 * rvSdpAddKeyMgmt2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the key-mgmt atribute to the message or media.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpKeyMgmtAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          msg - message where addition will be performed.
 *          commF - common fields pointer of message or media.
 *          prtclId - protocol id value.
 *          keyData - the key data value.
 *          badSyn - proprietary syntax key-mgmt attribute.
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpAddKeyMgmt2(RvSdpMsg* msg,  
                               RvSdpCommonFields* commF,
                               const char* prtclId,
                               const char* keyData,
                               const char* badSyn)
{
    RvSdpKeyMgmtAttr* km;
    int n;

    n = (prtclId)?strlen(prtclId):0;
    n += (keyData)?strlen(keyData):0;
    n += (badSyn)?strlen(badSyn):0;
    
    rvSdpMsgPromiseBuffer(msg,n);
    km = rvSdpKeyMgmtConstruct2(msg,prtclId,keyData,badSyn);
    if (!km)
        return NULL;    
    rvSdpAddAttr2(msg,commF,&(km->iKMAttribute),"key-mgmt",NULL);    
    return km;
}

/***************************************************************************
 * rvSdpKeyMgmtAttrCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpKeyMgmtAttr from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpKeyMgmtAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpKeyMgmtAttr object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 ***************************************************************************/
RvSdpKeyMgmtAttr* rvSdpKeyMgmtCopy2(RvSdpKeyMgmtAttr* dest, 
                                    const RvSdpKeyMgmtAttr* src, void* obj)
{
    dest = rvSdpKeyMgmtConstruct2(obj,src->iPrtclId,src->iKeyData,
        RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField));
    if (dest == NULL)
        return NULL;
        
    
    if (obj)
    {
        RvSdpMsg *msg;        
        msg = (RV_SDP_OBJ_IS_MESSAGE2(obj))?(RvSdpMsg*)obj:NULL;
        (void)rvSdpAttributeConstruct2(msg,&dest->iKMAttribute,src->iKMAttribute.iAttrName,
            src->iKMAttribute.iAttrValue,obj,RV_FALSE);
    }
    
    return dest;
}

/***************************************************************************
 * rvSdpKeyMgmtDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the instance of RvSdpKeyMgmtAttr.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyM - object to be destroyed.
 ***************************************************************************/
void rvSdpKeyMgmtDestruct(RvSdpKeyMgmtAttr* keym)
{
    
    rvSdpUnsetTextField(&keym->iPrtclId,keym->iObj);
    rvSdpUnsetTextField(&keym->iKeyData,keym->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&keym->iBadSyntaxField,keym->iObj);
#endif    
    rvSdpAttributeDestructNetto(&keym->iKMAttribute,RV_FALSE);    
    rvSdpAllocDeallocate(keym->iAlloc,sizeof(RvSdpKeyMgmtAttr),keym);
}

/***************************************************************************
 * rvSdpKeyMgmtGetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Prints the textual value of key management attribute  into provided 
 *      buffer.
 *          
 * Return Value: 
 *      Returns the buffer pointer on success or NULL if fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *          km - instance of RvSdpKeyMgmtAttr.
 *          txt - the buffer for the value.
 ***************************************************************************/
char* rvSdpKeyMgmtGetValue(RvSdpKeyMgmtAttr* km, char *txt)
{
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (km->iBadSyntaxField)
    {
        strcpy(txt,km->iBadSyntaxField);
    }
    else
#endif  
    {
        sprintf(txt,"%s %s",km->iPrtclId,km->iKeyData);
    }
    return txt;
}

RvSdpKeyMgmtPrtclType rvSdpKeyMgmtPrtclTypeTxt2Val(const char* keyMgmtPrtclTxt)
{
    if (!strcasecmp("mikey",keyMgmtPrtclTxt))
        return RV_SDPKEYMGMT_MIKEY;
    else 
        return RV_SDPKEYMGMT_UNKNOWN;
}

const char* rvSdpKeyMgmtPrtclTypeVal2Txt(RvSdpKeyMgmtPrtclType m)
{
    switch (m)
    {
    case RV_SDPKEYMGMT_MIKEY:
        return "mikey";
    default:
        return "Unknown";
    }
}

/***************************************************************************
 * rvSdpKeyMgmtDecodeKeyData
 * ------------------------------------------------------------------------
 * General: 
 *      Decodes (using B64 decoding) key data of key-mgmt attribute.
 *          
 * Return Value: 
 *      The size of decoded buffer.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 *      decodedData - the output buffer for B64 decoding.
 *      dataLen - the size 'decodedData' buffer
 ***************************************************************************/
RvSize_t rvSdpKeyMgmtDecodeKeyData(RvSdpKeyMgmtAttr* keyMgmt, unsigned char* decodedData, int dataLen)
{
    if (!keyMgmt->iKeyData)
        return 0;

    return rvDecodeB64((unsigned char*)keyMgmt->iKeyData,strlen(keyMgmt->iKeyData),decodedData,dataLen);
}


/*
 *	Called during message strings buffer reshuffle.
 *  Copies all string fields of key-mgmt into '*ptr' while
 *  increasing '*ptr' value by the length of copied strings.
 */
void rvSdpKeyMgmtReshuffle(RvSdpKeyMgmtAttr *km, char** ptr)
{
	rvSdpChangeText(ptr,&km->iPrtclId);
	rvSdpChangeText(ptr,&km->iKeyData);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
	rvSdpChangeText(ptr,&km->iBadSyntaxField);
#endif                	
}

/*
 *	Performs the parsing of key-mgmt attribute
 */
RvBool rvSdpParseKeyMgmtAttr(RvSdpParserData* pD, 
                             char *ptr, 
                             char** prtclId, 
                             char** kmgData, 
                             REPLACING_DECL)
{
    char *pp;

    if (!pD)
        return RV_FALSE;
    
    *kmgData = NULL;    
	
    rvSdpParseSpace(&ptr);
	
    *prtclId = ptr;
    if (!rvSdpParseText(&ptr,M_AD))
    {
        pD->iWarning.iWarningNum = RvSdpWarnKeyMgmtIllegalPrtcl;    
        return RV_FALSE;
    }
	
    pp = ptr;    
    if (!rvSdpParseSpace(&ptr))
    {
        pD->iWarning.iWarningNum = RvSdpWarnKeyMgmtIllegalPrtcl;
        return RV_FALSE;
    }
    REPLACING2(pp)
        
		
		*kmgData = ptr;     
    if (!rvSdpParseText(&ptr,M_B64) || *ptr)
    {
        pD->iWarning.iWarningNum = RvSdpWarnKeyMgmtIllegalData;    
        return RV_FALSE;
    }            
    return RV_TRUE;
}


/*
 *	Special attribute  parse function for the key-mgmt attribute. 
 *  Parses key-mgmt attribute field and constructs the RvSdpKeyMgmtAttr object.
 *  The constructed object is added to current context message or media 
 *  descriptor.
 *  If parsing or construction fails the correspondent status is returned.
 */
RvSdpSpecAttrParseSts rvSdpKeyMgmtParsing(
				const RvSdpSpecAttributeData* sad,  /* the special attribute data */
                RvBool createAsBadSyntax,           /* if set to RV_TRUE the bad 
                                                       syntax key-mgmt attribute will be  
                                                       created */
                RvSdpParserData* pD,    /* the parser data instance */        
                RvSdpCommonFields *cm,  /* the current context common fields instance,
                                           here the special attribute will be added */
				char* n,                /* the attribute name */
                char* v,                /* the attribute value to parse */
                REPLACING_DECL)         /* used for zero-substitutions in input buffer */
{
	UNUSE_REPLACING_DECL;
	RV_UNUSED_ARG(n);
	RV_UNUSED_ARG(sad);

	if (createAsBadSyntax)
	{
#ifdef RV_SDP_CHECK_BAD_SYNTAX
		if (rvSdpAddKeyMgmt2(pD->iMsg,cm,NULL,NULL,v)==NULL)
			return rvSdpSpAttrPrsAllocFail;
#else /*RV_SDP_CHECK_BAD_SYNTAX*/
        return rvSdpSpAttrPrsCrtRegular;
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/
	}
	else 
	{
		char *prtclId, *kmgData;
		
		if (!rvSdpParseKeyMgmtAttr(pD,v,&prtclId,&kmgData,REPLACING_ARGS2))
		{
			pD->iLineStatus = RvSdpValueParseFailed;
			return rvSdpSpAttrPrsFail;
		}
		else if (rvSdpAddKeyMgmt2(pD->iMsg,cm,prtclId,kmgData,NULL) == NULL)
			return rvSdpSpAttrPrsAllocFail;
	}
	return rvSdpSpAttrPrsOK;	
}


#ifndef RV_SDP_USE_MACROS

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpKeyMgmtIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the key-mgmt attribute is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
RvBool rvSdpKeyMgmtIsBadSyntax(RvSdpKeyMgmtAttr* keyMgmt)
{
    return (keyMgmt->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpKeyMgmtGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted key-mgmt attribute value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
const char* rvSdpKeyMgmtGetBadSyntax(const RvSdpKeyMgmtAttr* keyMgmt)
{
    return RV_SDP_EMPTY_STRING(keyMgmt->iBadSyntaxField);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpKeyMgmtGetPrtclIdTxt
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the protocol ID text value of the key-mgmt attribute.
 *          
 * Return Value: 
 *      The requested field of the key-mgmt attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
const char* rvSdpKeyMgmtGetPrtclIdTxt(const RvSdpKeyMgmtAttr* keyMgmt)
{
    return RV_SDP_EMPTY_STRING(keyMgmt->iPrtclId);
}

/***************************************************************************
 * rvSdpKeyMgmtSetPrtclIdTxt
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the protocol ID text value of the key-mgmt attribute.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 *      prtclId - the new value of protocol ID text.
 ***************************************************************************/
RvSdpStatus rvSdpKeyMgmtSetPrtclIdTxt(RvSdpKeyMgmtAttr* keyMgmt,const char* prtclId)
{
    return rvSdpSetTextField(&keyMgmt->iPrtclId,keyMgmt->iObj,prtclId);
}

/***************************************************************************
 * rvSdpKeyMgmtGetPrtclId
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the protocol ID value of the key-mgmt attribute.
 *          
 * Return Value: 
 *      The requested field of the key-mgmt attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
RvSdpKeyMgmtPrtclType rvSdpKeyMgmtGetPrtclId(const RvSdpKeyMgmtAttr* keyMgmt)
{
    return rvSdpKeyMgmtPrtclTypeTxt2Val(keyMgmt->iPrtclId);
}

/***************************************************************************
 * rvSdpKeyMgmtSetPrtclId
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the protocol ID value of the key-mgmt attribute.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 *      prtclId - the new value of protocol ID.
 ***************************************************************************/
RvSdpStatus rvSdpKeyMgmtSetPrtclId(RvSdpKeyMgmtAttr* keyMgmt, RvSdpKeyMgmtPrtclType prtclId)
{
    return rvSdpSetTextField(&keyMgmt->iPrtclId,keyMgmt->iObj,rvSdpKeyMgmtPrtclTypeVal2Txt(prtclId));
}


/***************************************************************************
 * rvSdpKeyMgmtGetKeyData
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the encryption key data value of the key-mgmt attribute.
 *          
 * Return Value: 
 *      The requested field of the key-mgmt attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
const char* rvSdpKeyMgmtGetKeyData(const RvSdpKeyMgmtAttr* keyMgmt)
{
    return RV_SDP_EMPTY_STRING(keyMgmt->iKeyData);
}

/***************************************************************************
 * rvSdpKeyMgmtSetKeyData
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the key data value of the key-mgmt attribute.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 *      keyData - the new value of protocol ID.
 ***************************************************************************/
RvSdpStatus rvSdpKeyMgmtSetKeyData(RvSdpKeyMgmtAttr* keyMgmt, const char* keyData)
{
    return rvSdpSetTextField(&keyMgmt->iKeyData,keyMgmt->iObj,keyData);
}

#endif /*RV_SDP_USE_MACROS*/


#endif /*RV_SDP_KEY_MGMT_ATTR*/

