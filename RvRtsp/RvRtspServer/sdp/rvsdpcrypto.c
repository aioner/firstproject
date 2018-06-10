/******************************************************************************
Filename    :rvsdpcrypto.c
Description :crypto attributes  manipulation routines.

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

#ifdef RV_SDP_CRYPTO_ATTR

/*
 *	Allocates and constructs the object using RvAlloc allocator.
 *  Returns the constructed object or NULL if fails.
 */
RvSdpCryptoAttr* rvSdpCryptoConstruct2(
            void* obj,              /* points to RvSdpMsg or RvAlloc instance */
            RvUint tag,             /* the crypto tag */
            const char* suite,      /* the crypto suite */
            const char* badSyn)     /* the proprietary formatted crypto attribute value
                                       or NULL */
{
    RvSdpCryptoAttr* crpt;
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
    
    crpt = rvSdpAllocAllocate(a,sizeof(RvSdpCryptoAttr));
    if (!crpt)
        return NULL;
    
    memset(crpt,0,sizeof(RvSdpCryptoAttr));
    
    crpt->iAlloc = a;
    if (!obj)
        obj = a;
    crpt->iObj = obj;
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&crpt->iBadSyntaxField,obj,badSyn) != RV_SDPSTATUS_OK)
            goto failed;
    }
    else
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    {
        crpt->iCryptoTag = tag;
        if (rvSdpSetTextField(&crpt->iCryptoSuite,obj,suite) != RV_SDPSTATUS_OK)
            goto failed;
    }    
	crpt->iCrptAttribute.iSpecAttrData = rvSdpFindSpecAttrDataByFieldType(SDP_FIELDTYPE_CRYPTO);
    return crpt;
    
failed:
    if (crpt)
        rvSdpCryptoDestruct(crpt);
    return NULL;
}



/***************************************************************************
 * rvSdpAddCrypto2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the crypto atribute to the message or media.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpCryptoAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          msg - message where addition will be performed.
 *          commF - common fields pointer of message or media.
 *          tag - the tag value.
 *          suite - the crypto suite.
 *          badSyn - proprietary syntax crypto attribute.
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpAddCrypto2(RvSdpMsg* msg,              /* message where addition will be performed */
                               RvSdpCommonFields* commF,   /* common fields pointer of message or media */
                               RvUint tag,
                               const char* suite,
                               const char* badSyn)         /* proprietary syntax crypto */
{
    RvSdpCryptoAttr* crpt;
    int n;

    n = (suite)?strlen(suite):0;
    n += (badSyn)?strlen(badSyn):0;    
    rvSdpMsgPromiseBuffer(msg,n+30); /* +30 because later the attribute itself (with name "crypto" will be added) */
    crpt = rvSdpCryptoConstruct2(msg,tag,suite,badSyn);
    if (!crpt)
        return NULL;    
    rvSdpAddAttr2(msg,commF,&(crpt->iCrptAttribute),"crypto",NULL);    
    return crpt;
}

/***************************************************************************
 * rvSdpCryptoCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpCryptoAttr from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpCryptoAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpCryptoAttr object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 ***************************************************************************/
RvSdpCryptoAttr* rvSdpCryptoCopy2(RvSdpCryptoAttr* dest, 
                                  const RvSdpCryptoAttr* src, void* obj)
{
    int cnt;
    dest = rvSdpCryptoConstruct2(obj,src->iCryptoTag,src->iCryptoSuite,
                    RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField));
    if (dest == NULL)
        return NULL;        
    
    if (obj)
    {
        RvSdpMsg *msg;        
        msg = (RV_SDP_OBJ_IS_MESSAGE2(obj))?(RvSdpMsg*)obj:NULL;
        (void)rvSdpAttributeConstruct2(msg,&dest->iCrptAttribute,src->iCrptAttribute.iAttrName,
            src->iCrptAttribute.iAttrValue,obj,RV_FALSE);
    }

    for (cnt = 0; cnt < src->iCryptoKeyParamsNum; cnt++)
        rvSdpCryptoAddKeyParam(dest,src->iCryptoKeyMethods[cnt],src->iCryptoKeyInfos[cnt]);
    for (cnt = 0; cnt < src->iCryptoSessParamsNum; cnt++)
        rvSdpCryptoAddSessionParam(dest,src->iCryptoSessParams[cnt]);
    
    return dest;
}

/***************************************************************************
 * rvSdpCryptoDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the instance of RvSdpCryptoAttr.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - object to be destroyed.
 ***************************************************************************/
void rvSdpCryptoDestruct(RvSdpCryptoAttr* crpt)
{
    rvSdpCryptoClearKeyParams(crpt);
    rvSdpCryptoClearSessionParams(crpt);
    
    rvSdpUnsetTextField(&crpt->iCryptoSuite,crpt->iObj);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&crpt->iBadSyntaxField,crpt->iObj);
#endif    
    rvSdpAttributeDestructNetto(&crpt->iCrptAttribute,RV_FALSE);    
    rvSdpAllocDeallocate(crpt->iAlloc,sizeof(RvSdpCryptoAttr),crpt);

}

/***************************************************************************
 * rvSdpCryptoGetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Prints the textual value of crypto attribute  into provided 
 *      buffer.
 *          
 * Return Value: 
 *      Returns the buffer pointer on success or NULL if fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *          crypto - instance of RvSdpCryptoAttr.
 *          txt - the buffer for the value.
 ***************************************************************************/
char* rvSdpCryptoGetValue(RvSdpCryptoAttr* crpt, char *txt)
{
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (crpt->iBadSyntaxField)
    {
        strcpy(txt,crpt->iBadSyntaxField);
    }
    else
#endif  
    {
        int cnt;
        char *p;
        sprintf(txt,"%d %s",crpt->iCryptoTag,crpt->iCryptoSuite);
        p = txt + strlen(txt);
        for (cnt = 0; cnt < crpt->iCryptoKeyParamsNum; cnt++)
        {
            if (cnt == 0)
                *(p++) = ' ';
            else
                *(p++) = ';';
            sprintf(p,"%s:%s",crpt->iCryptoKeyMethods[cnt],crpt->iCryptoKeyInfos[cnt]);
            p += strlen(p);
        }
        for (cnt = 0; cnt < crpt->iCryptoSessParamsNum; cnt++)
        {
            *(p++) = ' ';
            strcpy(p,crpt->iCryptoSessParams[cnt]);
            p += strlen(p);
        }
    }
    return txt;
}

/***************************************************************************
 * rvSdpCryptoAddKeyParam
 * ------------------------------------------------------------------------
 * General: 
 *      Adds the another pair of key parameters of the crypto special attribute.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      method - the key method to be added.
 *      info - the key info to be added.
 ***************************************************************************/
RvSdpStatus rvSdpCryptoAddKeyParam(RvSdpCryptoAttr* crypto, 
                                   const char* method, const char* info)
{
    RvSdpStatus ret;

    ret = rvSdpAddTextToArray(&(crypto->iCryptoKeyParamsNum),RV_SDP_CRYPTO_MAX_KEY_PARAMS,
                crypto->iCryptoKeyMethods,crypto->iObj,method);
    if (ret != RV_SDPSTATUS_OK)
        return ret;

    crypto->iCryptoKeyParamsNum--;
    return rvSdpAddTextToArray(&(crypto->iCryptoKeyParamsNum),RV_SDP_CRYPTO_MAX_KEY_PARAMS,
        crypto->iCryptoKeyInfos,crypto->iObj,info);
}

/***************************************************************************
 * rvSdpCryptoRemoveKeyParam
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the key parameter by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpCryptoGetNumOfKeyParams call. 
 ***************************************************************************/
void rvSdpCryptoRemoveKeyParam(RvSdpCryptoAttr* crypto, RvSize_t index)
{
    RvUint16 n = crypto->iCryptoKeyParamsNum;
    rvSdpRemoveTextFromArray(&(crypto->iCryptoKeyParamsNum),crypto->iCryptoKeyMethods,crypto->iObj,index);
    crypto->iCryptoKeyParamsNum = n;
    rvSdpRemoveTextFromArray(&(crypto->iCryptoKeyParamsNum),crypto->iCryptoKeyInfos,crypto->iObj,index);
}

/***************************************************************************
 * rvSdpCryptoClearKeyParams
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all key parameters set in crypto attribute.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
void rvSdpCryptoClearKeyParams(RvSdpCryptoAttr* crypto)
{
    RvUint16 n = crypto->iCryptoKeyParamsNum;
    rvSdpClearTxtArray(&(crypto->iCryptoKeyParamsNum),crypto->iCryptoKeyMethods,crypto->iObj);
    crypto->iCryptoKeyParamsNum = n;
    rvSdpClearTxtArray(&(crypto->iCryptoKeyParamsNum),crypto->iCryptoKeyInfos,crypto->iObj);
}

/*
 *	Called during message strings buffer reshuffle.
 *  Copies all string fields of crypto attribute into '*ptr' while
 *  increasing '*ptr' value by the length of copied strings.
 */
void rvSdpCryptoReshuffle(RvSdpCryptoAttr *cr, char** ptr)
{
	int cnt;
	rvSdpChangeText(ptr,&cr->iCryptoSuite);
	for (cnt = 0; cnt < cr->iCryptoKeyParamsNum; cnt++)
	{
		rvSdpChangeText(ptr,&cr->iCryptoKeyMethods[cnt]);
		rvSdpChangeText(ptr,&cr->iCryptoKeyInfos[cnt]);
	}
	for (cnt = 0; cnt < cr->iCryptoSessParamsNum; cnt++)
		rvSdpChangeText(ptr,&cr->iCryptoSessParams[cnt]);
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
	rvSdpChangeText(ptr,&cr->iBadSyntaxField);
#endif                
}

/*
 *	Syntax parsing of crypto attribute.
 */
RvBool rvSdpParseCryptoAttr(RvSdpParserData* pD, 
                            char *ptr, 
                            RvSdpCryptoAttr* cattr, 
                            REPLACING_DECL)
{
    char *pp;

    if (!pD)
        return RV_FALSE;

    if (!rvSdpParseInt(0,999999999,&cattr->iCryptoTag,RV_TRUE,RV_FALSE,&ptr,NULL) || !rvSdpParseSpace(&ptr))
    {
        pD->iWarning.iWarningNum = RvSdpWarnCryptoIllegalTag;
        return RV_FALSE;
    }

    cattr->iCryptoSuite = ptr;
    if (!rvSdpParseText2(&ptr,M_AD,'_'))
    {
        pD->iWarning.iWarningNum = RvSdpWarnCryptoIllegalSuite;    
        return RV_FALSE;
    }

    if (!*ptr)
    {
        pD->iWarning.iWarningNum = RvSdpWarnCryptoNoKeyParams;    
        return RV_FALSE;
    }

    pp = ptr;    
    if (!rvSdpParseSpace(&ptr))
    {
        pD->iWarning.iWarningNum = RvSdpWarnCryptoIllegalSuite;
        return RV_FALSE;
    }
    REPLACING2(pp)

    cattr->iCryptoKeyParamsNum = 0;
    cattr->iCryptoSessParamsNum = 0;

    for (;;)
    {
        /* parse key-param*/
        cattr->iCryptoKeyMethods[cattr->iCryptoKeyParamsNum] = ptr;
        if (!rvSdpParseText2(&ptr,M_AD,'_') || *ptr != ':')
        {
            pD->iWarning.iWarningNum = RvSdpWarnCryptoIllegalKeyMethod;    
            return RV_FALSE;
        }
        REPLACING2(ptr);
        if (cattr->iCryptoKeyParamsNum >= RV_SDP_CRYPTO_MAX_KEY_PARAMS)
        {
            pD->iWarning.iWarningNum = RvSdpWarnCryptoIllegalKeyParamsTooMany;    
            return RV_FALSE;
        }
        
        cattr->iCryptoKeyInfos[cattr->iCryptoKeyParamsNum++] = ++ptr;

        if (!rvSdpParseNonSpace(&ptr,';'))
        {
            pD->iWarning.iWarningNum = RvSdpWarnCryptoIllegalKeyInfo;    
            return RV_FALSE;
        }

        pp = ptr;
        if (rvSdpParseSpace(&ptr) || *ptr == 0)
        {
            if (*pp)
                REPLACING2(pp);
            break;
        }

        if (*ptr != ';')
        {
            pD->iWarning.iWarningNum = RvSdpWarnCryptoIllegalKeyInfo;    
            return RV_FALSE;
        }
        
        REPLACING2(ptr);
        ptr ++;
    }

    if (*ptr == 0)
        return RV_TRUE;

    for (;;)
    {
        pp = ptr;
        if (!rvSdpParseNonSpace(&ptr,0))
            break;
        if (cattr->iCryptoSessParamsNum >= RV_SDP_CRYPTO_MAX_SESSION_PARAMS)
        {
            pD->iWarning.iWarningNum = RvSdpWarnCryptoIllegalSessParamsTooMany;    
            return RV_FALSE;
        }        
        cattr->iCryptoSessParams[cattr->iCryptoSessParamsNum++] = pp;
        
        pp = ptr;
        if (*ptr == 0 || !rvSdpParseSpace(&ptr))
            break;
        REPLACING2(pp);
    }    
    return RV_TRUE;
}
    

/*
 *	Special attribute  parse function for the crypto attribute. 
 *  Parses crypto attribute field and constructs the RvSdpCryptoAttr object.
 *  The constructed object is added to current context message or media 
 *  descriptor.
 *  If parsing or construction fails the correspondent status is returned.
 */
RvSdpSpecAttrParseSts rvSdpCryptoParsing(
				const RvSdpSpecAttributeData* sad,  /* the special attribute data */
                RvBool createAsBadSyntax,           /* if set to RV_TRUE the bad 
                                                       syntax crypto attribute will be  
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
        if (rvSdpAddCrypto2(pD->iMsg,cm,0,NULL,v)==NULL)
            return rvSdpSpAttrPrsAllocFail;
#else /*RV_SDP_CHECK_BAD_SYNTAX*/
        return rvSdpSpAttrPrsCrtRegular;
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/
    }
    else 
    {
        RvSdpCryptoAttr cattr,*caP;
        
        if (!rvSdpParseCryptoAttr(pD,v,&cattr,REPLACING_ARGS2))
        {
            pD->iLineStatus = RvSdpValueParseFailed;
            return rvSdpSpAttrPrsFail;
        }
        else if ((caP=rvSdpAddCrypto2(pD->iMsg,cm,cattr.iCryptoTag,cattr.iCryptoSuite,NULL)) == NULL)
            return rvSdpSpAttrPrsAllocFail;
        else
        {
			/* add key and session params*/
            int cnt;
            for (cnt = 0; cnt < cattr.iCryptoKeyParamsNum; cnt++)
                rvSdpCryptoAddKeyParam(caP,cattr.iCryptoKeyMethods[cnt],cattr.iCryptoKeyInfos[cnt]);
            for (cnt = 0; cnt < cattr.iCryptoSessParamsNum; cnt++)
                rvSdpCryptoAddSessionParam(caP,cattr.iCryptoSessParams[cnt]);
        }
    }
    return rvSdpSpAttrPrsOK;
}



#ifndef RV_SDP_USE_MACROS

/***************************************************************************
 * rvSdpCryptoAddSessionParam
 * ------------------------------------------------------------------------
 * General: 
 *      Adds another session parameter of the crypto attribute.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      spar - the session parameter to be added.
 ***************************************************************************/
RvSdpStatus rvSdpCryptoAddSessionParam(RvSdpCryptoAttr* crypto, const char* spar)
{
    return rvSdpAddTextToArray(&(crypto->iCryptoSessParamsNum),RV_SDP_CRYPTO_MAX_KEY_PARAMS,
        crypto->iCryptoSessParams,crypto->iObj,spar);
}

/***************************************************************************
 * rvSdpCryptoRemoveSessionParam
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the crypto session parameter by index.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpCryptoGetNumOfSessionParams call. 
 ***************************************************************************/
void rvSdpCryptoRemoveSessionParam(RvSdpCryptoAttr* crypto, RvSize_t index)
{
    rvSdpRemoveTextFromArray(&(crypto->iCryptoSessParamsNum),crypto->iCryptoSessParams,crypto->iObj,index);
}

/***************************************************************************
 * rvSdpCryptoClearSessionParams
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all session parameters set in crypto special attribute.  
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
void rvSdpCryptoClearSessionParams(RvSdpCryptoAttr* crypto)
{
    rvSdpClearTxtArray(&(crypto->iCryptoSessParamsNum),crypto->iCryptoSessParams,crypto->iObj);
}


#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpCryptoIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the crypto special attribute is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttrRvSdpMsg object.
 ***************************************************************************/
RvBool rvSdpCryptoIsBadSyntax(RvSdpCryptoAttr* crypto)
{
    return (crypto->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpCryptoGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted crypto attribute value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
const char* rvSdpCryptoGetBadSyntax(const RvSdpCryptoAttr* crypto)
{
    return RV_SDP_EMPTY_STRING(crypto->iBadSyntaxField);
}

/***************************************************************************
 * rvSdpCryptoSetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the SDP crypto attribute value to proprietary format.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpCryptoAttr object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
RvSdpStatus rvSdpCryptoSetBadSyntax(
            RvSdpCryptoAttr* o, const char* bs)
{
    return rvSdpSetTextField(&o->iBadSyntaxField,o->iObj,bs);
}

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpCryptoGetTag
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the tag value of the crypto special attribute.
 *          
 * Return Value: 
 *      The requested field of the crypto special attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
RvUint rvSdpCryptoGetTag(const RvSdpCryptoAttr* crypto)
{
    return crypto->iCryptoTag;
}

/***************************************************************************
 * rvSdpCryptoSetTag
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the tag value of the crypto special attribute.
 *          
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      tag - the new value crypto tag.
 ***************************************************************************/
void rvSdpCryptoSetTag(RvSdpCryptoAttr* crypto, RvUint tag)
{
    crypto->iCryptoTag = tag;
}

/***************************************************************************
 * rvSdpCryptoGetSuite
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the suite value of the crypto special attribute.
 *          
 * Return Value: 
 *      The requested field of the crypto special attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
const char* rvSdpCryptoGetSuite(const RvSdpCryptoAttr* crypto)
{
    return RV_SDP_EMPTY_STRING(crypto->iCryptoSuite);
}

/***************************************************************************
 * rvSdpCryptoSetSuite
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the suite value of the crypto special attribute.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      suite - the new value of suite.
 ***************************************************************************/
RvSdpStatus rvSdpCryptoSetSuite(RvSdpCryptoAttr* crypto, const char* suite)
{
    return rvSdpSetTextField(&crypto->iCryptoSuite,crypto->iObj,suite);
}

/***************************************************************************
 * rvSdpCryptoGetNumOfKeyParams
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the number of key parameters set in crypto special attribute.
 *          
 * Return Value: 
 *      Number of key parameters set in crypto special attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
RvSize_t rvSdpCryptoGetNumOfKeyParams(const RvSdpCryptoAttr* crypto)
{
    return crypto->iCryptoKeyParamsNum;
}

/***************************************************************************
* rvSdpCryptoGetKeyMethod
* ------------------------------------------------------------------------
* General: 
*      Gets a crypto method of crypto special attribute by index. 
*          
* Return Value: 
*      The requested crypto method.
* ------------------------------------------------------------------------
* Arguments:
*      crypto - a pointer to the RvSdpCryptoAttr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpCryptoGetNumOfKeyParams call. 
***************************************************************************/
const char* rvSdpCryptoGetKeyMethod(const RvSdpCryptoAttr* crypto, RvSize_t index)
{
    return RV_SDP_EMPTY_STRING((const char*) (crypto->iCryptoKeyMethods[index]));
}

/***************************************************************************
* rvSdpCryptoGetKeyInfo
* ------------------------------------------------------------------------
* General: 
*      Gets a key info of crypto special attribute by index. 
*          
* Return Value: 
*      The requested key info.
* ------------------------------------------------------------------------
* Arguments:
*      crypto - a pointer to the RvSdpCryptoAttr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpCryptoGetNumOfKeyParams call. 
***************************************************************************/
const char* rvSdpCryptoGetKeyInfo(const RvSdpCryptoAttr* crypto, RvSize_t index)
{
    return RV_SDP_EMPTY_STRING((const char*) (crypto->iCryptoKeyInfos[index]));
}

RvSize_t rvSdpCryptoGetNumOfSessionParams(const RvSdpCryptoAttr* crypto)
{
    return crypto->iCryptoSessParamsNum;
}


/***************************************************************************
* rvSdpCryptoGetSessionParam
* ------------------------------------------------------------------------
* General: 
*      Gets a session paramter of crypto special attrbute object by index. 
*          
* Return Value: 
*      The requested session paramter.
* ------------------------------------------------------------------------
* Arguments:
*      crypto - a pointer to the RvSdpCryptoAttr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpCryptoGetNumOfSessionParams() call. 
***************************************************************************/
const char* rvSdpCryptoGetSessionParam(const RvSdpCryptoAttr* crypto, RvSize_t index)
{
    return RV_SDP_EMPTY_STRING((const char*) (crypto->iCryptoSessParams[index]));
}


#endif /*RV_SDP_USE_MACROS*/


#endif /*RV_SDP_CRYPTO_ATTR*/

