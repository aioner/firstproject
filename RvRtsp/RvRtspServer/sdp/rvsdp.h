/******************************************************************************
Filename    : rvsdp.h
Description : all public APIs of the SDP library

  ******************************************************************************
  Copyright (c) 2005 RADVision Inc.
  ************************************************************************
  NOTICE:
  This document contains information that is proprietary to RADVision LTD.
  No part of this publication may be reproduced in any form whatsoever
  without written prior approval by RADVision LTD.

    RADVision LTD. reserves the right to revise this publication and make
    changes without obligation to notify any person of such revisions or
    changes.
 Author:Rafi Kiel
******************************************************************************/

#ifndef _rvsdp_h_
#define _rvsdp_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "rvsdpconfig.h"
#include "rvsdpenums.h"
#include "rvalloc.h"
#include "rvsdpdataprivate.h"
#include "rvsdpapiprivate.h"
#include "rvsdplist.h"
#include "rvsdpstrings.h"
#include "rvsdpversion.h"

#include "rvsdpdatastruct.h"

#if defined(RV_SDP_ADS_IS_USED)
#include "rpool_API.h"
#endif

/* Default version of the protocol, not of this software */
#ifndef RV_SDP_SDPVERSION
#define RV_SDP_SDPVERSION "0"
#endif

#define RV_SDPMAX_VERSION_SIZE   4

/* For backward compatibility */
#define rvFalse 0
#define rvTrue  1

/* RvStatus */
#define rvOk    0

/* Defines the log filters to be used by the SDP Stack module. */

typedef enum {
        RVSDP_LOG_DEBUG_FILTER = 0x01,
        RVSDP_LOG_INFO_FILTER  = 0x02,
        RVSDP_LOG_WARN_FILTER  = 0x04,
        RVSDP_LOG_ERROR_FILTER = 0x08,
        RVSDP_LOG_EXCEP_FILTER = 0x10,
        RVSDP_LOG_SYNC_FILTER  = 0x20,
        RVSDP_LOG_ENTER_FILTER = 0x40,
        RVSDP_LOG_LEAVE_FILTER = 0x80
} RvSdpLogFilters;


/***************************************************************************
 * RvSdpStackPrintLogEntryEv
 * ------------------------------------------------------------------------
 * General:
 *          Notifies the application each time a line should be printed to
 *          the log. The application can decide wether to print the line
 *          to the screen, file or other output device.
 * Return Value:
 *          None.
 * ------------------------------------------------------------------------
 * Arguments:
 * Input:   filter -    The filter that this message is using (info, error..)
 *          formattedText - The text to be printed to the log. The text
 *                          is formatted as follows:
 *                          <filer> - <module> - <message>
 *                          for example:
 *                          "INFO  - STACK - Stack was constructed successfully"
 ***************************************************************************/
typedef void
        (RVCALLCONV * RvSdpStackPrintLogEntryEv)(
                                         IN void*           context,
                                         IN RvSdpLogFilters filter,
                                         IN const RvChar   *formattedText);


typedef struct
{
  RvBool                      disableSdpLogs;  /* the SDP logs are disabled */
  RvSdpStackPrintLogEntryEv   pfnPrintLogEntryEvHandler; /* application defined log
                                                            callback */
  void*                       logContext;   /* application defined log context,
                                            if set will be given as a parameter when
                                            calling pfnPrintLogEntryEvHandler */

  void*	                      logManagerPtr; /* log handle of other RV module,
                                                when using with SIP TK use
                                                RvSipStackGetLogHandle to fetch the SIP
                                                module log handle */
}RvSdpStackCfg;

/*
 *	SDP API Functions
 */

/****  --- Start of SDP Library Construction --- ****/
/*
    This section contains the functions used for library construction & destruction.
    There are also functions used to construct & destruct an SDP allocator but these
    can be used only if SDP library is used together with ADS module. Some of the
    functions defined in this chapter present only if compilation switch
    RV_SDP_ADS_IS_USED defined in the file rvsdpconfig.h is set.
*/

#if defined(RV_SDP_ADS_IS_USED)

/***************************************************************************
 * RvSdpAllocConstruct
 * ------------------------------------------------------------------------
 * General:
 *     Constructs an SDP allocator. An SDP allocator is used whenever
 *     you need to allocate space from RPOOL.
 *     This function is defined only if the RV_SDP_ADS_IS_USED
 *     compilation switch is enabled.
 * Return Value:
 *     Returns RV_OK if the function succeeds, or an error code if the
 *     function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *  hPool    - the pool which the SDP library will use.
 *  sdpAlloc - a pointer to the initialized allocator. Must point to valid memory.
 ***************************************************************************/
RVSDPCOREAPI RvStatus RvSdpAllocConstruct(
            HRPOOL hPool,
            RvAlloc * sdpAlloc);

 /***************************************************************************
 * RvSdpAllocDestruct
 * ------------------------------------------------------------------------
 * General:
 *          Destroys an SDP allocator. This function is called after an SDP
 *          message is destroyed.
 *          This function is defined only if the RV_SDP_ADS_IS_USED
 *          compilation switch is enabled.
 * Return Value:
 *          None.
 * ------------------------------------------------------------------------
 * Arguments:
 *  sdpAlloc - an sdp allocator of the message.
 ***************************************************************************/
RVSDPCOREAPI void RvSdpAllocDestruct(
            RvAlloc * sdpAlloc);

#endif /*RV_SDP_ADS_IS_USED*/

/***************************************************************************
 * RvSdpMgrConstructWithConfig
 * ------------------------------------------------------------------------
 * General:
 *      Constructs an SDP library and defines its logging behavior. This function
 *      has to be called prior to any other SDP API call.
 *      There are three different ways to define the logging behavior of the
 *      SDP library.
 *      1. pStackConfig is NULL. This is the default logging behavior.
 *         The SDP will create log file named as defined by RV_SDP_LOG_FILE_NAME
 *         (defined in rvsdpconfig.h). This file will be used for parsing error
 *         messages.
 *      2. pStackConfig is not NULL and pStackConfig->disableSdpLogs is set to RV_TRUE.
 *         The SDP module will not produce log messages.
 *      3. pStackConfig is not NULL and pStackConfig->logManagerPtr is set to log
 *         handle of some other RV module. In this case the log messages produced
 *         by SDP module will be printed by other RV module.
 *         When SDP is used with SIP TK use RvSipStackGetLogHandle SIP API function
 *         to get SIP module's log handle.
 *      4. pStackConfig is not NULL and pStackConfig->pfnPrintLogEntryEvHandler is set.
 *         In this case the application supplied callback (pfnPrintLogEntryEvHandler)
 *         will be called each time the log message has to be printed. The
 *         pfnPrintLogEntryEvHandler will be called with logContext as a function
 *         argument.
 *
 * Return Value:
 *      Returns RV_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *          pStackCfg - Structure containing SDP Stack configuration parameters.
 *          sizeOfCfg - The size of the configuration structure.
 ***************************************************************************/
RVSDPCOREAPI RvStatus RvSdpMgrConstructWithConfig(
                RvSdpStackCfg *pStackConfig,
                RvUint32     sizeOfCfg);

/***************************************************************************
 * RvSdpMgrConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs an SDP library with default logging behavior. This function
 *      has to be called prior to any other SDP API call. See
 *      RvSdpMgrConstructWithConfig for default logging behavior description.
 *
 * Return Value:
 *      Returns RV_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 ***************************************************************************/
RVSDPCOREAPI RvStatus RvSdpMgrConstruct(void);

/***************************************************************************
 * RvSdpMgrDestruct
 * ------------------------------------------------------------------------
 * General:
 *    Destruct the sdp library. No SDP API function can be called after the
 *    library was destructed.
 * Return Value:
 *    None.
 * ------------------------------------------------------------------------
 * Arguments:
 ***************************************************************************/
RVSDPCOREAPI void RvSdpMgrDestruct(void);

#define RvSdpConstruct(_pStackConfig) RvSdpMgrConstructWithConfig(_pStackConfig,0)
#define rvSdpConstruct(_pStackConfig) RvSdpMgrConstructWithConfig(_pStackConfig,0)
#define RvSdpDestruct RvSdpMgrDestruct
#define rvSdpDestruct RvSdpMgrDestruct

/****  --- End of SDP Library Construction --- ****/

/****  --- Start of Message Parse & Encode Functions --- ****/
/*
    This section contains the functions used for message parsing and encoding.
*/

/***************************************************************************
 * rvSdpMsgConstructParse
 * ------------------------------------------------------------------------
 * General:
 *      Parses an SDP text message and constructs an RvSdpMsg from the SDP text
 *      message.
 *
 * Return Value:
 *      A pointer to the new SDP message, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg to be constructed. Must point to valid memory.
 *      txt - contains a pointer to the beginning of the SDP text message.
 *      len - the length of the parsed SDP message.
 *      stat - the status of parsing termination. Will be RV_SDPPARSER_STOP_ERROR
 *             if there was a parsing error, otherwise another status specifying
 *             a legal character that caused the parser to stop. For more
 *             information on RvSdpParseStatus, see the Enumerations section.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsg* rvSdpMsgConstructParse(
            RvSdpMsg* msg,
            char* txt,
            int* len,
            RvSdpParseStatus* stat);

/***************************************************************************
 * rvSdpMsgConstructParseA
 * ------------------------------------------------------------------------
 * General:
 *      Parses an SDP text message and constructs an RvSdpMsg from the SDP text
 *      message using provided allocator.
 *
 * Return Value:
 *      A pointer to the new SDP message, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg to be constructed. Must point to valid memory.
 *      txt - contains a pointer to the beginning of the SDP text message.
 *      len - the length of the parsed SDP message.
 *      stat - the status of parsing termination. Will be RV_SDPPARSER_STOP_ERROR
 *             if there was a parsing error, otherwise another status specifying
 *             a legal character that caused the parser to stop. For more
 *             information on RvSdpParseStatus, see the Enumerations section.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsg* rvSdpMsgConstructParseA(
            RvSdpMsg* msg,
            char* txt,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);


/***************************************************************************
 * rvSdpMsgConstructParserData
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpParserData object. This parser data is used
 *      in further call to rvSdpMsgConstructParse2. The parser data object
 *      has to be destroyed with rvSdpMsgDestroyParserData API function.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      pD - a pointer to the RvSdpParserData object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
***************************************************************************/
RVSDPCOREAPI RvSdpParserData* rvSdpMsgConstructParserData(
            RvSdpParserData *pD,
            RvAlloc *a);

/***************************************************************************
 * rvSdpMsgDestroyParserData
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the RvSdpParserData object and frees up all internal memory.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      pD - pointer to a RvSdpParserData object to be destructed.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgDestroyParserData(
            RvSdpParserData *pD);

/***************************************************************************
 * rvSdpMsgConstructParse2
 * ------------------------------------------------------------------------
 * General:
 *      Parses an SDP text message and constructs an RvSdpMsg from the SDP text
 *      message. Collects parse warnings during the parse process.
 *
 * Return Value:
 *      A pointer to the new SDP message, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      pD - pointer to constructed RvSdpParserData object to be used during
 *           the parsing.
 *      msg - a pointer to the RvSdpMsg to be constructed. Must point to valid memory.
 *      txt - contains a pointer to the beginning of the SDP text message.
 *      len - the length of the parsed SDP message.
 *      stat - the status of parsing termination. Will be RV_SDPPARSER_STOP_ERROR
 *             if there was a parsing error, otherwise another status specifying
 *             a legal character that caused the parser to stop. For more
 *             information on RvSdpParseStatus, see the Enumerations section.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsg* rvSdpMsgConstructParse2(
            RvSdpParserData* pD,
            RvSdpMsg* msg,
            char* txt,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/*
 *	SDP Parse API warning functions
 */

#ifdef RV_SDP_PARSE_WARNINGS

/***************************************************************************
 * rvSdpParserGetFirstWarning
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first warning object in the warnings list of parser data.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpParserWarningData  object or the NULL pointer if
 *      there are no warnings in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      pD - a pointer to the RvSdpParserData object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpParserGetNextWarning calls.
 ***************************************************************************/
RVSDPCOREAPI RvSdpParserWarningData* rvSdpParserGetFirstWarning(
            RvSdpParserData* pD,
            RvSdpListIter* iter);

/***************************************************************************
 * rvSdpParserGetNextWarning
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next warning object in the warnings list of parser data.
 *
 * Return Value:
 *      Pointer to the RvSdpParserWarningData  object or the NULL pointer if
 *      there are no more warnings in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull
 *             rvSdpParserGetFirst/NextWarning call.
 ***************************************************************************/
RVSDPCOREAPI RvSdpParserWarningData* rvSdpParserGetNextWarning(
            RvSdpListIter* iter);

#endif /*RV_SDP_PARSE_WARNINGS*/

/***************************************************************************
 * rvSdpGetWarningText
 * ------------------------------------------------------------------------
 * General:
 *      Fetches the text from the parse waring.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      wd - a pointer to the valid RvSdpParserWarningData object.
 *      txt -  buffer to be used for the parse waring text.
 *      len - the  length of 'txt' buffer.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpGetWarningText(
            RvSdpParserWarningData* wd,
            char *txt,
            int len);


/*
 *	SDP encode
 */

/***************************************************************************
 * rvSdpMsgEncodeToBuf
 * ------------------------------------------------------------------------
 * General:
 *      Takes an RvSdpMsg as input and encodes it as text into a buffer (according
 *      to the SDP syntax).
 *
 * Return Value:
 *      Returns a pointer past the end of the encoding (buf+number of encoded bytes)
 *      or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the SDP message to be encoded.
 *      buf - a pointer to a buffer where the message will be encoded.
 *      len - the length of the buffer.
 *      stat - a pointer to a variable where the status of the encoding will be set.
 *             If encoding was successful, stat should be equal to rvOk in
 *             return. Any other value means that encoding failed.
 ***************************************************************************/
RVSDPCOREAPI char* rvSdpMsgEncodeToBuf(
            RvSdpMsg* msg,
            char* buf,
            int len,
            RvSdpStatus* stat);

/****  --- End of Message Parse & Encode Functions --- ****/
/****  --- Start of Message Functions --- ****/
/*
    This section contains the functions for operating on RvSdpMsg objects.
    Please note that no matter how the RvSdpMsg was constructed it has to be
    destructed with rvSdpMsgDestruct.
*/



/*
 *	Constructors
 */


/***************************************************************************
 * rvSdpMsgConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the instance of RvSdpMsg, initializes all internal fields,
 *      allocates memory for the strings buffer and pools of reusable objects.
 *
 * Return Value:
 *      Valid RvSdpMsg pointer on success or NULL on failure.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg instance to be constructed, if the value is
 *            NULL the instance will be allocated within the function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMsg* rvSdpMsgConstruct(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgConstruct(_msg) rvSdpMsgConstruct2((_msg),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the instance of RvSdpMsg, initializes all internal fields,
 *      allocates memory for the strings buffer and pools of reusable objects.
 *
 * Return Value:
 *      Valid RvSdpMsg pointer on success or NULL on failure.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg instance to be constructed, if the value is
 *            NULL the instance will be allocated within the function.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMsg* rvSdpMsgConstructA(
            RvSdpMsg* msg,
            RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgConstructA(_msg,_a) rvSdpMsgConstruct2((_msg),(_a))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the instance of RvSdpMsg from 'src' to 'dest'. The destination
 *      object will be constructed.
 *      If the destination object is NULL pointer the destination
 *      object will be allocated within the function.
 *
 * Return Value:
 *      A pointer to the input RvSdpMsg object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpMsg object or NULL.
 *      src - a pointer to the source object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMsg* rvSdpMsgConstructCopy(
            RvSdpMsg* dest,
            const RvSdpMsg* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgConstructCopy(_dest,_src) rvSdpMsgCopy2((_dest),(_src),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Copies the instance of RvSdpMsg from 'src' to 'dest'. The destination
 *      object will be constructed.
 *      If the destination object is NULL pointer the destination
 *      object will be allocated within the function.
 *
 * Return Value:
 *      A pointer to the input RvSdpMsg object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to valid RvSdpMsg object or NULL.
 *      src - a pointer to the source object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsg* rvSdpMsgConstructCopyA(
            RvSdpMsg* dest,
            const RvSdpMsg* src,
            RvAlloc* a);

/***************************************************************************
 * rvSdpMsgDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the RvSdpMsg object and frees up all internal memory.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to a RvSdpMsg object to be destructed.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgDestruct(
            RvSdpMsg* msg);

/***************************************************************************
 * rvSdpMsgCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the instance of RvSdpMsg from 'src' to 'dest'. The destination
 *      object must be constructed prior to function call.
 *
 * Return Value:
 *      A pointer to the input RvSdpMsg object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - pointer to constructed RvSdpMsg object.
 *      src - a pointer to the source object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMsg* rvSdpMsgCopy(
            RvSdpMsg* dest,
            const RvSdpMsg* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgCopy(_dest,_src) rvSdpMsgCopy2((_dest),(_src),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/* Version */

/***************************************************************************
 * rvSdpMsgGetVersion
 * ------------------------------------------------------------------------
 * General:
 *      Gets the version field value of SDP message.
 *
 * Return Value:
 *      Returns the version text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMsgGetVersion(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetVersion(_msg) RV_SDP_EMPTY_STRING((_msg)->iVersion.iVersionTxt)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetVersionN
 * ------------------------------------------------------------------------
 * General:
 *      Sets the version field of the SDP message.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      version - the new version value.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetVersionN(
            RvSdpMsg* msg,
            const char* version);

/***************************************************************************
 * rvSdpMsgDestroyVersion
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the version field of the SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgDestroyVersion(
            RvSdpMsg* msg);

/***************************************************************************
 * rvSdpMsgCopySdpVersion
 * ------------------------------------------------------------------------
 * General:
 *      Sets the version field of 'dstMsg' SDP message as of 'src'.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dstMsg - a pointer to the RvSdpMsg object.
 *      src - the source SDP version.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMsgCopySdpVersion(
            const RvSdpVersion* src,
            RvSdpMsg* dstMsg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgCopySdpVersion(_src,_dstMsg) \
            rvSdpMsgSetVersion2((_dstMsg),(_src)->iVersionTxt)
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgGetOrigin
 * ------------------------------------------------------------------------
 * General:
 *      Gets a pointer to the origin field.
 *
 * Return Value:
 *      A pointer to the origin field, or NULL if the origin field is not
 *      set in the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOrigin* rvSdpMsgGetOrigin(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetOrigin(_msg) \
			(RV_SDP_ORIGIN_IS_USED(&(_msg)->iOrigin) ? \
                                (RvSdpOrigin*) &(_msg)->iOrigin : NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetOrigin
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP origin field.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code
 *      if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      username - the user name.
 *      session_id - the session id.
 *      version - the version.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      address - the address, depending on the network type. For example, an
 *                IP address for an IP network, and so on.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetOrigin(
            RvSdpMsg* msg,
            const char* username,
            const char* session_id,
            const char* version,
            RvSdpNetType nettype,
            RvSdpAddrType addrtype,
            const char* address);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMsgSetBadSyntaxOrigin
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP origin field with a proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code
 *      if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      origin - The proprietary formatted origin to be set.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetBadSyntaxOrigin(
            RvSdpMsg* msg,
            const char* origin);
#endif

/* Name (Id) */

/***************************************************************************
 * rvSdpMsgGetSessionName
 * ------------------------------------------------------------------------
 * General:
 *      Gets the session name field value of SDP message.
 *
 * Return Value:
 *      Returns the session name text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMsgGetSessionName(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetSessionName(_msg) RV_SDP_EMPTY_STRING((_msg)->iSessId.iSessIdTxt)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetSessionName
 * ------------------------------------------------------------------------
 * General:
 *      Sets the session name field of the SDP message.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      session_name - the new session name value.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetSessionName(
            RvSdpMsg* msg,
            const char* session_name);

/***************************************************************************
 * rvSdpMsgDestroySessionName
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the session name field of the SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgDestroySessionName(
            RvSdpMsg* msg);

/***************************************************************************
 * rvSdpMsgCopySdpSessionId
 * ------------------------------------------------------------------------
 * General:
 *      Sets the session name field of 'dstMsg' SDP message as of 'src'.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dstMsg - a pointer to the RvSdpMsg object.
 *      src - the source SDP session name.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMsgCopySdpSessionId(
            const RvSdpSessId* src,
            RvSdpMsg* dstMsg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgCopySdpSessionId(_src,_dstMsg) \
            rvSdpMsgSetSessionName((_dstMsg),(_src)->iSessIdTxt)
#endif /*RV_SDP_USE_MACROS*/

/* Information */

/***************************************************************************
 * rvSdpMsgGetSessionInformation
 * ------------------------------------------------------------------------
 * General:
 *      Gets the session information field value of SDP message.
 *
 * Return Value:
 *      Returns the session information text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMsgGetSessionInformation(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetSessionInformation(_msg) \
			RV_SDP_EMPTY_STRING((_msg)->iCommonFields.iInfo.iInfoTxt)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetSessionInformation
 * ------------------------------------------------------------------------
 * General:
 *      Sets the information field of the SDP message.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      info - the new information value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetSessionInformation(
            RvSdpMsg* msg,
            const char* info);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgSetSessionInformation(_msg,_info) \
            rvSdpSetSdpInformation(&(_msg)->iCommonFields.iInfo,(_info),(_msg))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgDestroyInformation
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the information field of the SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgDestroyInformation(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgDestroyInformation(_msg) \
            rvSdpDestroySdpInformation(&(_msg)->iCommonFields.iInfo,(_msg))
#endif /*RV_SDP_USE_MACROS*/

/* URI */

/***************************************************************************
 * rvSdpMsgGetURI
 * ------------------------------------------------------------------------
 * General:
 *      Gets the URI field value of SDP message.
 *
 * Return Value:
 *      Returns the URI text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMsgGetURI(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetURI(_msg) RV_SDP_EMPTY_STRING((_msg)->iUri.iUriTxt)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetURI
 * ------------------------------------------------------------------------
 * General:
 *      Sets the URI field of the SDP message.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      uri - the new URI value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetURI(
            RvSdpMsg* msg,
            const char* uri);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgSetURI(_msg,_uri) rvSdpMsgSetURI2((_msg),(_uri),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgDestroyUri
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the URI field of the SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgDestroyUri(
            RvSdpMsg* msg);

/***************************************************************************
 * rvSdpMsgCopyURI
 * ------------------------------------------------------------------------
 * General:
 *      Sets the URI field of 'dstMsg' SDP message as of 'src'.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dstMsg - a pointer to the RvSdpMsg object.
 *      src - the source SDP URI.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMsgCopyURI(
            const RvSdpUri* src,
            RvSdpMsg* dstMsg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgCopyURI(_src,_dstMsg) \
            rvSdpMsgSetURI2((_dstMsg),(_src)->iUriTxt,\
                            RV_SDP_BAD_SYNTAX_PARAM((_src)->iUriBadSyntax))
#endif /*RV_SDP_USE_MACROS*/


#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpGetBadSyntaxUri
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted URI value
 *      or empty string ("") if the value is legal or is not set.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMsgGetBadSyntaxUri(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetBadSyntaxUri(_msg) RV_SDP_EMPTY_STRING((_msg)->iUri.iUriTxt)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgUriIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the URI field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpMsgUriIsBadSyntax(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgUriIsBadSyntax(_msg) (_msg)->iUri.iUriBadSyntax
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetBadSyntaxURI
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP uri field with a proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      uri - The proprietary formatted URI to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetBadSyntaxURI(
            RvSdpMsg* msg,
            const char* uri);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgSetBadSyntaxURI(_msg,_uri) rvSdpMsgSetURI2((_msg),(_uri),RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/* Email */

/***************************************************************************
 * rvSdpMsgGetNumOfEmail
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of elements in the session level emails list.
 *
 * Return Value:
 *      Size of emails list of SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfEmail(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfEmail(_msg) (_msg)->iEmailList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstEmail
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first email object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpEmail object or the NULL pointer if there are no
 *      emails defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpMsgGetFirstEmail(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstEmail(_msg,_iter) \
            (RvSdpEmail*) rvSdpListGetFirst(&(_msg)->iEmailList,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextEmail
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next email object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpEmail object or the NULL pointer if there is no
 *      more emails defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpMsgGetNextEmail(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextEmail(_iter) (RvSdpEmail*) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetEmail
* ------------------------------------------------------------------------
* General:
*      Gets an email object by index.
*
* Return Value:
*      The requested email object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpMsgGetNumOfEmails() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpMsgGetEmail(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetEmail(_msg,_index) (RvSdpEmail*) \
            rvSdpListGetByIndex(&(_msg)->iEmailList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddEmail
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new RvSdpEmail object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpEmail object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      email_addr - the new email address.
 *      string - Optional free text. Set to "" (string of zero (0) length)
 *               if not required.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpMsgAddEmail(
            RvSdpMsg* msg,
            const char* email_addr,
            const char* string);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddEmail(_msg,_email_addr,_string) \
            rvSdpMsgAddEmail2((_msg),(_email_addr),(_string),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddBadSyntaxEmail
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpEmail object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpEmail object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      text - the proprietary value of email field.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpMsgAddBadSyntaxEmail(
            RvSdpMsg* msg,
            const char* text);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddBadSyntaxEmail(_msg,_text) \
            rvSdpMsgAddEmail2((_msg),NULL,NULL,(_text))
#endif /*RV_SDP_USE_MACROS*/
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentEmail
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the email object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentEmail(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentEmail(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveEmail
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the email object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfEmails call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveEmail(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveEmail(_msg,_index) \
            rvSdpListRemoveByIndex(&(_msg)->iEmailList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearEmail
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all emails set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearEmail(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearEmail(_msg) rvSdpListClear(&(_msg)->iEmailList)
#endif /*RV_SDP_USE_MACROS*/


/* Phone */

/***************************************************************************
 * rvSdpMsgGetNumOfPhones
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of elements in the session level phones list.
 *
 * Return Value:
 *      Size of phones list of SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfPhones(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfPhones(_msg) (_msg)->iPhoneList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstPhone
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first phone object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpPhone object or the NULL pointer if there are no
 *      phones defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpMsgGetFirstPhone(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstPhone(_msg,_iter) (RvSdpPhone*) \
            rvSdpListGetFirst(&(_msg)->iPhoneList,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextPhone
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next phone object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpPhone object or the NULL pointer if there is no
 *      more phones defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpMsgGetNextPhone(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextPhone(_iter) (RvSdpPhone*) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetPhone
* ------------------------------------------------------------------------
* General:
*      Gets a phone object by index.
*
* Return Value:
*      The requested phone object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpMsgGetNumOfPhones() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpMsgGetPhone(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetPhone(_msg,_index) (RvSdpPhone*) \
            rvSdpListGetByIndex(&(_msg)->iPhoneList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddPhone
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new phone object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpPhone object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      phone - the new phone number.
 *      string - Optional free text. Set to "" (string of zero (0) length)
 *               if not required.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpMsgAddPhone(
            RvSdpMsg* msg,
            const char* phone,
            const char* string);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddPhone(_msg,_phone,_string) \
            rvSdpMsgAddPhone2((_msg),(_phone),(_string),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddBadSyntaxPhone
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpPhone object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpPhone object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of the phone field.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpMsgAddBadSyntaxPhone(
            RvSdpMsg* msg,
            const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddBadSyntaxPhone(_msg,_phone) \
            rvSdpMsgAddPhone2((_msg),NULL,NULL,(_phone))
#endif /*RV_SDP_USE_MACROS*/
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentPhone
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the phone object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentPhone(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentPhone(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemovePhone
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the phone object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfPhones call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemovePhone(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemovePhone(_msg,_index) \
            rvSdpListRemoveByIndex(&(_msg)->iPhoneList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearPhones
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all phones set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearPhones(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearPhones(_msg) rvSdpListClear(&(_msg)->iPhoneList)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNumOfConnections
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of elements in the session level connections list.
 *
 * Return Value:
 *      Size of connections list of SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfConnections(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfConnections(_msg) \
                        (_msg)->iCommonFields.iConnectionList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstConnection
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first connection object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpConnection object or the NULL pointer if there are no
 *      connections defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             rvSdpMsgGetNextConnection calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMsgGetFirstConnection(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstConnection(_msg,_iter) \
            rvSdpListGetFirst(&(_msg)->iCommonFields.iConnectionList,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextConnection
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next connection object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpConnection object or the NULL pointer if there are no
 *      more connections defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMsgGetFirstConnection/rvSdpMsgGetNextConnection function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMsgGetNextConnection(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextConnection(_iter) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetConnection
 * ------------------------------------------------------------------------
 * General:
 *      Gets a pointer to the first connection object set in the message.
 *
 * Return Value:
 *      A pointer to the connection field, or NULL if there are not connection
 *      fields set in the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMsgGetConnection(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetConnection(_msg) (RvSdpConnection*) \
            rvSdpListGetByIndex(&(_msg)->iCommonFields.iConnectionList,0)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetConnectionByIndex
 * ------------------------------------------------------------------------
 * General:
 *      Gets a connection object by index.
 *
 * Return Value:
 *      The requested connection object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfConnections call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMsgGetConnectionByIndex(
            const RvSdpMsg* descr,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetConnectionByIndex(_msg,_index) \
        (RvSdpConnection*) rvSdpListGetByIndex(&(_msg)->iCommonFields.iConnectionList,\
                                            (_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddConnection
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new connection object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpConnection object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP
 *             address for an IP network, and so on.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMsgAddConnection(
            RvSdpMsg* msg,
            RvSdpNetType net_type,
            RvSdpAddrType addr_type,
            const char* addr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddConnection(_msg,_type,_addr_type,_addr) \
            rvSdpAddConnection2((_msg),(_type),(_addr_type),(_addr),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetConnection
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new connection object at the session level.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code
 *      if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP
 *             address for an IP network, and so on.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetConnection(
            RvSdpMsg* msg,
            RvSdpNetType net_type,
            RvSdpAddrType addr_type,
            const char* addr);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMsgSetBadSyntaxConnection
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP connection field with a proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code
 *      if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - The proprietary formatted connection to be set.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetBadSyntaxConnection (
            RvSdpMsg* msg,
            const char* badSyn);
#endif /* RV_SDP_CHECK_BAD_SYNTAX */

/***************************************************************************
 * rvSdpMsgRemoveCurrentConnection
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the connection object pointed by list iterator.
 *       The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentConnection(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentConnection(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgRemoveConnection
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the connection object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfConnections call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveConnection(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveConnection(_msg,_index) \
            rvSdpListRemoveByIndex(&(_msg)->iCommonFields.iConnectionList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearConnection
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all connections set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearConnection(
            RvSdpMsg* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearConnection(_msg) \
            rvSdpListClear(&(_msg)->iCommonFields.iConnectionList)
#endif /*RV_SDP_USE_MACROS*/

/* Bandwidth */

/***************************************************************************
 * rvSdpMsgGetNumOfBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of elements in the session level bandwiths list.
 *
 * Return Value:
 *      Size of bandwiths list of SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfBandwidth(
            const RvSdpMsg* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfBandwidth(_msg) \
            (_msg)->iCommonFields.iBandwidthList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first bandwidth object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpBandwidth object or the NULL pointer if there are no
 *      bandwidths defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMsgGetFirstBandwidth(
            RvSdpMsg* descr,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstBandwidth(_msg,_iter) \
            rvSdpListGetFirst(&(_msg)->iCommonFields.iBandwidthList,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next bandwidth object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpBandwidth object or the NULL pointer if there are no
 *      more bandwidths defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMsgGetNextBandwidth(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextBandwidth(_iter) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Gets a pointer to the first bandwidth object set in the message.
 *
 * Return Value:
 *      A pointer to the bandwidth field, or NULL if there are no bandwidth fields
 *      set in the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMsgGetBandwidth(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetBandwidth(_msg) (RvSdpBandwidth*) \
            rvSdpListGetByIndex(&(_msg)->iCommonFields.iBandwidthList,0)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetBandwidthByIndex
 * ------------------------------------------------------------------------
 * General:
 *      Gets a bandwidth object by index.
 *
 * Return Value:
 *      The requested bandwidth object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements in
 *              the list is retrieved by correspondent rvSdp...GetNum..() call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMsgGetBandwidthByIndex(
            const RvSdpMsg* descr,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetBandwidthByIndex(_msg,_index) \
            (RvSdpBandwidth*) rvSdpListGetByIndex(&(_msg)->iCommonFields.iBandwidthList,\
                                                  (_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new bandwidth object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpBandwidth object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      bwtype - bandwidth type, such as Conference Total (CT) or Application-Specific
 *               Maximum (AS).
 *      b - Bandwidth value in kilobits per second (kbps).
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMsgAddBandwidth(
            RvSdpMsg* msg,
            const char* bwtype,
            int b);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddBandwidth(_msg,_bwtype,_b) \
            rvSdpAddBandwidth2((_msg),(_bwtype),(_b),NULL);
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgSetBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new bandwidth object at the session level.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      bwtype - bandwidth type, such as Conference Total (CT) or Application-Specific
 *               Maximum (AS).
 *      b - bandwidth value in kilobits per second (kbps).
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetBandwidth(
            RvSdpMsg* msg,
            const char* bwtype,
            int b);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMsgSetBadSyntaxBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP bandwidth field with a proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - The proprietary formatted bandwidth to be set.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetBadSyntaxBandwidth(
            RvSdpMsg* msg,
            const char* badSyn);
#endif


/***************************************************************************
 * rvSdpMsgRemoveCurrentBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the bandwidth object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentBandwidth(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentBandwidth(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgRemoveBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the bandwidth object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfBandwidths call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveBandwidth(
            RvSdpMsg* descr,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveBandwidth(_msg,_index) \
            rvSdpListRemoveByIndex(&(_msg)->iCommonFields.iBandwidthList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all bandwidths set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearBandwidth(
            RvSdpMsg* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearBandwidth(_msg) \
            rvSdpListClear(&(_msg)->iCommonFields.iBandwidthList)
#endif /*RV_SDP_USE_MACROS*/

/* Session time */
/***************************************************************************
 * rvSdpMsgGetNumOfSessionTime
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of elements in the session level times list.
 *
 * Return Value:
 *      Size of session times list of SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfSessionTime(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfSessionTime(_msg) (_msg)->iSessionTimeList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstSessionTime
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first session time object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpSessionTime object or the NULL pointer if there are
 *      no session times defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpMsgGetFirstSessionTime(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstSessionTime(_msg,_iter) \
            (RvSdpSessionTime*) rvSdpListGetFirst(&(_msg)->iSessionTimeList,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextSessionTime
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next session time object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpSessionTime object or the NULL pointer if there is no
 *      more session times defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpMsgGetNextSessionTime(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextSessionTime(_iter) (RvSdpSessionTime*) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetSessionTime
* ------------------------------------------------------------------------
* General:
*      Gets a session time object by index.
*
* Return Value:
*      The requested session time object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpMsgGetNumOfSessionTime() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpMsgGetSessionTime(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetSessionTime(_msg,_index) (RvSdpSessionTime*) \
            rvSdpListGetByIndex(&(_msg)->iSessionTimeList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddSessionTime
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new session time object.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpSessionTime object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      start - the start time of the SDP session.
 *      stop - the end time of the SDP session.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpMsgAddSessionTime(
            RvSdpMsg* msg,
            RvUint32 start,
            RvUint32 stop);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddSessionTime(_msg,_start,_stop) \
			rvSdpMsgAddSessionTime2((_msg),(_start),(_stop),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddBadSyntaxSessionTime
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpSessionTime object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpSessionTime object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of session time field.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpMsgAddBadSyntaxSessionTime(
            RvSdpMsg* msg,
            const char *badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddBadSyntaxSessionTime(_msg,_session) \
            rvSdpMsgAddSessionTime2((_msg),0,0,(_session))
#endif /*RV_SDP_USE_MACROS*/
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentSessionTime
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the session time object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentSessionTime(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentSessionTime(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveSessionTime
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the session time object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfSessionTimes call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveSessionTime(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveSessionTime(_msg,_index) \
            rvSdpListRemoveByIndex(&(_msg)->iSessionTimeList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearSessionTime
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all session time objects set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearSessionTime(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearSessionTime(_msg) rvSdpListClear(&(_msg)->iSessionTimeList)
#endif /*RV_SDP_USE_MACROS*/

/* Time Zone Adjustments */

/***************************************************************************
 * rvSdpMsgTZACopy
 * ------------------------------------------------------------------------
 * General:
 *      Sets the time zone adjustment field of 'dest' SDP message as the time
 *      zone adjustments of 'src' SDP message.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to the destination RvSdpMsg object.
 *      src - a pointer to the source RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgTZACopy(
            RvSdpMsg* dest,
            const RvSdpMsg* src);

/***************************************************************************
 * rvSdpMsgTZADestroy
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the time zone adjustment field of SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgTZADestroy(
            RvSdpMsg* msg);

/***************************************************************************
 * rvSdpMsgTZAIsUsed
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the time zone adjustment field ('z=') of SDP message is set.
 *
 * Return Value:
 *      RV_TRUE if there are time zone adjustments set or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI RvBool rvSdpMsgTZAIsUsed(
            RvSdpMsg* msg);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMsgGetBadSyntaxZoneAdjustment
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted time zone adjustment field of SDP message
 *      or empty string ("") if the value is either legal or is not set.
 *
 * Return Value:
 *      Gets the proprietary formatted time zone adjustment field of SDP message
 *      or empty string ("") if the value is either legal or is not set.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMsgGetBadSyntaxZoneAdjustment(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetBadSyntaxZoneAdjustment(_msg) \
			RV_SDP_EMPTY_STRING((_msg)->iTZA.iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgIsBadSyntaxZoneAdjustment
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the time zone adjustment field of SDP message
 *      is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpMsgIsBadSyntaxZoneAdjustment(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgIsBadSyntaxZoneAdjustment(_msg) ((_msg)->iTZA.iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgSetBadSyntaxZoneAdjustment
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP time zone adjustment field with a proprietary formatted value.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - The proprietary formatted value to be set.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetBadSyntaxZoneAdjustment(
            RvSdpMsg* msg,
            const char* badSyn);

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/***************************************************************************
 * rvSdpMsgGetNumOfZoneAdjustments
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of elements in the session level time zone adjustments list.
 *
 * Return Value:
 *      Size of time zone adjustments list of SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfZoneAdjustments(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfZoneAdjustments(_msg) (_msg)->iTZA.iTimeZoneList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstZoneAdjustment
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first time zone adjustment  object defined in the SDP
 *      message. Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpZoneAdjustment object or the NULL pointer if there
 *      are no time zone adjustments defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpMsgGetFirstZoneAdjustment(
            RvSdpMsg* msg,
            RvSdpListIter *iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstZoneAdjustment(_msg,_iter) \
                (RvSdpTimeZoneAdjust*) rvSdpListGetFirst(&(_msg)->iTZA.iTimeZoneList,\
                                                         (_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextZoneAdjustment
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next time zone adjustment object defined in the SDP message.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpTimeZoneAdjust object or the NULL pointer if there is no
 *      more time zone adjustments defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpMsgGetNextZoneAdjustment(
            RvSdpListIter *iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextZoneAdjustment(_iter) \
            (RvSdpTimeZoneAdjust*) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetZoneAdjustment
* ------------------------------------------------------------------------
* General:
*      Gets a time zone adjustment object by index.
*
* Return Value:
*      The requested time zone adjustment object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMsgGetNumOfZoneAdjustments call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpMsgGetZoneAdjustment(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetZoneAdjustment(_msg,_index) \
            (RvSdpTimeZoneAdjust*) rvSdpListGetByIndex(&(_msg)->iTZA.iTimeZoneList,\
                                                       (_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgTimeAddZoneAdjustment
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new time zone adjustment to the list specified in 'z=' line.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpTimeZoneAdjust object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      time - the time at which the adjustment is applied.
 *      adjust_time - the time shift length.
 *      units - The units of the time shift.
 ***************************************************************************/
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpMsgTimeAddZoneAdjustment(
            RvSdpMsg* msg,
            RvUint32 time,
            RvInt32 adjust_time,
            RvSdpTimeUnit units);


/***************************************************************************
 * rvSdpMsgRemoveCurrentZoneAdjustment
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the time zone adjustment object pointed by list
 *      iterator. The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentZoneAdjustment(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentZoneAdjustment(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveTimeZoneAdjust
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the time zone adjustment object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfZoneAdjustments call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveTimeZoneAdjust(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveTimeZoneAdjust(_msg,_index) \
            rvSdpListRemoveByIndex(&(_msg)->iTZA.iTimeZoneList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearZoneAdjustment
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all time zone adjustments set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearZoneAdjustment(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearZoneAdjustment(_msg) rvSdpListClear(&(_msg)->iTZA.iTimeZoneList)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetKey
 * ------------------------------------------------------------------------
 * General:
 *      Gets a pointer to the key field.
 *
 * Return Value:
 *      A pointer to the key field, or NULL if the key field is not
 *      set in the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpMsgGetKey(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetKey(_msg) \
			(RV_SDP_KEY_IS_USED(&(_msg)->iCommonFields.iKey) ? \
					((RvSdpKey*) &(_msg)->iCommonFields.iKey) : NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetKey
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP key field.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      em - the key encryption method.
 *      key - the key value.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetKey(
            RvSdpMsg* msg,
            RvSdpEncrMethod em,
            const char* key);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMsgSetBadSyntaxKey
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP key field with a proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - The proprietary formatted key to be set.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetBadSyntaxKey(
            RvSdpMsg* msg,
            const char* badSyn);
#endif

/* Attribute */

/***************************************************************************
 * rvSdpMsgGetNumOfAttr
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of elements in the session level attributes list (generic
 *      attributes). Special attributes (RTP map, connection mode, key management,
 *      crypto, frame-rate and fmtp)
 *      are not counted among the attributes treated by this function. Use
 *      rvSdpMsgGetNumOfAttr2 function for all attributes number.
 *
 * Return Value:
 *      Number of generic attributes (except for special) of the message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfAttr(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfAttr(_msg) \
			rvSdpGetNumOfSpecialAttr(&(_msg)->iCommonFields,SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstAttribute
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first generic attribute object defined in the SDP message.
 *      Also sets the list iterator for the further use. Use rvSdpMsgGetFirstAttribute2
 *      for iterating on all (generic and special) attributes.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there are no
 *      generic attributes defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMsgGetFirstAttribute(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstAttribute(_msg,_iter) \
			(RvSdpAttribute*)rvSdpGetFirstSpecialAttr(&(_msg)->iCommonFields,\
                                                      (_iter),SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextAttribute
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next generic attribute object defined in the SDP message.
 *      The 'next' object is defined based on the list iterator state. Use
 *      rvSdpMsgGetNextAttribute2 for iterating on all (generic and special) attributes.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there is no
 *      more generic attributes defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMsgGetNextAttribute(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextAttribute(_iter) \
			(RvSdpAttribute*)rvSdpGetNextSpecialAttr((_iter),SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetAttribute
* ------------------------------------------------------------------------
* General:
*      Gets a generic attribute object by index. Use rvSdpMsgGetAttribute2 to get
*      attribute of all (generic and special) attributes.
*
* Return Value:
*      The requested generic attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent rvSdpMsgGetNumOfAttr() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMsgGetAttribute(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetAttribute(_msg,_index)   \
			rvSdpGetSpecialAttr((RvSdpCommonFields*)&(_msg)->iCommonFields,\
                                                 (_index),SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddAttr
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new generic attribute object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpAttribute object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      name - the generic attribute name.
 *      value - the generic attribute value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMsgAddAttr(
            RvSdpMsg* msg,
            const char* name,
            const char* value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddAttr(_msg,_name,_value) \
            rvSdpAddAttr2((_msg),&(_msg)->iCommonFields,NULL,(_name),(_value))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentAttribute
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the attribute object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentAttribute(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentAttribute(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgRemoveAttribute
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the generic attribute object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfAttr call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveAttribute(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveAttribute(_msg,_index) \
			rvSdpRemoveSpecialAttr(&(_msg)->iCommonFields,(_index),SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearAttr
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all generic attributes set in SDP message.
 *      The special attributes will not be removed. Use rvSdpMsgClearAttr2 to
 *      remove all (generic and special) attributes.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearAttr(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearAttr(_msg) \
			rvSdpClearSpecialAttr(&(_msg)->iCommonFields,SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNumOfAttr2
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of elements in the session level attributes list.
 *      Special attributes are counted as well as generic.
 *
 * Return Value:
 *      Size of attributes list of SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfAttr2(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfAttr2(_msg) (_msg)->iCommonFields.iAttrList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstAttribute2
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first attribute object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there are no
 *      attributes defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMsgGetFirstAttribute2(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstAttribute2(_msg,_iter) \
            rvSdpListGetFirst(&(_msg)->iCommonFields.iAttrList,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextAttribute2
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next attribute object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there is no
 *      more attributes defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMsgGetNextAttribute2(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextAttribute2(_iter) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetAttribute2
* ------------------------------------------------------------------------
* General:
*      Gets an attribute object by index.
*
* Return Value:
*      The requested attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpMsgGetNumOfAttr2() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMsgGetAttribute2(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetAttribute2(_msg,_index) \
            (RvSdpAttribute*) rvSdpListGetByIndex(&(_msg)->iCommonFields.iAttrList,\
                                                  (_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentAttribute2
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the attribute object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentAttribute2(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentAttribute2(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveAttribute2
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the attribute object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfAttr2 call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveAttribute2(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveAttribute2(_msg,_index) \
            rvSdpListRemoveByIndex(&(_msg)->iCommonFields.iAttrList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearAttr2
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all (generic and special) attributes set in
 *      SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearAttr2(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearAttr2(_msg) rvSdpListClear(&(_msg)->iCommonFields.iAttrList)
#endif /*RV_SDP_USE_MACROS*/

/* Rtp map */

/***************************************************************************
 * rvSdpMsgGetNumOfRtpMaps
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of RTP map special attributes set in SDP message context.
 *
 * Return Value:
 *      Number of RTP map special attributes set in SDP message context.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfRtpMap(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfRtpMap(_msg) \
            rvSdpGetNumOfSpecialAttr(&(_msg)->iCommonFields,SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first RTP map object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpRtpMap object or the NULL pointer if there are no
 *      RTP maps defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMsgGetFirstRtpMap(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstRtpMap(_msg,_iter) \
            (RvSdpRtpMap*)rvSdpGetFirstSpecialAttr(&(_msg)->iCommonFields,\
                                                   (_iter),SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next RTP map object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpRtpMap object or the NULL pointer if there are no
 *      more RTP maps defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMsgGetNextRtpMap(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextRtpMap(_iter) \
            (RvSdpRtpMap*)rvSdpGetNextSpecialAttr((_iter),SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetRtpMap
* ------------------------------------------------------------------------
* General:
*      Gets an RTP map special attribute object by index.
*
* Return Value:
*      The requested RTP map attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpMsgGetNumOfRtpMaps() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMsgGetRtpMap(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetRtpMap(_msg,_index) \
            (RvSdpRtpMap*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&(_msg)->iCommonFields,\
                                              (_index),SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Adds a new RTP map to the session-level RTP map list.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpRtpMap object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      payload - an RTP dynamic payload number.
 *      encoding_name - the name of the codec.
 *      rate - the clock rate.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMsgAddRtpMap(
            RvSdpMsg* msg,
            int payload,
            const char* encoding_name,
            int rate);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddRtpMap(_msg,_payload,_encoding_name,_rate) \
            rvSdpAddRtpMap2((_msg),&(_msg)->iCommonFields,(_payload),\
                            (_encoding_name),(_rate),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddBadSyntaxRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpRtpMap object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpRtpMap object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of RTP map special attribute.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMsgAddBadSyntaxRtpMap(
            RvSdpMsg* msg,
            const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddBadSyntaxRtpMap(_msg,_rtpmap) \
            rvSdpAddRtpMap2((_msg),&(_msg)->iCommonFields,0,NULL,0,(_rtpmap))
#endif /*RV_SDP_USE_MACROS*/
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the RTP map special attribute object pointed
 *      by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentRtpMap(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentRtpMap(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgRemoveRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the RTP map special attribute object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfRtpMaps call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveRtpMap(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveRtpMap(_msg,_index) rvSdpRemoveSpecialAttr(&(_msg)->iCommonFields,\
                                                                 (_index),\
                                                                 SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all RTP map special attributes set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearRtpMap(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearRtpMap(_msg)  \
            rvSdpClearSpecialAttr(&(_msg)->iCommonFields,SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

#ifdef RV_SDP_KEY_MGMT_ATTR

/* Connection Mode */

/***************************************************************************
 * rvSdpMsgGetConnectionMode
 * ------------------------------------------------------------------------
 * General:
 *      Gets the connection mode of the SDP message or RV_SDPCONNECTMODE_NOTSET
 *      if the correspondent attribute is not set.
 *
 * Return Value:
 *      Returns the connection mode.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnectionMode rvSdpMsgGetConnectionMode(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetConnectionMode(_msg) rvSdpGetConnectionMode(&(_msg)->iCommonFields)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgSetConnectionMode
 * ------------------------------------------------------------------------
 * General:
 *      Sets/modifies the connection mode of the SDP message.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      mode - the new value of connection mode.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMsgSetConnectionMode(
            RvSdpMsg* msg,
            RvSdpConnectionMode mode);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgSetConnectionMode(_msg,_mode) \
            rvSdpSetConnectionMode((_msg),&(_msg)->iCommonFields,(_mode))
#endif /*RV_SDP_USE_MACROS*/

/* Key Mgmt Attribute */

/***************************************************************************
 * rvSdpMsgGetNumOfKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of key mgmt special attributes set in SDP message context.
 *
 * Return Value:
 *      Number of key mgmt special attributes set in SDP message context.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfKeyMgmt(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfKeyMgmt(_msg) \
            rvSdpGetNumOfSpecialAttr(&(_msg)->iCommonFields,\
                                     SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first key management object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpKeyMgmt object or the NULL pointer if there are no
 *      key management special attributes defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMsgGetFirstKeyMgmt(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstKeyMgmt(_msg,_iter) \
            (RvSdpKeyMgmtAttr*)rvSdpGetFirstSpecialAttr(&(_msg)->iCommonFields,\
                                                        (_iter),SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next key management special attribute object defined in the SDP
 *      message. The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpKeyMgmtAttr object or the NULL pointer if there is no
 *      more key management special attributes defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMsgGetNextKeyMgmt(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextKeyMgmt(_iter) \
            (RvSdpKeyMgmtAttr*)rvSdpGetNextSpecialAttr((_iter),SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetKeyMgmt
* ------------------------------------------------------------------------
* General:
*      Gets a key management attribute object by index.
*
* Return Value:
*      The requested key management attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpMsgGetNumOfKeyMgmt() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMsgGetKeyMgmt(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetKeyMgmt(_msg,_index) \
      (RvSdpKeyMgmtAttr*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&(_msg)->iCommonFields,\
                                                   (_index),SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new key management special attribute at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpKeyMgmtAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      prtclId - the protocol ID.
 *      keyData - the encryption key data.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMsgAddKeyMgmt(
            RvSdpMsg* msg,
            const char* prtclId,
            const char* keyData);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddKeyMgmt(_msg,_prtclId,_keyData) \
            rvSdpAddKeyMgmt2((_msg),&(_msg)->iCommonFields,(_prtclId),\
                            (_keyData),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddBadSyntaxKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpKeyMgmtAttr object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpKeyMgmtAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of KeyMgmt special attribute field.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMsgAddBadSyntaxKeyMgmt(
            RvSdpMsg* msg,
            const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddBadSyntaxKeyMgmt(_msg,_KeyMgmt) \
            rvSdpAddKeyMgmt2((_msg),&(_msg)->iCommonFields,NULL,NULL,(_KeyMgmt))
#endif /*RV_SDP_USE_MACROS*/
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the key-mgmt special attribute object pointed
 *      by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentKeyMgmt(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentKeyMgmt(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the key-mgmt special attribute object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfKeyMgmt call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveKeyMgmt(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveKeyMgmt(_msg,_index) \
            rvSdpRemoveSpecialAttr(&(_msg)->iCommonFields,\
                                   (_index),SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all key-mgmt special attributes set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearKeyMgmt(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearKeyMgmt(_msg)  \
            rvSdpClearSpecialAttr(&(_msg)->iCommonFields,SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_KEY_MGMT_ATTR*/


#ifdef RV_SDP_CRYPTO_ATTR

/* Crypto Attr */

/***************************************************************************
 * rvSdpMsgGetNumOfCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of crypto special attributes set in SDP message context.
 *
 * Return Value:
 *      Number of crypto special attributes set in SDP message context.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfCrypto(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfCrypto(_msg) \
            rvSdpGetNumOfSpecialAttr(&(_msg)->iCommonFields,SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first crypto attribute object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpCryptoAttr object or the NULL pointer if there are no
 *      crypto special attributes defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMsgGetFirstCrypto(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstCrypto(_msg,_iter) \
            (RvSdpCryptoAttr*)rvSdpGetFirstSpecialAttr(&(_msg)->iCommonFields,\
                                                       (_iter),SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next crypto attribute object defined in the SDP message.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpCryptoAttr object or the NULL pointer if there are no
 *      more crypto special attributes defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMsgGetNextCrypto(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextCrypto(_iter) \
            (RvSdpCryptoAttr*)rvSdpGetNextSpecialAttr((_iter),SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetCrypto
* ------------------------------------------------------------------------
* General:
*      Gets a crypto attribute object by index.
*
* Return Value:
*      The requested crypto attribute object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpMsgGetNumOfCrypto() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMsgGetCrypto(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetCrypto(_msg,_index) \
            (RvSdpCryptoAttr*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&(_msg)->iCommonFields,\
                                                  (_index),SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new crypto special attribute object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpCryptoAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      tag - the crypto attribute tag number.
 *      suite - the crypto attribute suite value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMsgAddCrypto(
            RvSdpMsg* msg,
            RvUint tag,
            const char* suite);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddCrypto(_msg,_tag,_suite) \
            rvSdpAddCrypto2((_msg),&(_msg)->iCommonFields,(_tag),(_suite),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddBadSyntaxCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpCryptoAttr object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpCryptoAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of crypto special attribute field.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMsgAddBadSyntaxCrypto(
            RvSdpMsg* msg,
            const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddBadSyntaxCrypto(_msg,_crypto) \
            rvSdpAddCrypto2((_msg),&(_msg)->iCommonFields,0,NULL,(_crypto))
#endif /*RV_SDP_USE_MACROS*/

#endif /*(RV_SDP_CHECK_BAD_SYNTAX)*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the crypto special attribute object pointed
 *      by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentCrypto(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentCrypto(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMsgRemoveCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the crypto special attribute object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfCrypto() call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCrypto(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCrypto(_msg,_index) \
            rvSdpRemoveSpecialAttr(&(_msg)->iCommonFields,(_index),SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all crypto special attributes set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearCrypto(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearCrypto(_msg)  \
            rvSdpClearSpecialAttr(&(_msg)->iCommonFields,SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_CRYPTO_ATTR*/

/* Media descriptors */

/***************************************************************************
 * rvSdpMsgGetNumOfMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptors set in SDP message.
 *
 * Return Value:
 *      Number of media descriptors set in SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfMediaDescr(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfMediaDescr(_msg) (_msg)->iMediaDescriptors.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first media descriptor object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpMediaDescr object or the NULL pointer if there are no
 *      media descriptors defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMsgGetFirstMediaDescr(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstMediaDescr(_msg,_iter) \
            (RvSdpMediaDescr*) rvSdpListGetFirst(&(_msg)->iMediaDescriptors,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next media descriptor object defined in the SDP message.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpMediaDescr object or the NULL pointer if there is no
 *      more media descriptors defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMsgGetNextMediaDescr(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextMediaDescr(_iter) (RvSdpMediaDescr*) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetMediaDescr
* ------------------------------------------------------------------------
* General:
*      Gets a media descriptor object by index.
*
* Return Value:
*      The requested media descriptor object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMsgGetNumOfMediaDescr call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMsgGetMediaDescr(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetMediaDescr(_msg,_index) (RvSdpMediaDescr*) \
            rvSdpListGetByIndex(&(_msg)->iMediaDescriptors,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new media descriptor object to the SDP message.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpMediaDescr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      media_type - the type of media (audio, video, data).
 *      port - the port number.
 *      protocol - the protocol used to transport the media, such as RTP/RTCP.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMsgAddMediaDescr(
            RvSdpMsg* msg,
            RvSdpMediaType media_type,
            RvUint16 port,
            RvSdpProtocol protocol);

/***************************************************************************
 * rvSdpMsgInsertMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new media descriptor object to the SDP message as the copy
 *      of 'descr'.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object where new media descriptor will
 *            be added.
 *      descr - the new media descriptor will be copied from this one.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgInsertMediaDescr(
            RvSdpMsg* msg,
            RvSdpMediaDescr* descr);

/***************************************************************************
 * rvSdpMsgAddBadSyntaxMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpMediaDescr object at the session level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpMediaDescr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      badSyn - the proprietary value of media descriptor field.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMsgAddBadSyntaxMediaDescr(
            RvSdpMsg* msg,
            const char *badSyn);
#endif

/***************************************************************************
 * rvSdpMsgRemoveCurrentMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the media descriptor object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentMediaDescr(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentMediaDescr(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the media descriptor object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMsgGetNumOfMediaDescr call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveMediaDescr(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveMediaDescr(_msg,_index) \
            rvSdpListRemoveByIndex(&(_msg)->iMediaDescriptors,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearMediaDescr
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all media descriptors set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearMediaDescr(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearMediaDescr(_msg) rvSdpListClear(&(_msg)->iMediaDescriptors)
#endif /*RV_SDP_USE_MACROS*/

/* Other */

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMsgGetNumOfOther
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of proprietary tag objects set in SDP message.
 *
 * Return Value:
 *      Number of proprietary tag objects set in SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfOther(
            const RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNumOfOther(_msg) (_msg)->iCommonFields.iOtherList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetFirstOther
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first proprietary-tag object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpOther object or the NULL pointer if there are no
 *      'other' objects defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpMsgGetFirstOther(
            RvSdpMsg* msg,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetFirstOther(_msg,_iter) \
            (RvSdpOther*)(rvSdpListGetFirst(&(_msg)->iCommonFields.iOtherList,(_iter)))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgGetNextOther
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next 'other' object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpOther object or the NULL pointer if there is no
 *      more 'other' objects defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpMsgGetNextOther(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetNextOther(_iter) (RvSdpOther*)rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMsgGetOther
* ------------------------------------------------------------------------
* General:
*      Gets a RvSdpOther object by index.
*
* Return Value:
*      The requested RvSdpOther object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMsgGetNumOfOther call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpMsgGetOther(
            const RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgGetOther(_msg,_index) \
        (RvSdpOther*)rvSdpListGetByIndex(&(_msg)->iCommonFields.iOtherList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgAddOther
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary tag SDP object to the session level list of
 *      RvSdpOther objects.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpOther object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      tag - the tag letter of the line.
 *      value - the proprietary text of the line.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpMsgAddOther(
            RvSdpMsg* msg,
            const char tag,
            const char *value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgAddOther(_msg,_tag,_value) \
            rvSdpAddOther((_msg),&(_msg)->iCommonFields,(_tag),(_value))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveCurrentOther
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the 'other' object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveCurrentOther(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveCurrentOther(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgRemoveOther
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the 'other' object by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent rvSdpMsgGetNumOfOther call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgRemoveOther(
            RvSdpMsg* msg,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgRemoveOther(_msg,_index) \
        rvSdpListRemoveByIndex(&(_msg)->iCommonFields.iOtherList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMsgClearOther
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all RvSdpOther objects set in SDP message.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMsgClearOther(
            RvSdpMsg* msg);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMsgClearOther(_msg) rvSdpListClear(&(_msg)->iCommonFields.iOtherList)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/


/* Bad Syntax iterator */

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

typedef void RvSdpBadSyntax;

/***************************************************************************
 * rvSdpMsgGetNumOfBadSyntax2
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of proprietary formatted elements (of all types) in the
 *      SDP message including bad syntax elements set in the context of media
 *      descriptors.
 *
 * Return Value:
 *      Number of bad syntax objects of the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfBadSyntax2(
            const RvSdpMsg* msg);

 /***************************************************************************
 * rvSdpMsgGetNumOfBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of proprietary formatted elements (of all types) in the
 *      SDP message not-including bad syntax elements set in the context of media
 *      descriptors. If some of RvSdpMediaDescr object is proprietary formatted
 *      it is counted but none of bad syntax objects owned by this media descriptor
 *      is counted.
 *
 * Return Value:
 *      Number of bad syntax objects of the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI RvSize_t rvSdpMsgGetNumOfBadSyntax(
            const RvSdpMsg* msg);

/***************************************************************************
 * rvSdpMsgGetFirstBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first bad syntax object defined in the SDP message. Also sets
 *      the list iterator for the further use.
 *      The bad syntax objects owned by media descriptors are not treated by
 *      the function.
 *
 * Return Value:
 *      Pointer to the RvSdpBadSyntax object or the NULL pointer if there not
 *      bad syntax objects defined for the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - a pointer to the RvSdpMsg object.
 *      iter - pointer to RvSdpListIter to be used for further
 *             GetNext calls.
 ***************************************************************************/
RVSDPCOREAPI RvSdpBadSyntax* rvSdpMsgGetFirstBadSyntax(
            RvSdpMsg* msg,
            RvSdpLineObjIter* iter);

/***************************************************************************
 * rvSdpMsgGetNextBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next bad syntax object defined in the SDP message. The 'next'
 *      object is defined based on the list iterator state.
 *      The bad syntax objects owned by media descriptors are not treated by
 *      the function.
 *
 * Return Value:
 *      Pointer to the RvSdpBadSyntax object or the NULL pointer if there is no
 *      more bad syntax objects defined in the SDP message.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
RVSDPCOREAPI RvSdpBadSyntax* rvSdpMsgGetNextBadSyntax(
            RvSdpLineObjIter* iter);

/***************************************************************************
* rvSdpMsgGetBadSyntax
* ------------------------------------------------------------------------
* General:
*      Gets a bad syntax object by index.
*      The bad syntax objects owned by media descriptors are not treated by
*      the function.
*
* Return Value:
*      The requested RvSdpBadSyntax object.
* ------------------------------------------------------------------------
* Arguments:
*      msg - a pointer to the RvSdpMsg object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpMsgGetNumOfBadSyntax() call.
***************************************************************************/
RVSDPCOREAPI RvSdpBadSyntax* rvSdpMsgGetBadSyntax(
            const RvSdpMsg* msg,
            RvSize_t index);

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/****  --- End of Message Functions --- ****/

/****  --- Start of Media Descriptor Functions --- ****/
/*
 *	SDP Media descriptors API Functions
 */

/*
    This section contains the functions for operating on RvSdpMediaDescr objects.
    The RvSdpMediaDescr Type represents a media descriptor section of an SDP
    message.
*/

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpMediaDescrConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpMediaDescr object using default allocator.
 *      This function is obsolete. The 'rvSdpMsgAddMediaDescr' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      mediaType - the type of media.
 *      port - the media's port.
 *      protocol - the media's protocol.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMediaDescrConstruct(
                    RvSdpMediaDescr* descr,
                    RvSdpMediaType mediaType,
                    RvUint32 port,
                    RvSdpProtocol protocol);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrConstruct(_descr,_mediaType,_port,_protocol) \
                    rvSdpMediaDescrConstructEx(NULL,(_descr),(_mediaType),\
                                              (_port),(_protocol),NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpMediaDescr object.
 *      This function is obsolete. The 'rvSdpMsgAddMediaDescr' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      mediaType - the type of media.
 *      port - the media's port.
 *      protocol - the media's protocol.
 *      badSyn - the proprietary formatted media field or NULL if standard media is
 *               constructed.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMediaDescrConstructA(
                    RvSdpMediaDescr* descr,
                    RvSdpMediaType mediaType,
                    RvUint32 port,
                    RvSdpProtocol protocol,
                    RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrConstructA(_descr,_mediaType,_port,_protocol,_a) \
                    rvSdpMediaDescrConstructEx(NULL,(_descr),(_mediaType),(_port),\
                                              (_protocol),NULL,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a media descriptor and copies the values from a source media
 *      descriptor.
 *      This  function is obsolete. The 'rvSdpMsgInsertMediaDescr' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to a media descriptor to be constructed. Must point
 *             to valid memory.
 *      src - a source media descriptor.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMediaDescrConstructCopy(
                    RvSdpMediaDescr* dest,
                    const RvSdpMediaDescr* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrConstructCopy(_dest,_src) \
                    rvSdpMediaDescrCopyEx((_dest),(_src),NULL,RV_FALSE,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a media descriptor and copies the values from a source media
 *      descriptor.
 *      This  function is obsolete. The 'rvSdpMsgInsertMediaDescr' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to a media descriptor to be constructed. Must point
 *             to valid memory.
 *      src - a source media descriptor.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMediaDescrConstructCopyA(
                    RvSdpMediaDescr* dest,
                    const RvSdpMediaDescr* src,
                    RvAlloc* a);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxMediaDescrConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpMediaDescr object with proprietary format using
 *      default allocator.
 *      This function is obsolete. The 'rvSdpMsgAddBadSyntaxMediaDescr' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      badSyn - the proprietary format of media descriptor.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpBadSyntaxMediaDescrConstruct(
                    RvSdpMediaDescr* descr,
                    const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxMediaDescrConstruct(_descr,_badSyn) \
                    rvSdpMediaDescrConstructEx(NULL,(_descr),0,0,0,\
                                               (_badSyn),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxMediaDescrConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpMediaDescr object with proprietary format using
 *      provided allocator.
 *      This function is obsolete. The 'rvSdpMsgAddBadSyntaxMediaDescr' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      badSyn - the proprietary format of media descriptor.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpBadSyntaxMediaDescrConstructA(
                    RvSdpMediaDescr* descr,
                    const char* badSyn,
                    RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxMediaDescrConstructA(_descr,_badSyn,_a) \
                    rvSdpMediaDescrConstructEx(NULL,(_descr),0,0,0,\
                                               (_badSyn),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetBadSyntaxValue
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted media descriptor field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMediaDescrGetBadSyntaxValue(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetBadSyntaxValue(_descr)  \
                    RV_SDP_EMPTY_STRING((_descr)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the media descriptor field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpMediaDescrIsBadSyntax(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrIsBadSyntax(_descr) ((_descr)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP media descriptor field value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpMediaDescr object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetBadSyntax(
            RvSdpMediaDescr* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/


/***************************************************************************
 * rvSdpMediaDescrDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the media descriptor object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
***************************************************************************/
RVSDPCOREAPI void rvSdpMediaDescrDestruct(
                    RvSdpMediaDescr* descr);

/***************************************************************************
 * rvSdpMediaDescrCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source media descriptor to destination.
 *      This function is obsolete.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination media descriptor. Must point
 *             to constructed RvSdpMediaDescr object.
 *      src - a source media descriptor.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMediaDescrCopy(
                    RvSdpMediaDescr* dest,
                    const RvSdpMediaDescr* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrCopy(_dest,_src) \
                    rvSdpMediaDescrCopyEx((_dest),(_src),NULL,RV_TRUE,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Setters And Getters
 */

/* format */

/***************************************************************************
 * rvSdpMediaDescrGetNumOfFormats
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor codec formats.
 *
 * Return Value:
 *      Number of codec formats defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfFormats(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfFormats(_descr) (_descr)->iMediaFormatsNum
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetFormat
* ------------------------------------------------------------------------
* General:
*      Gets a media descriptor format by index.
*
* Return Value:
*      The requested codec format name object.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfFormats() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMediaDescrGetFormat(
                    const RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFormat(_descr,_index) \
					RV_SDP_EMPTY_STRING(((const char*) ((_descr)->iMediaFormats[(_index)])))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrAddFormatN
 * ------------------------------------------------------------------------
 * General:
 *      Adds another codec format to the media descriptor object.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      fmt - the name of the format.
 *      len - the 'fmt' length.
***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrAddFormatN(
					RvSdpMediaDescr* descr,
					const char* fmt,
					int len);

/***************************************************************************
 * rvSdpMediaDescrAddFormat
 * ------------------------------------------------------------------------
 * General:
 *      Adds another codec format to the media descriptor object.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      fmt - the name of the format.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrAddFormat(
                    RvSdpMediaDescr* descr,
                    const char* fmt);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddFormat(_descr,_fmt) \
                    rvSdpAddTextToArray(&((_descr)->iMediaFormatsNum),\
                                        RV_SDP_MEDIA_FORMATS_MAX,\
                                        (_descr)->iMediaFormats,(_descr)->iObj,(_fmt))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrRemoveFormat
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and deallocates) the codec format name by index in the
 *      context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfConnections call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveFormat(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveFormat(_descr,_index) \
                    rvSdpRemoveTextFromArray(&((_descr)->iMediaFormatsNum),\
                                             (_descr)->iMediaFormats,\
                                             (_descr)->iObj,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearFormat
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all codec formats set in the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearFormat(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearFormat(_descr) \
                    rvSdpClearTxtArray(&((_descr)->iMediaFormatsNum),\
                                       (_descr)->iMediaFormats,(_descr)->iObj)
#endif /*RV_SDP_USE_MACROS*/


/* payload */

/***************************************************************************
 * rvSdpMediaDescrGetNumOfPayloads
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor payloads.
 *
 * Return Value:
 *      Number of payloads defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfPayloads(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfPayloads(_descr) (_descr)->iMediaFormatsNum
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetPayload
* ------------------------------------------------------------------------
* General:
*      Gets a media descriptor payload by index.
*
* Return Value:
*      The requested media descriptor payload.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfPayloads() call.
***************************************************************************/
RVSDPCOREAPI int rvSdpMediaDescrGetPayload(
                    RvSdpMediaDescr* descr,
                    int index);
#define rvSdpMediaDescrGetPayloadNumber rvSdpMediaDescrGetPayload

/***************************************************************************
 * rvSdpMediaDescrAddPayloadNumber
 * ------------------------------------------------------------------------
 * General:
 *      Adds another payload number to the media descriptor object.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      payload - the payload to be added.
***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrAddPayloadNumber(
                    RvSdpMediaDescr* descr,
                    int payload);


/***************************************************************************
 * rvSdpMediaDescrRemovePayloadNumber
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the codec payload number by index in the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfPayloads call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemovePayloadNumber(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemovePayloadNumber(_descr,_index) \
                    rvSdpMediaDescrRemoveFormat((_descr),(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearPayloads
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all codec payload numbers set in the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearPayloads(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearPayloads(_descr) rvSdpMediaDescrClearFormat((_descr))
#endif /*RV_SDP_USE_MACROS*/

/* Fetch associated message object for this media descr.
   If such an object doesn't exist - return 0 */
#define rvSdpMediaDescrGetMessage(_descr) (_descr)->iSdpMsg

/***************************************************************************
 * rvSdpMediaDescrGetMediaType
 * ------------------------------------------------------------------------
 * General:
 *      Gets the media type of media descriptor.
 *
 * Return Value:
 *      Media type value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpMediaType rvSdpMediaDescrGetMediaType(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetMediaType(_descr) \
                    rvSdpMediaTypeTxt2Val((_descr)->iMediaTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetMediaType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the media type of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      type - the new media type.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrSetMediaType(
                    RvSdpMediaDescr* descr,
                    RvSdpMediaType type);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetMediaType(_descr,_type) \
					rvSdpSetTextField(&(_descr)->iMediaTypeStr,(_descr)->iObj,\
									  rvSdpMediaTypeVal2Txt(_type))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetMediaTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Gets the media type string of media descriptor.
 *
 * Return Value:
 *      Media type text value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMediaDescrGetMediaTypeStr(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetMediaTypeStr(_descr) \
                    RV_SDP_EMPTY_STRING((_descr)->iMediaTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetMediaTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the media type string of media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      type - the new value of media type string.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetMediaTypeStr(
                    RvSdpMediaDescr* descr,
                    const char* type);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetMediaTypeStr(_descr,_type) \
					rvSdpSetTextField(&(_descr)->iMediaTypeStr,(_descr)->iObj,(_type))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetProtocol
 * ------------------------------------------------------------------------
 * General:
 *      Gets the protocol of media descriptor.
 *
 * Return Value:
 *      Media protocol value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpProtocol rvSdpMediaDescrGetProtocol(
					const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetProtocol(_descr) rvSdpMediaProtoTxt2Val((_descr)->iProtocolStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetProtocol
 * ------------------------------------------------------------------------
 * General:
 *      Sets the protocol type of the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      protocol - the new media protocol value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrSetProtocol(
					RvSdpMediaDescr* descr,
					RvSdpProtocol protocol);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetProtocol(_descr,_protocol) \
					rvSdpSetTextField(&(_descr)->iProtocolStr,(_descr)->iObj,\
									  rvSdpMediaProtoVal2Txt(_protocol))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrGetProtocolStr
 * ------------------------------------------------------------------------
 * General:
 *      Gets the media protocol name string of the media descriptor.
 *
 * Return Value:
 *      Media protocol text value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMediaDescrGetProtocolStr(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetProtocolStr(_descr) RV_SDP_EMPTY_STRING((_descr)->iProtocolStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetProtocolStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the media protcol name string of the media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      protocol - the new value of the media type string.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetProtocolStr(
                    RvSdpMediaDescr* descr,
                    const char* protocol);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetProtocolStr(_descr,_proto) \
					rvSdpSetTextField(&(_descr)->iProtocolStr,(_descr)->iObj,(_proto))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrGetPort
 * ------------------------------------------------------------------------
 * General:
 *      Gets the media port number.
 *
 * Return Value:
 *      The port number.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint32 rvSdpMediaDescrGetPort(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetPort(_descr) (_descr)->iPort
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetPort
 * ------------------------------------------------------------------------
 * General:
 *      Sets the media port number.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      port - the new value of the media port number.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrSetPort(
                    RvSdpMediaDescr* descr,
                    RvUint32 port);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetPort(_descr,_port) (_descr)->iPort = (_port)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNumOfPorts
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of subsequent ports defined for the media descriptor.
 *
 * Return Value:
 *      Number of subsequent ports defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI int rvSdpMediaDescrGetNumOfPorts(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfPorts(_descr) (_descr)->iNumOfPorts
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetNumOfPorts
 * ------------------------------------------------------------------------
 * General:
 *      Sets the number of subsequent ports defined for the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      numPorts - the new number of subsequent ports.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrSetNumOfPorts(
                    RvSdpMediaDescr* descr,
                    int numPorts);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetNumOfPorts(_descr,_numPorts) \
                            (_descr)->iNumOfPorts = (_numPorts)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetInformation
 * ------------------------------------------------------------------------
 * General:
 *      Gets the information field of the media descriptor.
 *
 * Return Value:
 *      Returns the media descriptor information field text of empty string
 *      if the information field is not set.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpMediaDescrGetInformation(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetInformation(_descr) \
					RV_SDP_EMPTY_STRING((_descr)->iCommonFields.iInfo.iInfoTxt)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetInformation
 * ------------------------------------------------------------------------
 * General:
 *      Sets the information field of the media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      info - the new information value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetInformation(
                    RvSdpMediaDescr* descr,
                    const char* info);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetInformation(_descr,_info) \
                rvSdpSetSdpInformation(&(_descr)->iCommonFields.iInfo,\
                                (_info),(_descr)->iSdpMsg)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrDestroyInformation
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the information field of the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrDestroyInformation(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrDestroyInformation(_descr) \
        rvSdpDestroySdpInformation(&(_descr)->iCommonFields.iInfo,(_descr)->iSdpMsg)
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrGetNumOfConnections
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor connections fields.
 *
 * Return Value:
 *      Number of connections defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfConnections(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfConnections(_descr) \
                    (_descr)->iCommonFields.iConnectionList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstConnection
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first connection object defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpConnection  object or the NULL pointer if there are no
 *      connections defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextConnection calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMediaDescrGetFirstConnection(
                    RvSdpMediaDescr* descr,
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstConnection(_descr,_iter) \
        (RvSdpConnection*) rvSdpListGetFirst(&(_descr)->iCommonFields.iConnectionList,\
                                             (_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextConnection
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next connection object defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpConnection object or the NULL pointer if there are no
 *      more connections defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Connection function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMediaDescrGetNextConnection(
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextConnection(_iter) \
                    (RvSdpConnection*) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetConnection
* ------------------------------------------------------------------------
* General:
*      Gets a first connection object (in the media descriptor context).
*
* Return Value:
*      The first connection object or NULL if there are no connections.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMediaDescrGetConnection(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetConnection(_descr)  \
                    (RvSdpConnection*) rvSdpListGetByIndex(&(_descr)->iCommonFields.iConnectionList,0)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetConnectionByIndex
* ------------------------------------------------------------------------
* General:
*      Gets a connection object by index (in the media descriptor context).
*
* Return Value:
*      The requested connection object.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfConnections call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMediaDescrGetConnectionByIndex(
                    const RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetConnectionByIndex(_descr,_index) \
        (RvSdpConnection*) rvSdpListGetByIndex(&(_descr)->iCommonFields.iConnectionList,\
                                               (_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrAddConnection
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new connection object to the media descriptor object.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpConnection object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP
 *             address for an IP network, and so on.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpMediaDescrAddConnection(
                    RvSdpMediaDescr* descr,
                    RvSdpNetType net_type,
                    RvSdpAddrType addr_type,
                    const char* addr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddConnection(_descr,_type,_addr_type,_addr) \
                    rvSdpAddConnection2((_descr),(_type),(_addr_type),(_addr),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetConnection
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new connection object to the media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      net_type - the network type.
 *      addr_type - the address type.
 *      addr - the address, depending on the network type. For example, an IP
 *             address for an IP network, and so on.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetConnection(
                    RvSdpMediaDescr* descr,
                    RvSdpNetType net_type,
                    RvSdpAddrType addr_type,
                    const char* addr);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMediaDescrSetBadSyntaxConnection
 * ------------------------------------------------------------------------
 * General:
 *      Adds the SDP connection field with a proprietary format for a specific
 *      media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      badSyn - The proprietary formatted connection field to be set.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetBadSyntaxConnection(
                    RvSdpMediaDescr* descr,
                    const char* badSyn);
#endif

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentConnection
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the connection object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentConnection(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentConnection(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrRemoveConnection
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the connection object by index in the
 *      context of a media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfConnections call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveConnection(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveConnection(_descr,_index) \
                    rvSdpListRemoveByIndex(&(_descr)->iCommonFields.iConnectionList,\
                                           (_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearConnection
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all connections set in the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearConnection(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearConnection(_descr) \
                    rvSdpListClear(&(_descr)->iCommonFields.iConnectionList)
#endif /*RV_SDP_USE_MACROS*/

/* bandwidth */

/***************************************************************************
 * rvSdpMediaDescrGetNumOfBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor bandwidth fields.
 *
 * Return Value:
 *      Number of bandwidths defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfBandwidth(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfBandwidth(_descr) (_descr)->iCommonFields.iBandwidthList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first bandwidth object defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpBandwidth  object or the NULL pointer if there are no
 *      bandwidths defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextBandwidth calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMediaDescrGetFirstBandwidth(
                    RvSdpMediaDescr* descr,
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstBandwidth(_descr,_iter) \
        (RvSdpBandwidth*) rvSdpListGetFirst(&(_descr)->iCommonFields.iBandwidthList,\
                                            (_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next bandwidth object defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpBandwidth object or the NULL pointer if there is no
 *      more bandwidths defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescrGetFirstBandwidth/rvSdpMediaDescrGetNextBandwidth
 *             functions.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMediaDescrGetNextBandwidth(
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextBandwidth(_iter) \
                    (RvSdpBandwidth*) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetBandwidth
* ------------------------------------------------------------------------
* General:
*      Gets a first bandwidth object at the media descriptor level.
*
* Return Value:
*      The first bandwidth object or NULL if there are no bandwidths.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMediaDescrGetBandwidth(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetBandwidth(_descr) \
        (RvSdpBandwidth*) rvSdpListGetByIndex(&(_descr)->iCommonFields.iBandwidthList,0)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetBandwidthByIndex
* ------------------------------------------------------------------------
* General:
*      Gets a bandwidth object by index at the media descriptor level.
*
* Return Value:
*      The requested bandwidth object.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfBandwidths() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMediaDescrGetBandwidthByIndex(
                    const RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetBandwidthByIndex(_descr,_index) \
        (RvSdpBandwidth*) rvSdpListGetByIndex(&(_descr)->iCommonFields.iBandwidthList,\
                                              (_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrAddBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new bandwidth object at media descriptor level.
 *
 * Return Value:
 *      Pointer to the added RvSdpBandwidth  object or the NULL pointer if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      bwtype - bandwidth type, such as Conference Total (CT) or Application-Specific
 *               Maximum (AS).
 *      b - bandwidth value in kilobits per second (kbps).
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpMediaDescrAddBandwidth(
                    RvSdpMediaDescr* descr,
                    const char* bwtype,
                    int b);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddBandwidth(_descr,_bwtype,_b) \
                    rvSdpAddBandwidth2((_descr),(_bwtype),(_b),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new bandwidth object at media descriptor level.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      bwtype - bandwidth type, such as Conference Total (CT) or Application-Specific
 *               Maximum (AS).
 *      b - bandwidth value in kilobits per second (kbps).
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetBandwidth(
                    RvSdpMediaDescr* descr,
                    const char* bwtype,
                    int b);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMediaDescrSetBadSyntaxBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Adds the SDP bandwidth field with a proprietary format for a specific
 *      media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      bandwidth - The proprietary formatted bandwidth field to be set.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetBadSyntaxBandwidth(
                    RvSdpMediaDescr* descr,
                    const char* bandwidth);
#endif

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the bandwidth object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentBandwidth(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentBandwidth(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrRemoveBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the bandwidth object by index in the
 *      context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfBandwidths call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveBandwidth(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveBandwidth(_descr,_index) \
                rvSdpListRemoveByIndex(&(_descr)->iCommonFields.iBandwidthList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearBandwidth
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all bandwidths set in media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearBandwidth(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearBandwidth(_descr) \
                    rvSdpListClear(&(_descr)->iCommonFields.iBandwidthList)
#endif /*RV_SDP_USE_MACROS*/



/* key */

/***************************************************************************
 * rvSdpMediaDescrGetKey
 * ------------------------------------------------------------------------
 * General:
 *      Gets a pointer to the key field of media descriptor.
 *
 * Return Value:
 *      A pointer to the key field, or NULL if the key field is not
 *      set in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpMediaDescrGetKey(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetKey(_descr) \
					(RV_SDP_KEY_IS_USED(&(_descr)->iCommonFields.iKey) ? \
								((RvSdpKey*) &(_descr)->iCommonFields.iKey) : NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetKey
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP key field in media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      em - the key encryption method.
 *      key - the key value.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetKey(
                    RvSdpMediaDescr* descr,
                    RvSdpEncrMethod em,
                    const char* key);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMediaDescrSetBadSyntaxKey
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP key field with a proprietary format for a specific
 *      media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpMediaDescr object.
 *      badSyn - The proprietary formatted key field to be set.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetBadSyntaxKey(
                    RvSdpMediaDescr* descr,
                    const char* badSyn);
#endif

/***************************************************************************
 * rvSdpMediaDescrGetNumOfAttr
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor generic attributes.
 *
 * Return Value:
 *      Number of generic attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfAttr(
            const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfAttr(_descr) \
        rvSdpGetNumOfSpecialAttr(&(_descr)->iCommonFields,\
                            SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstAttribute
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first generic attribute object defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute  object or the NULL pointer if there are no
 *      generic attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextAttribute calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetFirstAttribute(
            RvSdpMediaDescr* descr,
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstAttribute(_descr,_iter) \
			(RvSdpAttribute*)rvSdpGetFirstSpecialAttr(&(_descr)->iCommonFields,\
                            (_iter),SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextAttribute
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next generic attribute defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there is no
 *      more generic attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Attribute function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetNextAttribute(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextAttribute(_iter) \
			(RvSdpAttribute*)rvSdpGetNextSpecialAttr((_iter),SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetAttribute
* ------------------------------------------------------------------------
* General:
*      Gets a generic attribute object by index (in media descriptor context).
*
* Return Value:
*      The requested RvSdpAttribute pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfAttr() call.
**************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetAttribute(
            const RvSdpMediaDescr* descr,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetAttribute(_descr,_index)   \
			rvSdpGetSpecialAttr(&(_descr)->iCommonFields,\
                                                 (_index),SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrAddAttr
* ------------------------------------------------------------------------
* General:
*      Adds new generic attribute to the media descriptor.
*
* Return Value:
*      The pointer to added RvSdpAttribute object or NULL if the function fails.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      name - the name of the new generic attribute.
*      value - the value of the new generic attribute.
**************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrAddAttr(
                    RvSdpMediaDescr* descr,
                    const char* name,
                    const char* value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddAttr(_descr,_name,_value) \
        rvSdpAddAttr2((_descr)->iSdpMsg,&(_descr)->iCommonFields,NULL,(_name),(_value))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentAttribute
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the attribute object pointed by list iterator
 *      in the context of media descriptor.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentAttribute(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentAttribute(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrRemoveAttribute
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the generic attribute object by index in the
 *      context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfAttr call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveAttribute(
            RvSdpMediaDescr* descr,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveAttribute(_descr,_index) \
			rvSdpRemoveSpecialAttr(&(_descr)->iCommonFields,(_index),\
                            SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearAttr
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all generic attributes set in media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearAttr(
            RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearAttr(_descr) \
			rvSdpClearSpecialAttr(&(_descr)->iCommonFields,SDP_FIELDTYPE_NOT_SET)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNumOfAttr2
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor attributes (generic and special).
 *
 * Return Value:
 *      Number of attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfAttr2(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfAttr2(_descr) (_descr)->iCommonFields.iAttrList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstAttribute2
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first attribute object (generic or special) defined in the
 *      media descriptor. Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute  object or the NULL pointer if there are no
 *      attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextAttribute2 calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetFirstAttribute2(
                    RvSdpMediaDescr* descr,
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstAttribute2(_descr,_iter) \
        (RvSdpAttribute*) rvSdpListGetFirst(&(_descr)->iCommonFields.iAttrList,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextAttribute2
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next attribute (generic or special) defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute object or the NULL pointer if there is no
 *      more generic attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Attribute2 function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetNextAttribute2(
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextAttribute2(_iter) \
                    (RvSdpAttribute*) rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetAttribute2
* ------------------------------------------------------------------------
* General:
*      Gets an attribute (generic or special) object by index (in media
*      descriptor context).
*
* Return Value:
*      The requested RvSdpAttribute pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfAttribute2() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetAttribute2(
                    const RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetAttribute2(_descr,_index) \
            (RvSdpAttribute*) rvSdpListGetByIndex(&(_descr)->iCommonFields.iAttrList,\
                                                    (_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentAttribute2
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the attribute object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentAttribute2(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentAttribute2(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrRemoveAttribute2
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the attribute (generic or special) object by
 *      index in the context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfAttr2 call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveAttribute2(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveAttribute2(_descr,_index) \
                    rvSdpListRemoveByIndex(&(_descr)->iCommonFields.iAttrList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearAttr2
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all attributes (generic and special) set in
 *      the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearAttr2(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearAttr2(_descr) \
                    rvSdpListClear(&(_descr)->iCommonFields.iAttrList)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNumOfRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor RTP map attributes.
 *
 * Return Value:
 *      Number of codec RTP maps defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfRtpMap(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfRtpMap(_descr) \
                    rvSdpGetNumOfSpecialAttr(&(_descr)->iCommonFields,\
                                             SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first RTP map special attribute defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpRtpMap  object or the NULL pointer if there are no
 *      RTP maps defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextRtpMap calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMediaDescrGetFirstRtpMap(
                    RvSdpMediaDescr* descr,
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstRtpMap(_descr,_iter) \
                    (RvSdpRtpMap*)rvSdpGetFirstSpecialAttr(&(_descr)->iCommonFields,\
                                                           (_iter),SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next RtpMap special attribute defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpRtpMap object or the NULL pointer if there are no
 *      more RtpMap special attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescrGetFirstRtpMap function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMediaDescrGetNextRtpMap(
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextRtpMap(_iter) \
                    (RvSdpRtpMap*)rvSdpGetNextSpecialAttr((_iter),SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetRtpMap
* ------------------------------------------------------------------------
* General:
*      Gets an RTP map special attribute object by index (in media descriptor
*      context).
*
* Return Value:
*      The requested RvSdpRtpMap pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfRtpMap() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMediaDescrGetRtpMap(
                    const RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetRtpMap(_descr,_index) \
        (RvSdpRtpMap*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&(_descr)->iCommonFields,\
                                                      (_index),SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrAddRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Adds a new RTP map to the media descriptor's RTP map list.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpRtpMap object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      payload - an RTP dynamic payload number.
 *      encoding_name - the name of the codec.
 *      rate - the clock rate.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMediaDescrAddRtpMap(
                    RvSdpMediaDescr* descr,
                    int payload,
                    const char* encoding_name,
                    int rate);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddRtpMap(_descr,_payload,_encoding_name,_rate) \
                    rvSdpAddRtpMap2((_descr)->iSdpMsg,&(_descr)->iCommonFields,\
                                    (_payload),(_encoding_name),(_rate),NULL)
#endif /*RV_SDP_USE_MACROS*/


#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMediaDescrAddBadSyntaxRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpRtpMap object at media
 *      descriptor level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpRtpMap object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      badSyn - the proprietary value of RTP map special attribute field.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpMediaDescrAddBadSyntaxRtpMap(
                    RvSdpMediaDescr* descr,
                    const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddBadSyntaxRtpMap(_descr,_rtpmap) \
                    rvSdpAddRtpMap2((_descr)->iSdpMsg,&(_descr)->iCommonFields,\
                                    0,NULL,0,(_rtpmap))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the RTP map special attribute object pointed
 *      by list iterator in the context  of media descriptor.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentRtpMap(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentRtpMap(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrRemoveRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the RTP map special attribute object by index in the
 *      context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfRtpMap call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveRtpMap(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveRtpMap(_descr,_index) \
                    rvSdpRemoveSpecialAttr(&(_descr)->iCommonFields,\
                                           (_index),SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearRtpMap
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all RTP map special attributes set in the
 *      media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearRtpMap(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearRtpMap(_descr) \
                    rvSdpClearSpecialAttr(&(_descr)->iCommonFields,SDP_FIELDTYPE_RTP_MAP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetConnectionMode
 * ------------------------------------------------------------------------
 * General:
 *      Gets the connection mode of the media descriptor or RV_SDPCONNECTMODE_NOTSET
 *      if the correspondent attribute is not set.
 *
 * Return Value:
 *      Returns the connection mode.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnectionMode rvSdpMediaDescrGetConnectionMode(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetConnectionMode(_descr) \
                    rvSdpGetConnectionMode(&(_descr)->iCommonFields)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrSetConnectionMode
 * ------------------------------------------------------------------------
 * General:
 *      Sets/modifies the connection mode of the media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      mode - the new value of connection mode.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetConnectionMode(
                    RvSdpMediaDescr* descr,
                    RvSdpConnectionMode mode);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetConnectionMode(_descr,_mode) \
                    rvSdpSetConnectionMode((_descr)->iSdpMsg,\
                                           &(_descr)->iCommonFields,(_mode))
#endif /*RV_SDP_USE_MACROS*/

#ifdef RV_SDP_KEY_MGMT_ATTR

/* key management */

/***************************************************************************
 * rvSdpMediaDescrGetNumOfKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor key-mgmt attributes.
 *
 * Return Value:
 *      Number of codec key-mgmt attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfKeyMgmt(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfKeyMgmt(_descr) \
                    rvSdpGetNumOfSpecialAttr(&(_descr)->iCommonFields,\
                                             SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first key-mgmt attribute defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpKeyMgmtAttr  object or the NULL pointer if there are no
 *      key-mgmt attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextKeyMgmt calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMediaDescrGetFirstKeyMgmt(
                    RvSdpMediaDescr* descr,
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstKeyMgmt(_descr,_iter) \
            (RvSdpKeyMgmtAttr*)rvSdpGetFirstSpecialAttr(&(_descr)->iCommonFields,\
                                                        (_iter),SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next KeyMgmt attribute defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpKeyMgmtAttr object or the NULL pointer if there
 *      are no more KeyMgmt attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)KeyMgmt function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMediaDescrGetNextKeyMgmt(
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextKeyMgmt(_iter) \
                    (RvSdpKeyMgmtAttr*)rvSdpGetNextSpecialAttr((_iter),\
                                                               SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetKeyMgmt
* ------------------------------------------------------------------------
* General:
*      Gets a key management special attribute object by index (in media
*      descriptor context).
*
* Return Value:
*      The requested RvSdpKeyMgmtAttr pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfKeyMgmt() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMediaDescrGetKeyMgmt(
                    const RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetKeyMgmt(_descr,_index) \
    (RvSdpKeyMgmtAttr*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&(_descr)->iCommonFields,\
                                           (_index),SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrAddKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new key management attribute to the media descriptor.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpKeyMgmtAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMiaDescr object.
 *      prtclId - the protocol ID.
 *      keyData - the encryption key data.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMediaDescrAddKeyMgmt(
                    RvSdpMediaDescr* descr,
                    const char* prtclId,
                    const char* keyData);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddKeyMgmt(_descr,_prtclId,_keyData) \
                    rvSdpAddKeyMgmt2((_descr)->iSdpMsg,&(_descr)->iCommonFields,\
                                    (_prtclId),(_keyData),NULL)
#endif /*RV_SDP_USE_MACROS*/


#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMediaDescrAddBadSyntaxKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpKeyMgmtAttr object at media
 *      descriptor level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpKeyMgmtAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      badSyn - the proprietary value of KeyMgmt attribute field.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtAttr* rvSdpMediaDescrAddBadSyntaxKeyMgmt(
                    RvSdpMediaDescr* descr,
                    const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddBadSyntaxKeyMgmt(_descr,_KeyMgmt) \
                    rvSdpAddKeyMgmt2((_descr)->iSdpMsg,&(_descr)->iCommonFields,\
                                     NULL,NULL,(_KeyMgmt))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the key-mgmt special attribute object pointed
 *      by list iterator in the context  of media descriptor.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentKeyMgmt(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentKeyMgmt(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrRemoveKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the key-mgmt special attribute object by index in the
 *      context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfKeyMgmt call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveKeyMgmt(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveKeyMgmt(_descr,_index) \
                    rvSdpRemoveSpecialAttr(&(_descr)->iCommonFields,(_index),\
                                           SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearKeyMgmt
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all key-mgmt special attributes set in media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearKeyMgmt(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearKeyMgmt(_descr) \
                rvSdpClearSpecialAttr(&(_descr)->iCommonFields,SDP_FIELDTYPE_KEY_MGMT)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_KEY_MGMT_ATTR*/


#ifdef RV_SDP_CRYPTO_ATTR

/* crypto attribute */

/***************************************************************************
 * rvSdpMediaDescrGetNumOfCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor crypto special attributes.
 *
 * Return Value:
 *      Number of crypto attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfCrypto(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfCrypto(_descr) \
                    rvSdpGetNumOfSpecialAttr(&(_descr)->iCommonFields,\
                                             SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first crypto special attribute defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpCryptoAttr  object or the NULL pointer if there are no
 *      crypto attriubutes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextCrypto calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMediaDescrGetFirstCrypto(
                    RvSdpMediaDescr* descr,
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstCrypto(_descr,_iter) \
                (RvSdpCryptoAttr*)rvSdpGetFirstSpecialAttr(&(_descr)->iCommonFields,\
                                                           (_iter),SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next Crypto attribute defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpCryptoAttr object or the NULL pointer if there are no
 *      more Crypto attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescrGetFirstCrypto function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMediaDescrGetNextCrypto(
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextCrypto(_iter) \
                (RvSdpCryptoAttr*)rvSdpGetNextSpecialAttr((_iter),SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Gets a crypto attribute object by index (in media descriptor context).
 *
 * Return Value:
 *      The requested RvSdpCryptoAttr pointer.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfCrypto() call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMediaDescrGetCrypto(
                    const RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetCrypto(_descr,_index) \
        (RvSdpCryptoAttr*)rvSdpGetSpecialAttr((RvSdpCommonFields*)&(_descr)->iCommonFields,\
                            (_index),SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrAddCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new crypto special attribute object to the media descriptor.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpCryptoAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      tag - the crypto attribute tag number.
 *      suite - the crypto attribute suite value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMediaDescrAddCrypto(
                    RvSdpMediaDescr* descr,
                    RvUint tag,
                    const char* suite);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddCrypto(_descr,_tag,_suite) \
                    rvSdpAddCrypto2((_descr)->iSdpMsg,&(_descr)->iCommonFields,\
                                     (_tag),(_suite),NULL)
#endif /*RV_SDP_USE_MACROS*/

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpMediaDescrAddBadSyntaxCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpCryptoAttr object at media
 *      descriptor level.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpCryptoAttr object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      badSyn - the proprietary value of crypto special attribute field.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpCryptoAttr* rvSdpMediaDescrAddBadSyntaxCrypto(
                    RvSdpMediaDescr* descr,
                    const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddBadSyntaxCrypto(_descr,_crypto) \
                    rvSdpAddCrypto2((_descr)->iSdpMsg,&(_descr)->iCommonFields,0,\
                                    NULL,(_crypto))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the crypto special attribute object pointed
 *      by list iterator in the context  of media descriptor.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentCrypto(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentCrypto(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrRemoveCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the crypto special attribute object by index in the
 *      context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfCrypto call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCrypto(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCrypto(_descr,_index) \
                    rvSdpRemoveSpecialAttr(&(_descr)->iCommonFields,(_index),\
                                           SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearCrypto
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all crypto special attributes set in media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearCrypto(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearCrypto(_descr) \
                    rvSdpClearSpecialAttr(&(_descr)->iCommonFields,SDP_FIELDTYPE_CRYPTO)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_CRYPTO_ATTR*/



#ifdef RV_SDP_FRAMERATE_ATTR

/* framerate attribute */

/***************************************************************************
 * rvSdpMediaDescrGetFrameRate
 * ------------------------------------------------------------------------
 * General:
 *      Gets the frame-rate special attribute value of the media descriptor.
 *
 * Return Value:
 *      Returns the attributes value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpMediaDescrGetFrameRate(
				RvSdpMediaDescr* descr);

/***************************************************************************
 * rvSdpMediaDescrSetFrameRate
 * ------------------------------------------------------------------------
 * General:
 *      Sets/modifies the frame-rate special attribute of the media descriptor.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      val - the frame rate attribute value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpMediaDescrSetFrameRate(
				RvSdpMediaDescr* descr,
				const char* val);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrSetFrameRate(_descr,_val) \
				((rvSdpSpecialAttrSetValue((_descr)->iSdpMsg,&(_descr)->iCommonFields,\
										 SDP_FIELDTYPE_FRAMERATE,(_val),RV_FALSE)) \
											? RV_SDPSTATUS_OK : RV_SDPSTATUS_ALLOCFAIL)
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrDestroyFrameRate
 * ------------------------------------------------------------------------
 * General:
 *      Destroys the frame-rate special attribute of the media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrDestroyFrameRate(
				RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrDestroyFrameRate(_descr) \
            rvSdpRemoveSpecialAttr(&(_descr)->iCommonFields,0,SDP_FIELDTYPE_FRAMERATE)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_FRAMERATE_ATTR*/

#ifdef RV_SDP_FMTP_ATTR

/* fmtp attribute */

/***************************************************************************
 * rvSdpMediaDescrGetNumOfFmtp
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor fmtp special attributes.
 *
 * Return Value:
 *      Number of fmtp attributes defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfFmtp(
                    const RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfFmtp(_descr) \
            rvSdpGetNumOfSpecialAttr(&(_descr)->iCommonFields,SDP_FIELDTYPE_FMTP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstFmtp
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first fmtp special attribute defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute (of fmtp)  object or the NULL pointer if there
 *      are no fmtp attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextFmtp calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetFirstFmtp(
                    RvSdpMediaDescr* descr,
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstFmtp(_descr,_iter) \
                    rvSdpGetFirstSpecialAttr(&(_descr)->iCommonFields,\
                                              (_iter),SDP_FIELDTYPE_FMTP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextFmtp
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next fmtp special attribute defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpAttribute (fmtp) object or the NULL pointer if there is no
 *      more fmtp attributes defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Fmtp function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetNextFmtp(
                    RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextFmtp(_iter) \
                    rvSdpGetNextSpecialAttr((_iter),SDP_FIELDTYPE_FMTP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetFmtp
* ------------------------------------------------------------------------
* General:
*      Gets an fmtp special attribute object by index (in media descriptor context).
*
* Return Value:
*      The requested RvSdpAttribute (of fmtp special attribute) pointer.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfFmtp() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrGetFmtp(
                    const RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFmtp(_descr,_index) \
                    rvSdpGetSpecialAttr((RvSdpCommonFields*)&(_descr)->iCommonFields,\
                                        (_index),SDP_FIELDTYPE_FMTP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrAddFmtp
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new fmtp special attribute object to the media descriptor.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpAttribute object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      val - the value of new fmtp special attribute.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpMediaDescrAddFmtp(
                    RvSdpMediaDescr* descr,
                    const char* val);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddFmtp(_descr,_val) \
				rvSdpSpecialAttrSetValue((_descr)->iSdpMsg,&(_descr)->iCommonFields,\
										 SDP_FIELDTYPE_FMTP,(_val),RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentFmtp
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the fmtp special attribute object pointed
 *      by list iterator in the context  of media descriptor.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentFmtp(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentFmtp(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpMediaDescrRemoveFmtp
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the fmtp special attribute object by index in the
 *      context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfFmtp call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveFmtp(
                    RvSdpMediaDescr* descr,
                    RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveFmtp(_descr,_index) \
            rvSdpRemoveSpecialAttr(&(_descr)->iCommonFields,(_index),SDP_FIELDTYPE_FMTP)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearFmtp
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all fmtp special attributes set in media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearFmtp(
                    RvSdpMediaDescr* descr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearFmtp(_descr) \
                    rvSdpClearSpecialAttr(&(_descr)->iCommonFields,SDP_FIELDTYPE_FMTP)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_FMTP_ATTR*/

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMediaDescrGetNumOfOther
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor RvSdpOther objects.
 *
 * Return Value:
 *      Number of RvSdpOther objects defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfOther(
                const RvSdpMediaDescr* media);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNumOfOther(_media) (_media)->iCommonFields.iOtherList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetFirstOther
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first RvSdpOther object defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpOther  object or the NULL pointer if there are no
 *      'other's defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextOther calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpMediaDescrGetFirstOther(
                RvSdpMediaDescr* media,
                RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetFirstOther(_media,_iter) \
         (RvSdpOther*)(rvSdpListGetFirst(&(_media)->iCommonFields.iOtherList,(_iter)))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrGetNextOther
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next 'other' object defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpOther object or the NULL pointer if there are no
 *      more 'other' objects defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)Other function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpMediaDescrGetNextOther(
                RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetNextOther(_iter) (RvSdpOther*)rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpMediaDescrGetOther
* ------------------------------------------------------------------------
* General:
*      Gets a 'other' object by index (in media descriptor context).
*
* Return Value:
*      The requested RvSdpOther object.
* ------------------------------------------------------------------------
* Arguments:
*      media - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfOther() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpMediaDescrGetOther(
                RvSdpMediaDescr* media,
                RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrGetOther(_media,_index) \
        (RvSdpOther*)rvSdpListGetByIndex(&(_media)->iCommonFields.iOtherList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrAddOther
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary tag SDP object to the media descriptor's list of
 *      RvSdpOther objects.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpOther object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      media - a pointer to the RvSdpMediaDescr object.
 *      tag - the tag letter of the line.
 *      value - the proprietary text of the line.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpMediaDescrAddOther(
                RvSdpMediaDescr* media,
                const char tag,
                const char *value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrAddOther(_media,_tag,_value) \
                rvSdpAddOther((_media)->iSdpMsg,&(_media)->iCommonFields,(_tag),(_value))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrRemoveCurrentOther
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the 'other' object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveCurrentOther(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveCurrentOther(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrRemoveOther
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the 'other' object by index in the
 *      context of media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpMediaDescrGetNumOfOther call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrRemoveOther(
                RvSdpMediaDescr* media,
                RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrRemoveOther(_media,_index) \
                rvSdpListRemoveByIndex(&(_media)->iCommonFields.iOtherList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpMediaDescrClearOther
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all RvSdpOther objects set in media descriptor.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpMediaDescrClearOther(
                RvSdpMediaDescr* media);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpMediaDescrClearOther(_media) \
                rvSdpListClear(&(_media)->iCommonFields.iOtherList)
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/* bad syntax iterator */

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpMediaDescrGetNumOfBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of media descriptor bad syntax objects.
 *
 * Return Value:
 *      Number of bad syntax objects defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 ***************************************************************************/
RVSDPCOREAPI RvSize_t rvSdpMediaDescrGetNumOfBadSyntax(
                    const RvSdpMediaDescr* descr);

/***************************************************************************
 * rvSdpMediaDescrGetFirstBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first bad syntax object defined in the media descriptor.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpBadSyntax  object or the NULL pointer if there are no
 *      bad syntax objects defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to the RvSdpMediaDescr object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMediaDescrGetNextBadSyntax calls.
 ***************************************************************************/
RVSDPCOREAPI RvSdpBadSyntax* rvSdpMediaDescrGetFirstBadSyntax(
                    RvSdpMediaDescr* descr,
                    RvSdpLineObjIter* iter);

/***************************************************************************
 * rvSdpMediaDescrGetNextBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next bad syntax object defined in the media descriptor.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpBadSyntax object or the NULL pointer if there is no
 *      more BadSyntaxs defined in the media descriptor.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMediaDescr(GetFirst/Next)BadSyntax function.
 ***************************************************************************/
RVSDPCOREAPI RvSdpBadSyntax* rvSdpMediaDescrGetNextBadSyntax(
                    RvSdpLineObjIter* iter);

/***************************************************************************
* rvSdpMediaDescrGetBadSyntax
* ------------------------------------------------------------------------
* General:
*      Gets a bad syntax object by index (in media descriptor context).
*
* Return Value:
*      The requested RvSdpBadSyntax object.
* ------------------------------------------------------------------------
* Arguments:
*      descr - a pointer to the RvSdpMediaDescr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpMediaDescrGetNumOfBadSyntax() call.
***************************************************************************/
RVSDPCOREAPI RvSdpBadSyntax* rvSdpMediaDescrGetBadSyntax(
                    const RvSdpMediaDescr* descr,
                    int index);

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/


/****  --- End of Media Descriptor Functions --- ****/

/****  --- Start of Message List Functions --- ****/
/*
This section contains the functions used to operate on RvSdpMsgList objects.
*/

/*
 *	Message List functions
 */

/***************************************************************************
 * rvSdpMsgListConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the list of SDP messages.
 *
 * Return Value:
 *      The constructed RvSdpMsgList object or NULL if function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - a pointer to valid RvSdpMsgList object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsgList* rvSdpMsgListConstruct(
            RvSdpMsgList* msgList);

/***************************************************************************
 * rvSdpMsgListConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the list of SDP messages using user provided allocator.
 *
 * Return Value:
 *      The constructed RvSdpMsgList object or NULL if function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - a pointer to valid RvSdpMsgList object.
 *      a - allocator to be used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsgList* rvSdpMsgListConstructA(
            RvSdpMsgList* msgList,
            RvAlloc* a);

/***************************************************************************
 * rvSdpMsgListConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the list of SDP messages 'dest' and copies all SDP messages
 *      contained in 'src' to 'dest'.
 *
 * Return Value:
 *      The constructed RvSdpMsgList object 'dest' or NULL if function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      d - the destination RvSdpMsgList object.
 *      s - the source RvSdpMsgList object.
 *      a - allocator to be used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsgList* rvSdpMsgListConstructCopyA(
            RvSdpMsgList* d,
            const RvSdpMsgList* s,
            RvAlloc* a);

/***************************************************************************
 * rvSdpMsgListDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs all SDP messages contained in msgList.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgListDestruct(
            RvSdpMsgList* msgList);

/***************************************************************************
 * rvSdpMsgListCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies all SDP messages contained in 'src' to 'dest'.
 *
 * Return Value:
 *      The RvSdpMsgList object 'dest' or NULL if function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      d - the destination RvSdpMsgList object.
 *      s - the source RvSdpMsgList object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsgList* rvSdpMsgListCopy(
            RvSdpMsgList* d,
            const RvSdpMsgList* s);

/***************************************************************************
 * rvSdpMsgListGetSize
 * ------------------------------------------------------------------------
 * General:
 *      Get the number of SDP messages in the list.
 *
 * Return Value:
 *      Returns the number of SDP messages in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
RVSDPCOREAPI RvSize_t rvSdpMsgListGetSize(
            const RvSdpMsgList* msgList);

/***************************************************************************
 * rvSdpMsgListGetFirstMsg
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first SDP message in the messages list.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpMsg  object or the NULL pointer if there are no
 *      messages in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - a pointer to the RvSdpMsgList object.
 *      li - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpMsgListGetNextMsg calls.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsg * rvSdpMsgListGetFirstMsg(
            RvSdpMsgList* msgList,
            RvSdpListIter* li);

/***************************************************************************
 * rvSdpMsgListGetNextMsg
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next RvSdpMsg object from the messages list.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpMsg object or the NULL pointer if there is no
 *      more messages in the list.
 * ------------------------------------------------------------------------
 * Arguments:
 *      li - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpMsgList(GetFirst/Next)Msg function.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsg * rvSdpMsgListGetNextMsg(
            RvSdpListIter* li);

/***************************************************************************
 * rvSdpMsgListGetElement
 * ------------------------------------------------------------------------
 * General:
 *      Get the SDP message with index 'i' contained in the list msgList.
 *
 * Return Value:
 *      Returns the SDP message with index 'i' contained in the list msgList.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsg* rvSdpMsgListGetElement(
            RvSdpMsgList* msgList,
            RvSize_t i);

/***************************************************************************
 * rvSdpMsgListAddMsg
 * ------------------------------------------------------------------------
 * General:
 *      Constructs new SDP message and adds it to the list.
 *
 * Return Value:
 *      Pointer to the constructed/added RvSdpMsg object or NULL if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMsg *rvSdpMsgListAddMsg(
            RvSdpMsgList* msgList);

/***************************************************************************
 * rvSdpMsgListInsertMsg
 * ------------------------------------------------------------------------
 * General:
 *      Appends the the SDP message to the list.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 *      msg - pointer to constructed RvSdpMsg object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpMsgListInsertMsg(
            RvSdpMsgList* msgList,
            const RvSdpMsg* msg);

/***************************************************************************
 * rvSdpMsgListRemoveCurrentMsg
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the message object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgListRemoveCurrentMsg(
            RvSdpListIter* li);

/***************************************************************************
 * rvSdpMsgListRemoveElement
 * ------------------------------------------------------------------------
 * General:
 *      Removes and destructs the message object contained in the list by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 *      i - the index of the message to remove.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgListRemoveElement(
            RvSdpMsgList* msgList,
            RvSize_t i);

/***************************************************************************
 * rvSdpMsgListClear
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all RvSdpMsg objects set in messages list.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msgList - the RvSdpMsgList object to be destructed.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpMsgListClear(
            RvSdpMsgList* msgList);

/****  --- End of Message List Functions --- ****/

/****  --- Start of Origin Functions --- ****/
/*
This section contains the functions used to operate on RvSdpOrigin objects.
The RvSdpOrigin Type represents the origin ('o=') field of an SDP message.
*/
/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpOriginConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpOrigin object.
 *      This function is obsolete. The rvSdpMsgSetOrigin should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to valid RvSdpOrigin object.
 *      username - the origin field user name.
 *      session_id - the origin field session ID.
 *      version - the origin field version.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOrigin* rvSdpOriginConstruct(
                RvSdpOrigin *origin,
                const char* username,
                const char* session_id,
                const char* version,
                RvSdpNetType nettype,
                RvSdpAddrType addrtype,
                const char* address);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginConstruct(_origin,_username,_session_id,_version,_nettype,_addrtype,_address) \
                rvSdpOriginConstruct2(NULL,(_origin),(_username),(_session_id),(_version),\
                                      (_nettype),(_addrtype),(_address),NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpOrigin object.
 *      This function is obsolete. The rvSdpMsgSetOrigin should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to valid RvSdpOrigin object.
 *      username - the origin field user name.
 *      session_id - the origin field session ID.
 *      version - the origin field version.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOrigin* rvSdpOriginConstructA(
                RvSdpOrigin *origin,
                const char* username,
                const char* session_id,
                const char* version,
                RvSdpNetType nettype,
                RvSdpAddrType addrtype,
                const char* address,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginConstructA(_origin,_username,_session_id,_version,_nettype,_addrtype,_address,_a) \
                rvSdpOriginConstruct2(NULL,(_origin),(_username),(_session_id),(_version),(_nettype),\
                                      (_addrtype),(_address),NULL,(_a),RV_FALSE);
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a origin object and copies the values from a source
 *      origin field.
 *      This function is obsolete. The rvSdpMsgSetOrigin should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to origin to be constructed. Must point
 *             to valid memory.
 *      src - a source origin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOrigin* rvSdpOriginConstructCopy(
                RvSdpOrigin* dest,
                const RvSdpOrigin* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginConstructCopy(_dest,_src) \
                rvSdpOriginCopy2((_dest),(_src),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a origin object and copies the values from a source
 *      origin field.
 *      This function is obsolete. The rvSdpMsgSetOrigin should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to origin to be constructed. Must point
 *             to valid memory.
 *      src - a source origin object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOrigin* rvSdpOriginConstructCopyA(
                RvSdpOrigin* dest,
                const RvSdpOrigin* src,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginConstructCopyA(_dest,_src,_a) \
                rvSdpOriginCopy2((_dest),(_src),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxOriginConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpOrigin object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxOrigin should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to valid RvSdpOrigin object.
 *      badSyn - the proprietary format of origin field.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOrigin* rvSdpBadSyntaxOriginConstruct(
                RvSdpOrigin* origin,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxOriginConstruct(_origin,_badSyn) \
                rvSdpOriginConstruct2(NULL,(_origin),NULL,NULL,NULL,\
                                      0,0,NULL,(_badSyn),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxOriginConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpOrigin object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxOrigin should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to valid RvSdpOrigin object.
 *      badSyn - the proprietary format of origin field.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOrigin* rvSdpBadSyntaxOriginConstructA(
                RvSdpOrigin* origin,
                const char* badSyn,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxOriginConstructA(_origin,_badSyn,_a) \
                rvSdpOriginConstruct2(NULL,(_origin),NULL,NULL,\
                                      NULL,0,0,NULL,(_badSyn),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the origin field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpOriginIsBadSyntax(
                const RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginIsBadSyntax(_origin) ((_origin)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted origin field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpOriginGetBadSyntax(
                const RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginGetBadSyntax(_origin) \
                RV_SDP_EMPTY_STRING((_origin)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP origin field value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpOrigin object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpOriginSetBadSyntax(
            RvSdpOrigin* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpOriginDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the origin object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
***************************************************************************/
RVSDPCOREAPI void rvSdpOriginDestruct(
                RvSdpOrigin* origin);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpOriginCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source origin object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination origin. Must point
 *             to constructed RvSdpOrigin object.
 *      src - a source origin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOrigin* rvSdpOriginCopy(
                RvSdpOrigin* dest,
                const RvSdpOrigin* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginCopy(_dest,_src) rvSdpOriginCopy2((_dest),(_src),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Setters And Getters
 */


/***************************************************************************
 * rvSdpOriginGetUsername
 * ------------------------------------------------------------------------
 * General:
 *      Get the origin's user name.
 * Return Value:
 *      Returns the origin's user name.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpOriginGetUsername(
                const RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginGetUsername(_origin) RV_SDP_EMPTY_STRING((_origin)->iUserName)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetUsername
 * ------------------------------------------------------------------------
 * General:
 *      Sets the origin's field user name.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      username - the origin's field user name.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpOriginSetUsername(
                RvSdpOrigin* origin,
                const char* username);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginSetUsername(_origin,_userName) \
                rvSdpSetTextField(&(_origin)->iUserName,(_origin)->iObj,(_userName))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginGetVersion
 * ------------------------------------------------------------------------
 * General:
 *      Get the origin's version.
 * Return Value:
 *      Returns the origin's version.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpOriginGetVersion(
                const RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginGetVersion(_origin) RV_SDP_EMPTY_STRING((_origin)->iVersion)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetVersion
 * ------------------------------------------------------------------------
 * General:
 *      Sets the origin's field version.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      version - the origin's field version.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpOriginSetVersion(
                RvSdpOrigin* origin,
                const char* version);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginSetVersion(_origin,_version) \
                rvSdpSetTextField(&(_origin)->iVersion,(_origin)->iObj,(_version))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginGetSessionId
 * ------------------------------------------------------------------------
 * General:
 *      Get the origin's session ID.
 * Return Value:
 *      Returns the origin's session ID.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpOriginGetSessionId(
                const RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginGetSessionId(_origin) RV_SDP_EMPTY_STRING((_origin)->iSessionId)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetSessionId
 * ------------------------------------------------------------------------
 * General:
 *      Sets the origin's field session ID.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      id - the origin's field session  ID.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpOriginSetSessionId(
                RvSdpOrigin* origin,
                const char* id);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginSetSessionId(_origin,_sessionId) \
                rvSdpSetTextField(&(_origin)->iSessionId,(_origin)->iObj,(_sessionId))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginGetNetType
 * ------------------------------------------------------------------------
 * General:
 *      Get the origin's network type.
 * Return Value:
 *      Returns the origin's network type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpNetType rvSdpOriginGetNetType(
                const RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginGetNetType(_origin) rvSdpNetTypeTxt2Val((_origin)->iNetTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetNetType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the origin's field network type.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      netType - the origin's field network type.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpOriginSetNetType(
                RvSdpOrigin* origin,
                RvSdpNetType netType);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginSetNetType(_origin,_netType) \
				rvSdpSetTextField(&(_origin)->iNetTypeStr,(_origin)->iObj,\
								  rvSdpNetTypeVal2Txt((_netType)))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpOriginGetNetTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Get the origin's network type string.
 * Return Value:
 *      Returns the origin's network type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpOriginGetNetTypeStr(
                RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginGetNetTypeStr(_origin) RV_SDP_EMPTY_STRING((_origin)->iNetTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetNetTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the origin's field network type string.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      type - the origin's field network type string.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpOriginSetNetTypeStr(
                RvSdpOrigin* origin,
                const char* type);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginSetNetTypeStr(_origin,_netTypeStr) \
                rvSdpSetTextField(&(_origin)->iNetTypeStr,(_origin)->iObj,(_netTypeStr))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginGetAddressType
 * ------------------------------------------------------------------------
 * General:
 *      Get the origin's address type.
 * Return Value:
 *      Returns the origin's address type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAddrType rvSdpOriginGetAddressType(
                const RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginGetAddressType(_origin) \
                rvSdpAddrTypeTxt2Val((_origin)->iAddrTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetAddressType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the origin's field address type.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      addrType - the origin's field address type.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpOriginSetAddressType(
                RvSdpOrigin* origin,
                RvSdpAddrType addrType);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginSetAddressType(_origin,_addrType) \
				rvSdpSetTextField(&(_origin)->iAddrTypeStr,(_origin)->iObj,\
								  rvSdpAddrTypeVal2Txt((_addrType)))

#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginGetAddressTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Get the origin's address type string.
 * Return Value:
 *      Returns the origin's address type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpOriginGetAddressTypeStr(
				RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOriginGetAddressTypeStr(_origin) \
                RV_SDP_EMPTY_STRING((_origin)->iAddrTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetAddressTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the origin's field address type string.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      t - the origin's field address type string.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpOriginSetAddressTypeStr(
                RvSdpOrigin* origin,
                const char* t);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginSetAddressTypeStr(_origin,_addrTypeStr) \
                rvSdpSetTextField(&(_origin)->iAddrTypeStr,\
                                  (_origin)->iObj,(_addrTypeStr))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginGetAddress
 * ------------------------------------------------------------------------
 * General:
 *      Get the origin's address.
 * Return Value:
 *      Returns the origin's address.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpOriginGetAddress(
                const RvSdpOrigin* origin);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginGetAddress(_origin) RV_SDP_EMPTY_STRING((_origin)->iAddress)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOriginSetAddress
 * ------------------------------------------------------------------------
 * General:
 *      Sets the origin's field address.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      origin - a pointer to the RvSdpOrigin object.
 *      addr - the origin's field address.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpOriginSetAddress(
                RvSdpOrigin* origin,
                const char* addr);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpOriginSetAddress(_origin,_address) \
                rvSdpSetTextField(&(_origin)->iAddress,(_origin)->iObj,(_address))
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of Origin Functions --- ****/

/****  --- Start of EMail Functions --- ****/
/*
This section contains the functions used to operate on RvSdpEmail objects.
The RvSdpEmail Type represents the email ('e=') field of an SDP message.
*/

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpEmailConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpEmail object.
 *      This function is obsolete. The rvSdpMsgAddEmail should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to valid RvSdpEmail object.
 *      address - the email address.
 *      text - optional email text.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpEmailConstruct(
                RvSdpEmail* email,
                const char* address,
                const char* text);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailConstruct(_email,_address,_text) \
            rvSdpEmailConstruct2(NULL,(_email),(_address),(_text),NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpEmail object.
 *      This function is obsolete. The rvSdpMsgAddEmail should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to valid RvSdpEmail object.
 *      address - the email address.
 *      text - optional email text.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpEmailConstructA(
                RvSdpEmail* email,
                const char* address,
                const char* text,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailConstructA(_email,_address,_text,_a) \
            rvSdpEmailConstruct2(NULL,(_email),(_address),(_text),NULL,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a email object and copies the values from a source
 *      email field.
 *      This function is obsolete. The rvSdpMsgAddEmail should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to email to be constructed. Must point
 *             to valid memory.
 *      src - a source email object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpEmailConstructCopy(
                RvSdpEmail* dest,
                const RvSdpEmail* source);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailConstructCopy(_dest,_source) \
                rvSdpEmailCopy2((_dest),(_source),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a email object and copies the values from a source
 *      email field.
 *      This function is obsolete. The rvSdpMsgAddEmail should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to email to be constructed. Must point
 *             to valid memory.
 *      src - a source email object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpEmail* rvSdpEmailConstructCopyA(
                RvSdpEmail* dest,
                const RvSdpEmail* source,
                RvAlloc* a);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxEmailConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpEmail object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxEmail should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to valid RvSdpEmail object.
 *      badSyn - the proprietary format of email.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpBadSyntaxEmailConstruct(
                RvSdpEmail* email,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxEmailConstruct(_email,_badSyn) \
                rvSdpEmailConstruct2(NULL,(_email),NULL,NULL,(_badSyn),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxEmailConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpEmail object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxEmail should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to valid RvSdpEmail object.
 *      badSyn - the proprietary format of email.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpBadSyntaxEmailConstructA(
                RvSdpEmail* email,
                const char* badSyn,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxEmailConstructA(_email,_badSyn,_a) \
                rvSdpEmailConstruct2(NULL,(_email),NULL,NULL,(_badSyn),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted email field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpEmailGetBadSyntax(
                const RvSdpEmail* email);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailGetBadSyntax(_email) RV_SDP_EMPTY_STRING((_email)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the email field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpEmailIsBadSyntax(
                RvSdpEmail* email);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailIsBadSyntax(_email) ((_email)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP email value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpEmail object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpEmailSetBadSyntax(
            RvSdpEmail* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpEmailDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the email object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpEmailDestruct(
                RvSdpEmail *email);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpEmailCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source email object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination email. Must point
 *             to constructed RvSdpEmail object.
 *      src - a source email object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEmail* rvSdpEmailCopy(
                RvSdpEmail* dest,
                const RvSdpEmail* source);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailCopy(_dest,_source) rvSdpEmailCopy2((_dest),(_source),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Setters And Getters
 */

/***************************************************************************
 * rvSdpEmailGetAddress
 * ------------------------------------------------------------------------
 * General:
 *      Get the email address.
 * Return Value:
 *      Returns the email address.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpEmailGetAddress(
                const RvSdpEmail* email);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailGetAddress(_email) RV_SDP_EMPTY_STRING((_email)->iAddress)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailSetAddress
 * ------------------------------------------------------------------------
 * General:
 *      Sets the email address.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 *      addr - the email address.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpEmailSetAddress(
                RvSdpEmail* email,
                const char* addr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailSetAddress(_email,_addr) \
                rvSdpSetTextField(&(_email)->iAddress,(_email)->iObj,(_addr))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailGetText
 * ------------------------------------------------------------------------
 * General:
 *      Get the email optional text.
 * Return Value:
 *      Returns the email optional text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpEmailGetText(
                const RvSdpEmail* email);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailGetText(_email) RV_SDP_EMPTY_STRING((_email)->iText)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpEmailSetText
 * ------------------------------------------------------------------------
 * General:
 *      Sets the email text.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      email - a pointer to the RvSdpEmail object.
 *      text - the email text.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpEmailSetText(
                RvSdpEmail* email,
                const char* text);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpEmailSetText(_email,_text) \
                rvSdpSetTextField(&(_email)->iText,(_email)->iObj,(_text))
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of EMail Functions --- ****/

/****  --- Start of Phone Functions --- ****/
/*
This section contains the functions used to operate on RvSdpPhone objects.
The RvSdpPhone Type represents the phone ('p=') field of an SDP message.
*/

/*
 *	SDP Phone API Functions
 */

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpPhoneConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpPhone object.
 *      This function is obsolete. The rvSdpMsgAddPhone should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to valid RvSdpPhone object.
 *      number - the phone number.
 *      text - optional phone text.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpPhoneConstruct(
                RvSdpPhone* phone,
                const char* number,
                const char* text);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneConstruct(_phone,_number,_text) \
            rvSdpPhoneConstruct2(NULL,(_phone),(_number),(_text),NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpPhone object.
 *      This function is obsolete. The rvSdpMsgAddPhone should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to valid RvSdpPhone object.
 *      number - the phone number.
 *      text - optional phone text.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpPhoneConstructA(
                RvSdpPhone* phone,
                const char* number,
                const char* text,
                RvAlloc* alloc);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneConstructA(_phone,_number,_text,_a) \
            rvSdpPhoneConstruct2(NULL,(_phone),(_number),(_text),NULL,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a phone object and copies the values from a source
 *      phone field.
 *      This function is obsolete. The rvSdpMsgAddPhone should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to phone to be constructed. Must point
 *             to valid memory.
 *      src - a source phone object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpPhoneConstructCopy(
                RvSdpPhone* dest,
                const RvSdpPhone* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneConstructCopy(_dest,_src) \
                rvSdpPhoneCopy2((_dest),(_src),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a phone object and copies the values from a source
 *      phone field.
 *      This function is obsolete. The rvSdpMsgAddPhone should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to phone to be constructed. Must point
 *             to valid memory.
 *      src - a source phone object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPhone* rvSdpPhoneConstructCopyA(
                RvSdpPhone* dest,
                const RvSdpPhone* src,
                RvAlloc* alloc);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxPhoneConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpPhone object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxPhone should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to valid RvSdpPhone object.
 *      badSyn - the proprietary format of phone.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpBadSyntaxPhoneConstruct(
                RvSdpPhone* phone,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxPhoneConstruct(_phone,_badSyn) \
            rvSdpPhoneConstruct2(NULL,(_phone),NULL,NULL,(_badSyn),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxPhoneConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpPhone object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxPhone should be
 *      used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to valid RvSdpPhone object.
 *      badSyn - the proprietary format of phone.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpBadSyntaxPhoneConstructA(
                RvSdpPhone* phone,
                const char* badSyn,
                RvAlloc* alloc);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxPhoneConstructA(_phone,_badSyn,_a) \
            rvSdpPhoneConstruct2(NULL,(_phone),NULL,NULL,(_badSyn),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted phone field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpPhoneGetBadSyntax(
                const RvSdpPhone* phone);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneGetBadSyntax(_phone) RV_SDP_EMPTY_STRING((_phone)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the phone field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpPhoneIsBadSyntax(
                RvSdpPhone* phone);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneIsBadSyntax(_phone) ((_phone)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP phone field value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpPhone object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpPhoneSetBadSyntax(
            RvSdpPhone* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpPhoneDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the phone object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
***************************************************************************/
RVSDPCOREAPI void rvSdpPhoneDestruct(
                RvSdpPhone* phone);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpPhoneCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source phone object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination phone. Must point
 *             to constructed RvSdpPhone object.
 *      src - a source phone object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpPhone* rvSdpPhoneCopy(
                RvSdpPhone* dest,
                const RvSdpPhone* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneCopy(_dest,_src) rvSdpPhoneCopy2((_dest),(_src),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Setters And Getters
 */

/***************************************************************************
 * rvSdpPhoneGetNumber
 * ------------------------------------------------------------------------
 * General:
 *      Get the phone number.
 * Return Value:
 *      Returns the phone number.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpPhoneGetNumber(
                const RvSdpPhone* phone);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneGetNumber(_phone) RV_SDP_EMPTY_STRING((_phone)->iPhoneNumber)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneSetNumber
 * ------------------------------------------------------------------------
 * General:
 *      Sets the phone number.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 *      number - the phone number.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpPhoneSetNumber(
                RvSdpPhone* phone,
                const char* number);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneSetNumber(_phone,_number) \
            rvSdpSetTextField(&(_phone)->iPhoneNumber,(_phone)->iObj,(_number))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneGetText
 * ------------------------------------------------------------------------
 * General:
 *      Get the phone text.
 * Return Value:
 *      Returns the phone text.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpPhoneGetText(
                const RvSdpPhone* phone);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneGetText(_phone) RV_SDP_EMPTY_STRING((_phone)->iText)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpPhoneSetText
 * ------------------------------------------------------------------------
 * General:
 *      Sets the phone text.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      phone - a pointer to the RvSdpPhone object.
 *      text - the phone text.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpPhoneSetText(
                RvSdpPhone* phone,
                const char* text);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpPhoneSetText(_phone,_text) \
                rvSdpSetTextField(&(_phone)->iText,(_phone)->iObj,(_text))
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of Phone Functions --- ****/

/****  --- Start of Connection Functions --- ****/
/*
This section contains the functions used to operate on RvSdpConnection objects.
The RvSdpConnection Type represents the connection ('c=') field of an SDP message.
*/

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpConnectionConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpConnection object.
 *      This function is obsolete. The rvSdpMsgAddConnection or
 *      rvSdpMediaDescrAddConnection should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to valid RvSdpConnection object.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpConnectionConstruct(
                RvSdpConnection* conn,
                RvSdpNetType nettype,
                RvSdpAddrType addrtype,
                const char* address);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionConstruct(_conn,_nettype,_addrtype,_address) \
                rvSdpConnectionConstruct2(NULL,(_conn),(_nettype),\
                                          (_addrtype),(_address),NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpConnection object.
 *      This function is obsolete. The rvSdpMsgAddConnection or
 *      rvSdpMediaDescrAddConnection should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to valid RvSdpConnection object.
 *      nettype - the network type.
 *      addrtype - the address type.
 *      addr - the connection address.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpConnectionConstructA(
                RvSdpConnection* conn,
                RvSdpNetType nettype,
                RvSdpAddrType addrtype,
                const char* address,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionConstructA(_conn,_nettype,_addrtype,_address,_a) \
                rvSdpConnectionConstruct2(NULL,(_conn),(_nettype),\
                                          (_addrtype),(_address),NULL,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a connection object and copies the values from a source
 *      connection field.
 *      This function is obsolete. The rvSdpMsgAddConnection or
 *      rvSdpMediaDescrAddConnection should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to connection to be constructed. Must point
 *             to valid memory.
 *      src - a source connection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpConnectionConstructCopy(
                RvSdpConnection* dest,
                const RvSdpConnection* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionConstructCopy(_dest,_src) \
                rvSdpConnectionCopy2((_dest),(_src),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a connection object and copies the values from a source
 *      connection field.
 *      This function is obsolete. The rvSdpMsgAddConnection or
 *      rvSdpMediaDescrAddConnection should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to connection to be constructed. Must point
 *             to valid memory.
 *      src - a source connection object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpConnection* rvSdpConnectionConstructCopyA(
                RvSdpConnection* dest,
                const RvSdpConnection* src,
                RvAlloc* a);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxConnectionConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpConnection object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxConnection or
 *      rvSdpMediaDescrSetBadSyntaxConnection should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to valid RvSdpConnection object.
 *      badSyn - the proprietary format of connection.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpBadSyntaxConnectionConstruct(
                RvSdpConnection* conn,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxConnectionConstruct(_conn,_badSyn) \
                rvSdpConnectionConstruct2(NULL,(_conn),0,0,NULL,(_badSyn),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxConnectionConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpConnection object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxConnection or
 *      rvSdpMediaDescrSetBadSyntaxConnection should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to valid RvSdpConnection object.
 *      badSyn - the proprietary format of connection.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpBadSyntaxConnectionConstructA(
                RvSdpConnection* conn,
                const char* badSyn,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxConnectionConstructA(_conn,_badSyn,_a) \
                rvSdpConnectionConstruct2(NULL,(_conn),0,0,NULL,(_badSyn),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted connection field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpConnectionGetBadSyntax(
                const RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionGetBadSyntax(_conn) RV_SDP_EMPTY_STRING((_conn)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the connection field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpConnectionIsBadSyntax(
                const RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionIsBadSyntax(_conn) ((_conn)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP connection field value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpConnection object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpConnectionSetBadSyntax(
            RvSdpConnection* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpConnectionDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the connection object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
***************************************************************************/
RVSDPCOREAPI void rvSdpConnectionDestruct(
                RvSdpConnection* conn);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpConnectionCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source connection object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination connection. Must point
 *             to constructed RvSdpConnection object.
 *      src - a source connection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpConnection* rvSdpConnectionCopy(
                RvSdpConnection* dest,
                const RvSdpConnection* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionCopy(_dest,_src) rvSdpConnectionCopy2((_dest),(_src),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/


/*
 *	Setters And Getters
 */


/***************************************************************************
 * rvSdpConnectionGetNetType
 * ------------------------------------------------------------------------
 * General:
 *      Get the connection's network type.
 * Return Value:
 *      Returns the connection's network type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpNetType rvSdpConnectionGetNetType(
                const RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionGetNetType(_conn) rvSdpNetTypeTxt2Val((_conn)->iNetTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionSetNetType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the connection's network type.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      type - the network type.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpConnectionSetNetType(
                RvSdpConnection* conn,
                RvSdpNetType type);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionSetNetType(_conn,_type) \
				rvSdpSetTextField(&(_conn)->iNetTypeStr,(_conn)->iObj,\
								  rvSdpNetTypeVal2Txt((_type)))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionGetNetTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Get the connection's network type string.
 * Return Value:
 *      Returns the connection's network type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpConnectionGetNetTypeStr(
                RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionGetNetTypeStr(_conn) RV_SDP_EMPTY_STRING((_conn)->iNetTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionSetNetTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the connection's network type string.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      type - the network type.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpConnectionSetNetTypeStr(
                RvSdpConnection* conn,
                const char* type);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionSetNetTypeStr(_conn,_netType) \
                rvSdpSetTextField(&(_conn)->iNetTypeStr,(_conn)->iObj,(_netType))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionGetAddrType
 * ------------------------------------------------------------------------
 * General:
 *      Get the connection's address type.
 * Return Value:
 *      Returns the connection's address type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAddrType rvSdpConnectionGetAddrType(
                const RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionGetAddrType(_conn) rvSdpAddrTypeTxt2Val((_conn)->iAddrTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionSetAddrType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the connection's address type.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      type - the address type.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpConnectionSetAddrType(
                RvSdpConnection* conn,
                RvSdpAddrType type);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionSetAddrType(_conn,_type) \
				rvSdpSetTextField(&(_conn)->iAddrTypeStr,(_conn)->iObj,\
								  rvSdpAddrTypeVal2Txt((_type)))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionGetAddrTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Get the connection's address type string.
 * Return Value:
 *      Returns the connection's address type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpConnectionGetAddrTypeStr(
                RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionGetAddrTypeStr(_conn) RV_SDP_EMPTY_STRING((_conn)->iAddrTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionSetAddrTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the connection's address type string.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      type - the address type string.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpConnectionSetAddrTypeStr(
                RvSdpConnection* conn,
                const char* type);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionSetAddrTypeStr(_conn,_addrType) \
                rvSdpSetTextField(&(_conn)->iAddrTypeStr,(_conn)->iObj,(_addrType))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionGetAddress
 * ------------------------------------------------------------------------
 * General:
 *      Get the connection's address.
 * Return Value:
 *      Returns the connection's address.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpConnectionGetAddress(
                const RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionGetAddress(_conn) RV_SDP_EMPTY_STRING((_conn)->iAddress)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionSetAddress
 * ------------------------------------------------------------------------
 * General:
 *      Sets the connection's address.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      addr - the connection's address.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpConnectionSetAddress(
                RvSdpConnection* conn,
                const char* addr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionSetAddress(_conn,_addr) \
                rvSdpSetTextField(&(_conn)->iAddress,(_conn)->iObj,(_addr))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionGetAddressTTL
 * ------------------------------------------------------------------------
 * General:
 *      Get the connection's address TTL.
 * Return Value:
 *      Returns the connection's address TTL.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI int rvSdpConnectionGetAddressTTL(
                const RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionGetAddressTTL(_conn) (_conn)->iTtl
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionSetAddressTTL
 * ------------------------------------------------------------------------
 * General:
 *      Sets the connection's address TTL.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      ttl - the connection's address TTL.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpConnectionSetAddressTTL(
                RvSdpConnection* conn,
                int ttl);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionSetAddressTTL(_conn,_ttl) (_conn)->iTtl = (_ttl)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionGetAddressNum
 * ------------------------------------------------------------------------
 * General:
 *      Get the connection's number of subsequent addresses.
 * Return Value:
 *      Returns the connection's number of subsequent addresses.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI int rvSdpConnectionGetAddressNum(
                const RvSdpConnection* conn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionGetAddressNum(_conn) (_conn)->iNumAddr
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpConnectionSetAddressNum
 * ------------------------------------------------------------------------
 * General:
 *      Sets the connection's number of subsequent addresses.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      conn - a pointer to the RvSdpConnection object.
 *      num - the connection's number of subsequent addresses.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpConnectionSetAddressNum(
                RvSdpConnection* conn,
                int num);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpConnectionSetAddressNum(_conn,_num) (_conn)->iNumAddr = (_num)
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of Connection Functions --- ****/

/****  --- Start of Bandwidth Functions --- ****/
/*
This section contains the functions used to operate on RvSdpBandwidth objects.
The RvSdpBandwidth Type represents the bandwidth ('b=') field of an SDP message.
*/

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpBandwidthConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpBandwidth object.
 *      This function is obsolete. The rvSdpMsgAddBandwidth or
 *      rvSdpMediaDescrAddBandwidth should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to valid RvSdpBandwidth object.
 *      type - the bandwidth type name.
 *      value - the bandwith value (in Kbs).
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpBandwidthConstruct(
                RvSdpBandwidth* bw,
                const char* type,
                RvUint32 value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthConstruct(_bw,_type,_value) \
                rvSdpBandwidthConstruct2(NULL,(_bw),(_type),(_value),NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpBandwidth object.
 *      This function is obsolete. The rvSdpMsgAddBandwidth or
 *      rvSdpMediaDescrAddBandwidth should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to valid RvSdpBandwidth object.
 *      type - the bandwidth type name.
 *      value - the bandwith value (in Kbs).
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpBandwidthConstructA(
                RvSdpBandwidth* bw,
                const char* type,
                RvUint32 value,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthConstructA(_bw,_type,_value,_a) \
                rvSdpBandwidthConstruct2(NULL,(_bw),(_type),(_value),NULL,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a bandwidth object and copies the values from a source
 *      bandwidth field.
 *      This function is obsolete. The rvSdpMsgAddBandwidth or
 *      rvSdpMediaDescrAddBandwidth should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to bandwidth to be constructed. Must point
 *             to valid memory.
 *      src - a source bandwidth object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpBandwidthConstructCopy(
                RvSdpBandwidth* dest,
                RvSdpBandwidth* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthConstructCopy(_dest,_src) \
                rvSdpBandwidthCopy2((_dest),(_src),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a bandwidth object and copies the values from a source
 *      bandwidth field.
 *      This function is obsolete. The rvSdpMsgAddBandwidth or
 *      rvSdpMediaDescrAddBandwidth should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to bandwidth to be constructed. Must point
 *             to valid memory.
 *      src - a source bandwidth object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpBandwidth* rvSdpBandwidthConstructCopyA(
                RvSdpBandwidth* dest,
                RvSdpBandwidth* src,
                RvAlloc* a);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpBadSyntaxBandwidthConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpBandwidth object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxBandwidth or
 *      rvSdpMediaDescrSetBadSyntaxBandwidth should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to valid RvSdpBandwidth object.
 *      badSyn - the proprietary format of bandwidth field.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpBadSyntaxBandwidthConstruct(
                RvSdpBandwidth* bw,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxBandwidthConstruct(_bw,_badSyn) \
                rvSdpBandwidthConstruct2(NULL,(_bw),NULL,0,(_badSyn),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxBandwidthConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpBandwidth object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxBandwidth or
 *      rvSdpMediaDescrSetBadSyntaxBandwidth should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to valid RvSdpBandwidth object.
 *      badSyn - the proprietary format of bandwidth field.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpBadSyntaxBandwidthConstructA(
                RvSdpBandwidth* bw,
                const char* badSyn,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxBandwidthConstructA(_bw,_badSyn,_a) \
                rvSdpBandwidthConstruct2(NULL,(_bw),NULL,0,(_badSyn),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted bandwidth field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpBandwidthGetBadSyntax(
                const RvSdpBandwidth* bw);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthGetBadSyntax(_bw) RV_SDP_EMPTY_STRING((_bw)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the bandwidth field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpBandwidthIsBadSyntax(
                RvSdpBandwidth* bw);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthIsBadSyntax(_bw) (((_bw)->iBadSyntaxField) ? RV_TRUE : RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP bandwidth field value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpBandwidth object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpBandwidthSetBadSyntax(
            RvSdpBandwidth* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpBandwidthDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the bandwidth object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
***************************************************************************/
RVSDPCOREAPI void rvSdpBandwidthDestruct(
                RvSdpBandwidth* bw);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpBandwidthCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source bandwidth object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination bandwidth. Must point
 *             to constructed RvSdpBandwidth object.
 *      src - a source bandwidth object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpBandwidth* rvSdpBandwidthCopy(
                RvSdpBandwidth* dest,
                RvSdpBandwidth* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthCopy(_dest,_src) rvSdpBandwidthCopy2((_dest),(_src),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Setters And Getters
 */

/***************************************************************************
 * rvSdpBandwidthGetType
 * ------------------------------------------------------------------------
 * General:
 *      Get the bandwidth type.
 * Return Value:
 *      Returns the bandwidth type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpBandwidthGetType(
                const RvSdpBandwidth* bw);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthGetType(_bw) RV_SDP_EMPTY_STRING((_bw)->iBWType)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthSetType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the bandwidth type.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 *      type - the bandwidth type.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpBandwidthSetType(
                RvSdpBandwidth* bw,
                const char* type);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthSetType(_bw,_type) \
                rvSdpSetTextField(&(_bw)->iBWType,(_bw)->iObj,(_type))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthGetValue
 * ------------------------------------------------------------------------
 * General:
 *      Get the bandwidth value.
 * Return Value:
 *      Returns the bandwidth value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint32 rvSdpBandwidthGetValue(
                const RvSdpBandwidth* bw);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthGetValue(_bw) (_bw)->iBWValue
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBandwidthSetValue
 * ------------------------------------------------------------------------
 * General:
 *      Sets the bandwidth value.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      bw - a pointer to the RvSdpBandwidth object.
 *      value - value bandwidth value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpBandwidthSetValue(
                RvSdpBandwidth* bw,
                RvUint32 value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBandwidthSetValue(_bw,_value) (_bw)->iBWValue = (_value)
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of Bandwidth Functions --- ****/

/****  --- Start of Session Time Functions --- ****/
/*
This section contains the functions used to operate on RvSdpSessionTime objects.
The RvSdpSessionTime Type represents the time ('t=') field of an SDP message.
*/

/*
 *	SDP Session Time API Functions
 */

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpSessionTimeConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpSessionTime object.
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used
 *      instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to valid RvSdpSessionTime object.
 *      start - the start time of the session.
 *      end - the end time of the session.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpSessionTimeConstruct(
                RvSdpSessionTime* sessTime,
                RvUint32 start,
                RvUint32 end);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeConstruct(_sessTime,_start,_end) \
                rvSdpSessionTimeConstruct2(NULL,(_sessTime),(_start),(_end),\
                                           NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpSessionTime object.
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used
 *      instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to valid RvSdpSessionTime object.
 *      start - the start time of the session.
 *      end - the end time of the session.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpSessionTimeConstructA(
                RvSdpSessionTime* sessTime,
                RvUint32 start,
                RvUint32 end,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeConstructA(_sessTime,_start,_end,_a) \
                rvSdpSessionTimeConstruct2(NULL,(_sessTime),(_start),(_end),\
                                           NULL,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a session time  object and copies the values from a source
 *      session time.
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used
 *      instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to session time to be constructed. Must point
 *             to valid memory.
 *      src - a source session time.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpSessionTimeConstructCopy(
                RvSdpSessionTime* dest,
                const RvSdpSessionTime* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeConstructCopy(_dest,_src) \
                rvSdpSessionTimeCopy2((_dest),(_src),NULL,RV_FALSE,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a session time  object and copies the values from a source
 *      session time.
 *      This function is obsolete. The 'rvSdpMsgAddSessionTime' should be used
 *      instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to session time to be constructed. Must point
 *             to valid memory.
 *      src - a source session time.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpSessionTime* rvSdpSessionTimeConstructCopyA(
                RvSdpSessionTime* dest,
                const RvSdpSessionTime* src,
                RvAlloc* a);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxSessionTimeConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpSessionTime object with proprietary format using
 *      default allocator.
 *      This function is obsolete. The 'rvSdpMsgAddBadSyntaxSessionTime' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpSessionTime object.
 *      badSyn - the proprietary format of session time.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpBadSyntaxSessionTimeConstruct(
                RvSdpSessionTime* sessTime,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxSessionTimeConstruct(_sessTime,_badSyn) \
                rvSdpSessionTimeConstruct2(NULL,(_sessTime),0,0,(_badSyn),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxSessionTimeConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpSessionTime object with proprietary format using
 *      provided allocator.
 *      This function is obsolete. The 'rvSdpMsgAddBadSyntaxSessionTime' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpSessionTime object.
 *      badSyn - the proprietary format of session time.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpBadSyntaxSessionTimeConstructA(
                RvSdpSessionTime* sessTime,
                const char* badSyn,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxSessionTimeConstructA(_sessTime,_badSyn,_a) \
                rvSdpSessionTimeConstruct2(NULL,(_sessTime),0,0,(_badSyn),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted session time field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpSessionTimeGetBadSyntax(
                const RvSdpSessionTime* sessTime);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeGetBadSyntax(_sessTime) \
                RV_SDP_EMPTY_STRING((_sessTime)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the session time field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpSessionTimeIsBadSyntax(
                RvSdpSessionTime* sessTime);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeIsBadSyntax(_sessTime) ((_sessTime)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP session time field value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpSessionTime object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpSessionTimeSetBadSyntax(
            RvSdpSessionTime* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpSessionTimeDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the session time object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpSessionTimeDestruct(
                RvSdpSessionTime* sessTime);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpSessionTimeCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source session time object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination session time. Must point
 *             to constructed RvSdpSessionTime object.
 *      src - a session time object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpSessionTime* rvSdpSessionTimeCopy(
                RvSdpSessionTime* dest,
                const RvSdpSessionTime* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeCopy(_dest,_src) \
                rvSdpSessionTimeCopy2((_dest),(_src),NULL,RV_TRUE,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/


/*
 *	Setters And Getters
 */

/***************************************************************************
 * rvSdpSessionTimeGetStart
 * ------------------------------------------------------------------------
 * General:
 *      Get the session start time.
 * Return Value:
 *      Returns the session start time.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to RvSdpSessionTime object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint32 rvSdpSessionTimeGetStart(
                RvSdpSessionTime* sessTime);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeGetStart(_sessTime) (_sessTime)->iStart
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeSetStart
 * ------------------------------------------------------------------------
 * General:
 *      Sets the session start time.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to RvSdpSessionTime object.
 *      start - the new session start time.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpSessionTimeSetStart(
                RvSdpSessionTime* sessTime,
                RvUint32 start);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeSetStart(_sessTime,_start) (_sessTime)->iStart = (_start)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeGetEnd
 * ------------------------------------------------------------------------
 * General:
 *      Get the session end time.
 * Return Value:
 *      Returns the session end time.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to RvSdpSessionTime object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint32 rvSdpSessionTimeGetEnd(
                RvSdpSessionTime* sessTime);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeGetEnd(_sessTime) (_sessTime)->iEnd
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeSetEnd
 * ------------------------------------------------------------------------
 * General:
 *      Sets the session end time.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      sessTime - a pointer to RvSdpSessionTime object.
 *      end - the new session end time.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpSessionTimeSetEnd(
                RvSdpSessionTime* sessTime,
                RvUint32 end);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeSetEnd(_sessTime,_end) (_sessTime)->iEnd = (_end)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeGetNumOfRepeatInterval
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of repeat intervals of session time object.
 *
 * Return Value:
 *      Number of repeat intervals defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpSessionTimeGetNumOfRepeatInterval(
                const RvSdpSessionTime* session);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeGetNumOfRepeatInterval(_session) \
                (_session)->iRepeatList.iListSize;
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeGetFirstRepeatInterval
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first repeat interval object defined in the session time.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      Pointer to the RvSdpRepeatInterval  object or the NULL pointer if there are no
 *      repeat intervals defined in the session time object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 *      iter - pointer to RvSdpListIter to be used for subsequent
 *             rvSdpSessionTimeGetNextRepeatInterval calls.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpSessionTimeGetFirstRepeatInterval(
                RvSdpSessionTime* session,
                RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeGetFirstRepeatInterval(_session,_iter) \
            (RvSdpRepeatInterval*)rvSdpListGetFirst(&(_session)->iRepeatList,(_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeGetNextRepeatInterval
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next repeat interval object defined in the session time.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      Pointer to the RvSdpRepeatInterval object or the NULL pointer if there is no
 *      more repeat intervals defined in the session time.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpSessionTime(GetFirst/Next)RepeatInterval function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpSessionTimeGetNextRepeatInterval(
                RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeGetNextRepeatInterval(_iter) \
                (RvSdpRepeatInterval*)rvSdpListGetNext((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpSessionTimeGetRepeatInterval
* ------------------------------------------------------------------------
* General:
*      Gets a repeat interval object by index (in session time context).
*
* Return Value:
*      The requested RvSdpRepeatInterval pointer.
* ------------------------------------------------------------------------
* Arguments:
*      session - a pointer to the RvSdpSessionTime object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by correspondent
*              rvSdpSessionTimeGetNumOfRepeatInterval() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpSessionTimeGetRepeatInterval(
                const RvSdpSessionTime* session,
                RvSize_t i);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeGetRepeatInterval(_session,_i) \
                rvSdpListGetByIndex(&(_session)->iRepeatList,(_i))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpSessionTimeAddRepeatInterval
* ------------------------------------------------------------------------
* General:
*      Adds new repeat interval to the session time.
*
* Return Value:
*      The pointer to added RvSdpRepeatInterval object or NULL if the function fails.
* ------------------------------------------------------------------------
* Arguments:
*      session - a pointer to the RvSdpSessionTime object.
*      time - the time length of the repeat times.
*      t_units - the time units of the repeat times.
*      duration - the length of the active duration.
*      d_units - the time units of the active duration.
**************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpSessionTimeAddRepeatInterval(
                RvSdpSessionTime* session,
                RvUint32 time,
                RvSdpTimeUnit t_units,
                RvUint32 duration ,
                RvSdpTimeUnit d_units);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeAddRepeatInterval(_session,_time,_t_units,_duration,_d_units) \
                rvSdpSessionTimeAddRepeatInterval2(NULL,(_session),(_time),(_t_units),\
                                                   (_duration),(_d_units),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeAddBadSyntaxRepeatInterval
 * ------------------------------------------------------------------------
 * General:
 *      Adds the new proprietary formatted RvSdpRepeatInterval object
 *      to the session time.
 *
 * Return Value:
 *      Returns the pointer to the newly created RvSdpRepeatInterval object if the
 *      function succeeds or NULL pointer if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 *      badSyn - the proprietary value of session time field.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpSessionTimeAddBadSyntaxRepeatInterval(
                RvSdpSessionTime* session,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeAddBadSyntaxRepeatInterval(_session,_badSyn) \
                rvSdpSessionTimeAddRepeatInterval2(NULL,(_session),0,0,0,0,(_badSyn))
#endif /*RV_SDP_USE_MACROS*/
#endif /*RV_SDP_CHECK_BAD_SYNTAX*/



/***************************************************************************
 * rvSdpSessionTimeRemoveCurrentRepeatInterval
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the repeat interval object pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpSessionTimeRemoveCurrentRepeatInterval(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeRemoveCurrentRepeatInterval(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeRemoveRepeatInterval
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the repeat interval object by index in the
 *      context of session time.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpSessionTimeGetNumOfRepeatInterval call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpSessionTimeRemoveRepeatInterval(
                RvSdpSessionTime* session,
                RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeRemoveRepeatInterval(_session,_index) \
                rvSdpListRemoveByIndex(&(_session)->iRepeatList,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpSessionTimeClearRepeatIntervals
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all repeat intervals set in session time object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      session - a pointer to the RvSdpSessionTime object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpSessionTimeClearRepeatIntervals(
                RvSdpSessionTime* session);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpSessionTimeClearRepeatIntervals(_session) \
                rvSdpListClear(&(_session)->iRepeatList)
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of Session Time Functions --- ****/

/****  --- Start of Time Repeat Intervals Functions --- ****/
/*
This section contains the functions used to operate on RvSdpRepeatInterval objects.
The RvSdpRepeatInterval Type represents the repeat interval ('r=') field of an SDP message.
*/

/*
 *	SDP Time Repeat Interval API Functions
 */

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpRepeatIntervalConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpRepeatInterval object.
 *      This function is obsolete. The rvSdpSessionTimeAddRepeatInterval should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to valid RvSdpRepeatInterval object.
 *      time - the length of repeat interval.
 *      t_units - the units of repeat interval length.
 *      duration - the length of session active duration.
 *      d_units - the units of session active duration length.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpRepeatIntervalConstruct(
                RvSdpRepeatInterval* interv,
                RvUint32 time,
                RvSdpTimeUnit t_units,
                RvUint32 duration,
                RvSdpTimeUnit d_units);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalConstruct(_interv,_time,_t_units,_duration,_d_units) \
                rvSdpRepeatIntervalConstruct2(NULL,(_interv),(_time),(_t_units),\
                                              (_duration),(_d_units),NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpRepeatInterval object.
 *      This function is obsolete. The rvSdpSessionTimeAddRepeatInterval should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to valid RvSdpRepeatInterval object.
 *      time - the length of repeat interval.
 *      t_units - the units of repeat interval length.
 *      duration - the length of session active duration.
 *      d_units - the units of session active duration length.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpRepeatIntervalConstructA(
                RvSdpRepeatInterval* interv,
                RvUint32 time,
                RvSdpTimeUnit t_units,
                RvUint32 duration,
                RvSdpTimeUnit d_units,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalConstructA(_interv,_time,_t_units,_duration,_d_units,_a) \
           rvSdpRepeatIntervalConstruct2(NULL,(_interv),(_time),(_t_units),(_duration),\
                                         (_d_units),NULL,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a repeat interval object and copies the values from a source
 *      repeat interval.
 *      This function is obsolete. The rvSdpSessionTimeAddRepeatInterval should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to repeat interval to be constructed. Must point
 *             to valid memory.
 *      src - a source repeat interval.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpRepeatIntervalConstructCopy(
                RvSdpRepeatInterval *d,
                const RvSdpRepeatInterval *s,
                RvAlloc* alloc);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalConstructCopy(_d,_s,_a) \
                rvSdpRepeatIntervalCopy2((_d),(_s),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/



/***************************************************************************
 * rvSdpBadSyntaxRepeatIntervalConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpRepeatInterval object with proprietary format.
 *      This function is obsolete. The rvSdpSessionTimeAddRepeatInterval should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpRepeatInterval object.
 *      badSyn - the proprietary format of repeat interval.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#if defined(RV_SDP_CHECK_BAD_SYNTAX)
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpRepeatIntervalBadSyntaxConstructA(
                RvSdpRepeatInterval* interv,
                char *badSyntax,
                RvAlloc* a);

/***************************************************************************
 * rvSdpBadSyntaxRepeatIntervalConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpRepeatInterval object with proprietary format.
 *      This function is obsolete. The rvSdpSessionTimeAddBadSyntaxRepeatInterval
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      descr - a pointer to valid RvSdpRepeatInterval object.
 *      badSyn - the proprietary format of repeat interval.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpRepeatIntervalBadSyntaxConstruct(
                RvSdpRepeatInterval* interv,
                char *badSyntax);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalBadSyntaxConstruct(_interv,_badSyntax) \
                rvSdpRepeatIntervalBadSyntaxConstructA((_interv),(_badSyntax),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted repeat interval field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpRepeatIntervalGetBadSyntax(
                RvSdpRepeatInterval* interv);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalGetBadSyntax(_interv) \
                RV_SDP_EMPTY_STRING((_interv)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the session repeat interval field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpRepeatIntervalIsBadSyntax(
                RvSdpRepeatInterval* interv);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalIsBadSyntax(_interv) \
                (((_interv)->iBadSyntaxField) ? RV_TRUE : RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP repeat interval field value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpRepeatInterval object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpRepeatIntervalSetBadSyntax(
            RvSdpRepeatInterval* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /*#if defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpRepeatIntervalDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the repeat interval object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpRepeatIntervalDestruct(
                RvSdpRepeatInterval* interv);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpRepeatIntervalCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source repeat interval object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination repeat interval. Must point
 *             to constructed RvSdpRepeatInterval object.
 *      src - a repeat interval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpRepeatIntervalCopy(
                RvSdpRepeatInterval *d,
                const RvSdpRepeatInterval *s);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalCopy(_d,_s) rvSdpRepeatIntervalCopy2((_d),(_s),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/


/*
 *	Setters And Getters
 */

/***************************************************************************
 * rvSdpRepeatIntervalGetNumOfOffset
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of offsets in repeat interval object.
 *
 * Return Value:
 *      Number of offsets defined.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpRepeatIntervalGetNumOfOffset(
               RvSdpRepeatInterval* repeat);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalGetNumOfOffset(_repeat) (_repeat)->iOffsetsList.iListSize
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalGetFirstOffset
 * ------------------------------------------------------------------------
 * General:
 *      Returns the first offset data defined in the repeat interval object.
 *      Also sets the list iterator for the further use.
 *
 * Return Value:
 *      RV_TRUE if there is at least one offset in the repeat interval object.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      i - pointer to RvSdpListIter to be used for subsequent
 *          rvSdpRepeatIntervalGetNextOffset calls.
 *      time - will be filled with offset time interval.
 *      t_unit - will be filled with offset's time units.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpRepeatIntervalGetFirstOffset(
                RvSdpRepeatInterval* repeat,
                RvSdpListIter* i,
                RvUint32* time,
                RvSdpTimeUnit* t_unit);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalGetFirstOffset(_repeat,_i,_time,_t_unit) \
            rvSdpBreakTT((RvSdpTypedTime*)rvSdpListGetFirst(&(_repeat)->iOffsetsList,\
                        (_i)),(_time),(_t_unit))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalGetNextOffset
 * ------------------------------------------------------------------------
 * General:
 *      Returns the next offset data defined in the repeat interval object.
 *      The 'next' object is defined based on the list iterator state.
 *
 * Return Value:
 *      RV_TRUE if the next offset exists, RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to rvSdpRepeatIntervalGetFirstOffset function.
 *      time - will be filled with offset time interval.
 *      t_unit - will be filled with offset's time units.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpRepeatIntervalGetNextOffset(
                RvSdpListIter* i,
                RvUint32* time,
                RvSdpTimeUnit* t_unit);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalGetNextOffset(_i,_time,_t_unit) \
                rvSdpBreakTT((RvSdpTypedTime*)rvSdpListGetNext(_i),(_time),(_t_unit))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalGetOffsetTime
 * ------------------------------------------------------------------------
 * General:
 *      Get the session start offset time by index.
 * Return Value:
 *      Returns the session start offset time by index.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      i - the index of the offset.
 ***************************************************************************/
RVSDPCOREAPI RvUint32 rvSdpRepeatIntervalGetOffsetTime(
                RvSdpRepeatInterval* repeat,
                RvSize_t i);

/***************************************************************************
 * rvSdpRepeatIntervalGetOffsetUnits
 * ------------------------------------------------------------------------
 * General:
 *      Get the session start offset time units by index.
 * Return Value:
 *      Returns the session start offset time units by index.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      i - the index of the offset.
 ***************************************************************************/
RVSDPCOREAPI RvSdpTimeUnit rvSdpRepeatIntervalGetOffsetUnits(
                RvSdpRepeatInterval* repeat,
                RvSize_t i);

/***************************************************************************
 * rvSdpRepeatIntervalAddOffset
 * ------------------------------------------------------------------------
 * General:
 *      Adds another session start offset time to the repeat interval object.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      time - the session start offset time length.
 *      units - the session start offset time units.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpRepeatIntervalAddOffset(
                RvSdpRepeatInterval* repeat,
                RvUint32 time,
                RvSdpTimeUnit units);

/***************************************************************************
 * rvSdpRepeatIntervalRemoveCurrentOffset
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the repeat interval offset pointed by list iterator.
 *      The value of iterator becomes undefined after the function call.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      iter - pointer to RvSdpListIter set/modified by previous successfull call
 *             to GetFirst/GetNext function.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRepeatIntervalRemoveCurrentOffset(
            RvSdpListIter* iter);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalRemoveCurrentOffset(_iter) \
            rvSdpListRemoveCurrent((_iter))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalRemoveOffset
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the repeat interval offset by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpRepeatIntervalGetNumOfOffset call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRepeatIntervalRemoveOffset(
                RvSdpRepeatInterval* repeat,
                RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalRemoveOffset(_repeat,_index) \
                rvSdpListRemoveByIndex(&(_repeat)->iOffsetsList,(_index))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpRepeatIntervalClearOffset
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all offsets set in repeat interval object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      repeat - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRepeatIntervalClearOffset(
                RvSdpRepeatInterval* repeat);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalClearOffset(_repeat) rvSdpListClear(&(_repeat)->iOffsetsList)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalGetDurationUnits
 * ------------------------------------------------------------------------
 * General:
 *      Get the session active duration units.
 * Return Value:
 *      Returns the session active duration units.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeUnit rvSdpRepeatIntervalGetDurationUnits(
                const RvSdpRepeatInterval* interv);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalGetDurationUnits(_interv) (_interv)->iDuration.iTimeType
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalSetDurationUnits
 * ------------------------------------------------------------------------
 * General:
 *      Sets the session active duration units.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 *      unit - the session active duration units.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRepeatIntervalSetDurationUnits(
                RvSdpRepeatInterval* interv,
                RvSdpTimeUnit unit);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalSetDurationUnits(_interv,_unit) \
                (_interv)->iDuration.iTimeType = (_unit)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalGetDurationTime
 * ------------------------------------------------------------------------
 * General:
 *      Get the session active duration time length.
 * Return Value:
 *      Returns the session active duration time length.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint32 rvSdpRepeatIntervalGetDurationTime(
                const RvSdpRepeatInterval* interv);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalGetDurationTime(_interv) (_interv)->iDuration.iTimeValue
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalSetDurationTime
 * ------------------------------------------------------------------------
 * General:
 *      Sets the session active duration time length.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 *      time - the session active duration time length.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRepeatIntervalSetDurationTime(
                RvSdpRepeatInterval* interv,
                RvUint32 time);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalSetDurationTime(_interv,_time) \
                (_interv)->iDuration.iTimeValue = (_time)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalGetIntervalUnits
 * ------------------------------------------------------------------------
 * General:
 *      Get the session repeat interval time units.
 * Return Value:
 *      Returns the session repeat interval time units.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeUnit   rvSdpRepeatIntervalGetIntervalUnits(
                const RvSdpRepeatInterval* interv);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalGetIntervalUnits(_interv) (_interv)->iInterval.iTimeType
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalSetIntervalUnits
 * ------------------------------------------------------------------------
 * General:
 *      Sets the session repeat interval length units.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 *      unit - the session repeat interval length units.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRepeatIntervalSetIntervalUnits(
                RvSdpRepeatInterval* interv,
                RvSdpTimeUnit unit);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalSetIntervalUnits(_interv,_unit) \
                (_interv)->iInterval.iTimeType = (_unit)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalGetIntervalTime
 * ------------------------------------------------------------------------
 * General:
 *      Get the session repeat interval time length.
 * Return Value:
 *      Returns the session repeat interval time length.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint32 rvSdpRepeatIntervalGetIntervalTime(
                const RvSdpRepeatInterval* interv);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalGetIntervalTime(_interv) (_interv)->iInterval.iTimeValue
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRepeatIntervalSetIntervalTime
 * ------------------------------------------------------------------------
 * General:
 *      Sets the session repeat interval time length.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      interv - a pointer to the RvSdpRepeatInterval object.
 *      time - the session repeat interval time length.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRepeatIntervalSetIntervalTime(
                RvSdpRepeatInterval* interv,
                RvUint32 time);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRepeatIntervalSetIntervalTime(_interv,_time) \
                (_interv)->iInterval.iTimeValue = (_time)
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of Time Repeat Intervals Functions --- ****/

/****  --- Start of Time Zone Adjustments Functions --- ****/
/*
This section contains the functions used to operate on RvSdpTimeZoneAdjust objects.
The RvSdpTimeZoneAdjust Type represents the time zone adjustments ('z=') field of
an SDP message.
*/

/*
 *	Constructors
 */


/***************************************************************************
 * rvSdpTimeZoneAdjustConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpTimeZoneAdjust object.
 *      This function is obsolete. The 'rvSdpMsgTimeAddZoneAdjustment' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to valid RvSdpTimeZoneAdjust object.
 *      t - when the time shift happens.
 *      offsetTime - the offset time.
 *      offsetUnits  - the units of the offset.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpTimeZoneAdjustConstruct(
                RvSdpTimeZoneAdjust* timeZone,
                RvUint32 t,
                RvInt32 offsetTime,
                RvSdpTimeUnit offsetUnits);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustConstruct(_timeZone,_t,_offsetTime,_offsetUnits) \
                rvSdpTimeZoneAdjustConstruct2(NULL,(_timeZone),(_t),\
                                    (_offsetTime),(_offsetUnits),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpTimeZoneAdjustConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpTimeZoneAdjust object.
 *      This function is obsolete. The 'rvSdpMsgTimeAddZoneAdjustment' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to valid RvSdpTimeZoneAdjust object.
 *      t - when the time shift happens.
 *      offsetTime - the offset time.
 *      offsetUnits  - the units of the offset.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpTimeZoneAdjustConstructA(
                RvSdpTimeZoneAdjust* timeZone,
                RvUint32 t,
                RvInt32 offsetTime,
                RvSdpTimeUnit offsetUnits,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustConstructA(_timeZone,_t,_offsetTime,_offsetUnits,_a) \
                rvSdpTimeZoneAdjustConstruct2(NULL,(_timeZone),(_t),\
                                (_offsetTime),(_offsetUnits),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpTimeZoneAdjustConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a time zone adjustment object and copies the values from
 *      a source time zone adjustment.
 *      This function is obsolete. The 'rvSdpMsgTimeAddZoneAdjustment' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to time zone adjustment to be constructed. Must point
 *             to valid memory.
 *      src - a source time zone adjustment.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpTimeZoneAdjustConstructCopy(
                RvSdpTimeZoneAdjust* dest,
                const RvSdpTimeZoneAdjust* src);
#else /*RV_SDP_USE_MACROS*/
#define  rvSdpTimeZoneAdjustConstructCopy(_dest,_src) \
                rvSdpTimeZoneAdjustConstruct2(NULL,(_dest),(_src)->iAdjustmentTime,\
                    (_src)->iOffsetTime,(_src)->iOffsetUnits,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpTimeZoneAdjustConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a time zone adjustment object and copies the values from
 *      a source time zone adjustment.
 *      This function is obsolete. The 'rvSdpMsgTimeAddZoneAdjustment' should
 *      be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to time zone adjustment to be constructed. Must point
 *             to valid memory.
 *      src - a source time zone adjustment.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpTimeZoneAdjustConstructCopyA(
                RvSdpTimeZoneAdjust* dest,
                const RvSdpTimeZoneAdjust* src,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustConstructCopyA(_dest,_src,_a) \
                rvSdpTimeZoneAdjustConstruct2(NULL,(_dest),(_src)->iAdjustmentTime,\
                        (_src)->iOffsetTime,(_src)->iOffsetUnits,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpTimeZoneAdjustDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the time zone adjustment object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpTimeZoneAdjustDestruct(
                RvSdpTimeZoneAdjust* timeZone);


/*
 *	Copy
 */

/***************************************************************************
 * rvSdpTimeZoneAdjustCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source time zone adjustment object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination time zone adjustment. Must point
 *             to constructed RvSdpTimeZoneAdjust object.
 *      src - a time zone adjustment object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeZoneAdjust* rvSdpTimeZoneAdjustCopy(
                RvSdpTimeZoneAdjust* dest,
                const RvSdpTimeZoneAdjust* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustCopy(_dest,_src) \
                rvSdpTimeZoneAdjustConstruct2(NULL,(_dest),(_src)->iAdjustmentTime,\
                        (_src)->iOffsetTime,(_src)->iOffsetUnits,NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Setters And Getters
 */

/***************************************************************************
 * rvSdpTimeZoneAdjustGetTime
 * ------------------------------------------------------------------------
 * General:
 *      Get the time of time-shift event.
 * Return Value:
 *      Returns the time of time-shift event.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint32 rvSdpTimeZoneAdjustGetTime(
                const RvSdpTimeZoneAdjust* timeZone);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustGetTime(_timeZone) (_timeZone)->iAdjustmentTime
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpTimeZoneAdjustSetTime
 * ------------------------------------------------------------------------
 * General:
 *      Sets the time of time-shift event.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 *      t - the time-shift event time.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpTimeZoneAdjustSetTime(
                RvSdpTimeZoneAdjust* timeZone,
                RvUint32 t);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustSetTime(_timeZone,_t) (_timeZone)->iAdjustmentTime = (_t)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpTimeZoneAdjustGetOffsetTime
 * ------------------------------------------------------------------------
 * General:
 *      Get the length of time-shift.
 * Return Value:
 *      Returns the length of time-shift.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvInt32 rvSdpTimeZoneAdjustGetOffsetTime(
                const RvSdpTimeZoneAdjust* timeZone);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustGetOffsetTime(_timeZone) (_timeZone)->iOffsetTime
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpTimeZoneAdjustSetOffsetTime
 * ------------------------------------------------------------------------
 * General:
 *      Sets the length of time-shift.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 *      offsetTime - the new offset length.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpTimeZoneAdjustSetOffsetTime(
                RvSdpTimeZoneAdjust* timeZone,
                RvInt32 offsetTime);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustSetOffsetTime(_timeZone,_offsetTime) \
                (_timeZone)->iOffsetTime = (_offsetTime)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpTimeZoneAdjustGetOffsetUnits
 * ------------------------------------------------------------------------
 * General:
 *      Get the units of time-shift length.
 * Return Value:
 *      Returns the units of time-shift length.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpTimeUnit rvSdpTimeZoneAdjustGetOffsetUnits(
                const RvSdpTimeZoneAdjust* timeZone);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustGetOffsetUnits(_timeZone) (_timeZone)->iOffsetUnits;
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpTimeZoneAdjustSetOffsetUnits
 * ------------------------------------------------------------------------
 * General:
 *      Sets the units of time-shift length.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      timeZone - a pointer to the RvSdpTimeZoneAdjust object.
 *      offsetUnits - the new offset length units.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpTimeZoneAdjustSetOffsetUnits(
                RvSdpTimeZoneAdjust* timeZone,
                RvSdpTimeUnit offsetUnits);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpTimeZoneAdjustSetOffsetUnits(_timeZone,_offsetUnits) \
                (_timeZone)->iOffsetUnits = (_offsetUnits)
#endif /*RV_SDP_USE_MACROS*/


/****  --- End of Time Zone Adjustments Functions --- ****/

/****  --- Start of Key Functions --- ****/
/*
This section contains the functions used to operate on RvSdpKey objects.
The RvSdpKey Type represents the key ('k=') field of
an SDP message.
*/

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpKeyConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpKey object.
 *      This function is obsolete. The rvSdpMsgSetKey or rvSdpMediaDescrSetKey
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to valid RvSdpKey object.
 *      type - the encryption method type.
 *      data - the encryption data.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpKeyConstruct(
                RvSdpKey* key,
                RvSdpEncrMethod type,
                const char* data);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyConstruct(_key,_type,_data) \
                rvSdpKeyConstruct2(NULL,(_key),(_type),(_data),NULL,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpKey object.
 *      This function is obsolete. The rvSdpMsgSetKey or rvSdpMediaDescrSetKey
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to valid RvSdpKey object.
 *      type - the encryption method type.
 *      data - the encryption data.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpKeyConstructA(
                RvSdpKey* key,
                RvSdpEncrMethod type,
                const char* data,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyConstructA(_key,_type,_data,_a) \
                rvSdpKeyConstruct2(NULL,(_key),(_type),(_data),NULL,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a key object and copies the values from a source
 *      key field.
 *      This function is obsolete. The rvSdpMsgSetKey or rvSdpMediaDescrSetKey
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to key to be constructed. Must point
 *             to valid memory.
 *      src - a source key object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpKeyConstructCopy(
                RvSdpKey* dest,
                const RvSdpKey* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyConstructCopy(_dest,_src) rvSdpKeyCopy2((_dest),(_src),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a key object and copies the values from a source
 *      key field.
 *      This function is obsolete. The rvSdpMsgSetKey or rvSdpMediaDescrSetKey
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to key to be constructed. Must point
 *             to valid memory.
 *      src - a source key object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpKeyConstructCopyA(
                RvSdpKey* dest,
                const RvSdpKey* src,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyConstructCopyA(_dest,_src,_a) rvSdpKeyCopy2((_dest),(_src),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxKeyConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpKey object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxKey or
 *      rvSdpMediaDescrSetBadSyntaxKey should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to valid RvSdpKey object.
 *      badSyn - the proprietary format of repeat interval.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpBadSyntaxKeyConstruct(
                RvSdpKey* key,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxKeyConstruct(_key,_badSyn) \
                rvSdpKeyConstruct2(NULL,(_key),0,NULL,(_badSyn),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxKeyConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpKey object with proprietary format.
 *      This function is obsolete. The rvSdpMsgSetBadSyntaxKey or
 *      rvSdpMediaDescrSetBadSyntaxKey should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to valid RvSdpKey object.
 *      badSyn - the proprietary format of key.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpBadSyntaxKeyConstructA(
                RvSdpKey* key,
                const char* badSyn,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxKeyConstructA(_key,_badSyn,_a) \
                rvSdpKeyConstruct2(NULL,(_key),0,NULL,(_badSyn),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted encryption key field value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char*  rvSdpKeyGetBadSyntax(
                const RvSdpKey* key);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyGetBadSyntax(_key) RV_SDP_EMPTY_STRING((_key)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the encryption key field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpKeyIsBadSyntax(
                RvSdpKey* key);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyIsBadSyntax(_key) ((_key)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeySetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP encryption key field value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpKey object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpKeySetBadSyntax(
            RvSdpKey* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeySetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /*RV_SDP_CHECK_BAD_SYNTAX*/

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpKeyDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the key object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpKeyDestruct(
                RvSdpKey* key);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpKeyCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source key object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination key. Must point
 *             to constructed RvSdpKey object.
 *      src - a key object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKey* rvSdpKeyCopy(
                RvSdpKey* dest,
                const RvSdpKey* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyCopy(_dest,_src) rvSdpKeyCopy2((_dest),(_src),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/


/*
 *	Setters And Getters
 */

/**************************************************************************
 * rvSdpKeyGetType
 * ------------------------------------------------------------------------
 * General:
 *      Get the key encryption type.
 * Return Value:
 *      Returns the key encryption type.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpEncrMethod rvSdpKeyGetType(
                const RvSdpKey* key);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyGetType(_key) rvSdpKeyTypeTxt2Val((_key)->iTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/**************************************************************************
 * rvSdpKeySetType
 * ------------------------------------------------------------------------
 * General:
 *      Sets the key encryption type.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 *      type - the encryption type.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpKeySetType(
                RvSdpKey* key,
                RvSdpEncrMethod type);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeySetType(_key,_type) \
				rvSdpSetTextField(&(_key)->iTypeStr,(_key)->iObj,\
                                  rvSdpKeyTypeVal2Txt(_type))
#endif /*RV_SDP_USE_MACROS*/

/**************************************************************************
 * rvSdpKeyGetTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Get the key encryption type string.
 * Return Value:
 *      Returns the key encryption type string.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpKeyGetTypeStr(
                RvSdpKey* key);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyGetTypeStr(_key) RV_SDP_EMPTY_STRING((_key)->iTypeStr)
#endif /*RV_SDP_USE_MACROS*/

/**************************************************************************
 * rvSdpKeySetTypeStr
 * ------------------------------------------------------------------------
 * General:
 *      Sets the key encryption type string.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 *      typeStr - the encryption type string.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpKeySetTypeStr(
                RvSdpKey* key,
                const char* typeStr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeySetTypeStr(_key,_typeStr) \
				rvSdpSetTextField(&(_key)->iTypeStr,(_key)->iObj,(_typeStr))
#endif /*RV_SDP_USE_MACROS*/

/**************************************************************************
 * rvSdpKeyGetData
 * ------------------------------------------------------------------------
 * General:
 *      Get the key encryption data.
 * Return Value:
 *      Returns the key encryption data.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpKeyGetData(
                const RvSdpKey* key);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyGetData(_key) RV_SDP_EMPTY_STRING((_key)->iData)
#endif /*RV_SDP_USE_MACROS*/

/**************************************************************************
 * rvSdpKeySetData
 * ------------------------------------------------------------------------
 * General:
 *      Sets the key encryption data.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      key - a pointer to the RvSdpKey object.
 *      data - the encryption data.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpKeySetData(
                RvSdpKey* key,
                const char* data);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeySetData(_key,_data) \
                rvSdpSetTextField(&(_key)->iData,(_key)->iObj,(_data))
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of Key Functions --- ****/


/****  --- Start of Attribute Functions --- ****/
/*
This section contains the functions used to operate on RvSdpAttribute objects.
The RvSdpAttribute Type represents the attribute ('a=') field of
an SDP message.
*/

/*
 *	SDP Attribute functions
 */

 /*
  *	Constructors
  */

/***************************************************************************
 * rvSdpAttributeConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpAttribute object using default allocator.
 *      This function is obsolete. The 'rvSdpMsgAddAttr' or 'rvSdpMediaDescrAddAttr'
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to valid RvSdpAttribute object.
 *      name - the name of the attribute.
 *      value - the value of the attribute.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpAttributeConstruct(
        RvSdpAttribute* attr,
        const char* name,
        const char* value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpAttributeConstruct(_attr,_name,_value) \
        rvSdpAttributeConstruct2(NULL,(_attr),(_name),(_value),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpAttributeConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpAttribute object.
 *      This function is obsolete. The 'rvSdpMsgAddAttr' or 'rvSdpMediaDescrAddAttr'
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to valid RvSdpAttribute object.
 *      name - the name of the attribute.
 *      value - the value of the attribute.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpAttributeConstructA(
        RvSdpAttribute* attr,
        const char* name,
        const char* value,
        RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpAttributeConstructA(_attr,_name,_value,_a) \
        rvSdpAttributeConstruct2(NULL,(_attr),(_name),(_value),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpAttributeConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs an attribute  object and copies the values from a source
 *      attribute.
 *      This function is obsolete. The 'rvSdpMsgAddAttr' or 'rvSdpMediaDescrAddAttr'
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to attribute to be constructed. Must point
 *             to valid memory.
 *      src - a source attribute.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpAttributeConstructCopy(
        RvSdpAttribute* dest,
        const RvSdpAttribute* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpAttributeConstructCopy(_dest,_src) \
        rvSdpAttributeConstruct2(NULL,(_dest),(_src)->iAttrName,\
                                 (_src)->iAttrValue,NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpAttributeConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs an attribute  object and copies the values from a source
 *      attribute.
 *      This function is obsolete. The 'rvSdpMsgAddAttr' or 'rvSdpMediaDescrAddAttr'
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to attribute to be constructed. Must point
 *             to valid memory.
 *      src - a source attribute.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpAttribute* rvSdpAttributeConstructCopyA(
        RvSdpAttribute* dest,
        const RvSdpAttribute* src,
        RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpAttributeConstructCopyA(_dest,_src,_a) \
        rvSdpAttributeConstruct2(NULL,(_dest),(_src)->iAttrName,\
                                 (_src)->iAttrValue,(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpAttributeDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the media descriptor object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to the RvSdpAttribute object.
***************************************************************************/
RVSDPCOREAPI void rvSdpAttributeDestruct(
        RvSdpAttribute* attr);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpAttributeCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source attribute to destination.
 *      Only generic attributes can be copied in this way. If one of the
 *      arguments is special attribute NULL will be returned.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination generic attribute. Must point
 *             to constructed RvSdpAttribute object.
 *      src - a source generic attribute.
 ***************************************************************************/
RVSDPCOREAPI RvSdpAttribute* rvSdpAttributeCopy(
        RvSdpAttribute* dest,
        const RvSdpAttribute* src);

/*
 *	Setters & Getters
 */

/***************************************************************************
 * rvSdpAttributeGetName
 * ------------------------------------------------------------------------
 * General:
 *      Get the attribute name.
 * Return Value:
 *      Returns the attribute name.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to RvSdpAttribute object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpAttributeGetName(
        RvSdpAttribute* attr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpAttributeGetName(_attr) RV_SDP_EMPTY_STRING(_attr->iAttrName)
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpAttributeSetName
 * ------------------------------------------------------------------------
 * General:
 *      Sets the attribute name.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to RvSdpAttribute object.
 *      name - the new attribute's name.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpAttributeSetName(
        RvSdpAttribute* a,
        const char* name);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpAttributeSetName(_attr,_name) \
    ((_attr)->iSpecAttrData != NULL) ? RV_SDPSTATUS_ILLEGAL_SET : \
        rvSdpSetTextField(&(_attr)->iAttrName,(_attr)->iObj,_name)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpAttributeGetValue
 * ------------------------------------------------------------------------
 * General:
 *      Get the attribute value or the empty string if the value is not set.
 * Return Value:
 *      Returns the attribute value or the empty string if the value is not set.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to RvSdpAttribute object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpAttributeGetValue(
        RvSdpAttribute* attr);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpAttributeGetValue(_attr) \
        (((_attr)->iSpecAttrData == NULL) ? RV_SDP_EMPTY_STRING(((_attr)->iAttrValue)) \
                                            : rvSdpSpecAttrGetValue(_attr))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpAttributeSetValue
 * ------------------------------------------------------------------------
 * General:
 *      Sets the attribute value.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      attr - a pointer to RvSdpAttribute object.
 *      name - the new attribute's value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpAttributeSetValue(
        RvSdpAttribute* attr,
        const char* value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpAttributeSetValue(_attr,_value) \
        ((_attr)->iSpecAttrData != NULL) ? RV_SDPSTATUS_ILLEGAL_SET : \
                       rvSdpSetTextField(&(_attr)->iAttrValue,(_attr)->iObj,_value)
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of Attribute Functions --- ****/


/****  --- Start of RTP Map Attribute Functions --- ****/
/*
This section contains the functions used to operate on RvSdpRtpMap objects.
The RvSdpRtpMap Type represents the RTP map attribute ('a=rtpmap:') field of
an SDP message.
*/

/*
 *	SDP Rtp map API Functions
 */

/*
 *	Constructors
 */

/***************************************************************************
 * rvSdpRtpMapConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpRtpMap object.
 *      This function is obsolete. The rvSdpMsgAddRtpMap or
 *      rvSdpMediaDescrAddRtpMap should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to valid RvSdpRtpMap object.
 *      payload - the RTP map payload number.
 *      encoding_name - the RTP map encoding name.
 *      rate - the RTP map rate value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpRtpMapConstruct(
                RvSdpRtpMap* rtpMap,
                int payload,
                const char* encoding_name,
                int rate);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapConstruct(_rtpMap,_payload,_encoding_name,_rate) \
                rvSdpRtpMapConstruct2(NULL,(_rtpMap),(_payload),\
                            (_encoding_name),(_rate),NULL,NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpRtpMap object.
 *      This function is obsolete. The rvSdpMsgAddRtpMap or
 *      rvSdpMediaDescrAddRtpMap should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to valid RvSdpRtpMap object.
 *      payload - the RTP map payload number.
 *      encoding_name - the RTP map encoding name.
 *      rate - the RTP map rate value.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpRtpMapConstructA(
                RvSdpRtpMap* rtpMap,
                int payload,
                const char* encoding_name,
                int rate,
                RvAlloc* alloc);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapConstructA(_rtpMap,_payload,_encoding_name,_rate,_a) \
                rvSdpRtpMapConstruct2(NULL,(_rtpMap),(_payload),\
                                    (_encoding_name),(_rate),NULL,(_a))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a RTP map object and copies the values from a source
 *      RTP map field.
 *      This function is obsolete. The rvSdpMsgAddRtpMap or
 *      rvSdpMediaDescrAddRtpMap should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to RTP map to be constructed. Must point
 *             to valid memory.
 *      src - a source RTP map object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpRtpMapConstructCopy(
                RvSdpRtpMap* dest,
                const RvSdpRtpMap* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapConstructCopy(_dest,_src) \
                rvSdpRtpMapCopy2((_dest),(_src),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs a RTP map object and copies the values from a source
 *      RTP map field.
 *      This function is obsolete. The rvSdpMsgAddRtpMap or
 *      rvSdpMediaDescrAddRtpMap should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to RTP map to be constructed. Must point
 *             to valid memory.
 *      src - a source RTP map object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpRtpMapConstructCopyA(
                RvSdpRtpMap* dest,
                const RvSdpRtpMap* src,
                RvAlloc* alloc);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapConstructCopyA(_dest,_src,_a) \
                rvSdpRtpMapCopy2((_dest),(_src),(_a))
#endif /*RV_SDP_USE_MACROS*/

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpBadSyntaxRtpMapConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpRtpMap object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxRtpMap or
 *      rvSdpMediaDescrAddBadSyntaxRtpMap should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to valid RvSdpRtpMap object.
 *      badSyn - the proprietary format of RTP map field.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpBadSyntaxRtpMapConstruct(
                RvSdpRtpMap* rtpMap,
                const char* badSyn);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxRtpMapConstruct(_rtpMap,_badSyn) \
                rvSdpRtpMapConstruct2(NULL,(_rtpMap),0,NULL,0,(_badSyn),NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpBadSyntaxRtpMapConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpRtpMap object with proprietary format.
 *      This function is obsolete. The rvSdpMsgAddBadSyntaxRtpMap or
 *      rvSdpMediaDescrAddBadSyntaxRtpMap should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to valid RvSdpRtpMap object.
 *      badSyn - the proprietary format of RTP map field.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpRtpMap* rvSdpBadSyntaxRtpMapConstructA(
                RvSdpRtpMap* rtpMap,
                const char* badSyn,
                RvAlloc* alloc);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpBadSyntaxRtpMapConstructA(_rtpMap,_badSyn,_a) \
                rvSdpRtpMapConstruct2(NULL,(_rtpMap),0,NULL,0,(_badSyn),(_a))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted RTP map attribute value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpRtpMapGetBadSyntax(
                const RvSdpRtpMap* rtpMap);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapGetBadSyntax(_rtpMap) RV_SDP_EMPTY_STRING((_rtpMap)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the RTP map attribute field is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpRtpMapIsBadSyntax(
                RvSdpRtpMap* rtpMap);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapIsBadSyntax(_rtpMap) ((_rtpMap)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP RTP map attribute value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpRtpMap object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpRtpMapSetBadSyntax(
            RvSdpRtpMap* o,
            const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpRtpMapDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the RTP map object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
***************************************************************************/
RVSDPCOREAPI void rvSdpRtpMapDestruct(
                RvSdpRtpMap* rtpMap);

/*
 *	Copy
 */

/***************************************************************************
 * rvSdpRtpMapCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source RTP map object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination RTP map. Must point
 *             to constructed RvSdpRtpMap object.
 *      src - a RTP map object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpRtpMap* rvSdpRtpMapCopy(
                RvSdpRtpMap* dest,
                const RvSdpRtpMap* src);


/*
 *	Setters And Getters
 */

/***************************************************************************
 * rvSdpRtpMapGetChannels
 * ------------------------------------------------------------------------
 * General:
 *      Get the number of channels. Used for media type audio instead of
 *      rvSdpRtpMapGetEncodingParameters().
 * Return Value:
 *      Returns the number of channels.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
RVSDPCOREAPI int rvSdpRtpMapGetChannels(
                const RvSdpRtpMap* rtpMap);

/***************************************************************************
 * rvSdpRtpMapSetChannels
 * ------------------------------------------------------------------------
 * General:
 *      Sets the number of channels. Used for media type audio instead of
 *      rvSdpRtpMapSetEncodingParameters().
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      channels - the number of channels.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpRtpMapSetChannels(
                RvSdpRtpMap* rtpMap,
                int channels);

/***************************************************************************
 * rvSdpRtpMapGetPayload
 * ------------------------------------------------------------------------
 * General:
 *      Get the payload number.
 * Return Value:
 *      Returns the payload number.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI int rvSdpRtpMapGetPayload(
                const RvSdpRtpMap* rtpMap);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapGetPayload(_rtpMap) (_rtpMap)->iPayload
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapSetPayload
 * ------------------------------------------------------------------------
 * General:
 *      Sets the payload number.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      payload - the payload number.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRtpMapSetPayload(
                RvSdpRtpMap* rtpMap,
                int payload);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapSetPayload(_rtpMap,_payload) (_rtpMap)->iPayload = (_payload)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapGetEncodingName
 * ------------------------------------------------------------------------
 * General:
 *      Get the RTP map encoding name.
 * Return Value:
 *      Returns the RTP map encoding name.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpRtpMapGetEncodingName(
                const RvSdpRtpMap* rtpMap);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapGetEncodingName(_rtpMap) RV_SDP_EMPTY_STRING((_rtpMap)->iEncName)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapSetEncodingName
 * ------------------------------------------------------------------------
 * General:
 *      Sets the RTP map emcoding name.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      name - the RTP map emcoding name.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpRtpMapSetEncodingName(
                RvSdpRtpMap* rtpMap,
                const char* name);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapSetEncodingName(_rtpMap,_name) \
                rvSdpSetTextField(&(_rtpMap)->iEncName,(_rtpMap)->iObj,(_name))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapGetClockRate
 * ------------------------------------------------------------------------
 * General:
 *      Get the RTP map clock-rate.
 * Return Value:
 *      Returns the RTP map clock-rate.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint32 rvSdpRtpMapGetClockRate(
                const RvSdpRtpMap* rtpMap);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapGetClockRate(_rtpMap) \
                ((_rtpMap)->iClockRate==RV_SDP_INT_NOT_SET) ? RV_SDP_DEFAULT_CLOCK_RATE : \
                                             (_rtpMap)->iClockRate
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapSetClockRate
 * ------------------------------------------------------------------------
 * General:
 *      Sets the RTP map clock-rate.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      rate - the RTP map clock-rate.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpRtpMapSetClockRate(
                RvSdpRtpMap* rtpMap,
                RvUint32 rate);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapSetClockRate(_rtpMap,_rate) (_rtpMap)->iClockRate = (_rate)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapGetEncodingParameters
 * ------------------------------------------------------------------------
 * General:
 *      Get the RTP map encoding parameters.
 * Return Value:
 *      Returns the RTP map encoding parameters.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpRtpMapGetEncodingParameters(
                const RvSdpRtpMap* rtpMap);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapGetEncodingParameters(_rtpMap) \
                RV_SDP_EMPTY_STRING((_rtpMap)->iEncParameters)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpRtpMapSetEncodingParameters
 * ------------------------------------------------------------------------
 * General:
 *      Sets the RTP map encoding parameters.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      rtpMap - a pointer to the RvSdpRtpMap object.
 *      s - the RTP map encoding parameters.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpRtpMapSetEncodingParameters(
                RvSdpRtpMap* rtpMap,
                const char* s);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpRtpMapSetEncodingParameters(_rtpMap,_s) \
                rvSdpSetTextField(&(_rtpMap)->iEncParameters,(_rtpMap)->iObj,(_s))
#endif /*RV_SDP_USE_MACROS*/

/****  --- End of RTP Map Attribute Functions --- ****/

/****  --- Start of Key Management Attribute Functions --- ****/
/*
This section contains the functions used to operate on RvSdpKeyMgmtAttr objects.
The RvSdpKeyMgmtAttr Type represents the key management attribute ('a=key-mgmt:')
field of an SDP message.
These functions can be used only if RV_SDP_KEY_MGMT_ATTR (defined in rvsdpconfig.h file)
compilation switch is enabled.
*/

#if defined(RV_SDP_KEY_MGMT_ATTR)

/***************************************************************************
 * rvSdpKeyMgmtGetPrtclIdTxt
 * ------------------------------------------------------------------------
 * General:
 *      Gets the protocol ID text value of the key-mgmt attribute.
 *
 * Return Value:
 *      The requested field of the key-mgmt attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpKeyMgmtGetPrtclIdTxt(
            const RvSdpKeyMgmtAttr* keyMgmt);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtGetPrtclIdTxt(_keyMgmt) RV_SDP_EMPTY_STRING((_keyMgmt)->iPrtclId)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyMgmtSetPrtclIdTxt
 * ------------------------------------------------------------------------
 * General:
 *      Sets the protocol ID text value of the key-mgmt attribute.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 *      prtclId - the new value of protocol ID text.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpKeyMgmtSetPrtclIdTxt(
            RvSdpKeyMgmtAttr* keyMgmt,
            const char* prtclId);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtSetPrtclIdTxt(_keyMgmt,_prtclId) \
            rvSdpSetTextField(&(_keyMgmt)->iPrtclId,(_keyMgmt)->iObj,(_prtclId))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyMgmtGetPrtclId
 * ------------------------------------------------------------------------
 * General:
 *      Gets the protocol ID value of the key-mgmt attribute.
 *
 * Return Value:
 *      The requested field of the key-mgmt attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpKeyMgmtPrtclType rvSdpKeyMgmtGetPrtclId(
            const RvSdpKeyMgmtAttr* keyMgmt);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtGetPrtclId(_keyMgmt) \
            rvSdpKeyMgmtPrtclTypeTxt2Val((_keyMgmt)->iPrtclId)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyMgmtSetPrtclId
 * ------------------------------------------------------------------------
 * General:
 *      Sets the protocol ID value of the key-mgmt attribute.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 *      prtclId - the new value of protocol ID.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpKeyMgmtSetPrtclId(
            RvSdpKeyMgmtAttr* keyMgmt,
            RvSdpKeyMgmtPrtclType prtclId);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtSetPrtclId(_keyMgmt,_prtclId) \
            rvSdpSetTextField(&(_keyMgmt)->iPrtclId,(_keyMgmt)->iObj,\
                              rvSdpKeyMgmtPrtclTypeVal2Txt(_prtclId))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpKeyMgmtGetKeyData
 * ------------------------------------------------------------------------
 * General:
 *      Gets the encryption key data value of the key-mgmt attribute.
 *
 * Return Value:
 *      The requested field of the key-mgmt attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpKeyMgmtGetKeyData(
            const RvSdpKeyMgmtAttr* keyMgmt);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtGetKeyData(_keyMgmt) RV_SDP_EMPTY_STRING((_keyMgmt)->iKeyData)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyMgmtSetKeyData
 * ------------------------------------------------------------------------
 * General:
 *      Sets the key data value of the key-mgmt attribute.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 *      keyData - the new value of protocol ID.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpKeyMgmtSetKeyData(
            RvSdpKeyMgmtAttr* keyMgmt,
            const char* keyData);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtSetKeyData(_keyMgmt,_keyData) \
            rvSdpSetTextField(&(_keyMgmt)->iKeyData,(_keyMgmt)->iObj,(_keyData))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyMgmtDecodeKeyData
 * ------------------------------------------------------------------------
 * General:
 *      Decodes (using B64 decoding) key data of key-mgmt attribute.
 *
 * Return Value:
 *      The size of decoded buffer.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 *      decodedData - the output buffer for B64 decoding.
 *      dataLen - the size 'decodedData' buffer.
 ***************************************************************************/
RVSDPCOREAPI RvSize_t rvSdpKeyMgmtDecodeKeyData(
            RvSdpKeyMgmtAttr* keyMgmt,
            unsigned char* decodedData,
            int dataLen);

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpKeyMgmtGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted key-mgmt attribute value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpKeyMgmtGetBadSyntax(
            const RvSdpKeyMgmtAttr* keyMgmt);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtGetBadSyntax(_keyMgmt) \
            RV_SDP_EMPTY_STRING((_keyMgmt)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyMgmtIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the key-mgmt attribute is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      keyMgmt - a pointer to the RvSdpKeyMgmtAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpKeyMgmtIsBadSyntax(
            RvSdpKeyMgmtAttr* keyMgmt);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtIsBadSyntax(_keyMgmt) ((_keyMgmt)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpKeyMgmtSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP key-mgmt attribute value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpKeyMgmtAttr object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpKeyMgmtSetBadSyntax(
            const RvSdpKeyMgmtAttr* o, const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpKeyMgmtSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

#endif /*RV_SDP_KEY_MGMT_ATTR*/

/****  --- End of Key Management Attribute Functions --- ****/

/****  --- Start of Crypto Attribute Functions --- ****/
/*
This section contains the functions used to operate on RvSdpCryptoAttr objects.
The RvSdpCryptoAttr Type represents the crypto attribute ('a=crypto:')
field of an SDP message.
These functions can be used only if RV_SDP_CRYPTO_ATTR (defined in rvsdpconfig.h file)
compilation switch is enabled.
*/

#if defined(RV_SDP_CRYPTO_ATTR)

/***************************************************************************
 * rvSdpCryptoGetTag
 * ------------------------------------------------------------------------
 * General:
 *      Gets the tag value of the crypto special attribute.
 *
 * Return Value:
 *      The requested field of the crypto special attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvUint rvSdpCryptoGetTag(
            const RvSdpCryptoAttr* crypto);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoGetTag(_crypto) (_crypto)->iCryptoTag
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoSetTag
 * ------------------------------------------------------------------------
 * General:
 *      Sets the tag value of the crypto special attribute.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      tag - the new value crypto tag.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpCryptoSetTag(
            RvSdpCryptoAttr* crypto,
            RvUint tag);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoSetTag(_crypto,_tag) (_crypto)->iCryptoTag = _tag
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoGetSuite
 * ------------------------------------------------------------------------
 * General:
 *      Gets the suite value of the crypto special attribute.
 *
 * Return Value:
 *      The requested field of the crypto special attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpCryptoGetSuite(
            const RvSdpCryptoAttr* crypto);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoGetSuite(_crypto) RV_SDP_EMPTY_STRING((_crypto)->iCryptoSuite)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoSetSuite
 * ------------------------------------------------------------------------
 * General:
 *      Sets the suite value of the crypto special attribute.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      suite - the new value of suite.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpCryptoSetSuite(
            RvSdpCryptoAttr* crypto,
            const char* suite);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoSetSuite(_crypto,_suite) \
            rvSdpSetTextField(&(_crypto)->iCryptoSuite,(_crypto)->iObj,(_suite))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoGetNumOfKeyParams
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of key parameters set in crypto special attribute.
 *
 * Return Value:
 *      Number of key parameters set in crypto special attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpCryptoGetNumOfKeyParams(
            const RvSdpCryptoAttr* crypto);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoGetNumOfKeyParams(_crypto) (_crypto)->iCryptoKeyParamsNum
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpCryptoGetKeyMethod
* ------------------------------------------------------------------------
* General:
*      Gets a crypto method of crypto special attribute by index.
*
* Return Value:
*      The requested crypto method.
* ------------------------------------------------------------------------
* Arguments:
*      crypto - a pointer to the RvSdpCryptoAttr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpCryptoGetNumOfKeyParams call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpCryptoGetKeyMethod(
            const RvSdpCryptoAttr* crypto,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoGetKeyMethod(_crypto,_index) \
			RV_SDP_EMPTY_STRING(((const char*) ((_crypto)->iCryptoKeyMethods[(_index)])))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpCryptoGetKeyInfo
* ------------------------------------------------------------------------
* General:
*      Gets a key info of crypto special attribute by index.
*
* Return Value:
*      The requested key info.
* ------------------------------------------------------------------------
* Arguments:
*      crypto - a pointer to the RvSdpCryptoAttr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpCryptoGetNumOfKeyParams call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpCryptoGetKeyInfo(
            const RvSdpCryptoAttr* crypto,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoGetKeyInfo(_crypto,_index) \
			RV_SDP_EMPTY_STRING(((const char*) ((_crypto)->iCryptoKeyInfos[(_index)])))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoAddKeyParam
 * ------------------------------------------------------------------------
 * General:
 *      Adds the another pair of key parameters of the crypto special attribute.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      method - the key method to be added.
 *      info - the key info to be added.
 ***************************************************************************/
RVSDPCOREAPI RvSdpStatus rvSdpCryptoAddKeyParam(
            RvSdpCryptoAttr* crypto,
            const char* method,
            const char* info);

/***************************************************************************
 * rvSdpCryptoRemoveKeyParam
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the key parameter by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpCryptoGetNumOfKeyParams call.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpCryptoRemoveKeyParam(
            RvSdpCryptoAttr* crypto,
            RvSize_t index);

/***************************************************************************
 * rvSdpCryptoClearKeyParams
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all key parameters set in crypto attribute.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
RVSDPCOREAPI void rvSdpCryptoClearKeyParams(
            RvSdpCryptoAttr* crypto);

/***************************************************************************
 * rvSdpCryptoGetNumOfSessionParams
 * ------------------------------------------------------------------------
 * General:
 *      Gets the number of session parameters set in crypto special attribute.
 *
 * Return Value:
 *      Number of session parameters set in crypto special attribute.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSize_t rvSdpCryptoGetNumOfSessionParams(
            const RvSdpCryptoAttr* crypto);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoGetNumOfSessionParams(_crypto) (_crypto)->iCryptoSessParamsNum
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
* rvSdpCryptoGetSessionParam
* ------------------------------------------------------------------------
* General:
*      Gets a session paramter of crypto special attrbute object by index.
*
* Return Value:
*      The requested session paramter.
* ------------------------------------------------------------------------
* Arguments:
*      crypto - a pointer to the RvSdpCryptoAttr object.
*      index - the index. The index should start at zero (0) and must be smaller
*              than the number of elements in the list. The number of elements
*              in the list is retrieved by rvSdpCryptoGetNumOfSessionParams() call.
***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpCryptoGetSessionParam(
            const RvSdpCryptoAttr* crypto,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoGetSessionParam(_crypto,_index) \
			RV_SDP_EMPTY_STRING(((const char*) ((_crypto)->iCryptoSessParams[(_index)])))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoAddSessionParam
 * ------------------------------------------------------------------------
 * General:
 *      Adds another session parameter of the crypto attribute.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      spar - the session parameter to be added.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpCryptoAddSessionParam(
            RvSdpCryptoAttr* crypto,
            const char* spar);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoAddSessionParam(_crypto,_spar) \
            rvSdpAddTextToArray(&((_crypto)->iCryptoSessParamsNum),\
                                RV_SDP_CRYPTO_MAX_KEY_PARAMS,\
                                (_crypto)->iCryptoSessParams,(_crypto)->iObj,(_spar))
#endif /*RV_SDP_USE_MACROS*/


/***************************************************************************
 * rvSdpCryptoRemoveSessionParam
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) the crypto session parameter by index.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 *      index - the index. The index should start at zero (0) and must be smaller
 *              than the number of elements in the list. The number of elements
 *              in the list is retrieved by correspondent
 *              rvSdpCryptoGetNumOfSessionParams call.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpCryptoRemoveSessionParam(
            RvSdpCryptoAttr* crypto,
            RvSize_t index);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoRemoveSessionParam(_crypto,_index) \
            rvSdpRemoveTextFromArray(&((_crypto)->iCryptoSessParamsNum),\
                                     (_crypto)->iCryptoSessParams,\
                                     (_crypto)->iObj,(_index))
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoClearSessionParams
 * ------------------------------------------------------------------------
 * General:
 *      Removes (and destructs) all session parameters set in crypto special attribute.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpCryptoClearSessionParams(
            RvSdpCryptoAttr* crypto);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoClearSessionParams(_crypto) \
            rvSdpClearTxtArray(&((_crypto)->iCryptoSessParamsNum),\
                               (_crypto)->iCryptoSessParams,(_crypto)->iObj)
#endif /*RV_SDP_USE_MACROS*/

#if defined(RV_SDP_CHECK_BAD_SYNTAX)

/***************************************************************************
 * rvSdpCryptoGetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Gets the proprietary formatted crypto attribute value
 *      or empty string ("") if the value is legal.
 *
 * Return Value:
 *      The bad syntax value.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttr object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpCryptoGetBadSyntax(
            const RvSdpCryptoAttr* crypto);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoGetBadSyntax(_crypto) RV_SDP_EMPTY_STRING((_crypto)->iBadSyntaxField)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoIsBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Tests whether the crypto special attribute is proprietary formatted.
 *
 * Return Value:
 *      RV_TRUE  if the field is bad syntax or RV_FALSE otherwise.
 * ------------------------------------------------------------------------
 * Arguments:
 *      crypto - a pointer to the RvSdpCryptoAttrRvSdpMsg object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvBool rvSdpCryptoIsBadSyntax(
            RvSdpCryptoAttr* crypto);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoIsBadSyntax(_crypto) ((_crypto)->iBadSyntaxField != NULL)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpCryptoSetBadSyntax
 * ------------------------------------------------------------------------
 * General:
 *      Sets the SDP crypto attribute value to proprietary format.
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      o - a pointer to the RvSdpCryptoAttr object.
 *      bs - The proprietary formatted value to be set.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpCryptoSetBadSyntax(
            RvSdpCryptoAttr* o, const char* bs);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpCryptoSetBadSyntax(_o,_bs) \
			rvSdpSetTextField(&(_o)->iBadSyntaxField,(_o)->iObj,(_bs))
#endif /*RV_SDP_USE_MACROS*/

#endif /* defined(RV_SDP_CHECK_BAD_SYNTAX) */

#endif /*RV_SDP_CRYPTO_ATTR*/

/****  --- End of Crypto Attribute Functions --- ****/

/****  --- Start of 'Other' Functions --- ****/
/*
This section contains the functions used to operate on RvSdpOther objects.
The RvSdpOther Type represents a SDP line with proprietary tag, or free text
SDP line.
These functions can be used only if RV_SDP_CHECK_BAD_SYNTAX (defined in
rvsdpconfig.h file) compilation switch is enabled.
*/

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/*
 *	Construct
 */

/***************************************************************************
 * rvSdpOtherConstruct
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpOther object.
 *      This function is obsolete. The rvSdpMsgAddOther or rvSdpMediaDescrAddOther
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to valid RvSdpOther object.
 *      tag - the tag letter of the line.
 *      value - the text of the line.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpOtherConstruct(
                RvSdpOther* oth,
                const char tag,
                const char* value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOtherConstruct(_oth,_tag,_value) \
                rvSdpOtherConstruct2(NULL,(_oth),(_tag),(_value),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOtherConstructA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs the RvSdpOther object.
 *      This function is obsolete. The rvSdpMsgAddOther or rvSdpMediaDescrAddOther
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to valid RvSdpOther object.
 *      tag - the tag letter of the line.
 *      value - the text of the line.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpOtherConstructA(
                RvSdpOther* oth,
                const char tag,
                const char* value,
                RvAlloc* a);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOtherConstructA(_oth,_tag,_value,_a) \
                rvSdpOtherConstruct2(NULL,(_oth),(_tag),(_value),(_a),RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOtherConstructCopy
 * ------------------------------------------------------------------------
 * General:
 *      Constructs an 'other' object and copies the values from a source
 *      'other' field.
 *      This function is obsolete. The rvSdpMsgAddOther or rvSdpMediaDescrAddOther
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to 'other' object to be constructed. Must point
 *             to valid memory.
 *      src - a source 'other' object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpOtherConstructCopy(
                RvSdpOther* dest,
                const RvSdpOther* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOtherConstructCopy(_dest,_src) \
                rvSdpOtherCopy2((_dest),(_src),NULL,RV_FALSE)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOtherConstructCopyA
 * ------------------------------------------------------------------------
 * General:
 *      Constructs an 'other' object and copies the values from a source
 *      'other' field.
 *      This function is obsolete. The rvSdpMsgAddOther or rvSdpMediaDescrAddOther
 *      should be used instead.
 *
 * Return Value:
 *      A pointer to the constructed object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to 'other' object to be constructed. Must point
 *             to valid memory.
 *      src - a source 'other' object.
 *      a  - allocator to be used for memory allocations, if allocator is NULL
 *           the default allocator is used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpOther* rvSdpOtherConstructCopyA(
                RvSdpOther* dest,
                const RvSdpOther* src,
                RvAlloc* a);

/*
 *	Destructors
 */

/***************************************************************************
 * rvSdpOtherDestruct
 * ------------------------------------------------------------------------
 * General:
 *      Destructs the 'other' object.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
***************************************************************************/
RVSDPCOREAPI void rvSdpOtherDestruct(
                RvSdpOther *oth);

/*
 *	Copy
 */
/***************************************************************************
 * rvSdpOtherCopy
 * ------------------------------------------------------------------------
 * General:
 *      Copies the values from a source 'other' object to destination.
 *
 * Return Value:
 *      A pointer to the destination object, or NULL if the function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      dest - a pointer to destination 'other' object. Must point
 *             to constructed RvSdpOther object.
 *      src - a 'other' object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpOther* rvSdpOtherCopy(
                RvSdpOther* dest,
                const RvSdpOther* src);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOtherCopy(_dest,_src) rvSdpOtherCopy2((_dest),(_src),NULL,RV_TRUE)
#endif /*RV_SDP_USE_MACROS*/

/*
 *	Setters And Getters
 */


/***************************************************************************
 * rvSdpOtherGetTag
 * ------------------------------------------------------------------------
 * General:
 *      Get the tag letter.
 * Return Value:
 *      Returns the tag letter.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI char rvSdpOtherGetTag(
                const RvSdpOther* oth);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOtherGetTag(_oth) (_oth)->iOtherTag
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOtherSetTag
 * ------------------------------------------------------------------------
 * General:
 *      Sets the tag letter of the line.
 *
 * Return Value:
 *      None.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
 *      tag - the line's tag letter.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI void rvSdpOtherSetTag(
                RvSdpOther* oth,
                const char tag);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOtherSetTag(_oth,_tag) (_oth)->iOtherTag = (_tag)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOtherGetValue
 * ------------------------------------------------------------------------
 * General:
 *      Get the SDP line's value (after the '=' symbol).
 * Return Value:
 *      Returns the SDP line's value (after the '=' symbol).
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI const char* rvSdpOtherGetValue(
                const RvSdpOther* oth);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOtherGetValue(_oth) RV_SDP_EMPTY_STRING((_oth)->iValue)
#endif /*RV_SDP_USE_MACROS*/

/***************************************************************************
 * rvSdpOtherSetValue
 * ------------------------------------------------------------------------
 * General:
 *      Sets the value of the line (after the '=' symbol).
 *
 * Return Value:
 *      Returns RV_SDPSTATUS_OK if the function succeeds, or an error code if the
 *      function fails.
 * ------------------------------------------------------------------------
 * Arguments:
 *      oth - a pointer to the RvSdpOther object.
 *      value - the SDP line's value.
 ***************************************************************************/
#ifndef RV_SDP_USE_MACROS
RVSDPCOREAPI RvSdpStatus rvSdpOtherSetValue(
                RvSdpOther* oth,
                const char *value);
#else /*RV_SDP_USE_MACROS*/
#define rvSdpOtherSetValue(_oth,_value) \
                rvSdpSetTextField(&(_oth)->iValue,(_oth)->iObj,(_value))
#endif /*RV_SDP_USE_MACROS*/

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/****  --- End of 'Other' Functions --- ****/

/****  --- Start of Bad Syntax Functions --- ****/
/*
This section contains the functions used to operate on RvSdpBadSyntax objects.
RvSdpBadSyntax is used to hold a standard SDP line that contains proprietary
information. Each SDP message and Media Descriptor contain a list of these
objects. This object is used to directly access the non-standard lines (or lines
with syntax errors).
These functions can be used only if RV_SDP_CHECK_BAD_SYNTAX (defined in
rvsdpconfig.h file) compilation switch is enabled.
*/

#if defined(RV_SDP_CHECK_BAD_SYNTAX)
/***************************************************************************
 * rvSdpBadSyntaxGetField
 * ------------------------------------------------------------------------
 * General:
 *      Gets the SDP object contained in the BadSyntax object.
 *
 * Return Value:
 *      Returns a handle to the contained object. This handle can be cast to
 *      the contained SDP object according to its type. The type can be retrieved
 *      using the rvSdpBadSyntaxGetFieldType() function.
 * ------------------------------------------------------------------------
 * Arguments:
 *      badS - a pointer to the RvSdpBadSyntax object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpGenericFieldPtr rvSdpBadSyntaxGetField(
                const RvSdpBadSyntax* badS);

/***************************************************************************
 * rvSdpBadSyntaxGetFieldType
 * ------------------------------------------------------------------------
 * General:
 *      Gets the type of SDP object contained in the BadSyntax object. The user
 *      may use this function to know which object is wrapped inside the BadSyntax
 *      object.
 *
 * Return Value:
 *      Returns the type of the wrapped object. This type is mapped by the
 *      RvSdpFieldTypes enumeration.
 * ------------------------------------------------------------------------
 * Arguments:
 *      badS - a pointer to the RvSdpBadSyntax object.
 ***************************************************************************/
RVSDPCOREAPI RvSdpFieldTypes rvSdpBadSyntaxGetFieldType(
                const RvSdpBadSyntax* badS);

#endif /*defined(RV_SDP_CHECK_BAD_SYNTAX)*/

/****  --- End of Bad Syntax Functions --- ****/

/****  --- Start of SDP Objects Reparse Functions --- ****/
/*
This section contains the functions used to reparse bad syntax SDP objects.
These functions can be used only if RV_SDP_ENABLE_REPARSE (defined in
rvsdpconfig.h file) compilation switch is enabled.
*/


#ifdef RV_SDP_ENABLE_REPARSE

/***************************************************************************
 * rvSdpOriginReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpOrigin object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the origin field
 *      becomes valid SDP object. Use rvSdpOriginGetBadSyntax and
 *      rvSdpOriginSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpOrigin  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the origin object is contained.
 *      c - pointer to reparse RvSdpOrigin object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpOrigin* rvSdpOriginReparse(
            RvSdpMsg* msg,
            RvSdpOrigin* c,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpUriReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of URI field of the message.
 *
 * Return Value:
 *      Pointer to valid URI field in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the URI is contained.
 *      u - URI line for reparse.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI const char* rvSdpUriReparse(
            RvSdpMsg* msg,
            const char* u,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpEmailReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpEmail object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the email field
 *      becomes valid SDP object. Use rvSdpEmailGetBadSyntax and
 *      rvSdpEmailSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpEmail  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the email object is contained.
 *      c - pointer to reparse RvSdpEmail object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpEmail* rvSdpEmailReparse(
            RvSdpMsg* msg,
            RvSdpEmail* c,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpPhoneReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpPhone object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the phone field
 *      becomes valid SDP object. Use rvSdpPhoneGetBadSyntax and
 *      rvSdpPhoneSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpPhone  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the phone object is contained.
 *      c - pointer to reparse RvSdpPhone object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpPhone* rvSdpPhoneReparse(
            RvSdpMsg* msg,
            RvSdpPhone* c,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpConnectionReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpConnection object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the connection field
 *      becomes valid SDP object. Use rvSdpConnectionGetBadSyntax and
 *      rvSdpConnectionSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpConnection  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the connection object is contained.
 *      c - pointer to reparse RvSdpConnection object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpConnection* rvSdpConnectionReparse(
            RvSdpMsg* msg,
            RvSdpConnection* c,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpBandwidthReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpBandwidth object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the bandwidth field
 *      becomes valid SDP object. Use rvSdpBandwidthGetBadSyntax and
 *      rvSdpBandwidthSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpBandwidth  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the bandwidth object is contained.
 *      c - pointer to reparse RvSdpBandwidth object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpBandwidth* rvSdpBandwidthReparse(
            RvSdpMsg* msg,
            RvSdpBandwidth* b,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpSessionTimeReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpSessionTime object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the session time field
 *      becomes valid SDP object. Use rvSdpSessionTimeGetBadSyntax and
 *      rvSdpSessionTimeSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpSessionTime  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the session time object is contained.
 *      c - pointer to reparse RvSdpSessionTime object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpSessionTime* rvSdpSessionTimeReparse(
            RvSdpMsg* msg,
            RvSdpSessionTime* b,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpRepeatIntReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpRepeatInterval object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the repeat interval field
 *      becomes valid SDP object. Use rvSdpRepeatIntervalGetBadSyntax and
 *      rvSdpRepeatIntervalSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpRepeatInterval  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the repeat interval object is contained.
 *      c - pointer to reparse RvSdpRepeatInterval object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpRepeatInterval* rvSdpRepeatIntReparse(
            RvSdpMsg* msg,
            RvSdpRepeatInterval* b,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpKeyReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpKey object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the key field
 *      becomes valid SDP object. Use rvSdpKeyGetBadSyntax and
 *      rvSdpKeySetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpKey  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the key object is contained.
 *      c - pointer to reparse RvSdpKey object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpKey* rvSdpKeyReparse(
            RvSdpMsg* msg,
            RvSdpKey* b,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpMediaDescrReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpMediaDescr object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the media descriptor field
 *      becomes valid SDP object. Use rvSdpMediaDescrGetBadSyntax and
 *      rvSdpMediaDescrSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpMediaDescr  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the media descriptor object is contained.
 *      c - pointer to reparse RvSdpMediaDescr object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpMediaDescr* rvSdpMediaDescrReparse(
            RvSdpMsg* msg,
            RvSdpMediaDescr* b,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

/***************************************************************************
 * rvSdpRtpMapReparse
 * ------------------------------------------------------------------------
 * General:
 *      Performs reparse of RvSdpRtpMap object. The object's bad syntax
 *      is used for reparse. If the reparse succeeds the RTP map field
 *      becomes valid SDP object. Use rvSdpRtpMapGetBadSyntax and
 *      rvSdpRtpMapSetBadSyntax to access bad syntax field of the object.
 *      If the reparse fails the object remains unchanged.
 *
 * Return Value:
 *      Pointer to the RvSdpRtpMap  in case of successfull reparse
 *      otherwise the NULL pointer is returned.
 * ------------------------------------------------------------------------
 * Arguments:
 *      msg - pointer to RvSdpMsg where the RTP map object is contained.
 *      c - pointer to reparse RvSdpRtpMap object.
 *      len - this  parameter is not used.
 *      stat - the reparse status is set at the end of the reparse process.
 *      a - this parameter is not used.
 ***************************************************************************/
RVSDPCOREAPI RvSdpRtpMap* rvSdpRtpMapReparse(
            RvSdpMsg* msg,
            RvSdpRtpMap* b,
            int* len,
            RvSdpParseStatus* stat,
            RvAlloc* a);

#endif /*RV_SDP_ENABLE_REPARSE*/

/****  --- End of SDP Objects Reparse Functions --- ****/

#ifdef __cplusplus
}
#endif



#endif/*#ifndef _rvsdp_h_*/


/*
 *          Implementation notes
 *
 *
 *  The SDP data structures are 'C' structs used to hold data specific for given SDP tag.
 *  Thus there is SDP data structure for every SDP tag ('c','m','b' etc)
 *  The  RvSdpMsg is data struct used to represent the hold SDP message.
 *  The RvSdpMediaDescr struct represents one media descriptor. This struct contains
 *  data members for all fields of 'm=' line and contains SDP struct instances regarding
 *  the media.
 *  There is RvSdpCommonFields struct used in RvSdpMsg and RvSdpMediaDescr. This struct
 *  holds data common to message and media descriptor context.
 *
 *  * Different approaches used for SDP tags allowing multiple appearances in
 *    the same SDP context (message or media) and tags with at most single appearance;
 *
 *  * Multiple tags: most of structure instances (RvSdpConnection, RvSdpAttribute etc)
 *    are preallocated when RvSdpMsg is initialized and stored in dedicated pools within
 *    RvSdpMsg struct.
 *
 *  * Multiple tags: Objects pools are implemented as linked list (RvSdpList) where
 *    not yet used structs instances are kept.
 *
 *  * Multiple tags: When structure instance is used in a context of
 *    the message or media descriptor it is attached to the correspondent linked list.
 *    For example instances of RvSdpConnection are inserted in the iConnectionList of
 *    message or media descriptor.
 *
 *  * Multiple tags: Since those structures instances are kept in linked lists (in the
 *    the pool when not used and in the message or media descriptor when used) each
 *    structure used for multiple SDP tag contains 'iNext...' data member. For example
 *    data field iNextConnection of RvListNode type points to the next instance of
 *    RvSdpConnection struct located in the one-directional linked list. RvSdpList
 *    struct contains an offset (iNodeOffset) between the beginning of the struct owning
 *    the instance  of RvListNode and that RvListNode instance itself. Thus having the
 *    instance of RvListNode and
 *    knowing the type of the owning struct it is possible to get the pointer to owning
 *    struct. For example if we know that 'nd' belongs to the instance of RvSdpConnection
 *    and having the pointer to the list struct:
 *      RvListNode *nd;
 *      RvSdpList  *ll;
 *      RvSdpConnection *conn;
 *      nd = .... // get somehow the value
 *		conn = (RvSdpConnection*) ((char*)nd - ll->iNodeOffset);
 *
 *  * Single tags: The correspondent SDP structures appear as a part of SDP context
 *    (message or media) and not as a pointer. Thus there is a need to determine whether
 *    the instance of such structure (RvSdpKey, RvSdpOrigin and RvSdpTZA) is in use or
 *    not. For example struct RvSdpMsg contains the data member iOrigin of type
 *    RvSdpOrigin (and not 'RvSdpOrigin*' ). iOrigin field represents the 'o=' line
 *    of correspondent SDP message. In order to test whether iOrigin is used (the
 *    correspondent SDP message contains 'o=' line) the RV_SDP_ORIGIN_IS_USED macro
 *    can help.
 *
 *	* Memory allocations: most of SDP structures have text strings as data members. It
 *    is undesirable to perform memory allocations for each string and in order to avoid
 *    such allocations there is a strings buffer in each instance of RvSdpMsg that will
 *    serve all SDP data structures hold in the message.  Thus every SDP data structure
 *    needs to have an access to the owning instance of RvSdpMsg. Another reason for
 *    having this access is for objects allocated from one of objects pool.
 *    Since all pools are in the context of SDP message (RvSdpMsg) when object allocated
 *    from the pool is being destroyed the correspondent SDP data structure instance (for
 *    example RvSdpConnection) needs to be returned to the correspondent pool
 *    (iConnectionsPool). Thus when any  SDP data structure is constructed it needs to be
 *    be given a pointer to the RvSdpMsg where this data structure will be kept.
 *    But there are old-style obsolete constructions API functions (for example
 *    rvSdpConnectionConstruct) which do not have RvSdpMsg pointer among its
 *    arguments. Thus SDP data structures instances constructed in this old way will not
 *    have an access to the owning SDP message (RvSdpMsg). That means such SDP data
 *    objects will not be able to use strings buffer and objects pools, instead it will
 *    perform direct memory allocations using given RvAlloc object.
 *    Given above considerations every SDP data object will either have a pointer to the
 *    owning RvSdpMsg instance or RvAlloc instance (if constructed through obsolete API).
 *    The 'void* iObj' data member in all SDP data structures is used for this purpose.
 *    The macro RV_SDP_OBJ_IS_MESSAGE is used to define whether 'iObj' points to
 *	  RvSdpMsg or to RvAlloc.
 *
 *  * Strings used in SDP objects.
 *    Each string in SDP data structure uses either strings buffer (the SDP data structure
 *    instance was constructed using RvSdpMsg object) or points to memory allocated for
 *    this string using the RvAlloc allocator (the SDP data structure was constructed in
 *    the obsolete way).
 *    There are two functions: rvSdpSetTextField and rvSdpUnsetTextField used to create
 *    and destroy the  string in SDP data structures. Both functions takes as one of
 *    their parameters the 'void* obj' which must point to either RvSdpMsg instance or
 *    RvAlloc instance. If 'obj' points to RvSdpMsg instance its strings buffer will
 *    be used for string creation or destroy. It is obvious that if string was created
 *    using strings buffer ('obj' was pointing to RvSdpMsg) it cannot be destroyed using
 *    the allocator and vice versa.
 *
 *  * Strings buffer (RvStringBuffer): Each instance of RvSdpMsg contains preallocated
 *	  strings buffer. This buffer contains text strings of all SDP data structures owned
 *    by this RvSdpMsg instance. The buffer is implemented as characters array.When some
 *    new string needs to be allocated the memory chunk at the end of the consumed part
 *    of the buffer is used. This approach may lead to unused pieces of memory when
 *    some strings change its value and the new one is longer then previous (the new
 *    string value  can not be copied to the chunk currently used by the 'old' string
 *    value and the new value will be allocated at the end while 'losing' the currently
 *    used chunk). The 'reshuffling' mechanism is used as a mean of garbage collection.
 *    That is when there is no room in the buffer for some string allocation all data
 *    structures contained within the SDP message are sorted out in order to reallocate
 *    its strings. For effective SDP objects sort out the line object (RvSdpLineObject)
 *    mechanism is used.
 *    The tricky problem can arise with this approach. There are SDP objects having
 *    more than one string as data members. For example RvSdpAttribute has two strings:
 *    iAttrName and iAttrValue. When constructing the new RvSdpAttribute  object the
 *    string buffer of owning RvSdpMsg object can reach out of space state when the first
 *    string (iAttrName) is already added to the string buffer and the second is being
 *    added. As was said earlier when there is no room for the next string (iAttrValue) the
 *    'reshuffling' procedure is envoked. During this procedure we first allocate memory
 *    chunk to hold all strings and
 *    then we have to pass over all objects held in the  RvSdpMsg instance and copy their
 *    strings from the old memory chunk to the new. After all owned objects are sorted out the
 *    old strings buffer chunk is deallocated. Thus if some of SDP objects owned by the message
 *    was not handled during 'reshuffling' procedure it will have strings pointing to
 *    'freed' memory region. But the RvSdpAttribute that is constructed is not yet part
 *    of the message (it will be appended to the correspondent linked list iAttrList only
 *    at the end of construction) thus it will not be handled during 'reshuffling' and
 *    iAttrName that was successfully allocated in the strings buffer just before it was
 *    reshuffled will point to the 'freed' memory. In order to prevent the problem we must
 *    'promise' that string buffer will have enough room for all strings of object being
 *    constructed prior to construction. This applies to the SDP structs with two
 *    conditions:
 *      1. The struct contains more than one string.
 *      2. It is kept in one of linked lists of the SDP message or media descriptor.
 *         The second condition is important because otherwise the SDP data object is
 *         part of the message structure itself and does not need to be attached to one
 *         of its linked lists.
 *
 *	* Line Object: All SDP data structures contain
 *    RvSdpLineObject instance. When the RvSdpMsg instance is built during the parsing
 *    process all SDP objects making the message are attached to the special list
 *    in order of appearance in the parsed input. When the message is built/modified
 *    using API functions this list is also updated. That is the list of line objects.
 *    The list allows to maintain the order of SDP
 *    objects as it appeared in the SDP input or as it were inserted in the message using
 *    the API functions. The list of line objects (iHeadLO and iTailLO data-fields in
 *    RvSdpMsg) is bidirectional linked list. It is used for effective sort out of
 *    all SDP objects making
 *    the SDP message without analyzing the internal structure of RvSdpMsg and
 *    RvSdpMediaDescr structs. Such sort out is needed for reshuffling procedure.
 *    Each instance of RvSdpLineObject contains the  pointers to the next and previous
 *    nodes in the list and also data (iOffset,iFieldType)
 *    allowing to get the pointer to struct owning the instance of RvSdpLineObject.
 *    For example:
 *              RvSdpLineObject* lo;
 *              RvSdpOrigin *origin;
 *              if (lo->iFieldType == SDP_FIELDTYPE_ORIGIN)
 *                  // the 'lo' belongs to RvSdpOrigin struct instance
 *              {
 *                  origin = (RvSdpOrigin*) ((char*)lo - lo->iOffset);
 *              }
 *
 *  * Special and generic attributes: special attributes are the attributes having dedicated
 *    API functions besides getting/setting attributes name and value. Some of special
 *    attributes are implemented using its own dedicated data structures (RvSdpRtpMap)
 *    some do not have own data structure (connection mode) but it always have field
 *    'iSpecAttrData' set to point to one of the special attributes data sets within the
 *    'gcSpecAttributesData' array. Each set of special attribute data (struct
 *    RvSdpSpecAttributeData) contains set of callback functions used for some typical
 *    operations on the  attribute: copy, destroy, parse etc.
 *    Special attributes having dedicated data structs (RvSdpRtpMap, RvSdpCryptoAttr,
 *    RvSdpKeyMgmtAttr) always have the first struct field of type RvSdpAttribute. For
 *    example when iterating on the attributes of the message or media descriptor we get
 *    the pointer to the next RvSdpAttribute. It is possible to test iSpecAttrData and if
 *    it is not zero and iSpecAttrData->iFieldType is SDP_FIELDTYPE_RTP_MAP we can cast
 *    from the pointer of RvSdpAttribute to the pointer of RvSdpRtpMap.
 *    An attribute without dedicated API is called generic attribute. The 'iSpecAttrData'
 *    field of generic attribute is always NULL.
 *    Please note there are two sets of attribute related API functions. One set is
 *    for all attributes and the other for generic attributes only. For example API
 *    function rvSdpMsgGetNumOfAttr will return the number of generic attributes set
 *    in SDP message but will not count special attributes. The rvSdpMsgGetNumOfAttr2 will
 *    return total number of attributes (generic and special) set in the media.
 *
 *  * Bad syntax support: Bad syntax support depends on compilation switch
 *    (RV_SDP_CHECK_BAD_SYNTAX). If some SDP object can't be successfully parsed its
 *    value (after the '=' sign in the SDP input line) is set to specific 'iBadSyntaxField'
 *    string. The value of iBadSyntaxField can be accessed/modified. Every SDP object
 *    having iBadSyntaxField field can be reparsed. If the value of iBadSyntaxField was
 *    set to legal value of specific SDP tag type the reparse process will result in the
 *    normal SDP object and iBadSyntaxField field will have empty value. All bad syntax
 *    objects of the message/media can be iterated.
 *
 *  * Proprietary tags: It is possible to use non-standard SDP tags (depends on
 *    RV_SDP_CHECK_BAD_SYNTAX compilation switch). The RvSdpOther related APIs should
 *    be used.
 *
 *  * Macros support: The RV_SDP_USE_MACROS compilation switch converts small functions
 *    (usually get/set functions) into macros. Enabling this macro significantly reduces
 *    the footprint of SDP library  but makes the debugging process less convinient. We
 *    suggest to disable the switch during development/debugging stage and to enable it
 *    while in production stage.
 *
 *  * Parsing warnings: When SDP message is built from text input by parsing process the
 *    parsing warnings are collected. The warnings usually point to incompability with
 *    the SDP standard. The whole SDP parsing warnings functionality and API can be
 *    enabled/disabled by RV_SDP_PARSE_WARNINGS compilation switch.
 *
 */

