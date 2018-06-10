/******************************************************************************
Filename    :rvsdpcodecparseutils.h
Description : definitions regarding use of codecs utils

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

#ifdef RV_SDP_CODECS_SUPPORTED

#ifndef _rvsdpcodecparseutils_h_
#define _rvsdpcodecparseutils_h_


/*The 'a=fmtp' attribute is a line that composed of a list of ' name=value'
  or just 'name' (with no value),for example:
  "a = fmtp XX name:value ;name:value ;name:value ;name ;name" .
  Each pair is seperated by a seperator.
  Parsing the 'a=fmtp' line is performed by getting the name and value pairs and 
  put them into RvSdpNameValuesList.This list is hashed by the first letter of the 
  'name' to enable quick and efficiant search*/
/*
 *	Get the construction 'name = value' or 'name' separated by spaces and 'sep'
 *  The 't' points to the beginnig of searched buffer and is moved when the 
 *  construction is found.
 */
RvSdpStatus RvSdpCodecGetNameEqualValue(char** txt, char** name, int*  nameLen, char** value,int* valueLen, char sep);

/*
 *	Construct the instance of RvSdpNameValue and initialize it to 
    'name' and 'value'.
 */
RvSdpNameValue* RvSdpConstructNameValue(RvAlloc* a, const char* name, const char* value);

void RvSdpNameValuesListInitialize(RvSdpNameValuesList* nvl);

/*
 *	Free the memory used for list contents
 */
void RvSdpNameValuesListEnd(RvAlloc* a, RvSdpNameValuesList* nvl);


/* add the new name-value pair to the list */
RvSdpStatus RvSdpAddNameValue(RvSdpNameValuesList* nvl, RvSdpNameValue *nv);


#define SKIP_SPACES(_p) \
{\
    while (*(_p) == ' ' || *(_p) == '\t')\
     (_p)++;\
}

#endif /*RV_SDP_CODECS_SUPPORTED*/


#endif
