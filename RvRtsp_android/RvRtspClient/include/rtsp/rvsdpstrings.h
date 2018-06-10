/******************************************************************************
Filename    :rvsdpstrings.h
Description : functions for strings treatment

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
#ifndef _file_rvstrings_h_
#define _file_rvstrings_h_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "rvsdpenums.h"
#include "rvtypes.h"

/*
 *	The RvStringBuffer structure is used by RvSdpMsg to keep text values;
 *  The string buffer is able to reallocate itself thus it needs to update
 *  all pointers kept by RvSdpMsg.
 *  
 *  When the string kept within the buffer needs to be changed we first try to fit the new
 *  value instead of the old value and if it can't be done (the new value is bigger than the old and 
 *  that is not the string at the end of the used part) we 'forget' the old value causing it to become
 *  the lost memory within the buffer. During the next call to 'reshuffle' procedure all garbage will be 
 *  collected.
 */

typedef char* RvSdpString;

typedef void (*rvSdpReshuffleFunc)(void *usrCx, char** buff);

typedef struct {
    rvSdpReshuffleFunc iReshuffleFunc;  /* this function is called when the buffer is re-allocating the memory */
    void* iUsrData;                /* this pointer is given as an argument when iReshuffleFunc is called */
    char* iBuffer;                 /* the actual buffer where the strings are kept;
                                      the strings are saved together with zero-byte terminator */
    RvInt32 iBufferSz;            /* the curent allocated size of iBuffer */
    RvInt32 iTotalUsed;           /* the current used part of the buffer (does not include the lost parts) */
    RvInt32 iBufferOffset;        /* used and lost parts of the buffer 
                                      after reshuffling the iTotalUsed and iBufferOffset become the same */
    RvAlloc  *iAllocator;          /* for buffer allocations */
} RvStringBuffer;


/* to initialize the strings buffer 
 * return RV_SDPSTATUS_OK if succeeds */
RvSdpStatus rvSdpStringBufferInitialize(
        RvStringBuffer* b,   /* the buffer to initialize */
        RvAlloc *a,          /* the allocator to be used for memory allocations */
        RvSize_t initSz,     /* the initial size of the buffer */
        void *usrData,       /* argument to be used when calling reshuffle function */
        rvSdpReshuffleFunc reshF);/* the reshuffle function */

/* to deallocate all memory used by strings buffer */
void rvSdpStringBufferDestruct(
        RvStringBuffer *b,      /* the buffer */
        RvAlloc* a);            /* the allocator to be used */

void rvSdpStringBufferReset(
        RvStringBuffer *b);     /* resets the counters but does not free the memory */

/* allocates the memory for the string using the allocator *
 * the string length is put in the two bytes before the string itself
 * thus there is no need to know the string size when deallocating 
 * returns the alloacted string on success or NULL if fails */
RvSdpString rvSdpStringAlloc(
        const char* txt,        /* points to the string which will be copied into allocated memory */
        RvAlloc* a);            /* the allocator to be used */

/* to reallocate the string *
 * returns the alloacted string on success or NULL if fails */
RvSdpString rvSdpStringRealloc(
        RvSdpString oldTxt,     /* the previously allocated (using rvSdpStringAlloc)  string */
        const char* newTxt,     /* the new value of the string to be reallocated */
        RvAlloc* a);            /* the allocator to be used, must be the same as used when the string
                                 * was first allocated */

/* to deallocate the string 
 * the string length is kept in two bytes before 
 * the string itself */
void rvSdpStringDealloc(
        RvSdpString str,        
        RvAlloc* a);

/* to allocate the string in the context of RvStringBuffer *
 * returns the alloacted string on success or NULL if fails */
RvSdpString rvSdpStringBufferAlloc(
        const char* txt,    /* string to be copied */
        RvStringBuffer *b); /* the buffer to use */


/* to re-allocate the string in the context of RvStringBuffer *
 * returns the allocated string on success or NULL if fails */
RvSdpString rvSdpStringBufferRealloc(
        RvSdpString oldTxt,  /* the previously allocated (using rvSdpStringBufferAlloc)  string */
        const char* newTxt,  /* the new value of the string to be reallocated */
        RvStringBuffer *b);

/* to de-allocate the string in the context of RvStringBuffer */
void rvSdpStringBufferDealloc(
        RvSdpString str,     /* the previously allocated string */   
        RvStringBuffer *b);

/* this function is called when the RvStringBuffer needs to reallocate its internal buffer */
RvBool rvSdpStringBufferReshuffle(
        RvStringBuffer *b,     
        int l);                     /* how many bytes (at least) have to be added to the buffer*/

/* returns the text representation of unsigned integer */
char* rvSdpUitoa(   
        char* txt,      /* the buffer to use & return */
        RvUint32 n);    /* the number to print */

/* returns the text representation of integer */
RVSDPCOREAPI char* rvSdpItoa(
        char* txt,      /* the buffer to use & return */
        RvInt32 n);     /* the number to print */

#ifdef __cplusplus
}
#endif
    
#endif

