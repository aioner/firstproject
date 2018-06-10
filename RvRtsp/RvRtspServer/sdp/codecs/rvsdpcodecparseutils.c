/******************************************************************************
Filename    :rvsdpcodecparseutils.c
Description : 

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
#include <string.h>
#include <ctype.h>
#include "rvsdpprivate.h"

#ifdef RV_SDP_CODECS_SUPPORTED

#include "rvsdpcodecs.h"
#include "rvsdpcodecsinternal.h"
#include "rvsdpcodecparseutils.h"

/*
 *	Get the construction 'name = value' or 'name' separated by spaces and 'sep'
 *  The 't' points to the beginnig of searched buffer and is moved when the
 *  construction is found.
 */
RvSdpStatus RvSdpCodecGetNameEqualValue(char** t, char** name, int*  nameLen,
                                     char** value,int* valueLen, char sep)
{
    char *p, *txt;

    txt = *t;

    if (sep == 0)
        sep = ' ';

    /* skip spaces and separator */
    while (*txt == ' ' || *txt == '\t' || *txt == sep)
        txt++;

    if (*txt == 0)
        return RV_SDPSTATUS_PARSEFAIL;

    p = txt;

    *value = NULL;
    *valueLen = 0;

    /* go over the name */
    while (*p != '=' && *p != ' ' && *p != '\t' && *p != sep && *p != 0)
        p++;

    *name = txt;
    *nameLen = p-txt;

    txt = p;

    /* skip spaces possibly coming after the name */
    while (*txt == ' ' || *txt == '\t')
        txt++;

    if (*txt == 0 || *txt == sep)
    {
        /* there is no value ; that is 'name' construction  (and not 'name = value') */
        if (*txt)
            txt++;
        *t = txt;
        return RV_SDPSTATUS_OK;
    }

    /* only '=' may be here */
    if (*txt != '=')
    {
        if (sep == ' ')
        {
            *t = txt;
            *value = NULL;
            *valueLen = 0;
            return RV_SDPSTATUS_OK;
        }
        else
            return RV_SDPSTATUS_PARSEFAIL;
    }
    txt++;

    /* skip spaces possibly coming after the name */
    while (*txt == ' ' || *txt == '\t')
        txt++;

    /* the value must be here */
    if (*txt == 0 || *txt == sep)
    {
        *t = txt;
        *value = NULL;
        *valueLen = 0;
        return RV_SDPSTATUS_OK;
    }

    p = txt;

    /* go over the value */
    while (*p != ' ' && *p != '\t' && *p != sep && *p != 0)
        p++;

    /* save the value beginnig and length */
    *value = txt;
    *valueLen = p-txt;

    if (*p)
        p++;
    *t = p;

    return RV_SDPSTATUS_OK;
}

/*
 *	Construct the instance of RvSdpNameValue and initialize it to
    'name' and 'value'.
 */
RvSdpNameValue* RvSdpConstructNameValue(RvAlloc* a, const char* name, const char* value)
{
    RvSdpNameValue* nv;
    nv = rvSdpAllocAllocate(a,sizeof(RvSdpNameValue));
    if (!nv)
        return NULL;
    memset(nv,0,sizeof(RvSdpNameValue));
    nv->iName = name;
    nv->iValue = value;
    return nv;
}


void RvSdpNameValuesListInitialize(RvSdpNameValuesList* nvl)
{
    memset(nvl,0,sizeof(RvSdpNameValuesList));
}

/*
 *	Free the memory used for list contents
 */
void RvSdpNameValuesListEnd(RvAlloc* a, RvSdpNameValuesList* nvl)
{
    int cnt;

    for (cnt = 0; cnt < RV_SDP_MAX_PARAMS; cnt++)
    {
        if (nvl->iNVArray[cnt])
        {
            rvSdpAllocDeallocate(a,sizeof(RvSdpNameValue),nvl->iNVArray[cnt]);
            if (--nvl->iNVCount == 0)
                break;
        }
    }
}

/* add the new name-value pair to the list.
   In order to enable efficient search-the name-value pair is 
   sorted by the first letter (alphabetic) */
RvSdpStatus RvSdpAddNameValue(RvSdpNameValuesList* nvl, RvSdpNameValue *nv)
{
    int index;

    if (!isalpha((int)nv->iName[0]))
        return RV_SDPSTATUS_PARSEFAIL;

    if (nvl->iNVCount == RV_SDP_MAX_PARAMS)
        return RV_SDPSTATUS_PARSEFAIL;

    nvl->iNVArray[nvl->iNVCount++] = nv;
    index = tolower(nv->iName[0]) - 'a';

    nv->iNextInList = NULL;
    if (nvl->iHeadsVector[index] == 0)
        nvl->iHeadsVector[index] = nvl->iTailsVector[index] = nv;
    else
    {
        nvl->iTailsVector[index]->iNextInList = nv;
        nvl->iTailsVector[index] = nv;
    }

    return RV_SDPSTATUS_OK;
}


#endif /*RV_SDP_CODECS_SUPPORTED*/

