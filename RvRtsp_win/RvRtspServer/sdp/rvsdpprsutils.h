/******************************************************************************
Filename    :rvsdpprsutils.h
Description : functions used during parse & encode process

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

#ifndef _rvsdpprsutils_h_
#define _rvsdpprsutils_h_

#ifdef __cplusplus
extern "C" {
#endif
    

#define M_SF               0x0001
#define M_H                0x0002
#define M_A                0x0004
#define M_D                0x0008
#define M_AD               0x0010
#define M_PD               0x0020
#define M_PH               0x0040
#define M_ADH              0x0080  /* alpha numeric and hyphen */
#define M_MD               0x0100
#define M_PR               0x0200
#define M_FMT              0x0400
#define M_BWT              0x0800
#define M_D_SL             0x1000
#define M_ADDR             0x2000
#define M_B64              0x4000


#define RV_SDP_IS_HEX(c)        ((unsigned char)(c)<127&&(gsSymbolMsks[(unsigned)c]&M_H))
#define RV_SDP_IS_ALPHA(c)      ((unsigned char)(c)<127&&(gsSymbolMsks[(unsigned)c]&M_A))
#define RV_SDP_IS_DIGIT(c)      ((unsigned char)(c)<127&&(gsSymbolMsks[(unsigned)c]&M_D))
#define RV_SDP_IS_POS_DIGIT(c)  ((unsigned char)(c)<127&&(gsSymbolMsks[(unsigned)c]&M_PD))
#define RV_SDP_IS_PHONE(c)      ((unsigned char)(c)<127&&(gsSymbolMsks[(unsigned)c]&M_PH))
#define RV_SDP_IS_D_SL(c)       ((unsigned char)(c)<127&&(gsSymbolMsks[(unsigned)c]&M_D_SL))

/*
 *	Parses the integer number. Moves the '*ptr' to a position after the number.
 *  Returns RV_TRUE if successfull, otherwise returns RV_FALSE and the 'w' is set
 *  to correspondent value.
 */

RvBool rvSdpParseInt(
        RvInt minNum,               /* min possible number */ 
        RvUint maxNum,              /* max possible number */
                                    /* if the range is not important set minNum and 
                                       maxNum to zero values */
        RvUint* res,                /* the parsed number value will be set here */ 
        RvBool allowLeadingZero,    /* whether the leading zeros are allowed */
        RvBool isHex,               /* whetherthe number is hexadecimal or decimal */
        RvChar** ptr,               /* the start of the number, the '*ptr' will be moved
                                       to a position after the number end */
        RvSdpIntegerParseWarning* w);   /* the possible failure  reason will be set here */
            

/* 
 * Moves '*ptr' to the next space, tab, 'c1' symbol (if 'c1' is not zero) or the end of
 * line.
 * RV_TRUE is returned if '*ptr' could be moved (there were some printable, non-space 
 * characters) or RV_FALSE otherwise. 
 */
RvBool rvSdpParseNonSpace(
        RvChar** ptr,
        char c1);
/*
 * Moves '*ptr' to the end of buffer.
 * RV_TRUE is returned if '*ptr' could be moved or RV_FALSE otherwise. 
 */
RvBool rvSdpParseAnyText(
        RvChar** ptr);

/*
 * Moves '*ptr' to next character different from the one defined by 'msk' in 
 * 'gsSymbolMsks' array or to the end of buffer.
 * RV_TRUE is returned if '*ptr' could be moved (there were some symbols marked
 * by 'msk' in 'gsSymbolMsks' array) or RV_FALSE otherwise. 
 */
RvBool rvSdpParseText(
        RvChar** ptr, 
        RvUint msk);

/*
 * Moves '*ptr' to next character different from the one defined by 'msk' in 
 * 'gsSymbolMsks' array or 'c1' character OR to the end of buffer.
 * RV_TRUE is returned if '*ptr' could be moved (there were some symbols marked
 * by 'msk' in 'gsSymbolMsks' array or 'c1' character) or RV_FALSE otherwise. 
 */
RvBool rvSdpParseText2(
        RvChar** ptr, 
        RvUint msk, 
        char c1);

/*
 * Moves '*ptr' to the next non-space character or to the end of the buffer.
 * RV_TRUE is returned if '*ptr' could be moved (there  were some spaces) or 
 * RV_FALSE otherwise. 
 */
RvBool rvSdpParseSpace(
        RvChar** ptr);

/*
 *	Encodes the SDP version line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeVersion(
        char **p, 
        char *pEnd,
        char* v);

/*
 *	Encodes the SDP origin line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeOrigin(
        char **p, 
        char *pEnd,
        RvSdpOrigin *o);

/*
 *	Encodes the SDP session name line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeSessionName(
        char **p, 
        char *pEnd,
        char *n);

/*
 *	Encodes the SDP session information line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeSessionInfo(
        char **p, 
        char *pEnd,
        char *i);

/*
 *	Encodes the SDP URI line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeURI(
        char **p, 
        char *pEnd,
        char *u);

/*
 *	Encodes the SDP email line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeEmail(
        char **p, 
        char *pEnd,
        RvSdpEmail *e);

/*
 *	Encodes the SDP phone line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodePhone(
        char **p, 
        char *pEnd,
        RvSdpPhone *ph);

/*
 *	Encodes the SDP connection line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeConnection(
        char **p, 
        char *pEnd,
        RvSdpConnection *cn);

/*
 *	Encodes the SDP bandwidth line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeBandwidth(
        char **p, 
        char *pEnd,
        RvSdpBandwidth* bw);

/*
 *	Encodes the SDP session time line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeSessionTime(
        char** p, 
        char *pEnd,
        RvSdpSessionTime *sessT);

/*
 *	Encodes the SDP repeat interval line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeRepeatInterval(
        char** p, 
        char *pEnd,
        RvSdpRepeatInterval *rinterv);

/*
 *	Encodes the SDP time zone adjustment line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeTimeZone(
        char **p, 
        char *pEnd,
        RvSdpTZA *tza);

/*
 *	Encodes the SDP key line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeKey(
        char **p, 
        char *pEnd,
        RvSdpKey *k);

/*
 *	Encodes the SDP attribute line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeAttr(
        char **p, 
        char *pEnd,
        RvSdpAttribute *a);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)        
/*
 *	Encodes the SDP proprietary tag line into '*p'.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeOther(
        char **p, 
        char *pEnd,
        RvSdpOther *o);
#endif

/*
 *	Encodes the SDP media descriptor line into '*p' and all objects
 *  contained in the media descriptor object.
 *  '*p' is moved at the end of encoded portion.
 */
RVSDPCOREAPI RvBool rvSdpEncodeMediaDescrFields(
        char **p, 
        char *pEnd,
        RvSdpMediaDescr* md);


/*
 *	Parses the version line ('v='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseVersion(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the origin line ('o='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseOrigin(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the session name line ('s='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseSessionName(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the information line ('i='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseInformation(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the URI line ('u='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseURI(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the email line ('e='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseEmail(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the phone line ('p='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParsePhone(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the connection line ('c='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseConnection(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the bandwidth line ('b='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseBandwidth(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the session time line ('t='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseTime(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the repeat interval line ('r='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseTimeRepeat(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the time zone adjustment line ('z='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseZoneAdjust(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the key line ('k='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseKey(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the attribute line ('a='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseAttribute(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

/*
 *	Parses the media description line ('m='), constructs correspondent 
 *  SDP object and and appends it to the RvSdpMsg object.
 *  Returns the RV_TRUE if successful and RV_FALSE otherwise. 
 */
RvBool rvSdpParseMediaDescr(
		RvSdpParserData* pD,        /* the parser data object */
		RvBool createAsBadSyntax,   /* If set to RV_TRUE the bad syntax object
                                       will be constructed. This is not applicable to 
                                       the all possible SDP objects.
                                       Usually this parameter is set to RV_TRUE
                                       if the previous call to the function has failed
                                       due to parsing error. */
		RvBool replaceExisting);    /* set to RV_TRUE for the tags which appears 
                                       more than once in the input and that 
                                       contradicts the standard */

#ifdef __cplusplus
}
#endif
    
#endif

