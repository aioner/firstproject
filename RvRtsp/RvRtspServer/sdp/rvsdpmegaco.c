#include <string.h>
#include "rvsdpprivate.h"
#include "rvcbase.h"

#ifdef RV_SDP_MEGACO_ENABLED

/*
 *	Parses the VCID construction that may appear instead of port
 */
RvBool rvSdpParseVcId(RvChar** vcidVals, RvSdpVcId *vcidTypes, RvSize_t* vcidNum, 
                      char** p, RvSdpParseWarning* w, REPLACING_DECL)
{
    RvSize_t cnt;
    char *pp, *ptr, c;
    RvSdpVcId t;

    *vcidNum = 0;
    *w = RvSdpWarnNoWarning;

    /*
     *	the vcid format is as follows:
         type-value[/type-value[/type-value[/type-value]]]
         where type is one of values defined for RvSdpVcId
     */

    ptr = *p;
    cnt = 0;
    for (;;)
    {  
        pp = ptr;
        if (!rvSdpParseText(&ptr,M_AD) || *ptr != '-')
        {
            *w = RvSdpWarnIllegalVcid;
            break;
        }
        REPLACING2(ptr);
        ptr++;

        t = rvSdpVcidTypeTxt2Val(pp);
        if (t == RV_SDPVCID_UNKNOWN)
        {
            *w = RvSdpWarnIllegalVcid;
            break;
        }

        pp = ptr;
        if (!rvSdpParseNonSpace(&ptr,'/'))
        {
            *w = RvSdpWarnIllegalVcid;
            break;
        }

        vcidVals[cnt] = pp;
        vcidTypes[cnt] = t;
        *vcidNum = ++cnt;
        c = *ptr;
        REPLACING2(ptr); 
        
        /* out of vcid part */
        if (c != '/')
        {
            /* it must be space or the end of the buffer */
            *p = ptr+1;
            return RV_TRUE;
        }
        else if (cnt == RV_SDP_MEDIA_VCID_MAX)
        {
            *w = RvSdpWarnIllegalVcid;
            break;
        }
        ptr++;
    }    
    
    if (cnt == 0)
        /* if this is the first suspected vcid field we do not want to fail the parsing */
        *w = RvSdpWarnNoWarning;
    return RV_FALSE;
}


RvBool rvSdpEncodeVcId(RvChar* txt, RvInt* txtLen, RvSdpMediaDescr *md)
{
    RvInt i, tlen = 0, tlenTry, vcidTLen, vcidVLen, initTxtLen = *txtLen;
    const char* vcidT;
    char *ptr = txt;

    *txtLen = 0;

    if (!md->iMediaVcIdsNum)
        return RV_TRUE;

    for (i = 0; i < md->iMediaVcIdsNum; i++)
    {
        vcidT = rvSdpVcidTypeTypeVal2Txt(md->iMediaVcIdTypes[i]);
        vcidTLen = (RvInt)strlen(vcidT);
        vcidVLen = (RvInt)strlen(RV_SDP_EMPTY_STRING(md->iMediaVcIdValues[i]));
        if (!vcidVLen)
            continue;
        tlenTry = tlen + vcidTLen+vcidVLen+1+1;
        if (tlenTry > initTxtLen)
            return RV_FALSE;
        tlen = tlenTry;
        *txtLen = tlen;
        if (i) /* this is not the first vcid, thus put the '/' after the previous one */
            *ptr++ = '/';
        strncpy(ptr,vcidT,vcidTLen);
        ptr += vcidTLen;
        *ptr++ = '-';
        strncpy(ptr,md->iMediaVcIdValues[i],vcidVLen);
        ptr += vcidVLen;
    }
    (*txtLen) --; /* remove the last '/' */
    return RV_TRUE;
}


/***************************************************************************
 * rvSdpMediaDescrAddVcId
 * ------------------------------------------------------------------------
 * General:
 *      Adds another VC id to the media descriptor object.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      type - the type of VC id to be added
 *      value - the value of VC id to be added
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrAddVcId(
                    RvSdpMediaDescr* descr,
                    RvSdpVcId type,
                    const char* value)
{
    if (rvSdpAddTextToArray(&descr->iMediaVcIdsNum,RV_SDP_MEDIA_VCID_MAX,
        descr->iMediaVcIdValues,descr->iObj,value) != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;
    descr->iMediaVcIdTypes[descr->iMediaVcIdsNum-1] = type;
    return RV_SDPSTATUS_OK;
}


/***************************************************************************
 * rvSdpMediaDescrRemoveVcId
 * ------------------------------------------------------------------------
 * General:
 *      Removes the VC id with index 'idx'.
 *
 * Return Value:
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      idx - the index number
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMediaDescrRemoveVcId (
                    RvSdpMediaDescr* descr,
                    RvSize_t idx)
{
    if ((RvInt)idx < descr->iMediaVcIdsNum-1)
        memcpy((void*)(descr->iMediaVcIdTypes+idx),(void*)(descr->iMediaVcIdTypes+idx+1),
                (descr->iMediaVcIdsNum-idx-1)*sizeof(RvSdpVcId));
    rvSdpRemoveTextFromArray(&descr->iMediaVcIdsNum,descr->iMediaVcIdValues,descr->iObj,idx);
}

                    
#ifndef RV_SDP_USE_MACROS


/***************************************************************************
 * rvSdpMediaDescrGetNASCtrlMethod
 * ------------------------------------------------------------------------
 * General:
 *      Gets the NAS control method of the media descriptor.
 *
 * Return Value:
 *      Returns the NAS control method of the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpMediaDescrGetNASCtrlMethod(
                    RvSdpMediaDescr* descr)
{
    return RV_SDP_EMPTY_STRING(descr->iMediaNASCtrlMethod);
}

/***************************************************************************
 * rvSdpMediaDescrSetNASCtrlMethod
 * ------------------------------------------------------------------------
 * General:
 *      Sets the NAS control method of the media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      method - new NAS control method value.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetNASCtrlMethod(
                    RvSdpMediaDescr* descr,
                    const char* method)
{
    return rvSdpSetTextField(&descr->iMediaNASCtrlMethod,descr->iObj,method);
}


/***************************************************************************
 * rvSdpMediaDescrGetVcIdNum
 * ------------------------------------------------------------------------
 * General:
 *      Returns the number of VC ids defined for media descriptor.
 *
 * Return Value:
 *      Number of VC ids.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetVcIdNum(
                    const RvSdpMediaDescr* descr)
{
    return descr->iMediaVcIdsNum;
}

/***************************************************************************
 * rvSdpMediaDescrGetVcIdType
 * ------------------------------------------------------------------------
 * General:
 *      Returns the type of VC id with index 'idx'
 *
 * Return Value:
 *      Returns the type of VC id with index 'idx'
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      idx - the index number
 ***************************************************************************/
RVSDPCOREAPI RvSdpVcId rvSdpMediaDescrGetVcIdType(
                    const RvSdpMediaDescr* descr,
                    RvSize_t idx)
{
    return descr->iMediaVcIdTypes[idx];
}

/***************************************************************************
 * rvSdpMediaDescrGetVcIdValue
 * ------------------------------------------------------------------------
 * General:
 *      Returns the value of VC id with index 'idx'
 *
 * Return Value:
 *      Returns the value of VC id with index 'idx'
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      idx - the index number
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpMediaDescrGetVcIdValue (
                    const RvSdpMediaDescr* descr,
                    RvSize_t idx)
{
    return RV_SDP_EMPTY_STRING(descr->iMediaVcIdValues[idx]);
}



/***************************************************************************
 * rvSdpMediaDescrClearVcId
 * ------------------------------------------------------------------------
 * General:
 *      Clears all VC ids defined for the media descriptor.
 *
 * Return Value:
 *      None
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      idx - the index number.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMediaDescrClearVcId (
                    RvSdpMediaDescr* descr)
{
    rvSdpClearTxtArray(&descr->iMediaVcIdsNum,descr->iMediaVcIdValues,descr->iObj);
}


#endif /*RV_SDP_USE_MACROS*/


#endif /*RV_SDP_MEGACO_ENABLED*/





















