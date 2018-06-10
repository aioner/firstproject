/******************************************************************************
Filename    :rvsdprivate.h
Description : SDP functions not intended for use by library user

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

#ifndef _file_rvsdpprivate_h_
#define _file_rvsdpprivate_h_

#ifdef __cplusplus
extern "C" {
#endif
    
#include "rvsdpconfig.h"
#include "rvtypes.h"
#include "rvalloc.h"
#include "rvsdpdataprivate.h"  
#include "rvsdplist.h"
#include "rvsdpenums.h"
#include "rvsdplist.h"
#include "rvsdpstrings.h"
#include "rvsdp.h"
#include "rvsdpglobals.h"
#include "rvsdpprsutils.h"


#define RV_SDP_MESSAGE_MAGIC_NUMBER 18367492
#define RV_SDP_OBJ_IS_MESSAGE(_p)   (*(RvUint32*)((_p)->iObj) == RV_SDP_MESSAGE_MAGIC_NUMBER)
#define RV_SDP_OBJ_IS_MESSAGE2(_p)  (*(RvUint32*)(_p) == RV_SDP_MESSAGE_MAGIC_NUMBER)


#ifdef RV_SDP_USE_DEBUG_ALLOC_MODEL    
/*
 *	these functions are used for memory usage monitoring
 */

/*
 *	gets the current number of bytes allocated and the number of allocations
 */
RVSDPCOREAPI void rvSdpGetMemoryCounters(
                int* memSize,   /* number of currently allocated bytes */
                int* memCnt);   /* number of current allocations */

/* gets the max usage of memory */
RVSDPCOREAPI RvUint32 rvSdpGetMemoryMaxUsage();
                                         
/*
 *	Wraps the regular rvSdpAllocAllocate function.
 *  Saves in a linked list some allocation details (number of bytes, pointer
 *  to allocated chunk and  the file name and line number of function call)
 */
void* sdpRvAllocAllocate(
                RvAlloc* a,     /* allocator */
                RvSize_t s,     /* size of requested chunk */
                char* fileN,    /* file name where the function was called */
                int lineN);     /* line number of the function call */

/*
 *	Wraps the regular rvSdpAllocDeallocate function.
 *  Cleans from a linked list the allocation details.
 */
void sdpRvAllocDeallocate(
                RvAlloc* a,     /* allocator */
                RvSize_t s,     /* size of freed chunk */
                void* ptr);     /* a chunk to free */

void sdpRvAllocInitialize();
void sdpRvAllocEnd();

#define rvSdpAllocDeallocate sdpRvAllocDeallocate
#define rvSdpAllocAllocate(n1,n2) sdpRvAllocAllocate(n1,n2,__FILE__,__LINE__)
#define rvSdpAllocInitialize()  sdpRvAllocInitialize()
#define rvSdpAllocEnd()  sdpRvAllocEnd()
#else   /*RV_SDP_USE_DEBUG_ALLOC_MODEL*/
#define rvSdpAllocDeallocate rvAllocDeallocate
#define rvSdpAllocAllocate(n1,n2) rvAllocAllocate(n1,n2)
#define rvSdpAllocInitialize()  
#define rvSdpAllocEnd()  
#define rvSdpGetMemoryCounters(p1,p2) {*(p1) = *(p2) = 0;} 
#define rvSdpGetMemoryMaxUsage() 0


#endif  /*RV_SDP_USE_DEBUG_ALLOC_MODEL*/


#if defined(RV_SDP_CHECK_BAD_SYNTAX)
#define RV_SDP_BAD_SYNTAX_PARAM(_p) _p
#else /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/
#define RV_SDP_BAD_SYNTAX_PARAM(_p) 0
#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/*
 *  SDP Attribute private functions
 */

/*
 *	To allocate the memory for RvSdpAttribute object (called by the pool)
 *  Return: 
 *      valid RvSdpAttribute pointer
 *      or NULL if fails
 */
RvSdpAttribute* rvSdpAttributeCreateByPool(
        RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpAttribute object (called by the pool)
 */
void rvSdpAttributeDestroyByPool(
        RvSdpAttribute* p);

/*
 *	SDP Line object functions
 */

/* to insert the line object within the list of line objects */
void rvSdpLineObjsListInsert(
            RvSdpMsg* msg,          /* message where line object has to added */
            RvSdpFieldTypes tag,    /* the tag of line object */
            RvSdpLineObject *lo,    /* the line object */
            int offs);              /* an offset of RvSdpLineObject struct 
                                       in the SDP object structure*/
 
/* to remove the line object from the list */
void rvSdpLineObjsListRemove(
            RvSdpMsg* msg,          /* message where line object is removed */
            RvSdpLineObject *lo);   /* the line object */


/*
 *	SDP Message private functions
 */

/* destructs the common (to message and media descriptor) fields  */
void rvSdpCommonFieldsDestruct(
            RvSdpCommonFields* fields,  /* the common fields */
            void* obj);                 /* context: RvSdpMsg or RvAlloc */
        
/* initializes the common (to message and media descriptor) fields  */
void rvSdpCommonFieldsInitialize(
           RvSdpCommonFields* fields);  /* the common fields */

/* Deallocates RvSdpAttribute netto memory	*/
void rvSdpAttributeDestructNetto(
            RvSdpAttribute* attr,   /* the attribute pointer */ 
            RvBool retToPool);      /* whether to return to attributes pool */


/* resets the contents of SDP messages (pools are not freed) */
void rvSdpMsgDestructForReuse(
            RvSdpMsg* msg);

/* guarantee that strings buffer will be big enough to fit 'len' more bytes; 
 * as a result reshuffle procedure may be performed */
void rvSdpMsgPromiseBuffer(
            RvSdpMsg* msg,  /* message owning strings buffer */
            int len);       /* number of bytes to guarantee */


/*
 *	Bandwidth private functions
 */

/*
 *	To allocate the memory for RvSdpBandwidth object (called by the pool)
 *  Return: 
 *      valid RvSdpBandwidth pointer
 *      or NULL if fails
 */
RvSdpBandwidth* rvSdpBandwidthCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpBandwidth object (called by the pool)
 */
void rvSdpBandwidthDestroyByPool(
                RvSdpBandwidth* bw);

/*
 *	Sets the internal RvSdpBandwidth fields to the supplied values.
 *  Returns the 'bw' of NULL if fails.
 */
RvSdpBandwidth* rvSdpBandwidthFill(
               RvSdpBandwidth* bw, 
               const char* type, 
               RvUint32 value);

/*
 *	Origin private functions
 */

/*
 *	Sets the internal RvSdpOrigin fields to the supplied values.
 *  Returns the 'origin' of NULL if fails.
 */
RvSdpOrigin* rvSdpOriginFill(   
                RvSdpOrigin *origin,    /* RvSdpOrigin instance */  
                const char* userName,   /* the origin field user name */ 
                const char* sessionId,  /* the origin field session ID */ 
                const char* version,    /* the origin field version */ 
                const char* netType,    /* the origin field network type */ 
                const char* addrType,   /* the origin field address type */ 
                const char* address);   /* the origin field address */              

/*
 *	Connection private functions
 */

/*
 *	To allocate the memory for RvSdpConnection object (called by the pool)
 *  Return: 
 *      valid RvSdpConnection pointer
 *      or NULL if fails
 */
RvSdpConnection* rvSdpConnectionCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpConnection object (called by the pool)
 */
void rvSdpConnectionDestroyByPool(
                RvSdpConnection* p);

/*
 *	Sets the internal RvSdpConnection fields to the supplied values.
 *  Returns the 'conn' of NULL if fails.
 */
RvSdpConnection* rvSdpConnectionFill(
                RvSdpConnection* conn, 
                const char* addr, 
                const char* addrType, 
                const char* netType, 
                int numAddr, 
                int ttl);
   
/*
 *	Uri private functions
 */
/*
 *	Sets the URI fields of the message.
 *  Returns RV_SDPSTATUS_OK on success or RV_SDPSTATUS_ALLOCFAIL if fails.
 */
RvSdpStatus rvSdpMsgFillURI(
				RvSdpMsg* msg, 
				const char* uri, 
				RvBool badSyntax);

/*
 *	rtp map private functions
 */
             

/*
 *	Sets the internal RvSdpRtpMap fields to the supplied values.
 *  Returns the 'rtpMap' of NULL if fails.
 */
RvSdpRtpMap* rvSdpRtpMapFill(
				RvSdpRtpMap* rtpMap,    /* the RvSdpRtpMap instance to fill */
				int payload,            /* the payload of RTP map */
				const char* encName,    /* the encoding name */ 
				int rate,               /* the rate */
				const char* encParams,  /* the encoding parameters */
                const char* badSyn);    /* the proprietary formatted syntax of RTP map
                                           or NULL */
				
/*
 *	To allocate the memory for RvSdpRtpMap object (called by the pool)
 *  Return: 
 *      valid RvSdpRtpMap pointer
 *      or NULL if fails
 */
RvSdpRtpMap* rvSdpRtpMapCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpRtpMap object (called by the pool)
 */
void rvSdpRtpMapDestroyByPool(
                RvSdpRtpMap* p);

/*
 *	Called during message strings buffer reshuffle.
 *  Copies all string fields of RTP map into '*ptr' while
 *  increasing '*ptr' value by the length of copied strings.
 */
void rvSdpRtpMapReshuffle(
                RvSdpRtpMap *rtpMap,  /* the RTP map to reshuffle */
                char** ptr);          /* points inside the new buffer string chunk */

/*
 *	Parses the RTP map value. Sets the supplied RTP map fields pointer to the 
 *  values found in the 'ptr' input. 
 *  Sets the zero bytes at the end of all textual fields (encName and encParams).
 *  Returns RV_FALSE if the parsing fails.
 */
RvBool rvSdpParseRtpMap(
                RvSdpParserData* pD,    /* the parser data  instance;
                                           used to create parse warnings */
                char *ptr,              /* the input pointer */
                int* payload,           /* will be set to payload */
                char** encName,         /* will point to the start of encoding name in
                                           the input buffer */
                int* encRate,           /* will be set to encoding rate */
                char** encParams,       /* will point to the start of encoding param in
                                           the input buffer */
                REPLACING_DECL);        /* used to set zero bytes in the input buffer */

/*
 *	Special attribute  parse function for the RTP map. 
 *  Parses RTP map attribute field and constructs the RvSdpRtpMap object.
 *  The constructed object is added to current context message or media 
 *  descriptor.
 *  If parsing or construction fails the correspondent status is returned.
 */
RvSdpSpecAttrParseSts rvSdpRtpMapParsing(
				const RvSdpSpecAttributeData* sad,  /* the special attribute data */
                RvBool createAsBadSyntax,           /* if set to RV_TRUE the bad 
                                                       syntax RTP map attribute will be  
                                                       created */
                RvSdpParserData* pD,    /* the parser data instance */        
                RvSdpCommonFields *cm,  /* the current context common fields instance,
                                           here the special attribute will be added */
				char* n,                /* the attribute name */
                char* v,                /* the attribute value to parse */
                REPLACING_DECL);        /* used for zero-substitutions in input buffer */

/*
 *	email private functions
 */

/*
 *	To allocate the memory for RvSdpEmail object (called by the pool)
 *  Return: 
 *      valid RvSdpEmail pointer
 *      or NULL if fails
 */
RvSdpEmail* rvSdpEmailCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpEmail object (called by the pool)
 */
void rvSdpEmailDestroyByPool(
                RvSdpEmail* p);

/*
 *	Sets the internal RvSdpEmail fields to the supplied values.
 *  Returns the 'email' of NULL if fails.
 */
RvSdpEmail* rvSdpEmailFill(
                RvSdpEmail* email, 
                const char* address, 
                const char* text, 
                RvChar separSymbol);

/*
 *	phone private functions
 */

/*
 *	To allocate the memory for RvSdpPhone object (called by the pool)
 *  Return: 
 *      valid RvSdpPhone pointer
 *      or NULL if fails
 */
RvSdpPhone* rvSdpPhoneCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpPhone object (called by the pool)
 */
void rvSdpPhoneDestroyByPool(
                RvSdpPhone* phone);

/*
 *	Sets the internal RvSdpPhone fields to the supplied values.
 *  Returns the 'phone' of NULL if fails.
 */
RvSdpPhone* rvSdpPhoneFill(
                RvSdpPhone* phone, 
                const char* phNumber, 
                const char* text, 
                RvChar separSymbol);

/*
 *	key private functions
 */

/*
 *	Sets the internal RvSdpKey fields to the supplied values.
 *  Returns the 'key' of NULL if fails.
 */
RvSdpKey* rvSdpKeyFill(
                RvSdpKey* key, 
                const char* type, 
                const char* data);

/*
 *	media private functions
 */

/* 
 * Constructs the RvSdpMediaDescr object, copies the 'descr' instance to the
 * constructed one and adds the new RvSdpMediaDescr object to RvSdpMessage object 
 */
RvSdpMediaDescr* rvSdpMsgInsertMediaDescr2(
                    RvSdpMsg* msg,
                    RvSdpMediaDescr* descr);

/*
 *	Sets the internal RvSdpMediaDescr fields of 'dest' from the 'src'
 *  RvSdpMediaDescr instance.   
 *  Returns the new constructed RvSdpMediaDescr object.
 */
RvSdpMediaDescr* rvSdpMediaDescrFill(
                    RvSdpMediaDescr* dest,  /* the new constructed object has to
                                               copied from this one */
                    const RvSdpMediaDescr* src);    /* here the new object will be 
                                                       inserted */

/*
 *	To allocate the memory for RvSdpMediaDescr object (called by the pool)
 *  Return: 
 *      valid RvSdpMediaDescr pointer
 *      or NULL if fails
 */
RvSdpMediaDescr* rvSdpMediaDescrCreateByPool(
                    RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpMediaDescr object (called by the pool)
 */
void rvSdpMediaDescrDestroyByPool(
                    RvSdpMediaDescr* media);

/* 
 * tests whether the line object 'lo' is bad syntax and
 * belongs to media descriptor 'media'
 */
RvBool rvSdpLineObjectBelongsToMediaAndBad(
                    RvSdpLineObject *lo, 
                    RvSdpMediaDescr *media);

/*
 *	session time private functions
 */

/*
 *	To allocate the memory for RvSdpSessionTime object (called by the pool)
 *  Return: 
 *      valid RvSdpSessionTime pointer
 *      or NULL if fails
 */
RvSdpSessionTime* rvSdpSessTimeCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpSessionTime object (called by the pool)
 */
void rvSdpSessTimeDestroyByPool(
                RvSdpSessionTime* sessTime);

/*
 *	Sets the internal RvSdpSessionTime fields to the supplied values.
 *  Returns the 'sessTime' of NULL if fails.
 */
RvSdpSessionTime* rvSdpSessionTimeFill(
                RvSdpSessionTime* sessTime, 
                RvUint32 start, 
                RvUint32 end);

/*
 *	session time zone adjustments private functions
 */

/*
 *	To allocate the memory for RvSdpTimeZoneAdjust object (called by the pool)
 *  Return: 
 *      valid RvSdpTimeZoneAdjust pointer
 *      or NULL if fails
 */
RvSdpTimeZoneAdjust* rvSdpTimeZoneCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpTimeZoneAdjust object (called by the pool)
 */
void rvSdpTimeZoneAdjustDestroyByPool(
                RvSdpTimeZoneAdjust* p);

/*
 *	session time repeat intervals private functions
 */

/*
 *	To allocate the memory for RvSdpRepeatInterval object (called by the pool)
 *  Return: 
 *      valid RvSdpRepeatInterval pointer
 *      or NULL if fails
 */
RvSdpRepeatInterval* rvSdpRepeatIntervalCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpRepeatInterval object (called by the pool)
 */
void rvSdpRepeatIntervalDestroyByPool(
                RvSdpRepeatInterval* interv);


/* copies the 'src' RvSdpRepeatInterval to the 'dst' RvSdpRepeatInterval */
RvSdpRepeatInterval* rvSdpRepeatIntervalCopyForSessTime(
                RvSdpRepeatInterval* dst,
                const RvSdpRepeatInterval* src,
                RvSdpMsg* msg);

/*
 *	Sets the internal RvSdpRepeatInterval fields to the supplied values.
 *  Returns the 'dst' of NULL if fails.
 */
RvSdpRepeatInterval* rvSdpRepeatIntervalFill(
				RvSdpRepeatInterval* dst, 
				const RvSdpRepeatInterval* src);

/*
 *	typed time private functions
 */

/*
 *	To allocate the memory for RvSdpTypedTime object (called by the pool)
 *  Return: 
 *      valid RvSdpTypedTime pointer
 *      or NULL if fails
 */
RvSdpTypedTime* rvSdpTypedTimeCreateByPool(
                RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpTypedTime object (called by the pool)
 */
void rvSdpTypedTimeDestroyByPool(
                RvSdpTypedTime* typedTime);

/*
 *	destructs (frees the memory or returns to pool) the RvSdpTypedTime instance
 */
void rvSdpTypedTimeDestruct(
                RvSdpTypedTime *tt);

/*
 *	returns the time and units of RvSdpTypedTime instance
 */
RvBool rvSdpBreakTT(RvSdpTypedTime *tt, 
                RvUint32* time, 
                RvSdpTimeUnit* t_unit);


/*
 *	Used during reshuffle;
 *  If the '*oldPtr' is not NULL it is copied to the '*ptr'.
 *  The '*ptr' is increased by the length of '*oldPtr'.
 */ 
void rvSdpChangeText(char** ptr,char **oldPtr);


#ifdef RV_SDP_KEY_MGMT_ATTR

/*
 *	Called during message strings buffer reshuffle.
 *  Copies all string fields of key-mgmt into '*ptr' while
 *  increasing '*ptr' value by the length of copied strings.
 */
void rvSdpKeyMgmtReshuffle(
                struct _RvSdpKeyMgmtAttr *km, 
                char** ptr);

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
                REPLACING_DECL);        /* used for zero-substitutions in input buffer */

#endif /*RV_SDP_KEY_MGMT_ATTR*/


#ifdef RV_SDP_CRYPTO_ATTR

/*
 *	Called during message strings buffer reshuffle.
 *  Copies all string fields of crypto attribute into '*ptr' while
 *  increasing '*ptr' value by the length of copied strings.
 */
void rvSdpCryptoReshuffle(
                struct _RvSdpCryptoAttr *crpt, 
                char** ptr);

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
                REPLACING_DECL);        /* used for zero-substitutions in input buffer */


#endif /*RV_SDP_CRYPTO_ATTR*/

#ifdef RV_SDP_CHECK_BAD_SYNTAX

/*
 *	To allocate the memory for RvSdpOther object (called by the pool)
 *  Return: 
 *      valid RvSdpOther pointer
 *      or NULL if fails
 */
struct _RvSdpOther* rvSdpOtherCreateByPool(
                struct _RvSdpMsg* msg);

/*
 *	To free the memory for RvSdpOther object (called by the pool)
 */
void rvSdpOtherDestroyByPool(
                struct _RvSdpOther* oth);

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/* gets default SDP allocator */
RvAlloc* rvSdpGetDefaultAllocator();

#ifdef __cplusplus
}
#endif
#endif 


