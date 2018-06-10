/******************************************************************************
Filename    : rvsdpdatastruct.h
Description : the definition of public SDP library data structures

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

#ifndef _rvsdpdatastruct_h_
#define _rvsdpdatastruct_h_

/*
 *	SDP Data structures
 */


/*
 * Represents the session time data, 't=' lines
 */
 
typedef struct _RvSdpSessionTime
{
    RvListNode              iNextSessionTime;   /* session time instances kept 
											     * in linked-list in the message */
    RvSdpLineObject         iLineObj;			
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;    /* for not standard compliant 't=' lines*/
#endif
    RvUint32                iStart;				/* session start time */
    RvUint32                iEnd;				/* session end time */
    RvSdpList               iRepeatList;		/* all repeat intervals ('r=') 
												 * of this session time */ 
    void*                   iObj;				/* allocator or message instance */
} RvSdpSessionTime;


/*
 *	session repeat data ('r=' lines)
 */
typedef struct _RvSdpRepeatInterval
{
    RvListNode              iNextRepeatInterval; /* repeat interval instances kept 
											      * in linked-list in the session time */
    RvSdpLineObject         iLineObj;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;    /* for non standard 'r=' lines*/
#endif
    
    RvSdpTypedTime          iInterval;			/* repeat interval length */
    RvSdpTypedTime          iDuration;			/* active duration */
    RvSdpList               iOffsetsList;		/* list of offsets from session start time */
    
    RvBool                  iPrivateAllocation;  /* whether was allocated directly or 
												  * from the pool */
    void*                   iObj;				 /* allocator or message instance */
} RvSdpRepeatInterval;

/*
 *	time zone adjustments ('z=' lines)
 *  there are two structs the first is for one Time Zone Adjustment
 *  while the second combines few adjustments in a single list
 */
typedef struct _RvSdpTimeZoneAdjust {
    RvListNode          iNextTimeZone;		
    RvUint32            iAdjustmentTime;	/* the length of light saving time period */
    RvInt32             iOffsetTime;		/* the offset time */
    RvSdpTimeUnit       iOffsetUnits;		/* the  offset units */
    void*               iObj;				/* allocator or message instance */
} RvSdpTimeZoneAdjust;

typedef struct {
    RvSdpLineObject         iLineObj;
    RvSdpList               iTimeZoneList;	/* the list of tza */
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField; /* for non standard 'z=' lines*/
#endif
} RvSdpTZA;

/*
 *	session bandwith data ('b=' lines)
 */
typedef struct _RvSdpBandwidth
{
    RvListNode              iNextBandwidth;
    RvSdpLineObject         iLineObj;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;
#endif
    RvSdpString             iBWType;			/* bandwidth type */
    RvUint32                iBWValue;			/* and value */
    void*                   iObj;				/* allocator or message instance */
} RvSdpBandwidth;


/*
 *	session connection data ('c=' lines)
 */
typedef struct _RvSdpConnection
{
    RvListNode              iNextConnection;
    RvSdpLineObject         iLineObj;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;
#endif
    RvSdpString             iNetTypeStr;		/* network type */
    RvSdpString             iAddrTypeStr;		/* address type */
    RvSdpString             iAddress;			/* the address */
    int                     iTtl;				/* ttl value */
    int                     iNumAddr;			/* number of subsequent addresses */
    void*                   iObj;				/* allocator or message instance */
} RvSdpConnection;

/*
 *	'a=' lines
 */
typedef struct _RvSdpAttribute {
    RvListNode              iNextAttribute;
    RvSdpLineObject         iLineObj;
    RvSdpString             iAttrName;				/* attribute name */
    RvSdpString             iAttrValue;				/* attribute value */
    void*                   iObj;					/* allocator or message instance */
	const RvSdpSpecAttributeData	*iSpecAttrData; /* if the pointer is set the
													 * attribute is special (RtpMap, 
													 * ConnMode etc) */
} RvSdpAttribute;

/*
 *	'k=' lines
 */
typedef struct _RvSdpKey
{
    RvSdpLineObject         iLineObj;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;
#endif
    RvSdpString             iTypeStr;			/* the encryption key type */
    RvSdpString             iData;				/* the encryption key value */
    void*                   iObj;				/* allocator or message instance */
    RvInt                   iKeyIsUsed;			/* if the value is magic number the 
												 * the instance is in use */
} RvSdpKey;  

/*
 *	rtp map special attributes 'a=rtpmap:'
 */
typedef struct _RvSdpRtpMap
{
    RvSdpAttribute          iRMAttribute;	
    RvListNode              iNextRtpMap;   
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;
#endif
    int                     iPayload;			/* the payload value */
    RvSdpString             iEncName;			/* encryption name */
    RvInt32                 iClockRate;			/* the clock rate */
    RvSdpString             iEncParameters;		/* encryption parameters */
    void*                   iObj;				/* allocator or message instance */
} RvSdpRtpMap;

#ifdef RV_SDP_KEY_MGMT_ATTR
/*
 *	key management special attributes 'a=keymgmt:'
 *  the key management special attribute support depends on the value 
 *  of 'RV_SDP_KEY_MGMT_ATTR' comilation switch.
 *  Unlike other SDP data structures the instances of RvSdpKeyMgmtAttr are not 
 *  allocated from dedicated pool but directly using RvAlloc (iAlloc).
 *  Still we need 'iObj' to allocate the strings in the context of RvSdpMsg instance.
 */
typedef struct _RvSdpKeyMgmtAttr
{
    RvSdpAttribute          iKMAttribute;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;
#endif
    RvSdpString             iPrtclId;			/* the protocol ID */
    RvSdpString             iKeyData;			/* the encrtyption key data */
    RvAlloc*                iAlloc;				/* the allocator used for RvSdpKeyMgmtAttr 
												 *  instance allocation */
    void*                   iObj;				/* allocator or message instance */
} RvSdpKeyMgmtAttr;
#endif /*RV_SDP_KEY_MGMT_ATTR*/

#ifdef RV_SDP_CRYPTO_ATTR

#define RV_SDP_CRYPTO_MAX_KEY_PARAMS        5
#define RV_SDP_CRYPTO_MAX_SESSION_PARAMS    5

/*
 *	crypto special attributes 'a=crypto:'
 *  the crypto special attribute support depends on the value 
 *  of 'RV_SDP_CRYPTO_ATTR' comilation switch.
 *  Unlike other SDP data structures the instances of RvSdpCryptoAttr are not 
 *  allocated from dedicated pool but directly using RvAlloc (iAlloc).
 *  Still we need 'iObj' to allocate the strings in the context of RvSdpMsg instance.
 */
typedef struct _RvSdpCryptoAttr
{
    RvSdpAttribute          iCrptAttribute;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;
#endif
    RvUint                  iCryptoTag;		/* the tag */
    RvSdpString             iCryptoSuite;	/* the name of the suite */

    RvSdpString             iCryptoKeyMethods[RV_SDP_CRYPTO_MAX_KEY_PARAMS];
    RvSdpString             iCryptoKeyInfos[RV_SDP_CRYPTO_MAX_KEY_PARAMS];
    RvUint16                iCryptoKeyParamsNum;
    
    RvSdpString             iCryptoSessParams[RV_SDP_CRYPTO_MAX_SESSION_PARAMS];
    RvUint16                iCryptoSessParamsNum;

    RvAlloc*                iAlloc;			/* the allocator used for RvSdpKeyMgmtAttr 
										     *  instance allocation */
    void*                   iObj;			/* allocator or message instance */
} RvSdpCryptoAttr;	
#endif /*RV_SDP_CRYPTO_ATTR*/



#if defined(RV_SDP_CHECK_BAD_SYNTAX)

typedef void* RvSdpGenericFieldPtr;
/*
 *	for proprietary tags
 */
typedef struct _RvSdpOther
{
    RvListNode              iNextOther;
    RvSdpLineObject         iLineObj;
    char                    iOtherTag;      /* the tag */
    RvSdpString             iValue;         /* the value after the '=' sign */
    void*                   iObj;			/* allocator or message instance */
} RvSdpOther;

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/*
 *	for 'p=' lines
 */
typedef struct _RvSdpPhone
{
    RvListNode          iNextPhone;
    RvSdpLineObject     iLineObj;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString         iBadSyntaxField;
#endif
    RvSdpString         iPhoneNumber;   /* the phone  number */
    RvSdpString         iText;          /* the description text */
    RvChar              iSeparSymbol;   /* the symbol used as text closure ('(' or '<') */
    void*               iObj;			/* allocator or message instance */	
} RvSdpPhone;


typedef struct _RvSdpEmail
{
    RvListNode                  iNextEmail;
    RvSdpLineObject             iLineObj;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString                 iBadSyntaxField;
#endif
    RvSdpString                 iAddress;     /* the email address */
    RvSdpString                 iText;        /* the description text */
    RvChar                      iSeparSymbol; /* the symbol used as text closure ('(' or '<') */
    void*                       iObj;		  /* allocator or message instance */
} RvSdpEmail;

/*
 *	'o=' lines
 */
typedef struct _RvSdpOrigin {
    RvSdpLineObject     iLineObj;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString         iBadSyntaxField;
#endif
    RvSdpString         iUserName;      /* user name */
    RvSdpString         iSessionId;     /* session id */
    RvSdpString         iVersion;       /* version */
    RvSdpString         iNetTypeStr;    /* network type */
    RvSdpString         iAddrTypeStr;   /* address type */
    RvSdpString         iAddress;       /* Connection address: multicast,or FQDN  */
    RvInt               iOriginIsUsed;  /* whether the struct instance is filled */
    void*               iObj;			/* allocator or message instance */
} RvSdpOrigin;

/*
 *	'i=' lines
 */
typedef struct _RvSdpInformation
{
    RvSdpLineObject         iLineObj;
    RvSdpString             iInfoTxt;    
} RvSdpInformation;

/*
 *	this struct combines data common to the message and media descriptor
 */
typedef struct _RvSdpCommonFields {
    RvSdpInformation        iInfo;              /* 'i=' line */
    RvSdpList               iBandwidthList;     /* 'b=' lines */
    RvSdpList               iConnectionList;    /* 'c=' lines */
    
    RvSdpKey                iKey;               /* 'k=' line */
    RvSdpList               iAttrList;          /* 'a=' lines */
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    /* List of unknown fields */
    RvSdpList               iOtherList;         /* proprietary tags */
#endif
    
} RvSdpCommonFields;       

#define RV_SDP_MEDIA_FORMATS_MAX    32

/*
 *	This struct contains data set in 'm=' line itself and through
 *  the 'iCommonFields' all SDP lines regarding this specific
 *  media descriptors
 */

typedef struct _RvSdpMediaDescr {

    RvListNode              iNextMediaDescr;
    RvSdpLineObject         iLineObj;
    
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvSdpString             iBadSyntaxField;
#endif
    
    RvUint32                iPort;              /* media port number */
    int                     iNumOfPorts;        /* number of subsequent ports */
    
    RvSdpString             iProtocolStr;       /* media protocol */
    RvSdpString             iMediaTypeStr;      /* type  of media */
    
    RvSdpString             iMediaFormats[RV_SDP_MEDIA_FORMATS_MAX];  
                                                /* array of formats names */
    RvUint16                iMediaFormatsNum;
        
    RvSdpCommonFields       iCommonFields;  /* SDP lines relating this media descriptor */
    struct _RvSdpMsg*       iSdpMsg;        /* pointer to owning message */
    void*                   iObj;			/* allocator or message instance */
    struct _RvSdpMsg*       iObsMsg;        /* this is used as media's pseudo-owning message in case
                                               media is constructed in the obsolete way using
                                               rvSdpMediaDescrConstruct and likes */
} RvSdpMediaDescr;


/*
 *	'v=' line data
 */
typedef struct {
    RvSdpLineObject             iLineObj;
    RvSdpString                 iVersionTxt;        /* the version text*/
	RvBool						iSetByMsgConstruct; 
    /* in order to keep backward 
     * compability the there is default version ('v=' line) in each RvSdpMsg instance.
     * If 'iSetByMsgConstruct' is TRUE the version of SDP message was set by default when
     * the message was constructed (intialized) and not because the 'v=' line was parsed
     * in SDP input (or the SDP version was added by explicit API call) */
} RvSdpVersion;


/*
 *	's=' lines
 */
typedef struct {
    RvSdpLineObject             iLineObj;
    RvSdpString                 iSessIdTxt;
} RvSdpSessId;

/*
 *	'u=' lines
 */

typedef struct {
    RvSdpLineObject             iLineObj;
    RvSdpString                 iUriTxt;
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
    RvBool                      iUriBadSyntax;
#endif
} RvSdpUri;

/*
 *	the whole SDP message
 */
typedef struct _RvSdpMsg {

    RvUint32                    iMagicNumber;  
                        /* this field has to be the first field of the structure 
                         * and its value has to be RV_SDP_MESSAGE_MAGIC_NUMBER,
                         * this is done for easy distinction of SDP message pointed
                         * by 'iObj' of SDP data structs */

    RvListNode                  iNextMessage;
    
    RvAlloc*                    iAllocator;     
                                        /* the allocator used memory allocations */
    RvBool                      iIsAllocated;   
                                        /* whether the memory for RvSdpMsg instance was 
                                         * allocated at the time of message construction
                                         * or the instance was given externally */

    RvSdpVersion                iVersion;   /* 'v=' line */
    
    RvSdpOrigin                 iOrigin;    /* 'o=' line */
    
    RvSdpSessId                 iSessId;    /* 's=' line */
    
    RvSdpUri                    iUri;       /* 'u=' line */
    
    RvSdpList                   iEmailList; /* 'e=' lines */
    RvSdpList                   iPhoneList; /* 'p=' lines */
    
    /* Fields common to session level and media level */
    /* Information,Connection,Bandwidth, encryption key */
    /* and attributes and other field*/
    RvSdpCommonFields           iCommonFields;  
    
    /* Time fields */
    RvSdpList                   iSessionTimeList;   /* 't=' and 'r' lines */
    RvSdpTZA                    iTZA;               /* 'z=' lines */
                
    RvSdpList                   iMediaDescriptors;  /* 'm=' lines with all its data */
    
    RvSdpLineObject*            iHeadLO;    /* the first line object */
    RvSdpLineObject*            iTailLO;    /* the last line object */
    RvUint16                    iLOCnt;     /* number of line objects */
    
    RvStringBuffer              iStrBuffer; /* strings  buffer */
    
        /* Pools of reusable SDP objects */
    RvSdpObjPool                iAttrsPool;
    RvSdpObjPool                iOthersPool; 
    RvSdpObjPool                iEmailsPool;
    RvSdpObjPool                iPhonesPool;
    RvSdpObjPool                iMediasPool;    
    RvSdpObjPool                iRtpMapsPool;    
    RvSdpObjPool                iSessTimesPool;
    RvSdpObjPool                iConnectionsPool;
    RvSdpObjPool                iBandwidthsPool;
    RvSdpObjPool                iRepeatIntervalPool;
    RvSdpObjPool                iTypedTimePool;
    RvSdpObjPool                iTimeZonesPool;
} RvSdpMsg;  


/* list of SDP messages */
typedef struct {
    RvSdpList  iList;
    RvAlloc*   iAllocator;
} RvSdpMsgList;

#endif/*#ifndef _rvsdpdatastuct_h_*/
