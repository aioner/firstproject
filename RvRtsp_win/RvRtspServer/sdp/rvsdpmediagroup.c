/******************************************************************************
Filename    :rvsdpmediagroup.c
Description : Media Group manipulation routines.

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
Author:Galit Edri-Domani
******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rvsdpprivate.h"
#include "rvstrutils.h"
#include "rvsdpprsutils.h"
#include "rvsdpdatastruct.h"
#include "rvsdp.h"
#include "rvbase64.h"

#ifdef RV_SDP_MEDIA_GROUPING_ATTR

    
/***************************************************************************
 * rvSdpMediaGroupAttrConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *	Allocates and constructs the object using RvAlloc allocator.
 *  Returns the constructed object or NULL if it was failed.
 .
 * Return Value: 
 *      A pointer to the new added RvSdpMediaGroupAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          obj - points to RvSdpMsg or RvAlloc instance.
 *          eSemantics - the semantics str.
 *          badSyn - proprietary syntax Media-Group attribute or NULL.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMediaGroupAttr* rvSdpMediaGroupAttrConstruct2(
            void*                   obj,        
            const char*             semantics, 
            const char*             badSyn)     
{
    RvSdpMediaGroupAttr*  mediaGroupAttr;
    RvAlloc            *  alloc;
    RV_UNUSED_ARG(badSyn);
    
    /*
     *	unlike other SDP data structures this one does not allocate itself
     *  through dedicated objects pool but directly
     */
    
    /*check which alloactor to use: in the old way or the new way*/
    if (RV_SDP_OBJ_IS_MESSAGE2(obj))
        alloc = ((RvSdpMsg*)obj)->iAllocator;
    else /*old way*/
        alloc = (RvAlloc*) obj;

    if (!alloc)
        alloc = rvSdpGetDefaultAllocator();
    
    /*create the "a=group" attribute*/
    mediaGroupAttr = rvSdpAllocAllocate(alloc,sizeof(RvSdpMediaGroupAttr));
    if (!mediaGroupAttr)
        return NULL;
    
    memset(mediaGroupAttr,0,sizeof(RvSdpMediaGroupAttr));
    
    mediaGroupAttr->iAlloc = alloc;
    
    if (!obj)
        obj = alloc;
    
    mediaGroupAttr->iObj = obj;
    mediaGroupAttr->iMidIdParamsNum = 0;

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&mediaGroupAttr->iBadSyntaxField,obj,badSyn) != RV_SDPSTATUS_OK)
            goto failed;
    }
    else
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    {
        if (rvSdpSetTextField(&mediaGroupAttr->ieSemanticsType,obj,semantics) != RV_SDPSTATUS_OK )
            goto failed;
    } 
    
    mediaGroupAttr->iGroupAttribute.iSpecAttrData = rvSdpFindSpecAttrDataByFieldType(SDP_FIELDTYPE_MEDIA_GROUP);
    return mediaGroupAttr;
    
failed:
    if (mediaGroupAttr)
        rvSdpMediaGroupAttrDestruct(mediaGroupAttr);
    return NULL;
}


/***************************************************************************
 * rvSdpAddMediaGroupAttr2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the Group atribute to msg obj.
 *          
 * Return Value: 
 *      A pointer to the added RvSdpMediaGroupAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          msg - message where addition will be performed.
 *          commF - common fields pointer of message or media.
 *          semantics - the semantics string.
 *          badSyn - proprietary syntax Media-Group attribute.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMediaGroupAttr* rvSdpAddMediaGroupAttr2(
                                   RvSdpMsg  *             msg,  
                                   RvSdpCommonFields*      commF,
                                   const char*             semantics,
                                   const char*             badSyn)
{
    RvSdpMediaGroupAttr* mediaGroupAttr;
    int buffSize;
    
    buffSize = (badSyn)?(int)strlen(badSyn):0;
    buffSize += (semantics)?(int)strlen(semantics+2):0;

    rvSdpMsgPromiseBuffer(msg,buffSize);
    
    mediaGroupAttr = rvSdpMediaGroupAttrConstruct2(msg,semantics,badSyn);
    if (!mediaGroupAttr)
        return NULL;    
    
    rvSdpAddAttr2(msg,commF,&(mediaGroupAttr->iGroupAttribute),"group",NULL);    
    return mediaGroupAttr;
}

/***************************************************************************
 * rvSdpMediaGroupAttrCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpMediaGroupAttr from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpMediaGroupAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpMediaGroupAttr object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMediaGroupAttr* rvSdpMediaGroupAttrCopy2(
                                       RvSdpMediaGroupAttr      * dest, 
                                       const RvSdpMediaGroupAttr* src, 
                                       void                  * obj)
{
    int cnt;

    dest = rvSdpMediaGroupAttrConstruct2(obj,
                                     src->ieSemanticsType,
                                     RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField));
    if (dest == NULL)
        return NULL;

        
    if (obj != NULL)
    {
        RvSdpMsg *msg;        
        
        msg = (RV_SDP_OBJ_IS_MESSAGE2(obj))?(RvSdpMsg*)obj:NULL;
        
        (void)rvSdpAttributeConstruct2( msg,
                                       &dest->iGroupAttribute,
                                        src->iGroupAttribute.iAttrName,
                                        src->iGroupAttribute.iAttrValue,
                                        obj,
                                        RV_FALSE);
    }
    /*add the midId tag list*/
    for (cnt = 0; cnt < src->iMidIdParamsNum; cnt++)
    {
        rvSdpMediaGroupAttrAddMid(dest,src->iMidIdParams[cnt]);
    }

    return dest;
}

/***************************************************************************
 * rvSdpMediaGroupAttrDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the instance of RvSdpMediaGrouptAttr.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - object that should be destructed.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMediaGroupAttrDestruct(RvSdpMediaGroupAttr* mediaGroupAttr)
{
    rvSdpUnsetTextField(&mediaGroupAttr->ieSemanticsType,mediaGroupAttr->iObj);

    /*free mediaGroupAttr->iMidIdParams */
    rvSdpMediaGroupAttrClearMidParams(mediaGroupAttr);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&mediaGroupAttr->iBadSyntaxField,mediaGroupAttr->iObj);
#endif    
    /* Deallocates RvSdpAttribute netto memory	*/
    rvSdpAttributeDestructNetto(&mediaGroupAttr->iGroupAttribute,RV_FALSE);    
    rvSdpAllocDeallocate(mediaGroupAttr->iAlloc,sizeof(RvSdpMediaGroupAttr),mediaGroupAttr);
}

/***************************************************************************
 * rvSdpMediaGroupAttrGetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Prints the textual value of Media Group attribute  into provided 
 *      buffer (prints all the fields values following "a=group:____________________".
 *      for example :"FID 1 2 3", "SRF 21 22 23"   
 * Return Value: 
 *      Returns the buffer pointer on success or NULL if fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *          mediaGroupAttr - instance of RvSdpMediaGroupAttr.
 *          txtVal         - the buffer for the value.
 ***************************************************************************/
RVSDPCOREAPI char* rvSdpMediaGroupAttrGetValue(RvSdpMediaGroupAttr* mediaGroupAttr, char *txtVal)
{
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (mediaGroupAttr->iBadSyntaxField)
    {
        strcpy(txtVal,mediaGroupAttr->iBadSyntaxField);
    }
    else
#endif  
    {
        int   cnt;
        char *p;
        
        sprintf(txtVal,"%s",RV_SDP_STRING_NOT_SET(mediaGroupAttr->ieSemanticsType));
        p = txtVal + strlen(txtVal);
        for (cnt = 0; cnt < mediaGroupAttr->iMidIdParamsNum; cnt++)
        {
            *(p++) = ' ';
           
            sprintf(p,"%s",mediaGroupAttr->iMidIdParams[cnt]);
            p += strlen(p);
        }
    }
    return txtVal;

}


/***************************************************************************
* rvSdpMediaGroupAttrGetMid
* ------------------------------------------------------------------------
* General: 
*      Gets one MidId Parameter of Media-Group special attrbute by index. 
*          
* Return Value: 
*      The requested session paramter.
* ------------------------------------------------------------------------
* Arguments:
*      mediaGroupAttr - a pointer to the RvSdpCryptoAttr object.
*      index - the index. The index should start at zero (0) and must be smaller 
*              than the number of elements in the list. The number of elements 
*              in the list is retrieved by rvSdpMediaGroupAttrGetNumOfMidParams() call. 
***************************************************************************/
RVSDPCOREAPI const char* rvSdpMediaGroupAttrGetMid(const RvSdpMediaGroupAttr* mediaGroupAttr, 
                                                RvSize_t                   index)
{
    /*if the index is bigger then the num of midId tags*/
    if(index >= mediaGroupAttr->iMidIdParamsNum)
    {
       return "";
    }
    return RV_SDP_EMPTY_STRING((const char*)(mediaGroupAttr->iMidIdParams[index]));
}


/***************************************************************************
 * rvSdpMediaGroupAttrReshuffle
 * ------------------------------------------------------------------------
 * General: 
 *      Called during message strings buffer reshuffle.
 *      Copies all string fields of Media-Group into new '*ptr' while
 *      increasing '*ptr' value by the length of copied strings. 
 *      reshuffle- is called when 
 * Return Value: void
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 *      ptr            - new pointer to the new string buffer (after reshuffling)
 *
 ***************************************************************************/
void rvSdpMediaGroupAttrReshuffle(RvSdpMediaGroupAttr* mediaGroupAttr, char** ptr)
{
    int cnt;

	rvSdpChangeText(ptr,&mediaGroupAttr->ieSemanticsType);
    
    for (cnt = 0; cnt < mediaGroupAttr->iMidIdParamsNum; cnt++)
        rvSdpChangeText(ptr,&mediaGroupAttr->iMidIdParams[cnt]);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
	rvSdpChangeText(ptr,&mediaGroupAttr->iBadSyntaxField);
#endif                	
}


/***************************************************************************
 * rvSdpParseMediaGroupAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Performs the parsing of Media-Group attribute and updates the
 *      mediaGroupAttr struct.
 * Return Value: 
 *      TRUE if the parsing operation succeed.FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      parserData - a pointer to the RvSdpParserData object.
 *      ptr - a pointer to the begining of the group value string to be parsed.
 *      mediaGroupAttr - the mediaGroupAttr struct that is filled during parsing.
 ***************************************************************************/
RvBool rvSdpParseMediaGroupAttr(RvSdpParserData    * parserData, 
                                char               * ptr, 
                                RvSdpMediaGroupAttr* mediaGroupAttr, 
                                REPLACING_DECL)
{
    char * pp;
    char * ppMid;
    
    if (!parserData)
        return RV_FALSE;
    
    /*skip the spaces (if exist) at the beginning*/
    rvSdpParseSpace(&ptr);

    /*save the position of the start of semantics str*/
    mediaGroupAttr->ieSemanticsType = ptr;
    rvSdpParseNonSpace(&ptr,0);
    /*save the end of semantics str*/
    pp=ptr;
    
    /*promote ptr to the start of midId list :skip the spaces till then*/
    if (!rvSdpParseSpace(&ptr)) 
    {
        parserData->iWarning.iWarningNum = RvSdpWarnMediaGroupNoMidIdParams;
        return RV_FALSE;
    }
    
    /*mark the end of semantics string*/
    REPLACING2(pp);

    /*save the start of the midId params*/
    ppMid=pp=ptr;
    
    mediaGroupAttr->iMidIdParamsNum = 0;
    
    for (;;)
    {
        mediaGroupAttr->iMidIdParams[mediaGroupAttr->iMidIdParamsNum++] = ppMid;

        if (!rvSdpParseNonSpace(&ptr,0))
            break;
        
        /*points to the space ahead of the next midId val in the list */
        pp=ptr;
        
        /*check if there aren't too many midId params*/
        if (mediaGroupAttr->iMidIdParamsNum >= RV_SDP_MAX_NUM_OF_MID_ID_IN_LIST)
        {
            parserData->iWarning.iWarningNum = RvSdpWarnMediaGroupMidIdParamsTooMany;    
            return RV_FALSE;
        }


        /*if it's the end of the list*/
        if (*ptr == 0 || !rvSdpParseSpace(&ptr))
        {
            /*check if the midId tag is a positive integer*/
            if (!rvSdpParseText(&ppMid,M_D))
            {
                /*we don't fail the parsing if the midVal is not an integer*/
                parserData->iWarning.iWarningNum = RvSdpWarnMediaGroupIllegalMidIdVal;    
            }

            return RV_TRUE;
        }

        REPLACING2(pp);
        
        /*check if the midId tag is a positive integer*/
        if (!rvSdpParseText(&ppMid,M_D))
        {
            parserData->iWarning.iWarningNum = RvSdpWarnMediaGroupIllegalMidIdVal;    
        }
        ppMid = ptr;
    } 
    return RV_TRUE;
}


/***************************************************************************
 * rvSdpMediaGroupAttrParsing
 * ------------------------------------------------------------------------
 * General: 
 *	   Special attribute  parse function for the media-group attribute. 
 *     Parses media-group attribute field and constructs the RvSdpMediaGroupAttr object.
 *     The constructed object is added to current context message. 
 *     (Media-Group attr can appear only in the message level.
 *
 *     An Example to group attribute that should be parsed may look like:    
 *          "a=group:FID 22 23 24"  or:  "a=group:LS 5 8" or: "a=group:LS "
 *
 * Return Value: 
 *     If parsing or construction fails the correspondent status is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      parserData - a pointer to the RvSdpParserData object.
 *      ptr - a pointer to the begining of the group value string to be parsed.
 *      mediaGroupAttr - the mediaGroupAttr struct that is filled during parsing.
 ***************************************************************************/
RvSdpSpecAttrParseSts rvSdpMediaGroupAttrParsing(
				const RvSdpSpecAttributeData* specAttrData, /* the special attribute data */
                RvBool              createAsBadSyntax, /* if set to RV_TRUE the bad 
                                                          syntax media-group attribute  
                                                          will be created */
                RvSdpParserData   * parserData,   /* the parser data instance */        
                RvSdpCommonFields * commonFields, /* the current context common fields instance,
                                                     here the special attribute will be added */
				char              * name,        /* the attribute name */
                char              * val,       /* the attribute value to parse */
                REPLACING_DECL)         /* used for zero-substitutions in input buffer */
{
	UNUSE_REPLACING_DECL;
	RV_UNUSED_ARG(name);
	RV_UNUSED_ARG(specAttrData);

	if (createAsBadSyntax)
	{
#ifdef RV_SDP_CHECK_BAD_SYNTAX
		if (rvSdpAddMediaGroupAttr2(parserData->iMsg,
                                    commonFields,
                                    NULL,
                                    val) == NULL)
            return rvSdpSpAttrPrsAllocFail;
        else 
            return rvSdpSpAttrPrsOK;

#else /*RV_SDP_CHECK_BAD_SYNTAX*/
        return rvSdpSpAttrPrsCrtRegular;
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/
	}
	else 
	{
        RvSdpMediaGroupAttr  mediaGroupAttr;
		RvSdpMediaGroupAttr* mediaGroupAttrPtr;

        /*parsing the group attribute - and fill in its fields*/
		if (!rvSdpParseMediaGroupAttr(parserData,val,&mediaGroupAttr,REPLACING_ARGS2))
		{
			parserData->iLineStatus = RvSdpValueParseFailed;
			return rvSdpSpAttrPrsFail;
		}
        else
        {
            /*add the media-group attribute */
            mediaGroupAttrPtr = rvSdpAddMediaGroupAttr2(
                                       parserData->iMsg,
                                       commonFields,
                                       mediaGroupAttr.ieSemanticsType,
                                       NULL);
            if(mediaGroupAttrPtr == NULL)
            {
                return rvSdpSpAttrPrsAllocFail;
            }
            else
            {
                /* add midId-tag parameters into the media-group attribute*/
                int cnt;
                for (cnt = 0; cnt < mediaGroupAttr.iMidIdParamsNum; cnt++)
                    rvSdpMediaGroupAttrAddMid(mediaGroupAttrPtr,
                    mediaGroupAttr.iMidIdParams[cnt]);
            }
        }
	}
	return rvSdpSpAttrPrsOK;	
}


/****************************************************************************/
/****************************************************************************/
/*The following are all the function that also appears in MACRO. 
  incase the flag RV_SDP_USE_MACROS was not defined*/
/****************************************************************************/


#ifndef RV_SDP_USE_MACROS
/***************************************************************************
* rvSdpMediaGroupAttrGetNumOfMidParams
* ------------------------------------------------------------------------
* General: 
*      Gets a iMidIdParamsNum of media-Group special attrbute. 
*          
* Return Value: 
*      The requested iMidIdParamsNum paramter.
* ------------------------------------------------------------------------
* Arguments:
*      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
*
***************************************************************************/
RVSDPCOREAPI RvSize_t rvSdpMediaGroupAttrGetNumOfMidParams(const RvSdpMediaGroupAttr* mediaGroupAttr)
{
    return mediaGroupAttr->iMidIdParamsNum;
}

/***************************************************************************
 * rvSdpMediaGroupAttrClearMidParams
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) all midId tag parameters set in media-Group 
 *      special attribute.  
 *          
 * Return Value: None
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMediaGroupAttrClearMidParams(RvSdpMediaGroupAttr* mediaGroupAttr)
{
    rvSdpClearTxtArray(&(mediaGroupAttr->iMidIdParamsNum),
                         mediaGroupAttr->iMidIdParams,
                         mediaGroupAttr->iObj);
    
}

/***************************************************************************
 * rvSdpMediaGroupAttrAddMid
 * ------------------------------------------------------------------------
 * General: 
 *      Adds another MidId tag parameter to the MediaGroup attribute.
 *      mediaGroupAttr->iMidIdParamsNum will be increased by 1 -if the 
 *      mid-tag was added.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 *      midTag - the midTag parameter to be added.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaGroupAttrAddMid(RvSdpMediaGroupAttr* mediaGroupAttr, 
                                                   const char *         midTag)
{

    return  rvSdpAddTextToArray(&(mediaGroupAttr->iMidIdParamsNum),
                               RV_SDP_MAX_NUM_OF_MID_ID_IN_LIST,
                               mediaGroupAttr->iMidIdParams,
                               mediaGroupAttr->iObj,
                               midTag);
}

/***************************************************************************
 * rvSdpMediaGroupAttrRemoveMid
 * ------------------------------------------------------------------------
 * General: 
 *      Removes (and destructs) the MediaGroup midId parameter by index.  
 *      mediaGroupAttr->iMidIdParamsNum will be dencreased by 1 -if the 
 *      mid-tag was removed.    
 * Return Value: 
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 *      index - the index. The index should start at zero (0) and must be smaller 
 *              than the number of elements in the list. The number of elements 
 *              in the list is retrieved by correspondent 
 *              rvSdpMediaGroupAttrGetNumOfMidParams() call. 
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMediaGroupAttrRemoveMid(RvSdpMediaGroupAttr* mediaGroupAttr, 
                                               RvSize_t             index)
{
    rvSdpRemoveTextFromArray(&(mediaGroupAttr->iMidIdParamsNum),
        mediaGroupAttr->iMidIdParams,
        mediaGroupAttr->iObj,
        index);
}


/***************************************************************************
 * rvSdpMediaGroupAttrGetSemanticsStr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the semantics as text value of the media-group attribute ("LS"/"FID"/"SRF").
 *          
 * Return Value: 
 *      The requested semantics str of the media-group attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpMediaGroupAttrGetSemanticsStr(
                                               const RvSdpMediaGroupAttr* mediaGroupAttr)
{    
    return RV_SDP_EMPTY_STRING(mediaGroupAttr->ieSemanticsType);
}

/***************************************************************************
 * rvSdpMediaGroupAttrGetSemantics
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the semantics as enum value of the media-group attribute ("LS"/"FID"/"SRF").
 *          
 * Return Value: 
 *      The requested field of the media-group attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpGroupSemanticsType rvSdpMediaGroupAttrGetSemantics(
                                               const RvSdpMediaGroupAttr* mediaGroupAttr)
{  
    return 
        rvSdpGroupSemanticTypeTxt2Val(RV_SDP_EMPTY_STRING(mediaGroupAttr->ieSemanticsType));
}

/***************************************************************************
 * rvSdpMediaGroupAttrSetSemanticsStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the semantics as str value in the media-group attribute ("LS"/"FID"/"SRF").
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 *      prtclId - the new value of protocol ID text.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaGroupAttrSetSemanticsStr(
                                               RvSdpMediaGroupAttr *  mediaGroupAttr,
                                               const char*            semanticsStr)
{ 

   return rvSdpSetTextField(&mediaGroupAttr->ieSemanticsType,
                            mediaGroupAttr->iObj,
                            semanticsStr);        
}

/***************************************************************************
 * rvSdpMediaGroupAttrSetSemantics
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the semantics as enum value to the media-group attribute ("LS"/"FID"/"SRF").
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 *      eSemantics - the new value of semantics.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaGroupAttrSetSemantics(
                                            RvSdpMediaGroupAttr *    mediaGroupAttr,
                                            RvSdpGroupSemanticsType  eSemantics)
{
    return rvSdpMediaGroupAttrSetSemanticsStr(mediaGroupAttr,
                                              rvSdpGroupSemanticTypeVal2Txt(eSemantics));
}



#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMediaGroupAttrIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the Media-Group attribute is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 ***************************************************************************/
RVSDPCOREAPI RvBool rvSdpMediaGroupAttrIsBadSyntax(RvSdpMediaGroupAttr* mediaGroupAttr)
{
    return (mediaGroupAttr->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpMediaGroupAttrGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted Media-Group attribute value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      mediaGroupAttr - a pointer to the RvSdpMediaGroupAttr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpMediaGroupAttrGetBadSyntax(const RvSdpMediaGroupAttr* mediaGroupAttr)
{
    return RV_SDP_EMPTY_STRING(mediaGroupAttr->iBadSyntaxField);
}
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */




#endif /*RV_SDP_USE_MACROS*/


#endif /*RV_SDP_MEDIA_GROUPING_ATTR*/

