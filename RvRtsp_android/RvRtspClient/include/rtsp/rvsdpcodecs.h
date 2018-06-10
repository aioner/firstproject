/******************************************************************************
Filename    :rvsdpcodecs.h
Description : definitions regarding use of codecs.

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
 Author:Rafi Kiel
******************************************************************************/
 
#ifndef _rvsdpcodecs_h_
#define _rvsdpcodecs_h_ 


#ifdef RV_SDP_CODECS_SUPPORTED


#include "rvalloc.h"
#include "rvsdpcodecsconfig.h"
#include "rvsdpprsaux.h"
#include "rvsdpcodecsenums.h"

/*
  The CODECS module is defined under the RV_SDP_CODECS_SUPPORTED flag.
  Codec is a specific coder-encoder,that uses us to decode compressed media data
  or encode this media data (audio/video).
  Each Codec is represented by a payload number.
  There are 'well known' Codecs that their payload number is between 0 --> 95,
  These Codecs can be configured by adding 'a=fmtp....' line,but they also have default 
  values for their parameters if this line is not added.
  Codecs parameters syntax are well defined in each codec's C file 
  (for ex: rvsdpcodec_XXX.c) in the struct RvSdpParamStaticParserData that 
  contains all the data necessary by the parser (such as parameter enum and parameter's syntax)
 
  We can also add new Codecs and define them by ourselef and give them a payload
  number that must be greater then 95 (dynamic payload).
  The proper way to do it is like in the following example:
  
	m=video 49170/2 RTP/AVP 98 
    a=rtpmap:98 MP4V-ES/90000  
    a=fmtp:98 profile-level-id=1;config=000001B001000001B509

  The a=rtpmap defines the Codec payload and the a=fmtp line configures the params.

  
 */  
/*
 *	This function initializes the SDP Codec module;
 *  It needs to be called exactly once before any other function from
 *  the module
 */
RVSDPCOREAPI RvSdpStatus rvSdpCodecInitialize();

/*
 *	This function de-initializes the SDP Codec module;
 *  It needs to be called exactly once at the end of module use.
 */
RVSDPCOREAPI RvSdpStatus rvSdpCodecEnd();

/*
 *	To represent the Codec parser structure;
 */
typedef void* RvSdpCodecParser;

/*
 *	To represent the parsed copec parameters;
 */
typedef void* RvSdpCodecParams;

/* 
 * Gets the name of the codec by its static payload number as defined in RFC 3551 (tables 4 & 5)
 */
RVSDPCOREAPI const char* rvSdpCodecNameGetByPayLoad(int payLoad);

/*
 *	To get the previously registered codec parser structure by its registered name;
 *  The name is case insensitive;
 */
RVSDPCOREAPI RvSdpCodecParser rvSdpCodecParserGetByName(const char* codecName);

/*
 *	To get the previously registered codec parser structure by well known (less than 96) 
 *  payload number.
 */
RVSDPCOREAPI RvSdpCodecParser rvSdpCodecParserGetByPayLoad(int payLoad);


/*
 * Parses the SDP media descr in order to build a specific codec parameters structure.
 * The parameters built for the codec that its payload number is specified as 'payLoad'
 * The allocator 'a' is optional, if NULL will be given-the default allocator will be 
 * used.
 */
RVSDPCOREAPI RvSdpCodecParams rvSdpCodecParamsConstruct(RvSdpCodecParser codecP, int payLoad, 
                                                           RvSdpMediaDescr* mediaD, RvAlloc* a);
/*
 *	Destructs the codec parameters structure;
 */
RVSDPCOREAPI void rvSdpCodecParamsDestruct(RvSdpCodecParams p);

/* 
 * Tests whether the codec parameters match the codec's predefined
 * grammar. Note that not all the codecs define grammar.
 */
RVSDPCOREAPI RvBool rvSdpCodecIsValidSyntax(RvSdpCodecParams p);

/*
 *	Gets the number of parsed parameters. 
 */
RVSDPCOREAPI RvSize_t rvSdpCodecGetNumOfParams(RvSdpCodecParams p);  

/*
 *	Gets the number of unknown parameters;
 */
RVSDPCOREAPI RvSize_t rvSdpCodecGetNumOfUnknownParams(RvSdpCodecParams p);  

/*
 *	Gets the number of parameters whose values did not match the predefined pattern;
 */
RVSDPCOREAPI RvSize_t rvSdpCodecGetNumOfInvalidParams(RvSdpCodecParams p);  

/*
 *	Get the name, value and patter-matching of the parameter based on an index. The index
 *  is zero based (to get the first parameter use '0' value for 'index')
 *  Any of 'name','value' or 'isOK' may be set to NULL when calling the function.
 */
RVSDPCOREAPI RvSdpStatus rvSdpGetParamByIndex(RvSdpCodecParams p, int index, 
                                              const char** name, const char** value, RvBool* isOK);
/*
 *	Get the value and patter-matching of the parameter based on its name.
 *  Any of 'value' or 'isOK' may be set to NULL when calling the function.
 */
RVSDPCOREAPI RvSdpStatus rvSdpGetParamByName(RvSdpCodecParams p, const char* name, 
                                             const char** value, RvBool* isOK);



/* 
 * to remove the pair 'name=value' with the given 'name' 
 * if 'removeAttrIfEmpty' set to RV_TRUE, the whole ftmtp attrbute will be removed in case
 * the last name=value pair is removed.
 * The operation is performed on the FMTP attribute with payload equal to 'payload'.
 * 'separator' - defines the character separating between the name=values pairs.
 * RV_SDPSTATUS_OK is returned if name=value was successfully removed or error otherwise.
 */
RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamRemoveByName(
    RvSdpMediaDescr* descr, 
    const RvChar* name,
    RvBool removeAttrIfEmpty,   
    RvUint32 payload,
    RvChar separator);


/* to remove the pair 'name=value' with the given index (zero based)
 * if 'removeAttrIfEmpty' set to RV_TRUE, the whole ftmtp attrbute will be removed in case
 * the last name=value pair is removed.
 * The operation is performed on the FMTP attribute with payload equal to 'payload'.
 * 'separator' - defines the character separating between the name=values pairs.
 * RV_SDPSTATUS_OK is returned if name=value was successfully removed or error otherwise.
 */  
RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamRemoveByIndex(
    RvSdpMediaDescr* descr, 
    RvInt index,
    RvBool removeAttrIfEmpty,
    RvUint32 payload,
    RvChar separator);

/* Finds the pair name name=value by 'name' and places the value of the pair
 * into the 'value' (if it is big enough)
 * If 'index' is not NULL and the 'name' can be found, zero based index of the
 * pair will be saved in 'index'.
 * If the value of the pair is empty (the pair appears as 'name' only) the empty
 * string will be placed in 'value'.
 * If the 'value' is NULL and the 'index' is not NULL, this function can be used for
 * getting the index of the 'name=value' pair for given 'name' (value will not be 
 * copied).
 * The operation is performed on the FMTP attribute with payload equal to 'payload'.
 * 'separator' - defines the character separating between the 'name=values' pairs.
 * RV_SDPSTATUS_OK is returned if 'name=value' could be found or error otherwise.
 */
RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamGetByName(
    RvSdpMediaDescr* descr, 
    const RvChar* name,
    RvUint32 payload,
    RvChar separator,
    RvChar* value,
    RvInt valLen,
    RvInt* index); 

/* Finds the pair name name=value by zero based 'index' and places the value of the pair
 * into the 'value' (if it is big enough).
 * If the value of the pair is empty (the pair appears as 'name' only) the empty
 * string will be placed in 'value. 
 * The operation is performed on the FMTP attribute with payload equal to 'payload'.
 * 'separator' - defines the character separating between the name=values pairs.
 * RV_SDPSTATUS_OK is returned if name=value could be found or error otherwise.
 */
RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamGetByIndex(
    RvSdpMediaDescr* descr, 
    RvInt index,
    RvUint32 payload,
    RvChar separator,
    RvChar* value,
    RvInt valLen);


/* 
 * Modifies/adds the 'name=value' pair,
 * If the pair with 'name' was not presented it is added with value as set by 'value'.
 * otherwise the existing value is set to 'value'. 
 * If fmtp attribute with the given payload does not 
 * exist it will added if addMftpAttr is set to RV_TRUE.
 * The 'value' may be NULL and the pair will be set to 'name' only. 
 * 'separator' - defines the character separating between the name=values pairs.
 * If 'a' is NULL the default allocator will be used.
 */
RVSDPCOREAPI RvSdpStatus RvSdpCodecFmtpParamSet(
    RvSdpMediaDescr* descr, 
    const RvChar* name,         
    const  RvChar* value,       
    RvBool addFmtpAttr,         
    RvUint32 payload,     
    RvChar separator,
    RvAlloc* a);

/* returns the number of name=value pairs for a fmtp attribute with the 
   given payload */
RVSDPCOREAPI RvInt RvSdpCodecFmtpParamGetNum(
    RvSdpMediaDescr* descr, 
    RvUint32 payload,
    RvChar separator);

#endif /* #ifdef RV_SDP_CODECS_SUPPORTED*/


#endif /* #ifdef _rvsdpcodecs_h_*/


 
