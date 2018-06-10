/******************************************************************************
Filename    :rvsdpprecondition.c
Description : Preconditions manipulation routines.

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

#ifdef RV_SDP_PRECONDITIONS_ATTR

    
/***************************************************************************
 * rvSdpPreconditionAttrConstruct2
 * ------------------------------------------------------------------------
 * General: 
 *	Allocates and constructs the object using RvAlloc allocator.
 *  Returns the constructed object or NULL if it was failed.
 *  Precondition attribute represents 3 precond attributes: 
 *     1.'a=curr'  (current)
 *     2.'a=des'   (desired)
 *     3.'a=conf'  (confirmed)
 *  The syntax is: 'a=curr:preconditionType  statusType directionTag'
 *                 'a=des :preconditionType  strengthTag statusType directionTag'
 *                 'a=conf:preconditionType  statusType directionTag
 *
 *  Notice- that only the "des" attr contains the 'strengthTag'.for the others it's null.
 *  This attribute can apear only in media-descr level multiple appearence.  
 *  
 * Return Value: 
 *      A pointer to the new added RvSdpPreconditionAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          obj - points to RvSdpMsg or RvAlloc instance.
 *          precondTypeStr      - the precondition-type str.
 *          precondStatusStr    - the precondition-status str.
 *          precondStrengthStr  - the precondition-strength str.
 *          precondDirectionStr - the precondition-direction str.
 *          badSyn              - proprietary syntax precondition attribute or NULL.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPreconditionAttr* rvSdpPreconditionAttrConstruct2(
                             void*                   obj,        
                             const char*             precondTypeStr, 
                             const char*             precondStatusStr,
                             const char*             precondStrengthStr,
                             const char*             precondDirectionStr,
                             const char*             badSyn)     
{

    RvSdpPreconditionAttr*  precondAttr;
    RvAlloc              *  alloc;
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
    
    /*create precondition attribute*/
    precondAttr = rvSdpAllocAllocate(alloc,sizeof(RvSdpPreconditionAttr));
    if (!precondAttr)
        return NULL;
    
    memset(precondAttr,0,sizeof(RvSdpPreconditionAttr));
    
    precondAttr->iAlloc = alloc;
    
    if (!obj)
        obj = alloc;
    
    precondAttr->iObj = obj;


#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (badSyn)
    {
        /* allocate the strings on the msg strings buffer */
        if (rvSdpSetTextField(&precondAttr->iBadSyntaxField,obj,badSyn) != RV_SDPSTATUS_OK) 

            goto failed;
    }
    else
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */
    {
        /*strengthTag is null if it's 'a=curr' or 'a=conf' preconditions.*/
        if((rvSdpSetTextField(&precondAttr->iPrecondType,obj,precondTypeStr) != RV_SDPSTATUS_OK )||
           (rvSdpSetTextField(&precondAttr->iStatusType,obj,precondStatusStr) != RV_SDPSTATUS_OK )||
           (rvSdpSetTextField(&precondAttr->iStrengthTag,obj,precondStrengthStr) != RV_SDPSTATUS_OK )||
           (rvSdpSetTextField(&precondAttr->iDirectionTag,obj,precondDirectionStr) != RV_SDPSTATUS_OK ))
            goto failed;
    } 
    
    precondAttr->iPreconditionAttribute.iSpecAttrData = rvSdpFindSpecAttrDataByFieldType(SDP_FIELDTYPE_PRECONDITION);
    return precondAttr;
    
failed:
    if (precondAttr)
        rvSdpPreconditionAttrDestruct(precondAttr);
    return NULL;
}

/***************************************************************************
 * rvSdpAddPreconditionAttr2
 * ------------------------------------------------------------------------
 * General: 
 *      Constructs and  adds the Precondition atribute to media descr obj.
 *      First promise the buffer size and then constructs the RvSdpPreconditionAttr,
 *      and set the iePrecondName value in the precond struct according to 
 *      the given precondName str.
 * 
 *      Precondition attriibute can be added only at media-descr level.
 *
 * Return Value: 
 *      A pointer to the added RvSdpPreconditionAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *          descr - media where precond att will be added.
 *          commF - common fields pointer of message or media.
 *          precondName  - the precondition-name str.("curr"/"des"/"conf")
 *          typeStr      - the precondition-type str.
 *          statusStr    - the precondition-status str.
 *          strengthStr  - the precondition-strength str.
 *          directionStr - the precondition-direction str.
 *          badSyn - proprietary syntax Media-Group attribute.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPreconditionAttr* rvSdpAddPreconditionAttr2(
                                   RvSdpMediaDescr  *      descr,  
                                   RvSdpCommonFields*      commF,
                                   const char*             precondName, 
                                   const char*             typeStr, 
                                   const char*             statusStr,
                                   const char*             strengthStr,
                                   const char*             directionStr,
                                   const char*             badSyn)
{
    RvSdpPreconditionAttr* precondAttr;
    int buffSize;
    
    /*promise buffer*/
    buffSize = (badSyn)?(int)strlen(badSyn):0;
    buffSize += (typeStr)?(int)strlen(typeStr):0;
    buffSize += (statusStr)?(int)strlen(statusStr):0;
    buffSize += (strengthStr)?(int)strlen(strengthStr):0;
    buffSize += (directionStr)?(int)strlen(directionStr):0;
    
    rvSdpMsgPromiseBuffer(descr->iObj,buffSize+10);
    
    precondAttr = rvSdpPreconditionAttrConstruct2(descr->iObj,
                                                  typeStr,
                                                  statusStr,
                                                  strengthStr,
                                                  directionStr,
                                                  badSyn);
    /*save the name as enum in the precond struct in order to save in calls to strcmp()*/
    precondAttr->iePrecondName = rvSdpPreconditionNameTxt2Val(precondName);
    
    if (!precondAttr)
        return NULL;    
    
    rvSdpAddAttr2(descr->iObj,
                  commF,
                  &(precondAttr->iPreconditionAttribute),
                  precondName,
                  NULL); 		
   
    return precondAttr;
}




/***************************************************************************
 * rvSdpPreconditionAttrCopy2
 * ------------------------------------------------------------------------
 * General: 
 *      Copies the instance of RvSdpPreconditionAttr from 'src' to 'dest'. 
 *      If the destination object is NULL pointer, the destination
 *      object will be allocated & constructed within the function.
 *          
 * Return Value: 
 *      A pointer to the input RvSdpPreconditionAttr object, or NULL if the 
 *      function fails
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpPreconditionAttr object or NULL. 
 *      src - a pointer to the source object
 *      obj - the RvSdpMsg instance that will own the destination object
 *            or pointer to allocator.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPreconditionAttr* rvSdpPreconditionAttrCopy2(
                                       RvSdpPreconditionAttr      * dest, 
                                       const RvSdpPreconditionAttr* src, 
                                       void                  * obj)
{

    dest = rvSdpPreconditionAttrConstruct2(obj,
                                           src->iPrecondType,
                                           src->iStatusType,
                                           src->iStrengthTag,
                                           src->iDirectionTag,
                                           RV_SDP_BAD_SYNTAX_PARAM(src->iBadSyntaxField));
    if (dest == NULL)
        return NULL;
        
    if (obj != NULL)
    {
        RvSdpMsg *msg;  
		
		/*copy iePrecondName*/
        dest->iePrecondName = src->iePrecondName;

        
        msg = (RV_SDP_OBJ_IS_MESSAGE2(obj))?(RvSdpMsg*)obj:NULL;
        
        (void)rvSdpAttributeConstruct2( msg,
                                       &dest->iPreconditionAttribute,
                                        src->iPreconditionAttribute.iAttrName,
                                        src->iPreconditionAttribute.iAttrValue,
                                        obj,
                                        RV_FALSE);
    }
    return dest;
}
/***************************************************************************
 * rvSdpPreconditionAttrDestruct
 * ------------------------------------------------------------------------
 * General: 
 *      Destroys the instance of RvSdpPreconditiontAttr.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr - object that should be destructed.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpPreconditionAttrDestruct(RvSdpPreconditionAttr* precondAttr)
{
    rvSdpUnsetTextField(&precondAttr->iPrecondType,precondAttr->iObj);
    rvSdpUnsetTextField(&precondAttr->iStatusType,precondAttr->iObj);
    rvSdpUnsetTextField(&precondAttr->iDirectionTag,precondAttr->iObj);
    rvSdpUnsetTextField(&precondAttr->iStrengthTag,precondAttr->iObj);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    rvSdpUnsetTextField(&precondAttr->iBadSyntaxField,precondAttr->iObj);
#endif    
    /* Deallocates RvSdpAttribute netto memory	*/
    rvSdpAttributeDestructNetto(&precondAttr->iPreconditionAttribute,RV_FALSE);    
    rvSdpAllocDeallocate(precondAttr->iAlloc,sizeof(RvSdpPreconditionAttr),precondAttr);
}

/***************************************************************************
 * rvSdpPreconditionAttrGetValue
 * ------------------------------------------------------------------------
 * General: 
 *      Prints the textual value of precondition attribute  into provided 
 *      buffer (prints all the fields values following one of the precondition names 
 *      1. For example for: "a=curr:____________________".
 *         value can be:  "qos e2e send", 
 *      2. For example for: "a=des:_______________________" 
 *         value  can be :"qos e2e optional sendrecv"
 * Return Value: 
 *      Returns the buffer pointer on success or NULL if fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *          precondAttr - instance of RvSdpPreconditionAttr.
 *          txtVal      - the buffer for the precond value.
 ***************************************************************************/
RVSDPCOREAPI char* rvSdpPreconditionAttrGetValue(RvSdpPreconditionAttr* precondAttr, 
                                                 char                 * txtVal)
{
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    if (precondAttr->iBadSyntaxField)
    {
        strcpy(txtVal,precondAttr->iBadSyntaxField);
    }
    else
#endif  
    {
        //if (strcmp(precondAttr->iPreconditionAttribute.iAttrName,"des") == 0)
		if(precondAttr->iePrecondName == RV_SDP_PRECOND_NAME_DESIRED_ATTR)
        {
            sprintf(txtVal,"%s %s %s %s",RV_SDP_STRING_NOT_SET(precondAttr->iPrecondType),
                                         RV_SDP_STRING_NOT_SET(precondAttr->iStrengthTag),
                                         RV_SDP_STRING_NOT_SET(precondAttr->iStatusType),
                                         RV_SDP_STRING_NOT_SET(precondAttr->iDirectionTag));
        }
        else
        {
            sprintf(txtVal,"%s %s %s",RV_SDP_STRING_NOT_SET(precondAttr->iPrecondType),
                                      RV_SDP_STRING_NOT_SET(precondAttr->iStatusType),
                                      RV_SDP_STRING_NOT_SET(precondAttr->iDirectionTag));
        }
    }
    return txtVal;
}

/***************************************************************************
 * rvSdpPreconditionAttrReshuffle
 * ------------------------------------------------------------------------
 * General: 
 *      Called during message strings buffer reshuffle.
 *      Copies all string fields of Media-Group into new '*ptr' while
 *      increasing '*ptr' value by the length of copied strings. 
 *      reshuffle- is called when 
 * Return Value: void
 * ------------------------------------------------------------------------
 * Arguments:
 *      PreconditionAttr - a pointer to the RvSdpPreconditionAttr object.
 *      ptr              - new pointer to the new string buffer (after reshuffling)
 *
 ***************************************************************************/
void rvSdpPreconditionAttrReshuffle(RvSdpPreconditionAttr* precondAttr, char** ptr)
{

    rvSdpChangeText(ptr,&precondAttr->iPrecondType);
    rvSdpChangeText(ptr,&precondAttr->iStatusType);
    rvSdpChangeText(ptr,&precondAttr->iDirectionTag);
    rvSdpChangeText(ptr,&precondAttr->iStrengthTag);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
	rvSdpChangeText(ptr,&precondAttr->iBadSyntaxField);
#endif                	
}


/***************************************************************************
 * rvSdpParsePreconditionAttr
 * ------------------------------------------------------------------------
 * General: 
 *      Performs the parsing of precondition attribute and updates the
 *      precondAttr struct with the parsed data.
 * Return Value: 
 *      TRUE if the parsing operation succeed.FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      parserData - a pointer to the RvSdpParserData object.
 *      ptr - a pointer to the begining of the group value string to be parsed.
 *      precondAttr - the PreconditionAttr struct that is filled during parsing.
 ***************************************************************************/
RvBool rvSdpParsePreconditionAttr(RvSdpParserData    * parserData, 
                                  char               * ptr,
                                  char               * name,   
                                  RvSdpPreconditionAttr* precondAttr, 
                                  REPLACING_DECL)
{
    char * pp;
    RvBool rv;
   /* int    cnt = 0;*/
    char* param2 = NULL,*param3= NULL,*param4= NULL;
    
    RV_UNUSED_ARG(name);

    if (!parserData)
        return RV_FALSE;
    
    /* save the original ptr and move with pp*/
    /* find the start of the 1'st str param*/
    rvSdpParseSpace(&ptr);

	/*if no parameter was found*/
    if(*ptr==0)
    {
        parserData->iWarning.iWarningNum = RvSdpWarnPrecondNoParameters;
        return RV_FALSE;
    }
    
    /* start parsing*/
    
    /* iPrecondType saves the position of the start of precondType str*/
	precondAttr->iPrecondType = ptr;
    /*cnt++;*/
	rvSdpParseNonSpace(&ptr,0);
    
    /* pp saves the end of precondType str*/
    pp=ptr;
    
    /* promote ptr to the start of 2'nd str param :skip the spaces till then*/
    rvSdpParseSpace(&ptr);

    /* mark the end of precondType string*/
    REPLACING2(pp);

    /* param2 saves the position of the start of 2'nd str parameter*/
    if (*ptr == 0) 
	{
		parserData->iWarning.iWarningNum = RvSdpWarnPrecondNumOfParametersIllegal;
		return RV_FALSE;
		
	}

    param2 = ptr;
    /*cnt++;*/
    rvSdpParseNonSpace(&ptr,0);

    /* pp saves the end of param2 str*/
    pp=ptr;

    /* promote ptr to the start of 3'rd str param :skip the spaces till then*/
    rvSdpParseSpace(&ptr);
    
    /* pp marks the end of param2 string*/
    REPLACING2(pp);
    
    /* param3 saves the position of the start of 3'rd str parameter*/
    if (*ptr == 0) 
	{
		parserData->iWarning.iWarningNum = RvSdpWarnPrecondNumOfParametersIllegal;
		return RV_FALSE;
	}

    param3 = ptr;
    /*cnt++;*/
    rvSdpParseNonSpace(&ptr,0);

	/* pp saves the end of param3 str*/
    pp=ptr;
    
    rvSdpParseSpace(&ptr);
    /*mark the end of param3 string*/
    REPLACING2(pp);

    /* lets try to look for a fourth field */
    if (*ptr)
    {
        param4 = ptr;
        rvSdpParseNonSpace(&ptr,0);
        pp = ptr;
        rv = rvSdpParseSpace(&ptr);
        REPLACING2(pp);
    }
    else
        rv = RV_FALSE;

    /* we want to catch the following illegal cases:
        1. the precondName is "curr" or "conf" and there is fourth parameter;
        2. the precondName is "des" and there is more than four parameters 
           (in this case the rv will be RV_TRUE) or less than four parameters 
           (param4 is NULL)
    */
    if ((param4 && 
            ((precondAttr->iePrecondName == RV_SDP_PRECOND_NAME_CURRENT_ATTR) ||  /* 1. */
            (precondAttr->iePrecondName == RV_SDP_PRECOND_NAME_CONFIRMED_ATTR))) ||
        (precondAttr->iePrecondName == RV_SDP_PRECOND_NAME_DESIRED_ATTR && (!param4 || rv))) /*2. */
    {
        parserData->iWarning.iWarningNum = RvSdpWarnPrecondTooManyParameters;
        return RV_FALSE;       
    }

    if (param4)
    {
        precondAttr->iStrengthTag=param2; 
        precondAttr->iStatusType=param3;
        precondAttr->iDirectionTag=param4;        
    }
    else
    {
        precondAttr->iStrengthTag=NULL;
        precondAttr->iStatusType=param2;
        precondAttr->iDirectionTag=param3;
    }

    return RV_TRUE;
}


/***************************************************************************
 * rvSdpPreconditionAttrParsing
 * ------------------------------------------------------------------------
 * General: 
 *	   Special attribute parse function for the precondition attribute. 
 *     Parses precondition attribute field and constructs the RvSdpPreconditionAttr object.
 *     The constructed object is added to current context media. 
 *     (precondition attr can appear only in the media-descr level.
 *
 *     An Example to precondition attribute that should be parsed may look like:    
 *          "a=curr:qos e2e send"                or:  
 *          "a=des:qos remote optional sendrecv" or:
 *          "a=conf:qos local recv "
 *
 * Return Value: 
 *     If parsing or construction fails the correspondent status is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *  specAttrData      - the special attribute data.
 *  createAsBadSyntax - if set to RV_TRUE the bad syntax 
 *                      precondition attribute will be created.
 *  parserData        - a pointer to the parser data instance.
 *  commonFields      - the current context common fields instance,
                        here the special attribute will be added
 *  name - the Precondition Attr name.
 *  val  - the Precondition Attr val.
 ***************************************************************************/
RvSdpSpecAttrParseSts rvSdpPreconditionAttrParsing(
				const RvSdpSpecAttributeData* specAttrData, /* the special attribute data */
                RvBool              createAsBadSyntax, /* if set to RV_TRUE the bad 
                                                          syntax precondition attribute  
                                                          will be created */
                RvSdpParserData   * parserData,   /* the parser data instance */        
                RvSdpCommonFields * commonFields, /* the current context comon fields instance,
                                                     here the special attribute will be added */
				char              * name,      /* the attribute name */
                char              * val,       /* the attribute value to parse */
                REPLACING_DECL)        /* used for zero-substitutions in input buffer */
{
	UNUSE_REPLACING_DECL;
	RV_UNUSED_ARG(specAttrData);
	if (createAsBadSyntax)
	{
#ifdef RV_SDP_CHECK_BAD_SYNTAX
		if (rvSdpAddPreconditionAttr2(parserData->iMediaDescr,
                                      commonFields,
                                      name,
                                      NULL,
                                      NULL,
                                      NULL,
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
        RvSdpPreconditionAttr  precondAttr;
		RvSdpPreconditionAttr* precondAttrPtr;
        
        /*save the name as enum in the precond struct in order to save in calls to strcmp()*/
        precondAttr.iePrecondName = rvSdpPreconditionNameTxt2Val(name);

        /*parsing the Precondition attribute - and fill in its fields*/
		if (!rvSdpParsePreconditionAttr(parserData,val,name,&precondAttr,REPLACING_ARGS2))
		{
			parserData->iLineStatus = RvSdpValueParseFailed;
			return rvSdpSpAttrPrsFail;
		}
        else
        {
            /*add the media-group attribute */
            precondAttrPtr = rvSdpAddPreconditionAttr2(
                                       parserData->iMediaDescr,
                                       commonFields,
                                       name,
                                       precondAttr.iPrecondType,
                                       precondAttr.iStatusType,
                                       precondAttr.iStrengthTag,
                                       precondAttr.iDirectionTag,
                                       NULL);
            if(precondAttrPtr == NULL)
            {
                return rvSdpSpAttrPrsAllocFail;
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

/*precondition type*/

/***************************************************************************
 * rvSdpPreconditionAttrGetPrecondTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the Precond-Type as text value of the Precondition attribute ("QOS").
 *          
 * Return Value: 
 *      The requested Precond-Type str of the Precondition attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpPreconditionAttrGetPrecondTypeStr(
                                               const RvSdpPreconditionAttr* precondAttr)
{    
    return RV_SDP_EMPTY_STRING(precondAttr->iPrecondType);
}

/***************************************************************************
 * rvSdpPreconditionAttrGetPrecondType
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the Precond-Type as enum value of the Precondition attribute ("QOS").
 *         
 * Return Value: 
 *      The requested field of the Precondition attribute.
 *      RV_SDP_PRECOND_TYPE_NOTSET is returned if it is ""

 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPreconditionType rvSdpPreconditionAttrGetPrecondType(
                                               const RvSdpPreconditionAttr* precondAttr)
{  
    return rvSdpPreconditionTypeTxt2Val(RV_SDP_EMPTY_STRING(precondAttr->iPrecondType)) ;
                                       
}

/***************************************************************************
 * rvSdpPreconditionAttrSetPrecondTypeStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the Precond-Type as str value in the Precondition attribute ("QOS").
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 *      precondTypeStr - the new value of precondition Type as text.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpPreconditionAttrSetPrecondTypeStr(
                                               RvSdpPreconditionAttr *  precondAttr,
                                               const char  *            precondTypeStr)
{ 
    return rvSdpSetTextField(&precondAttr->iPrecondType,
                             precondAttr->iObj,
                             precondTypeStr);       
}

/***************************************************************************
 * rvSdpPreconditionAttrSetPrecondType
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the Precond-Type as enum value in the Precondition attribute ("QOS").
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 *      ePrecondType    - the new value of precondition Type as enum.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpPreconditionAttrSetPrecondType(
                                            RvSdpPreconditionAttr *    precondAttr,
                                            RvSdpPreconditionType      ePrecondType)
{
    return rvSdpPreconditionAttrSetPrecondTypeStr(
                                              precondAttr,
                                              rvSdpPreconditionTypeVal2Txt(ePrecondType));
}


/*precondition status type*/

/***************************************************************************
 * rvSdpPreconditionAttrGetStatusStr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the precond-Status as text value of the precondition attribute 
 *      ("e2e"/"remote"/"local").
 *          
 * Return Value: 
 *      The requested precond-Status str of the precondition attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpPreconditionAttrGetStatusStr(
                                               const RvSdpPreconditionAttr* precondAttr)
{
    return RV_SDP_EMPTY_STRING(precondAttr->iStatusType);

}

/***************************************************************************
 * rvSdpPreconditionAttrGetStatus
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the precond-Status as enum value of the Precondition attribute 
 *      ("e2e"/"remote"/"local").
 *          
 * Return Value: 
 *      The requested enum of the precondition Status.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPreconditionStatusType rvSdpPreconditionAttrGetStatus(
                                               const RvSdpPreconditionAttr* precondAttr)
{
    return rvSdpPreconditionStatusTxt2Val(RV_SDP_EMPTY_STRING(precondAttr->iStatusType)); 
}

/***************************************************************************
 * rvSdpPreconditionAttrSetStatusStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the precond-Status as str value in the Precondition attribute ("QOS").
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 *      precondTypeStr - the new value of precondition status as text.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpPreconditionAttrSetStatusStr(
                                               RvSdpPreconditionAttr *  precondAttr,
                                               const char            *  precondStatusStr)
{
    return rvSdpSetTextField(&precondAttr->iStatusType,
                             precondAttr->iObj,
                             precondStatusStr);       

}

/***************************************************************************
 * rvSdpPreconditionAttrSetStatus
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the Precond-Type as enum value in the Precondition attribute ("QOS").
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr     - a pointer to the RvSdpPreconditionAttr object.
 *      ePrecondType    - the new value of precondition status as enum.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpPreconditionAttrSetStatus(
                                            RvSdpPreconditionAttr *    precondAttr,
                                            RvSdpPreconditionType      ePrecondStatus)
{
    return rvSdpPreconditionAttrSetStatusStr(
                                   precondAttr,
                                   rvSdpPreconditionStatusVal2Txt(ePrecondStatus));
}


/*precondition strength-tag*/

/***************************************************************************
 * rvSdpPreconditionAttrGetStrengthTagStr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the strength-tag as text value of the precondition attribute 
 *      ("mandatory"/"optional"/"none"/"failure"/"unknown").
 *          
 * Return Value: 
 *      The requested precond strength-tag str of the precondition attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpPreconditionAttrGetStrengthTagStr(
                                               const RvSdpPreconditionAttr* precondAttr)
{
    return RV_SDP_EMPTY_STRING(precondAttr->iStrengthTag);
}
     


/***************************************************************************
 * rvSdpPreconditionAttrGetStrengthTag
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the strength-tag as enum value of the Precondition attribute 
 *      ("mandatory"/"optional"/"none"/"failure"/"unknown").
 *          
 * Return Value: 
 *      The requested enum of the precondition StrengthTag.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPreconditionStrengthTag rvSdpPreconditionAttrGetStrengthTag(
                                               const RvSdpPreconditionAttr* precondAttr)
{

    return rvSdpPreconditionStrengthTxt2Val(RV_SDP_EMPTY_STRING(precondAttr->iStrengthTag));
                                        
}



/***************************************************************************
 * rvSdpPreconditionAttrSetStrengthTagStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the strength-tag as str value in the Precondition attribute 
 *      ("mandatory"/"optional"/"none"/"failure"/"unknown").
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr - a pointer to the RvSdpPreconditionAttr object.
 *      strengthStr - the new value of precondition strength-tag as text.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpPreconditionAttrSetStrengthTagStr(
                                               RvSdpPreconditionAttr *  precondAttr,
                                               const char  *            strengthStr)
{
    return rvSdpSetTextField(&precondAttr->iStrengthTag,
                              precondAttr->iObj,
                              strengthStr);        

}


/***************************************************************************
 * rvSdpPreconditionAttrSetStrengthTag
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the strength-tag as enum value of the Precondition attribute 
 *      ("mandatory"/"optional"/"none"/"failure"/"unknown").
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 *      eStrengthStr    - the new value of precondition strength-tag as enum.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpPreconditionAttrSetStrengthTag(
                                            RvSdpPreconditionAttr *    precondAttr,
                                            RvSdpPreconditionType      eStrengthStr)
{
    return rvSdpPreconditionAttrSetPrecondTypeStr(
                                         precondAttr,
                                         rvSdpPreconditionStrengthVal2Txt(eStrengthStr));

}






/*precondition direction-tag*/

/***************************************************************************
 * rvSdpPreconditionAttrGetDirectionTagStr
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the direction-tag as text value of the precondition attribute 
 *      ("send"/"recv"/"none"/"sendrecv").
 *          
 * Return Value: 
 *      The requested precond direction-tag str of the precondition attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpPreconditionAttrGetDirectionTagStr(
                                               const RvSdpPreconditionAttr* precondAttr)
{
     return RV_SDP_EMPTY_STRING(precondAttr->iDirectionTag);
}

/***************************************************************************
 * rvSdpPreconditionAttrGetDirectionTag
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the strength-tag as enum value of the Precondition attribute 
 *      ("send"/"recv"/"none"/"sendrecv").
 *          
 * Return Value: 
 *      The requested enum of the precondition Direction Tag.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPreconditionDirectionTag rvSdpPreconditionAttrGetDirectionTag(
                                               const RvSdpPreconditionAttr* precondAttr)
{
    return  rvSdpPreconditionDirectionTxt2Val(RV_SDP_EMPTY_STRING(precondAttr->iDirectionTag));
}



/***************************************************************************
 * rvSdpPreconditionAttrSetDirectionTagStr
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the direction-tag as str value of the Precondition attribute 
 *      ("send"/"recv"/"none"/"sendrecv")..
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 *      eDirection    - the new value of precondition direction-tag as enum.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpPreconditionAttrSetDirectionTagStr(
                                               RvSdpPreconditionAttr *  precondAttr,
                                               const char  *            directionStr)
{
    return rvSdpSetTextField(&precondAttr->iDirectionTag,
                              precondAttr->iObj,
                              directionStr);       
}

/***************************************************************************
 * rvSdpPreconditionAttrSetDirectionTag
 * ------------------------------------------------------------------------
 * General: 
 *      Sets the direction-tag as enum value of the Precondition attribute 
 *      ("send"/"recv"/"none"/"sendrecv")..
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 *      eDirection    - the new value of precondition direction-tag as enum.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpPreconditionAttrSetDirectionTag(
                                            RvSdpPreconditionAttr    *    precondAttr,
                                            RvSdpPreconditionDirectionTag eDirection)
{
    return rvSdpPreconditionAttrSetPrecondTypeStr(
                                   precondAttr,
                                   rvSdpPreconditionDirectionVal2Txt(eDirection));
}




#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpPreconditionAttrIsBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Tests whether the Precondition attribute is proprietary formatted.
 *          
 * Return Value: 
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI RvBool rvSdpPreconditionAttrIsBadSyntax(RvSdpPreconditionAttr* precondAttr)
{
    return (precondAttr->iBadSyntaxField != NULL);
}

/***************************************************************************
 * rvSdpPreconditionAttrGetBadSyntax
 * ------------------------------------------------------------------------
 * General: 
 *      Gets the proprietary formatted Precondition attribute value 
 *      or empty string ("") if the value is legal. 
 *          
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      precondAttr    - a pointer to the RvSdpPreconditionAttr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpPreconditionAttrGetBadSyntax
                                     (const RvSdpPreconditionAttr* precondAttr)
{
    return RV_SDP_EMPTY_STRING(precondAttr->iBadSyntaxField);
}


/***************************************************************************
 * rvSdpMediaDescrAddBadSyntaxPrecondition
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpPreconditionAttr object at media
 *      descriptor level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpPreconditionAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      badSyn - the proprietary value of Precondition special attribute field.
 ***************************************************************************/
RvSdpPreconditionAttr* rvSdpMediaDescrAddBadSyntaxPrecondition(RvSdpMediaDescr* descr, 
                                                               const char     * badSyn)
{
    rvSdpAddPreconditionAttr2( descr,
                              &descr->iCommonFields,\
                               NULL,NULL,NULL,NULL,NULL,badSyn)
}
#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */


#endif /*RV_SDP_USE_MACROS*/


#endif /*RV_SDP_PRECONDITIONS_ATTR*/

