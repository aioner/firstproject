/* rvlog.c - log handling */
/************************************************************************
        Copyright (c) 2001 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/

#include "rvmemory.h"
#include "rvtimestamp.h"
#include "rvthread.h"
#include "rvstdio.h"
#include "rvlock.h"
#include "rvlog.h"
#include "rvsocket.h"
#include "rvselect.h"
#include "rvqueue.h"
#include "rvtimer.h"
#include "rvhost.h"
#include "rvtm.h"
#include "rvclock.h"
#include "rvares.h"
#include "rvtls.h"
#if (RV_SCTP_INHOUSE == RV_YES)
#include "rvsctp.h"
#endif
#if (RV_IMS_IPSEC_ENABLED == RV_YES)
#include "rvimsipsec.h"
#endif
#include "rvassert.h"
#include "rvstdio.h"
#include "rvccoreglobals.h"


#if defined(__cplusplus)
extern "C" {
#endif


/* Lets make error codes a little easier to type */
#define RvLogErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_LOG, (_e))


#if (RV_LOGMASK != RV_LOGLEVEL_NONE)

/* Lock used for log buffer printing */
/* static RvLock rvLogLock; */


/* Structure containing definitions for module functions
   for constructing / destructing Log Sources  */
typedef struct {
    RvStatus (*srcConstruct)(RvLogMgr* logMgr);
} RvLogModuleFuncs;

/* Array of functions for constructing Log Sources */
static const RvLogModuleFuncs RvLogModules[] = {
    { RvSemaphoreSourceConstruct },
    { RvMutexSourceConstruct     },
    { RvLockSourceConstruct      },
    { RvMemorySourceConstruct    },
    { RvThreadSourceConstruct    },
    { RvQueueSourceConstruct     },
    { RvTimerSourceConstruct     },
    { RvTimestampSourceConstruct },
    { RvClockSourceConstruct     },
    { RvTmSourceConstruct        },
    { RvSocketSourceConstruct    },
    { RvPortRangeSourceConstruct },
    { RvSelectSourceConstruct    },
    { RvHostSourceConstruct      },
    { RvTLSSourceConstruct       },
    { RvAresSourceConstruct      },
#if (RV_SCTP_INHOUSE == RV_YES)
    { RvSctpSourceConstruct      },
#endif
#if (RV_IMS_IPSEC_ENABLED == RV_YES)
    { rvIpsecSourceConstruct     },
#endif
    { NULL                       }  /* List must end with NULLs */
};

#endif /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */


/********************************************************************************************
 * RvLogInit - Initiates a log module
 *
 * This function should be called only once in entire process
 *
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvLogInit(void)
{
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RV_USE_CCORE_GLOBALS;
#endif

    RvStatus status = RV_OK;

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    status = RvLockConstruct(NULL, &rvLogLock);
#endif

    return status;
}

/********************************************************************************************
 * RvLogEnd - Shutdown a log module
 *
 * This function should be called only once in entire process
 *
 * INPUT   : None
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RvLogEnd(void)
{
    RvStatus status = RV_OK;

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)

    RV_USE_CCORE_GLOBALS;

    /* See if there's a buffer to deallocate in this thread */
    status = RvLockDestruct(&rvLogLock,NULL);
#endif

    return status;
}


/********************************************************************************************
 * RvLogSourceConstruct - Create a new source of messages in a log manager
 *
 * INPUT   : logMgr     - Log manager
 *           source     - Source of messages to create
 *           name       - Name of the source
 *           description- Description of the source
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogSourceConstruct(
    IN RvLogMgr*        logMgr,
    IN RvLogSource*     source,
    IN const RvChar*    name,
    IN const RvChar*    description)
{
    RvStatus ret = RV_OK;
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RvLogSource curSource;
    int i;
    int vacantIndex = -1;

    /* The description argument is not used. We only want users to use it, so we'll be able
       to tell by the name what does it do when we look at the code */
    RV_UNUSED_ARG(description);

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (logMgr == NULL || source == NULL)
        return RV_ERROR_NULLPTR;
#endif

    ret = RvLockGet(&logMgr->lock,NULL);
    if (ret != RV_OK)
        return ret;



    /* Look for a source with the same name - we can't allow it... */
    curSource = logMgr->source;
    for (i = 0; i < logMgr->numSources; i++)
    {
        if ((vacantIndex < 0) && (curSource->timesConstructed == 0))
            vacantIndex = i; /* Found a place if we'll need one */

        else if ((curSource->timesConstructed > 0) && (strcmp(curSource->name, name) == 0))
        {
            /* This one was already constructed... */
            vacantIndex = i;
            break;
        }

        /* Get the next one */
        curSource++;
    }

    /* We have to add this as a new source */
    if (vacantIndex < 0)
    {
        /* No luck in finding a free place */
        if (logMgr->numSources == RV_LOG_MAX_SOURCES)
        {
            /* No more source elements left */
            ret =  RvLogErrorCode(RV_ERROR_OUTOFRESOURCES);
        }
        else
        {
            /* Add a new place in the array */
            vacantIndex = logMgr->numSources;
            logMgr->numSources++;
        }
    }

    if (ret == RV_OK && vacantIndex >= 0)
    {
        /* Set the new source information */
        curSource = &logMgr->source[vacantIndex];
        if (curSource->timesConstructed == 0)
        {
            curSource->logMgr = logMgr;
            strncpy(curSource->name, name, sizeof(curSource->name));
            curSource->name[sizeof(curSource->name)-1] = '\0';
            curSource->messageTypes = logMgr->defaultMask;
        }
        curSource->timesConstructed++;
        *source = curSource;
    }

    if (RvLockRelease(&logMgr->lock,NULL) != RV_OK)
        return RvLogErrorCode(RV_ERROR_UNKNOWN);

#else
    RV_UNUSED_ARG(logMgr);
    RV_UNUSED_ARG(source);
    RV_UNUSED_ARG(name);
    RV_UNUSED_ARG(description);
#endif /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */

    return ret;
}


/********************************************************************************************
 * RvLogSourceDestruct - Delete a source of messages from a log manager
 *
 * INPUT   : source     - Source of messages to delete
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogSourceDestruct(
    IN RvLogSource*     source)
{
    RvStatus ret = RV_OK;
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RvLogMgr* logMgr;

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if ((*source)->timesConstructed == 0)
        return RvLogErrorCode(RV_ERROR_OUTOFRANGE);
#endif

    logMgr = (*source)->logMgr;

    ret = RvLockGet(&logMgr->lock,NULL);
    if (ret != RV_OK)
        return ret;

    if ((*source)->timesConstructed > 0)
    {
        (*source)->timesConstructed--;
        if ((*source)->timesConstructed == 0)
            memset(*source, 0, sizeof(**source));
    }

    if (RvLockRelease(&logMgr->lock,NULL) != RV_OK)
        return RvLogErrorCode(RV_ERROR_UNKNOWN);

#else
    RV_UNUSED_ARG(source);
#endif /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */

    return ret;
}


#if (RV_LOGMASK != RV_LOGLEVEL_NONE)

/********************************************************************************************
 * RvLogDestructHere - Kill a log object
 *
 * INPUT   : logMgr - Log manager to destruct
 *           reason - required by user or by select engine
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
static
RvStatus RVCALLCONV RvLogDestructHere(
    IN RvLogMgr            *logMgr,
	IN RvLogDestructReason reason)
{
    RvStatus    ret;
    RvLogSource curSource;
	RvInt       selectUsageCnt;
	RvBool      isDestructed;
    RvInt       i;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (logMgr == NULL)
        return RvLogErrorCode(RV_ERROR_NULLPTR);
#endif

	ret = RvLockGet(&logMgr->lock,NULL);
	if (ret != RV_OK) {
		return ret;
	}

	if (reason == RV_LOG_DESTRUCT_REASON_STACK) {
		logMgr->isDestructed = RV_TRUE;
	}

	selectUsageCnt = logMgr->selectUsageCnt;
	isDestructed   = logMgr->isDestructed;

	ret = RvLockRelease(&logMgr->lock,NULL);
	if (ret != RV_OK) {
		return ret;
	}

	if (isDestructed == RV_FALSE) {
		/* only select engine was destructed.
		it should not affect the log manager */
		return RV_OK;
	}

	if (selectUsageCnt > 0) {
		/* Stack already asked to destruct the log manager,
		however there are still select engine & timer queue that
		uses this log manager. We will destruct log manager
		later. */
		return RvLogErrorCode(RV_ERROR_TRY_AGAIN);
	}

    /* This function is responsible to destruct all common core
       sources, assuming each source has been constructed only once ! */
    for (i = 0; i < logMgr->numSources; i++)
    {
        curSource = &logMgr->source[i];
        ret = RvLogSourceDestruct(&curSource);
        if(ret != RV_OK)
            break;
    }

    ret = RvLockDestruct(&logMgr->lock,NULL);

    return ret;
}

#endif /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */


/********************************************************************************************
 * RvLogSelectUsageDec - Decrements select usage counter of the log manager
 *
 * INPUT   : logMgr - Log manager to destruct
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RVCALLCONV RvLogSelectUsageDec(
	IN RvLogMgr *logMgr)
{
	RvStatus retCode = RV_OK;

#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#else
    if (logMgr == NULL)
        return RV_OK;

	retCode = RvLockGet(&logMgr->lock,NULL);
	if (retCode != RV_OK) {
		return retCode;
	}

	if (logMgr->selectUsageCnt <= 0) {
		logMgr->selectUsageCnt = 0;
		return RvLogErrorCode(RV_ERROR_UNKNOWN);
	}

	(logMgr->selectUsageCnt)--;

	retCode = RvLockRelease(&logMgr->lock,NULL);
	if (retCode != RV_OK) {
		return retCode;
	}

	retCode = RvLogDestructHere(logMgr,RV_LOG_DESTRUCT_REASON_SELECT);

	if (RvErrorGetCode(retCode) == RV_ERROR_TRY_AGAIN) {
		retCode = RV_OK;
	}
#endif

	return retCode;
}

/********************************************************************************************
 * RvLogSelectUsageInc - Increment select usage counter of the log manager
 *
 * INPUT   : logMgr - Log manager to destruct
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RvStatus RVCALLCONV RvLogSelectUsageInc(RvLogMgr *logMgr)
{
	RvStatus retCode = RV_OK;

#if (RV_LOGMASK == RV_LOGLEVEL_NONE)
	RV_UNUSED_ARG(logMgr);
#else
    if (logMgr == NULL)
        return RV_OK;

	retCode = RvLockGet(&logMgr->lock,NULL);
	if (retCode != RV_OK) {
		return retCode;
	}

	if (logMgr->selectUsageCnt < 0) {
		logMgr->selectUsageCnt = 0;
		return RvLogErrorCode(RV_ERROR_UNKNOWN);
	}

	(logMgr->selectUsageCnt)++;

	retCode = RvLockRelease(&logMgr->lock,NULL);
#endif

	return retCode;
}



#if (RV_LOGMASK != RV_LOGLEVEL_NONE)

/********************************************************************************************
 * RvLogConstruct
 *
 * Create a log object. Only a single such object is used by the core and the
 * stacks on top of it.
 *
 * INPUT   : logMgr - Log manager to construct
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogConstruct(
    IN RvLogMgr* logMgr)
{
    RvStatus ret;
    int i;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (logMgr == NULL)
        return RvLogErrorCode(RV_ERROR_NULLPTR);
#endif

    memset(logMgr, 0, sizeof(*logMgr));

    logMgr->defaultMask = RV_LOGMASK;
    logMgr->level = 1; /* Default is log by the masks */

    ret = RvLockConstruct(NULL, &logMgr->lock);

    for(i = 0; RvLogModules[i].srcConstruct != NULL; ++i) {
        ret = RvLogModules[i].srcConstruct(logMgr);
        if(ret != RV_OK)
            break;
    }

    return ret;
}




/********************************************************************************************
 * RvLogDestruct - Kill a log object
 *
 * INPUT   : logMgr - Log manager to destruct
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogDestruct(
    IN RvLogMgr* logMgr)
{
    RvStatus    ret;

	ret = RvLogDestructHere(logMgr,RV_LOG_DESTRUCT_REASON_STACK);

	if (RvErrorGetCode(ret) == RV_ERROR_TRY_AGAIN) {
		ret = RV_OK;
	}

    return ret;
}



/********************************************************************************************
 * RvLogRegisterListener - Set a listener function to the log.
 *
 * Multiple listeners can be set on each log
 * object. The listener is used to actually log the messages.
 *
 * INPUT   : logMgr     - Log manager to use
 *           listener   - Listener function, called on each message
 *           userData   - User data set as a parameter of the listener function
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogRegisterListener(
    IN RvLogMgr*    logMgr,
    IN RvLogPrintCb listener,
    IN void*        userData)
{
    RvStatus ret;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (logMgr == NULL)
        return RvLogErrorCode(RV_ERROR_NULLPTR);
#endif

    ret = RvLockGet(&logMgr->lock,NULL);
    if (ret == RV_OK)
    {
        if (logMgr->numListeners < RV_LOG_MAX_LISTENERS)
        {
#if (RV_CHECK_MASK & RV_CHECK_OTHER)
            RvInt32 i;

            /* Make sure this listener is not already registered */
            for (i = 0; i < logMgr->numListeners; i++)
            {
                if (logMgr->listener[i] == listener)
                {
                    ret = RvLogErrorCode(RV_LOG_ERROR_ALREADY_REGISTERED);
                    break;
                }
            }
#endif  /* (RV_CHECK_MASK & RV_CHECK_OTHER) */

            /* Add the listener */
            logMgr->listener[logMgr->numListeners] = listener;
            logMgr->listenerUserData[logMgr->numListeners] = userData;
            logMgr->numListeners++;
        }
        else
            ret = RvLogErrorCode(RV_ERROR_OUTOFRESOURCES);

        if (RvLockRelease(&logMgr->lock,NULL) != RV_OK)
            return RvLogErrorCode(RV_ERROR_UNKNOWN);
    }

    return ret;
}


/********************************************************************************************
 * RvLogUnregisterListener - Unset a listener function from the log.
 *
 * INPUT   : logMgr     - Log manager to use
 *           listener   - Listener function, called on each message
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogUnregisterListener(
    IN RvLogMgr*    logMgr,
    IN RvLogPrintCb listener)
{
    RvInt32 i;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (logMgr == NULL)
        return RvLogErrorCode(RV_ERROR_NULLPTR);
#endif

    for (i = 0; i < logMgr->numListeners; i++)
    {
        if (logMgr->listener[i] == listener)
        {
            if (i+1 != logMgr->numListeners)
            {
                /* Seems like it's not the last - move all the bunch after this one */
                memmove(&logMgr->listener[i], &logMgr->listener[i+1], (RvSize_t)(sizeof(listener) * (logMgr->numListeners-i-1)));
                memmove(&logMgr->listenerUserData[i], &logMgr->listenerUserData[i+1], (RvSize_t)(sizeof(void*) * (logMgr->numListeners-i-1)));
            }
            logMgr->numListeners--;
            return RV_OK;
        }
    }

    return RvLogErrorCode(RV_ERROR_UNINITIALIZED);
}


/********************************************************************************************
 * RvLogGetSourceByName - Get the source for a specific log source name
 *
 * INPUT   : logMgr     - Log manager
 *           name       - Name of the source to find
 * OUTPUT  : source     - Source found on success
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI RvStatus RVCALLCONV RvLogGetSourceByName(
    IN  RvLogMgr*       logMgr,
    IN  const RvChar*   name,
    OUT RvLogSource*    source)
{
    RvLogSource curSource;
    int i;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((logMgr == NULL) || (name == NULL) || (source == NULL))
        return RvLogErrorCode(RV_ERROR_NULLPTR);
#endif

    /* Look for a source with the same name */
    curSource = logMgr->source;
    for (i = 0; i < logMgr->numSources; i++)
    {
        if ((curSource->timesConstructed > 0) && (strcmp(curSource->name, name) == 0))
        {
            *source = curSource;
            return RV_OK;
        }

        /* Get the next one */
        curSource++;
    }

    /* We can't seem to find this source */
    return RvLogErrorCode(RV_ERROR_BADPARAM);
}



#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
/********************************************************************************************
 * RvLogIsSelected
 *
 * Check to see if a specific message type should be sent to the log by a given source
 *
 * INPUT   : source         - Source of message to log
 *           messageType    - Type of the message to log
 * OUTPUT  : None
 * RETURN  : RV_TRUE if message should be logged, RV_FALSE otherwise
 */
RVCOREAPI
RvBool RVCALLCONV RvLogIsSelected(
    IN RvLogSource*     source,
    IN RvLogMessageType messageType)
{
    RvInt32 level;
    RvBool selected;

#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (source == NULL)
        return RV_FALSE;
#endif

    /* First make sure log is not silenced */
    level = (*source)->logMgr->level;

    if (level == 1)
    {
        /* Check that the type of message should be logged for this source */
        selected = ((RvBool)(((*source)->messageTypes & messageType) != 0));
    }
    else
        selected = (level != 0);

    return selected;
}
#endif  /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */


/********************************************************************************************
 * RvLogSetLevel
 *
 * Set the level of logging, while leaving the masks of all log sources without a change.
 *
 * INPUT   : logMgr - Log manager
 *           level  - 0 stop logging, 1 log by the masks of the sources, 2 log everything
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogSetLevel(
    IN RvLogMgr*    logMgr,
    IN RvInt32      level)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (logMgr == NULL)
        return RvLogErrorCode(RV_ERROR_NULLPTR);
#endif

#if (RV_CHECK_MASK & RV_CHECK_RANGE)
    if ((level < 0) || (level > 2))
        return RvLogErrorCode(RV_ERROR_BADPARAM);
#endif

    logMgr->level = level;
    return RV_OK;
}


/********************************************************************************************
 * RvLogSetGlobalMask
 *
 * Set the mask of messages to log on all the sources of the log object
 *
 * INPUT   : logMgr         - Log manager
 *           messageMask    - Type of the messages to log
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogSetGlobalMask(
    IN RvLogMgr*        logMgr,
    IN RvLogMessageType messageMask)
{
    int i;

    logMgr->defaultMask = messageMask;
    for (i = 0; i < logMgr->numSources; i++)
        if (logMgr->source[i].timesConstructed > 0)
            logMgr->source[i].messageTypes = messageMask;

    return RV_OK;
}


/********************************************************************************************
 * RvLogSourceSetMask
 *
 * Set the mask of messages to log for a specific source
 *
 * INPUT   : source         - Source of messages to set
 *           messageMask    - Type of the messages to log
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogSourceSetMask(
    IN RvLogSource*     source,
    IN RvLogMessageType messageMask)
{
    (*source)->messageTypes = messageMask;

    return RV_OK;
}


/********************************************************************************************
 * RvLogSourceGetMask - Get the mask of messages to log for a specific source
 *
 * INPUT   : source         - Source of messages to get
 * OUTPUT  : None
 * RETURN  : Message mask of messages that are logged
 */
RVCOREAPI
RvLogMessageType RVCALLCONV RvLogSourceGetMask(
    IN RvLogSource*     source)
{
    return ((*source)->messageTypes);
}


/********************************************************************************************
 * RvLogSourceGetName - Get the name for a specific log source
 *
 * INPUT   : source         - Source of messages to get
 * OUTPUT  : None
 * RETURN  : Name of the source on success, NULL on failure
 */
RVCOREAPI
const RvChar* RVCALLCONV RvLogSourceGetName(
    IN RvLogSource*     source)
{
    return ((*source)->name);
}


/********************************************************************************************
 * RvLogTextAny
 *
 * Private function used by all RvLogTextXXX functions to format and execute the
 * actual listener functions.
 * NOTE: Although this function is defined as 'RVCOREAPI' it is used
 * only internally. It is defined as 'RVCOREAPI' to satisfy the GateKeaper needs.
 * To make sure that customer will not use that function, no prototype for it
 * will be provided.
 *
 *
 * INPUT   : source         - Source of message to log
 *           messageType    - Type of message to log
 *           line           - Formatted string to log
 *           varArg         - Argument list to log
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextAny(
    IN RvLogSource*     source,
    IN RvInt            messageType,
    IN const RvChar*    line,
    IN va_list*         varArg)
{
    RV_USE_CCORE_GLOBALS;
    RvStatus ret;
    RvLogRecord logRecord;
    RvThread* threadInfo;
    RvLogMgr* logMgr;
    RvChar* ptr;
    int i;
    /*static RvBool rvLogInsideTextAny = RV_FALSE; / * RV_TRUE if we're currently inside the function RvLogTextAny() */
    /*static RvThreadId rvLogCurId; / * Current thread Id locking this function */
    /*static RvChar rvLogTextStr[RV_LOG_MESSAGE_SIZE + RV_LOG_RESERVED_BYTES]; / * Buffer to use for formatted messages */


    if ((rvLogInsideTextAny == RV_TRUE) && (RvThreadIdEqual(RvThreadCurrentId(), rvLogCurId)))
    {
        /* Make sure we don't enter this function from within this function to avoid an endless
           recursion loops of errors */
        return RvLogErrorCode(RV_LOG_ERROR_RECURSION);
    }

    /* Lock it up */
    ret = RvLockGet(&rvLogLock,NULL);
    if (ret != RV_OK)
        return ret;

    /* Make sure we know we're inside this logging function */
    rvLogInsideTextAny = RV_TRUE;
    rvLogCurId = RvThreadCurrentId();

    /* Skip the reserved bytes - some of the listeners might need it */
    ptr = rvLogTextStr + RV_LOG_RESERVED_BYTES;

    /* Format the given line with the arguments */
    i = RvVsnprintf((char *)ptr, RV_LOG_MESSAGE_SIZE, (char *)line, *varArg);
    ptr[RV_LOG_MESSAGE_SIZE - 1] = '\0';

    /* Make sure the one logging this didn't put a darn 'newline' in his message */
    if (ptr[i-1] == '\n')
        ptr[i-1] = '\0';

    logMgr = (*source)->logMgr;

    /* Update the message statistics */
    logMgr->statInfo[messageType]++;

    /* Create the log record we will be using */
    logRecord.printThreadId = logMgr->printThreadId;

    /* Find out the thread's name / thread id*/
    if (logRecord.printThreadId == RV_TRUE)
    {
        logRecord.threadName = (RvChar *)((RvUintPtr)rvLogCurId);
    }
    else
    {
        threadInfo = RvThreadCurrent();
        if (threadInfo != NULL)
            logRecord.threadName = RvThreadGetName(threadInfo);
        else
            logRecord.threadName = (RvChar *)"Unknown";
    }

    logRecord.source = source;
    logRecord.messageType = messageType;
    logRecord.text = (const RvChar*)ptr;

    for (i = 0; i < logMgr->numListeners; i++)
    {
        /* Call the listeners that are waiting for log messages */
        logMgr->listener[i](&logRecord, logMgr->listenerUserData[i]);
    }

    /* Update the status as being outside this function */
    rvLogInsideTextAny = RV_FALSE;
    rvLogCurId = RV_THREAD_ILLEGAL_THREAD_ID;
    
    RvLockRelease(&rvLogLock,NULL);

    return ret;
}
#endif  /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */



/********************************************************************************************
 * RvLogTextXXX
 *
 * Log a specific message type text message with variable amount of arguments
 * INPUT   : source     - Source of message to log
 *           line       - Formatted string to log
 * OUTPUT  : None
 * RETURN  : RV_OK on success, other on failure
 */
#if (RV_LOGMASK & RV_LOGLEVEL_EXCEP)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextExcep(
    IN  RvLogSource*        source,
    IN  const char*         line, ...)
{
    RvStatus res;
    va_list varArg;

    va_start(varArg, line);
    res = RvLogTextAny(source, RV_LOGID_EXCEP, line, &varArg);
    va_end(varArg);

    return res;
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_EXCEP) */


#if (RV_LOGMASK & RV_LOGLEVEL_ERROR)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextError(
    IN  RvLogSource*        source,
    IN  const char*         line, ...)
{
    RvStatus res;
    va_list varArg;

    va_start(varArg, line);
    res = RvLogTextAny(source, RV_LOGID_ERROR, line, &varArg);
    va_end(varArg);

    return res;
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_ERROR) */


#if (RV_LOGMASK & RV_LOGLEVEL_WARNING)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextWarning(
    IN  RvLogSource*        source,
    IN  const char*         line, ...)
{
    RvStatus res;
    va_list varArg;

    va_start(varArg, line);
    res = RvLogTextAny(source, RV_LOGID_WARNING, line, &varArg);
    va_end(varArg);

    return res;
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_WARNING) */


#if (RV_LOGMASK & RV_LOGLEVEL_INFO)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextInfo(
    IN  RvLogSource*        source,
    IN  const char*         line, ...)
{
    RvStatus res;
    va_list varArg;

    va_start(varArg, line);
    res = RvLogTextAny(source, RV_LOGID_INFO, line, &varArg);
    va_end(varArg);

    return res;
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_INFO) */


#if (RV_LOGMASK & RV_LOGLEVEL_DEBUG)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextDebug(
    IN  RvLogSource*        source,
    IN  const char*         line, ...)
{
    RvStatus res;
    va_list varArg;

    va_start(varArg, line);
    res = RvLogTextAny(source, RV_LOGID_DEBUG, line, &varArg);
    va_end(varArg);

    return res;
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_DEBUG) */


#if (RV_LOGMASK & RV_LOGLEVEL_ENTER)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextEnter(
    IN  RvLogSource*        source,
    IN  const char*         line, ...)
{
    RvStatus res;
    va_list varArg;

    va_start(varArg, line);
    res = RvLogTextAny(source, RV_LOGID_ENTER, line, &varArg);
    va_end(varArg);

    return res;
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_ENTER) */


#if (RV_LOGMASK & RV_LOGLEVEL_LEAVE)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextLeave(
    IN  RvLogSource*        source,
    IN  const char*         line, ...)
{
    RvStatus res;
    va_list varArg;

    va_start(varArg, line);
    res = RvLogTextAny(source, RV_LOGID_LEAVE, line, &varArg);
    va_end(varArg);

    return res;
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_LEAVE) */


#if (RV_LOGMASK & RV_LOGLEVEL_SYNC)
RVCOREAPI
RvStatus RVCALLCONV RvLogTextSync(
    IN  RvLogSource*        source,
    IN  const char*         line, ...)
{
    RvStatus res;
    va_list varArg;

    va_start(varArg, line);
    res = RvLogTextAny(source, RV_LOGID_SYNC, line, &varArg);
    va_end(varArg);

    return res;
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_SYNC) */

/********************************************************************************************
 * RvLogSourcePrintInterfacesData - Prints information of the interfaces data to the log.
 *
 * The interfaces data string should be given from the function RvCCoreInterfaces().
 *
 * INPUT   : source             - Source of messages to log to
 *           interfacesString   - Strings to log, as given by RvCCoreInterfaces()
 * OUTPUT  : None
 * RETURN  : None
 */
#if (RV_LOGMASK & RV_LOGLEVEL_INFO)
RVCOREAPI void RVCALLCONV RvLogSourcePrintInterfacesData(
    IN RvLogSource*     source,
    IN const RvChar*    interfacesString)
{
    RvChar* curString;

    if (!RvLogIsSelected(source, RV_LOGLEVEL_INFO))
    {
        /* Seems like this message is filtered out... */
        return;
    }

    curString = (RvChar *)interfacesString;
    while ((*curString) != '\0')
    {
        /* We don't print the first 4 characters since they are used for Unix 'what' utility */
        RvLogTextInfo(source, "%s", curString + 4);
        curString += strlen(curString)+1;
    }
}
#endif  /* (RV_LOGMASK & RV_LOGLEVEL_INFO) */

/********************************************************************************************
 * RvLogGetMessageCount - Returns the number of messages of a specified type that
 *                        sent to log.
 *
 * INPUT   : logMgr         - The log manager
 *           messageType    - The required message type
 * OUTPUT  : None
 * RETURN  : None
 */
RVCOREAPI RvInt RVCALLCONV RvLogGetMessageCount(
    IN RvLogMgr*        logMgr,
    IN RvInt            messageType)
{
    return logMgr->statInfo[messageType];
}

/********************************************************************************************
 * RvLogGetMessageCount - Configures the log manager to print the thread id instead
 *                        of the thread name.
 *
 * INPUT   : logMgr     - The log manager
 * OUTPUT  : None
 * RETURN  : None
 */
RVCOREAPI void RVCALLCONV RvLogPrintThreadId(
    IN RvLogMgr*        logMgr)
{
    logMgr->printThreadId = RV_TRUE;
}

#if defined(__cplusplus)
}
#endif

#endif /* (RV_LOGMASK != RV_LOGLEVEL_NONE) */
