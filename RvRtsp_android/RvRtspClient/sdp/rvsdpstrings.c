/******************************************************************************
Filename    :rvsdpstrings.c
Description : strings & string buffer routines

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

/***************************************************************************
 * rvSdpSetTextField
 * ------------------------------------------------------------------------
 * General: 
 *      Allocate and copy string.
 *      If obj points to RvSdpMsg instance the allocation will be performed 
 *      in message strings buffer and the pointer within the strings buffer 
 *      will be returned. Otherwise obj must point to RvAlloc allocator which
 *      will be used for memory allocation.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      str - will point to allocated memory.
 *      obj - points to RvSdpMsg or RvAlloc instance.
 *      value - string to copy.
 ***************************************************************************/
RvSdpStatus 
rvSdpSetTextField(RvSdpString* str, void *obj, const char* value)
{
    if (value == NULL)
    {
        rvSdpUnsetTextField(str,obj);
        return RV_SDPSTATUS_OK;
    }
    if (*(RvUint32*)obj == RV_SDP_MESSAGE_MAGIC_NUMBER)
    {
        RvSdpMsg *msg = (RvSdpMsg*) obj;
        *str = rvSdpStringBufferRealloc(*str,value,&msg->iStrBuffer);
    }
    else
        *str = rvSdpStringRealloc(*str,value,obj);
    
    if (*str == NULL)
        return RV_SDPSTATUS_ALLOCFAIL;
    return RV_SDPSTATUS_OK;
}

/***************************************************************************
 * rvSdpUnsetTextField
 * ------------------------------------------------------------------------
 * General: 
 *      Deallocates the string within the RvSdpMessage strings buffer or
 *      using the allocator.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      str - pointer to memory to be freed.
 *      obj - points to RvSdpMsg or RvAlloc instance.
 ***************************************************************************/
void 
rvSdpUnsetTextField(RvSdpString* str, void *obj)
{
    if (*str == NULL)
        return;
    if (*(RvUint32*)obj == RV_SDP_MESSAGE_MAGIC_NUMBER)
    {
        RvSdpMsg *msg = (RvSdpMsg*) obj;
        rvSdpStringBufferDealloc(*str,&msg->iStrBuffer);
    }
    else
        rvSdpStringDealloc(*str,obj);
    *str = NULL;
}


/* allocates the memory for the string using the allocator *
 * the string length is put in the two bytes before the string itself
 * thus there is no need to know the string size when deallocating 
 * returns the alloacted string on success or NULL if fails */
RvSdpString rvSdpStringAlloc(
        const char* txt,        /* points to the string which will be copied into allocated memory */
        RvAlloc* a)             /* the allocator to be used */
{
    RvSdpString s;
    int l;
    l = strlen(txt);
    l += 1+sizeof(RvUint16);
    s = (RvSdpString) rvSdpAllocAllocate(a,l);
    if (s == NULL)
        return NULL;
    memcpy(s,&l,sizeof(RvUint16));
    s += sizeof(RvUint16);
    memcpy(s,txt,l-sizeof(RvUint16));
    return s;
}

/* to reallocate the string *
 * returns the alloacted string on success or NULL if fails */
RvSdpString rvSdpStringRealloc(
        RvSdpString oldTxt,     /* the previously allocated (using rvSdpStringAlloc)  string */
        const char* newTxt,     /* the new value of the string to be reallocated */
        RvAlloc* a)             /* the allocator to be used, must be the same as used when the string
                                 * was first allocated */
{
    if (oldTxt)
        rvSdpStringDealloc(oldTxt,a);
    return rvSdpStringAlloc(newTxt,a);
}

/* to deallocate the string 
 * the string length is kept in two bytes before 
 * the string itself */
void rvSdpStringDealloc(RvSdpString str, RvAlloc* a)
{
    RvUint16 l;

    if (str == NULL)
        return;

    str -= sizeof(RvUint16);
    memcpy(&l,str,sizeof(RvUint16));
    
    rvSdpAllocDeallocate(a,l,str);
}

/* to initialize the strings buffer 
 * return RV_SDPSTATUS_OK if succeeds */
RvSdpStatus rvSdpStringBufferInitialize(
        RvStringBuffer* b,   /* the buffer to initialize */
        RvAlloc *a,          /* the allocator to be used for memory allocations */
        RvSize_t initSz,     /* the initial size of the buffer */
        void *usrData,       /* argument to be used when calling reshuffle function */
        rvSdpReshuffleFunc reshF) /* the reshuffle function */
{
    memset(b,0,sizeof(RvStringBuffer));
    b->iBuffer = rvSdpAllocAllocate(a,initSz);
    if (!b->iBuffer)
        return RV_SDPSTATUS_ALLOCFAIL;

    memset(b->iBuffer,0,initSz);
    b->iBufferSz = initSz;
    b->iTotalUsed = b->iBufferOffset = 0;
    b->iUsrData = usrData;
    b->iReshuffleFunc = reshF;
    b->iAllocator = a;
    return RV_SDPSTATUS_OK;
}

/* to deallocate all memory used by strings buffer */
void rvSdpStringBufferDestruct(
                               RvStringBuffer *b,      /* the buffer */
                               RvAlloc* a)             /* the allocator to be used */
{
    if (b->iBufferSz && b->iBuffer)        
        rvSdpAllocDeallocate(a,b->iBufferSz,b->iBuffer);
}   

/* resets the counters but does not free the memory */
void rvSdpStringBufferReset(RvStringBuffer *b)
{
    b->iTotalUsed = b->iBufferOffset = 0;
}

/*
 *	Called when there is not enough room for new string allocation in a
 *  strings  buffer.
 *  Allocates new (not necessarily) bigger memory chunk and then calls 
 *  reshuffle callback defined in RvStringBuffer structure. After reshuffle
 *  callback complets the old memory chunk of strings buffer is freed.
 *  The reshuffle callback supplied by RvSdpMsg will pass over all objects
 *  contained in the message and will cause each of them to copy their owned
 *  strings to the newly allocated memory chunk.
 */
RvBool rvSdpStringBufferReshuffle(RvStringBuffer *b, int l)
{
    RvSdpString newB;
    int newSz;
    char* p;
    
    /* there is not enough room*/
    /* need to reshuffle */

/*
    if (b->iTotalUsed + l > (b->iBufferSz>>1) + (b->iBufferSz>>2))
        newSz = b->iBufferSz + RV_SDP_MSG_BUFFER_SIZE;
    else
        newSz = b->iBufferSz;            
*/
    newSz = b->iBufferSz;
    while (b->iTotalUsed + l > (newSz>>1) + (newSz>>2) /* this 0.75*newSz*/)
        newSz += RV_SDP_MSG_BUFFER_SIZE;


    newB = rvSdpAllocAllocate(b->iAllocator,newSz);
    if (newB == NULL)
        return RV_FALSE;            

    p = newB;
    b->iReshuffleFunc(b->iUsrData,&p);

    rvSdpAllocDeallocate(b->iAllocator,b->iBufferSz,b->iBuffer);
    b->iBuffer = newB;
    b->iBufferSz = newSz;
    b->iBufferOffset = b->iTotalUsed = p - newB;
    return RV_TRUE;
}


/* to allocate the string in the context of RvStringBuffer *
 * returns the alloacted string on success or NULL if fails */
RvSdpString rvSdpStringBufferAlloc(
        const char* txt,    /* string to be copied */
        RvStringBuffer *b)  /* the buffer to use */
{
    RvSdpString s;
    int l;

    l = strlen(txt)+1;
   
    if (b->iBufferSz - b->iBufferOffset < l)
    {
/*
        if (l > RV_SDP_MSG_BUFFER_SIZE)
        / * protect from too big allocations * /
            return NULL;
*/

        if (!rvSdpStringBufferReshuffle(b,l))
            return NULL;
    }

    /* there is enough room */
    s = b->iBuffer+b->iBufferOffset;
    memcpy(s,txt,l);
    b->iBufferOffset = b->iBufferOffset+l;
    b->iTotalUsed = b->iTotalUsed+l;
    return s;
}

/* to re-allocate the string in the context of RvStringBuffer *
 * returns the allocated string on success or NULL if fails */
RvSdpString rvSdpStringBufferRealloc(
        RvSdpString oldTxt,  /* the previously allocated (using rvSdpStringBufferAlloc)  string */
        const char* newTxt,  /* the new value of the string to be reallocated */
        RvStringBuffer *b) 
{
    int l1, l2;
    int offs;

    offs = oldTxt - b->iBuffer;    
    /* ALGR: if oldTxt is null pointer it's also not a part of the buffer
     * In case where oldTxt == 0 and b == 0 this case isn't covered
     */
    if (offs < 0 || offs > b->iBufferOffset || oldTxt == 0)
        /* the oldTxt string is not part of the buffer */
        return rvSdpStringBufferAlloc(newTxt,b);
    
    l1 = strlen(oldTxt) + 1;
    l2 = strlen(newTxt) + 1;

    if (l1 >= l2)
    /* the old string is not shorter than the new one; just replace the string */
    {
        memcpy(oldTxt,newTxt,l2);
        b->iTotalUsed = (b->iTotalUsed - (l1-l2));
        if (offs + l1 == b->iBufferOffset)
        /* that is the last string of the buffer */
            b->iBufferOffset -= (l1-l2);
        return oldTxt;
    }

    rvSdpStringBufferDealloc(oldTxt,b);
    return rvSdpStringBufferAlloc(newTxt,b);
}

/* to de-allocate the string in the context of RvStringBuffer */
void rvSdpStringBufferDealloc(
                              RvSdpString str,     /* the previously allocated string */   
                              RvStringBuffer *b)
{
    int l;
    int offs;

    if (!str)
        return;

    offs = str - b->iBuffer;

    if (offs < 0 || offs > b->iBufferOffset)
        /* the deallocated string is not part of the buffer */
        return;

    l = strlen(str) + 1;

    b->iTotalUsed -= l;
    
    if (offs + l == b->iBufferOffset)
        /* that is the last string in the buffer */
        b->iBufferOffset -= l;    
}

/*
 *	Converts the number into string.
 *  The string 'txt' is returned.
 */
char* rvSdpItoa2(char* txt, RvUint32 i, RvBool isNeg)
{
    char t[20], *p = t + sizeof(t) - 1;

    *p-- = 0;

    do {
        *p-- = (char) ('0' + (i % 10));
        i /= 10;
    } while(i);

    if (isNeg)
        *p = '-';
    else
        p++;
    
    strcpy(txt,p);
    return txt;
}

/* returns the text representation of unsigned integer */
char* rvSdpUitoa(   
                 char* txt,      /* the buffer to use & return */
                 RvUint32 n)    /* the number to print */
{
    return rvSdpItoa2(txt,n,RV_FALSE);
}

/* returns the text representation of integer */
char* rvSdpItoa(
             char* txt,      /* the buffer to use & return */
             RvInt32 n)     /* the number to print */
{
    RvBool isNeg = RV_FALSE;
    if (n < 0)
    {
        n *= -1;
        isNeg = RV_TRUE;
    }
    return rvSdpItoa2(txt,n,isNeg);
}

/***************************************************************************
 * rvSdpAddTextToArray
 * ------------------------------------------------------------------------
 * General: 
 *      Allocates new string (in RvSdpMsg strings buffer or using the alloctor)
 *      and adds the allocated string as the last entry in strings array.
 *          
 * Return Value: 
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the 
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      num - the current lenght of strings  array will be increased by one.
 *      maxNum - the max size of strings array.
 *      txtArr - the array of strings.
 *      obj - points to RvSdpMsg or RvAlloc instance.
 *      val - the string to copy to strinsg array.
 ***************************************************************************/
RvSdpStatus rvSdpAddTextToArray(
            RvUint16* num,    /* the  current lenght of the  strings  array,
                                 will be increased  if there is room */
            int maxNum,       /* the max size of strings array */
            char** txtArr,    /* the  strings array */
            void* obj,        /* pointer to RvSdpMsg or RvAlloc used 
                                 for string allocation */
            const char* val)  /* the value to add to strings array */
{
    char *p = NULL;
    
    if (*num >= maxNum)
        return RV_SDPSTATUS_ILLEGAL_SET;
    
    p = NULL;
    if (rvSdpSetTextField(&p,obj,val) != RV_SDPSTATUS_OK)
        return RV_SDPSTATUS_ALLOCFAIL;
    txtArr[(*num)++] = p;    
    return RV_SDPSTATUS_OK;    
}

/***************************************************************************
 * rvSdpRemoveTextFromArray
 * ------------------------------------------------------------------------
 * General: 
 *      Removes and deallocates the string in the array of strings. The 
 *      de-allocation is performed in RvSdpMsg strings buffer or using the
 *      allocator. If the removed string is not the last entry in the array
 *      all next entries are moved backwards to keep the array continuous.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      num - the current lenght of strings  array will be decreased by one.
 *      txtArr - the array of strings.
 *      obj - points to RvSdpMsg or RvAlloc instance.
 *      index - the index of array entry to remove.
 ***************************************************************************/
void rvSdpRemoveTextFromArray(
            RvUint16* num,
            char** txtArr,
            void* obj,
            RvSize_t index)
{
    RvSize_t cnt;
    if (index >= *num)
        return;
    (*num)--;
    rvSdpUnsetTextField(&txtArr[index],obj);
    for (cnt = index; cnt < *num; cnt++)
        txtArr[cnt] = txtArr[cnt+1];
}

/***************************************************************************
 * rvSdpRemoveTextFromArray
 * ------------------------------------------------------------------------
 * General: 
 *      Deallocates all strings in the array of strings. The 
 *      de-allocation is performed in RvSdpMsg strings buffer or using the
 *      allocator.
 *          
 * Return Value: 
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      num - the current lenght of strings array will be set to zero.
 *      txtArr - the array of strings.
 *      obj - points to RvSdpMsg or RvAlloc instance.
 ***************************************************************************/
void rvSdpClearTxtArray(
            RvUint16* num,
            char** txtArr,
            void* obj) 
{
    int cnt;
    for (cnt = 0; cnt < *num; cnt ++)
        rvSdpUnsetTextField(&txtArr[cnt],obj);
    *num = 0;
}

/*
 *	Used during reshuffle;
 *  If the '*oldPtr' is not NULL it is copied to the '*ptr'.
 *  The '*ptr' is increased by the length of '*oldPtr'.
 */ 
void rvSdpChangeText(char** ptr,char **oldPtr)
{
    if (*oldPtr)
    {
        strcpy(*ptr,*oldPtr);
        *oldPtr = *ptr;
        *ptr += strlen(*ptr)+1;
    }
}


