/* rvloglistener.c - Description: rvloglistener file - message log listener */
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
#include "rvthread.h"
#include "rvclock.h"
#include "rvtm.h"
#include "rvstdio.h"
#include "rvloglistener.h"
#include "rvsocket.h"
#include "rvtimestamp.h"
#include "rvccoreglobals.h"
#include <stdlib.h>

#if 0 /* this code is used if we want to save the debug messages in memory */

RVCOREAPI char BigBuff[8000];
RVCOREAPI char* BigBuffPtr;
RVCOREAPI int BigBuffCnt = 0;

void printToMemory(char* txt, int sz)
{
    {
        static unsigned int startTime = 0;
        char *c;
        if (startTime == 0)
        {
            startTime = (int)(RvTimestampGet(NULL)/1000);
        }
        c = strchr(txt+5,' ');
        if (c)
        {
            c++;
            memset(c,' ',15);
            sprintf(c,"%d",(int)(RvTimestampGet(NULL)/1000) - startTime);
            c[strlen(c)] = ' ';
        }
    }



    if (BigBuffCnt+sz > sizeof(BigBuff))
    {
        BigBuffCnt = 0;
        BigBuffPtr = BigBuff;
    }
    if (BigBuffCnt == 0)
    {
        BigBuffPtr = BigBuff;
        memset(BigBuffPtr,0,sizeof(BigBuff));
    }

    memcpy(BigBuffPtr,txt,sz);
    BigBuffCnt += sz;
    BigBuffPtr += sz;
}
#endif /* #if 0 */


#if (RV_LOGLISTENER_TYPE != RV_LOGLISTENER_NONE)
/* Make sure we have the internal listeners set */

/* Set high-precision times (milliseconds) on log messages */
/*#define LOG_LISTENER_HIGH_PRECISION*/


/* Lets make error codes a little easier to type */
#define RvLogListenerErrorCode(_e) RvErrorCode(RV_ERROR_LIBCODE_CCORE, RV_CCORE_MODULE_LOGLISTENER, (_e))



/* Types of listeners */
typedef enum
{
    RV_LOGLISTENER_TYPE_TERMINAL = 0,
    RV_LOGLISTENER_TYPE_LOGFILE,
    RV_LOGLISTENER_TYPE_DEBUG,
    RV_LOGLISTENER_TYPE_UDP,
    /* Should be last */
    RV_LOGLISTENER_TYPE_LAST
} RvLogListnerType;

/* LOG_RECORD_HEADER_SIZE = TaskName(8)+1+Date(2+1+2+1+2)+1+
                            Time(2+1+2+1+2+1+3)+1+ModuleName(12)+
                            2+LogType(5)+3*/
#if defined(LOG_LISTENER_HIGH_PRECISION)
#define LOG_RECORD_HEADER_SIZE 53
#else
#define LOG_RECORD_HEADER_SIZE 49
#endif

static const char msgTypeStr[][16] = {
    "EXCEP",
    "ERROR",
    "WARN ",
    "INFO ",
    "DEBUG",
    "ENTER",
    "LEAVE",
    "SYNC ",
    "????"
};

/********************************************************************************************
 *
 *                                  Internal functions
 *
 ********************************************************************************************/


/********************************************************************************************
 * logFormatMessage - Format a log record into a printable string
 * NOTE: Although this function is defined as 'RVCOREAPI' it is used
 * only internally. It is defined as 'RVCOREAPI' to satisfy the GateKeaper needs.
 * To make sure that customer will not use that function, no prototype for it
 * will be provided.
 *
 * INPUT   : logRecord  - Information related with the logged message
 * OUTPUT  : size       - Size of formatter message in characters
 *                        Can be passed as NULL
 * RETURN  : Pointer to string holding the formatted text
 */
RVCOREAPI
RvChar* RVCALLCONV logFormatMessage(
    IN  RvLogRecord*    logRecord,
    OUT RvUint32*       size)
{
    RvChar* buf;
    RvChar* threadName;
    RvChar  threadId[20];
    RvTime t;
    RvTm tm;
    RvUint32 len;

    /* Move back in the text - there's free space there for our formatting */
    buf = (RvChar *)RvLogRecordGetText(logRecord) - LOG_RECORD_HEADER_SIZE;

    /* Get the time of the log */
    RvClockGet(NULL,&t);
    RvTmConstructLocal(&t,NULL,&tm);

    /* Format the additional information into the reserved space in the beginning of the buffer */
    /* IF any changed are made here the LOG_RECORD_HEADER_SIZE & RV_LOG_RESERVED_BYTES
       should be fixed accordingly */
    /* TaskName(8)+1+Date(2+1+2+1+2)+1+Time(2+1+2+1+2)+1+ModuleName(12)+2+LogType(5)+3 = */

    threadName = RvLogRecordGetThread(logRecord);
    if (logRecord->printThreadId == RV_TRUE)
    {
        RvSprintf(threadId,"%p",threadName);
        threadName = threadId;
    }

#if defined(LOG_LISTENER_HIGH_PRECISION)
    RvSprintf(buf, "%8.8s %2.2u/%2.2u/%2.2u %2.2u:%2.2u:%2.2u.%3.3u %-12.12s: %s -",
        threadName,
        RvTmGetMon(&tm), RvTmGetMday(&tm), RvTmGetYear(&tm) % 100,
        RvTmGetHour(&tm), RvTmGetMin(&tm), RvTmGetSec(&tm),
        ((RvUint32)(RvTmGetNsec(&tm)/RV_TIME64_NSECPERMSEC)) % 1000,
#else
    RvSprintf(buf, "%8.8s %2.2u/%2.2u/%2.2u %2.2u:%2.2u:%2.2u %-12.12s: %s -",
        threadName,
        RvTmGetMon(&tm), RvTmGetMday(&tm), RvTmGetYear(&tm) % 100,
        RvTmGetHour(&tm), RvTmGetMin(&tm), RvTmGetSec(&tm),
#endif
        RvLogSourceGetName(RvLogRecordGetSource(logRecord)),
        msgTypeStr[RvLogRecordGetMessageType(logRecord)]);

    /* We're done with time information */
    RvTimeDestruct(&t);
    RvTmDestruct(&tm);

    /* This one is for the addition of the actual string after the formatted header */
    buf[LOG_RECORD_HEADER_SIZE - 1] = ' ';

    /* Add a newline in the end and calculate the size of message */
    len = (RvUint32)strlen(buf);
    buf[len] = '\0';
    if (size != NULL)
        *size = len;

    /* Return the formatted buffer */
    return buf;
}


/********************************************************************************************
 * logPrintTerminal
 *
 * Callback that is executed whenever a message has to be logged to the terminal
 *
 * INPUT   : logRecord  - Information related with the logged message
 *           userData   - stdout or stderr (FILE*)
 * OUTPUT  : None
 * RETURN  : None
 */
static void RVCALLCONV logPrintTerminal(
    IN RvLogRecord* logRecord,
    IN void*        userData)
{
#if (RV_OS_TYPE == RV_OS_TYPE_OSE)
	RvPrintf("%s\r\n", logFormatMessage(logRecord, NULL)); /* use dbgprintf, and it requires \r */
#else

    FILE* f = (FILE *)userData;

    /* We use %s so we don't get formatting characters by mistake into the message - this might
       cause us to crash */
    fprintf(f, "%s\n", logFormatMessage(logRecord, NULL));
#endif
}



#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)

RvStatus logPrintOpenFile(LogFileListener* logInfo)
{
    RvChar fname[300];
    RvChar* fnamePtr;

    /* Close the currently opened one */
    if (logInfo->openedFile != NULL)
    {
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
        CloseHandle(logInfo->openedFile);
#else
        fclose(logInfo->openedFile);
#endif
        logInfo->openedFile = NULL;
        logInfo->curSize = 0;
    }

    /* Determine the filename */
    if (logInfo->numFiles > 1)
    {
        logInfo->curFileNum = (logInfo->curFileNum + 1) % logInfo->numFiles;
		if (*logInfo->filenameExtention != 0)
		{ /* we need to add extention to the filename */
			RvSprintf(fname, "%s_%03d.%s", logInfo->baseFilename,
						logInfo->curFileNum, logInfo->filenameExtention);
		}
		else
		{ /* no need for file extention */
			RvSprintf(fname, "%s_%03d", logInfo->baseFilename, logInfo->curFileNum);
		}

        fnamePtr = fname;
    }
    else
	{
		if (*logInfo->filenameExtention != 0)
		{ /* we need to add extention to the filename no file Numbering required */
			RvSprintf(fname, "%s.%s", logInfo->baseFilename,
						logInfo->filenameExtention);
			fnamePtr = fname;
		}
		else
		{ /* no need for file extention or numbering of files*/
			fnamePtr = logInfo->baseFilename;
		}
	}


    /* Open the next logfile */
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    logInfo->openedFile = CreateFile((LPCTSTR)fnamePtr, GENERIC_WRITE,
        FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    logInfo->openError = (logInfo->openedFile == INVALID_HANDLE_VALUE);
#elif (RV_OS_TYPE == RV_OS_TYPE_WINCE)
    {
        WCHAR strwc[1024];
        RvInt stringlen = strlen(fnamePtr);

        /* We have to convert the (multibyte) string to wide-character-string */
        MultiByteToWideChar(CP_ACP, 0, fnamePtr, stringlen, strwc, stringlen);
        strwc[stringlen] = (WCHAR)0;
        logInfo->openedFile = CreateFile((LPCTSTR)strwc, GENERIC_WRITE,
            FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        logInfo->openError = (logInfo->openedFile == INVALID_HANDLE_VALUE);
    }
#endif /* Win types */
#else
    logInfo->openedFile = RvFopen(fnamePtr, "wb");
    logInfo->openError = (logInfo->openedFile == NULL);
#endif
    /* If we're having any error opening files we shouldn't continue here */
    if (logInfo->openError)
        return RV_ERROR_UNKNOWN;

    return RV_OK;
}


#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI)
void logPrintThread(RvThread* th, void* data)
{
    LogFileListener* logInfo = (LogFileListener *)data;
    RvStatus openSts;

    RV_UNUSED_ARG(th);

    openSts = logPrintOpenFile(logInfo);

    for (;;)
    {
        RvSemaphoreWait(&logInfo->message, NULL);

        if (openSts == RV_OK)
        {
            if (logInfo->logMessage)
                fwrite(logInfo->logMessage, logInfo->logMessageSize, 1, logInfo->openedFile);

            if (logInfo->flushLines)
                fflush(logInfo->openedFile);
        }

        RvSemaphorePost(&logInfo->logIsReady, NULL);

        if (logInfo->logMessage == NULL)
            break;
    }

    if (openSts == RV_OK)
        fclose(logInfo->openedFile);

    return;
}

void logSendMessage(LogFileListener* logInfo, RvChar* logMessage, RvUint32 logMessageSize)
{
    logInfo->logMessage     = logMessage;
    logInfo->logMessageSize = logMessageSize;
    RvSemaphorePost(&logInfo->message, NULL);
    RvSemaphoreWait(&logInfo->logIsReady, NULL);
}
#endif

/********************************************************************************************
 * logPrintLogfile
 *
 * Callback that is executed whenever a message has to be logged to the logfile
 *
 * INPUT   : logRecord  - Information related with the logged message
 *           userData   - LogFileListener struct
 * OUTPUT  : None
 * RETURN  : None
 */
static void RVCALLCONV logPrintLogfile(
    IN RvLogRecord* logRecord,
    IN void*        userData)
{
    RV_USE_CCORE_GLOBALS;
    LogFileListener* logInfo = (LogFileListener *)userData;
    RvChar* logMessage;
    RvUint32 logMessageSize;

    RV_UNUSE_GLOBALS;

    /* Make sure we didn't encounter any errors while trying to open a logfile */
    if (logInfo->openError) return;

    /* See if we have to switch to another file */
    if (logInfo->openedFile == NULL ||
        (logInfo->numFiles > 1 && logInfo->curSize > logInfo->maxFileSize))
    {
#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE)
        /* We need to open the next file... */
        if (logPrintOpenFile(logInfo) != RV_OK)
            return;
#else
        RvChar threadName[20];
/*      static int logThreadNum = 0; */

        /* Create log thread */
        RvSemaphoreConstruct(0, NULL, &logInfo->message);
        RvSemaphoreConstruct(0, NULL, &logInfo->logIsReady);

        logInfo->logThread = malloc(sizeof(RvThread));
        RvThreadConstruct((RvThreadFunc)logPrintThread, userData, NULL, (RvThread*)logInfo->logThread);
        sprintf(threadName, "LogThread%d", ++logThreadNum);
        RvThreadSetName((RvThread*)logInfo->logThread, threadName);
        RvThreadCreate((RvThread*)logInfo->logThread);
        RvThreadStart((RvThread*)logInfo->logThread);
#endif
    }

    if ((logInfo->numFiles == 1) && (logInfo->maxFileSize != 0) &&
        (logInfo->curSize > logInfo->maxFileSize))
    {
        /* move to the beginning of the file */
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
        SetFilePointer(logInfo->openedFile, 0, NULL, FILE_BEGIN);
#else
        RvFseek(logInfo->openedFile, 0, SEEK_SET);
#endif
        logInfo->curSize = 0;
    }

    /* At long last - put the message in the logfile */
    logMessage = logFormatMessage(logRecord, &logMessageSize);
    logMessage[logMessageSize] = '\n';

#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
    {
        DWORD bytesWritten;
        WriteFile(logInfo->openedFile, logMessage, logMessageSize, &bytesWritten, NULL);
        WriteFile(logInfo->openedFile, "\r\n", 2, &bytesWritten, NULL);
    }
#elif (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE)
    fwrite(logMessage, logMessageSize+1, 1, logInfo->openedFile);
#else
    /* Send the message to the log thread */
    logSendMessage(logInfo, logMessage, logMessageSize+1);
#endif

    logMessage[logMessageSize] = '\0';

    /* See if we have to flush the logfile after adding the message */
    if (logInfo->flushLines)
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
        FlushFileBuffers(logInfo->openedFile);
#elif (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE)
        fflush(logInfo->openedFile);
#endif

    /* Make sure we are aware of the logfile's size */
    logInfo->curSize += logMessageSize;
}

#endif /* ((RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || \
           (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)) */



#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)

#if 0
/********************************************************************************************
 * logPrintWindow
 *
 * Callback that is executed whenever a message has to be logged to a window
 *
 * INPUT   : logRecord  - Information related with the logged message
 *           userData   - WindowListener struct
 * OUTPUT  : None
 * RETURN  : None
 */
static void RVCALLCONV logPrintWindow(
    IN RvLogRecord* logRecord,
    IN void*        userData)
{
    WindowListener* winInfo = (WindowListener *)userData;
    RvChar* logMessage;
    RvUint32 logMessageSize;
    RvStatus ret;

    logMessage = logFormatMessage(logRecord, &logMessageSize);

    ret = RvLockGet(&winInfo->lock);
    assert(ret == RV_OK);

    /* Make sure we have enough space in pending messages buffer */
    while (logMessageSize > (winInfo->bufferSize - winInfo->curUsedBufSize))
    {
        RvLockRelease(&winInfo->lock); /* Must release this lock before calling SendMessage! */
        SendMessage(winInfo->hWnd, LOGWND_PENDING, 0, 0);
        ret = RvLockGet(&winInfo->lock);
        assert(ret == RV_OK);
    }

    /* Copy the message to the pending buffer */
    memcpy(winInfo->bufferedMessages + winInfo->curUsedBufSize, logMessage, logMessageSize);
    winInfo->curUsedBufSize += logMessageSize;

    RvLockRelease(&winInfo->lock);

    /* Send indication that we have a pending message */
    PostMessage(winInfo->hWnd, LOGWND_PENDING, 0, 0);
}
#endif

/********************************************************************************************
 * logPrintDebug
 *
 * Callback that is executed whenever a message has to be logged to debug
 *
 * INPUT   : logRecord  - Information related with the logged message
 *           userData   - NULL
 * OUTPUT  : None
 * RETURN  : None
 */
static void RVCALLCONV logPrintDebug(
    IN RvLogRecord* logRecord,
    IN void*        userData)
{
    RvChar* logMessage;
    RvUint32 logMessageSize;

    RV_UNUSED_ARG(userData);

    logMessage = logFormatMessage(logRecord, &logMessageSize);
    logMessage[logMessageSize] = '\n';
    logMessage[logMessageSize+1] = '\0';
    RvOutputDebugPrintf(logMessage);
    logMessage[logMessageSize] = '\0';
}

#endif  /* (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) */



#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_UDP)

#define LOG_BUFF_SIZE 1400

#define MAX_SHARSOCKETS 100

typedef struct
{
    RvChar              fileName[256];
    RvUint32            ipAddr;
    RvUint16            port;
    int                 udpSocket;
#if (RV_SOCKET_USE_SHARER == RV_YES)
    RvSocket            sharedSockets[MAX_SHARSOCKETS];
    RvThreadId          threadIds[MAX_SHARSOCKETS];
    RvUint32            sharedSocketsCnt;
#endif
    struct sockaddr_in  remoteAddress;
    int                 serialNum;
    RvChar*             logBufPtr;
    RvChar              logBuffer[LOG_BUFF_SIZE];
} LogUdpListener;

/********************************************************************************************
 * logPrintUdp
 *
 * Callback that is executed whenever a message has to be logged to the terminal
 *
 * INPUT   : logRecord  - Information related with the logged message
 *           userData   - stdout or stderr (FILE*)
 * OUTPUT  : None
 * RETURN  : None
 */
static void RVCALLCONV logPrintUdp(
    IN RvLogRecord* logRecord,
    IN void*        userData)
{
    LogUdpListener* logInfo = (LogUdpListener *)userData;
    RvChar* logText;

    if (logInfo == NULL)
        return;

    if (logInfo->udpSocket == -1)
    {
#if (RV_SOCKET_USE_SHARER == RV_YES)
        RvSocketConstruct(RV_ADDRESS_TYPE_IPV4,RvSocketProtocolUdp,NULL,&(logInfo->udpSocket));
#else  /* #if (RV_NET_TYPE != RV_NET_NONE) */
        logInfo->udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
#endif /* #if (RV_NET_TYPE != RV_NET_NONE) */

        memset(&logInfo->remoteAddress, 0, sizeof(logInfo->remoteAddress));
        logInfo->remoteAddress.sin_family      = AF_INET;
        logInfo->remoteAddress.sin_port        = logInfo->port;
#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
        memcpy(&(logInfo->remoteAddress.id.is_ip_addrs), &logInfo->ipAddr, 4);
#else  /* #if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS) */
        logInfo->remoteAddress.sin_addr.s_addr = logInfo->ipAddr;
#endif /* #if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS) */

        logInfo->serialNum = 1;
        logInfo->logBufPtr = logInfo->logBuffer;

        RvSprintf(logInfo->logBufPtr, "FILE OPEN: %s\n", logInfo->fileName);
        logInfo->logBufPtr += strlen(logInfo->logBufPtr);
    }

    if (logRecord != NULL)
        logText = logFormatMessage(logRecord, NULL);
    else
        logText = "FILE CLOSE:";

#if 0
    printToMemory(logText,strlen(logText));
    return;
#endif


    /* Note: 10 is added to strlen of logText because
       of the extra data printed in the following sprintf (nnnnnnnn ...NL) */
    if (logRecord == NULL ||
        (strlen(logText) + 10) >= (size_t)(LOG_BUFF_SIZE - (logInfo->logBufPtr - logInfo->logBuffer)))
    {
        int sockToUse;

#if (RV_SOCKET_USE_SHARER == RV_YES)
        /* first define the socket that will be used */
        {
            RvUint cnt = 0;
            RvThreadId myId;
            myId = RvThreadCurrentId();
            for (cnt = 0; cnt < logInfo->sharedSocketsCnt; cnt++)
            {
                if (myId == logInfo->threadIds[cnt])
                    break;
            }
            if (cnt < logInfo->sharedSocketsCnt)
                sockToUse = logInfo->sharedSockets[cnt];
            else
            {
                RvSocketSharerShare(NULL,&(logInfo->udpSocket),&sockToUse);
                if (sockToUse < 0)
                    return;

                logInfo->sharedSockets[logInfo->sharedSocketsCnt] = sockToUse;
                logInfo->threadIds[logInfo->sharedSocketsCnt] = myId;
                logInfo->sharedSocketsCnt ++;
            }
        }

#else
        sockToUse = logInfo->udpSocket;
#endif

        sendto(sockToUse, logInfo->logBuffer, logInfo->logBufPtr-logInfo->logBuffer, 0,
            (struct sockaddr *)&logInfo->remoteAddress, sizeof(logInfo->remoteAddress));

        logInfo->logBufPtr = logInfo->logBuffer;

        if (logRecord == NULL)
            sendto(sockToUse, logText, strlen(logText),0,(struct sockaddr *)&logInfo->remoteAddress, sizeof(logInfo->remoteAddress));
    }

    sprintf(logInfo->logBufPtr, "%8d %s\n", logInfo->serialNum++, logText);
    logInfo->logBufPtr += strlen(logInfo->logBufPtr);
}

#endif  /* (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_UDP) */



/********************************************************************************************
 * logListenerConstruct - Construct a generic log listener
 *
 * INPUT   : listener   - Listener to construct
 *           logMgr     - Log manager to listen to
 *           printCb    - Callback for message logs
 *           userData   - User data to use
 *           type       - Type of listener
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
static RvStatus logListenerConstruct(
    IN  RvLogListener*  listener,
    IN  RvLogMgr*       logMgr,
    IN  RvLogPrintCb    printCb,
    IN  void*           userData,
    IN  int             type)
{
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if ((listener == NULL) || (logMgr == NULL))
        return RvLogListenerErrorCode(RV_ERROR_NULLPTR);
#endif

    if (type < (int)RV_LOGLISTENER_TYPE_TERMINAL || type >= (int)RV_LOGLISTENER_TYPE_LAST) {
        return RvLogListenerErrorCode(RV_ERROR_UNKNOWN);
    }

    listener->logMgr = logMgr;
    listener->listenerType = type;
    return RvLogRegisterListener(logMgr, printCb, userData);
}


#endif  /* (RV_LOGLISTENER_TYPE != RV_LOGLISTENER_NONE) */


/********************************************************************************************
 *
 *                                  Public functions
 *
 ********************************************************************************************/


/********************************************************************************************
 * RvLogListenerInit - inits log listener module
 *
 * This function should be called only once in the process
 *
 * INPUT   : none
 * OUTPUT  : None.
 * RETURN  : always RV_OK
 */
RvStatus RvLogListenerInit(void)
{
    return RV_OK;
}

/********************************************************************************************
 * RvLogListenerEnd - shut down log listener module
 *
 * This function should be called only once in the process
 *
 * INPUT   : none
 * OUTPUT  : None.
 * RETURN  : always RV_OK
 */
RvStatus RvLogListenerEnd(void)
{
    return RV_OK;
}


#if  (RV_LOGLISTENER_TYPE != RV_LOGLISTENER_NONE)
/* Make sure we have the internal listeners set */


/********************************************************************************************
 * RvLogListenerConstructTerminal
 *
 * Construct a log listener that sends log messages to the terminal, using
 * standard output or standard error
 *
 * INPUT   : listener   - Listener to construct
 *           logMgr     - Log manager to listen to
 *           stdOut     - RV_TRUE for stdout, RV_FALSE for stderr
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerConstructTerminal(
    IN  RvLogListener*  listener,
    IN  RvLogMgr*       logMgr,
    IN  RvBool          stdOut)
{
    FILE* termStream;
    if (stdOut)
        termStream = stdout;
    else
        termStream = stderr;

    return logListenerConstruct(listener, logMgr, logPrintTerminal, termStream, RV_LOGLISTENER_TYPE_TERMINAL);
}


/********************************************************************************************
 * RvLogListenerDestructTerminal - Destruct the terminal listener
 *
 * INPUT   : listener   - Listener to destruct
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerDestructTerminal(
    IN  RvLogListener*  listener)
{
    return RvLogUnregisterListener(listener->logMgr, logPrintTerminal);
}




/********************************************************************************************
 * RvLogListenerConstructLogfileWithExtention
 *
 * Construct a log listener that sends log messages to a file
 *
 * INPUT   : listener           - Listener to construct
 *           logMgr             - Log manager to listen to
 *           fileName           - Name of the logfile
 *           numFiles           - Number of cyclic files to use
 *           fileSize           - Size of each file in cycle in bytes
 *                                This parameter is only applicable if numFiles > 1
 *           flushEachMessage   - RV_TRUE if we want to flush each message written to the
 *                                logfile
 *			 extention			- the extention of the logfile.
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerConstructLogfileWithExtention(
    IN  RvLogListener*  listener,
    IN  RvLogMgr*       logMgr,
    IN  const RvChar*   fileName,
    IN  RvUint32        numFiles,
    IN  RvUint32        fileSize,
    IN  RvBool          flushEachMessage,
	IN	const RvChar*	extention )
{
#if ((RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || \
    (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL))
    RvStatus ret;

    ret = logListenerConstruct(listener, logMgr, logPrintLogfile,
                               &listener->logFileListener, RV_LOGLISTENER_TYPE_LOGFILE);

    if (ret == RV_OK)
    {
        if (numFiles == 0) numFiles = 1;
        if (((fileSize != 0) || (numFiles > 1)) && (fileSize < 100000)) fileSize = 100000;
        strncpy(listener->logFileListener.baseFilename, fileName,
                sizeof(listener->logFileListener.baseFilename)-10);
		if ((extention != NULL) && (*extention != 0))
		{
			RvUint extLen = (RvUint)strlen(extention), up;
			up = extLen;
			if (up > sizeof(listener->logFileListener.filenameExtention)-1)
				up = sizeof(listener->logFileListener.filenameExtention)-1;
			strncpy(listener->logFileListener.filenameExtention, extention,up);
			listener->logFileListener.filenameExtention[up] = 0;
		}
		else
		{
			*(listener->logFileListener.filenameExtention) = 0;
		}

        listener->logFileListener.numFiles = numFiles;
        listener->logFileListener.maxFileSize = fileSize;
        listener->logFileListener.flushLines = flushEachMessage;
        listener->logFileListener.curFileNum = numFiles;
        listener->logFileListener.curSize = 0;
        listener->logFileListener.openError = RV_FALSE;
        listener->logFileListener.openedFile = NULL;
    }

    return  ret;
#else
    RV_UNUSED_ARG(listener);
    RV_UNUSED_ARG(logMgr);
    RV_UNUSED_ARG(fileName);
    RV_UNUSED_ARG(numFiles);
    RV_UNUSED_ARG(fileSize);
    RV_UNUSED_ARG(flushEachMessage);
	RV_UNUSED_ARG(extention);
    return RvLogListenerErrorCode(RV_ERROR_NOTSUPPORTED);
#endif  /* ((RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || \
    (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)) */
}


/********************************************************************************************
 * RvLogListenerLogfileGetCurrentFilename
 *
 * Get the filename of the current file being written.
 *
 * INPUT   : listener       - Listener to check
 *           fileNameLength - Maximum length of filename
 * OUTPUT  : fileName       - Filename of the current file being written
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerLogfileGetCurrentFilename(
    IN  RvLogListener*  listener,
    IN  RvUint32        fileNameLength,
    OUT RvChar*         fileName)
{
    RV_UNUSED_ARG(listener);

#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)
#if (RV_CHECK_MASK & RV_CHECK_NULL)
    if (fileName == NULL || listener == NULL)
        return RvLogListenerErrorCode(RV_ERROR_NULLPTR);
#endif

    if (fileNameLength < strlen(listener->logFileListener.baseFilename) + 5)
        return RvLogListenerErrorCode(RV_ERROR_OUTOFRANGE);

    if (listener->logFileListener.numFiles > 1)
        RvSprintf(fileName, "%s%.4d", listener->logFileListener.baseFilename,
                  listener->logFileListener.curFileNum);
    else
        strcpy(fileName, listener->logFileListener.baseFilename);

    return RV_OK;
#else
    RV_UNUSED_ARG(fileNameLength);
    RV_UNUSED_ARG(fileName);
    return RvLogListenerErrorCode(RV_ERROR_NOTSUPPORTED);
#endif
}


/********************************************************************************************
 * RvLogListenerDestructLogfile - Destruct the logfile listener
 *
 * INPUT   : listener   - Listener to destruct
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerDestructLogfile(
    IN  RvLogListener*  listener)
{
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
    if (listener->logFileListener.openedFile != INVALID_HANDLE_VALUE)
        CloseHandle(listener->logFileListener.openedFile);
#elif (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)
#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE)
    if (listener->logFileListener.openedFile != NULL)
        fclose(listener->logFileListener.openedFile);
#else
    logSendMessage(&listener->logFileListener, NULL, 0);
#endif
#endif

#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)
    return RvLogUnregisterListener(listener->logMgr, logPrintLogfile);
#else
    RV_UNUSED_ARG(listener);
    return RvLogListenerErrorCode(RV_ERROR_NOTSUPPORTED);
#endif
}


/********************************************************************************************
 * RvLogListenerConstructDebug
 *
 * Construct a log listener that sends log messages to the debug window of Visual C.
 * This one is only applicable for Win32 applications.
 *
 * INPUT   : listener   - Listener to construct
 *           logMgr     - Log manager to listen to
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerConstructDebug(
    IN  RvLogListener*  listener,
    IN  RvLogMgr*       logMgr)
{
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
    return logListenerConstruct(listener, logMgr, logPrintDebug, NULL, RV_LOGLISTENER_TYPE_DEBUG);
#else
    RV_UNUSED_ARG(listener);
    RV_UNUSED_ARG(logMgr);
    return RvLogListenerErrorCode(RV_ERROR_NOTSUPPORTED);
#endif
}


/********************************************************************************************
 * RvLogListenerDestructDebug - Destruct the debug listener
 *
 * INPUT   : listener   - Listener to destruct
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerDestructDebug(
    IN  RvLogListener*  listener)
{
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
    return RvLogUnregisterListener(listener->logMgr, logPrintDebug);
#else
    RV_UNUSED_ARG(listener);
    return RvLogListenerErrorCode(RV_ERROR_NOTSUPPORTED);
#endif
}


/********************************************************************************************
 * RvLogListenerConstructUdp
 *
 * Construct a log listener that sends log messages to server using UDP protocol.
 *
 * INPUT   : listener   - Listener to construct
 *           logMgr     - Log manager to listen to
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerConstructUdp(
    IN  RvLogListener*  listener,
    IN  RvLogMgr*       logMgr,
    IN  RvChar*         fileName,
    IN  RvChar*         serverIpAddr,
    IN  RvUint16        serverPort)
{
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_UDP)
    RvStatus ret;
    LogUdpListener* udpListener;

    if (RvMemoryAlloc(NULL, (RvSize_t)(sizeof(LogUdpListener)),NULL,(void**)&udpListener) != RV_OK)
        return RvLogListenerErrorCode(RV_ERROR_OUTOFRESOURCES);

    strncpy(udpListener->fileName, fileName,
        sizeof(udpListener->fileName)-10);
    udpListener->ipAddr = inet_addr(serverIpAddr);
    udpListener->port   = serverPort;
#if (RV_SOCKET_USE_SHARER == RV_YES)
    udpListener->sharedSocketsCnt = 0;
#endif
    udpListener->udpSocket = -1;
    listener->logUdpListener = udpListener;

    ret = logListenerConstruct(listener, logMgr, logPrintUdp,
                               listener->logUdpListener, RV_LOGLISTENER_TYPE_UDP);

    if (ret != RV_OK)
    {
        RvMemoryFree(listener->logUdpListener,NULL);
        listener->logUdpListener = NULL;
    }


    return  ret;
#else
    RV_UNUSED_ARG(listener);
    RV_UNUSED_ARG(logMgr);
    RV_UNUSED_ARG(fileName);
    RV_UNUSED_ARG(serverIpAddr);
    RV_UNUSED_ARG(serverPort);
    return RvLogListenerErrorCode(RV_ERROR_NOTSUPPORTED);
#endif  /* ((RV_LOGLISTENER_TYPE == RV_LOGLISTENER_UDP) */
}


/********************************************************************************************
 * RvLogListenerDestructUdp - Destruct the UDP listener
 *
 * INPUT   : listener   - Listener to destruct
 * OUTPUT  : None.
 * RETURN  : RV_OK on success, other values on failure
 */
RVCOREAPI
RvStatus RVCALLCONV RvLogListenerDestructUdp(
    IN  RvLogListener*  listener)
{
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_UDP)

    if (listener->logUdpListener)
    {
        logPrintUdp(NULL, &listener->logUdpListener);
#if (RV_SOCKET_USE_SHARER == RV_YES)
        RvSocketDestruct(&(((LogUdpListener*)(listener->logUdpListener))->udpSocket),RV_FALSE,NULL,NULL);
#else
        CLOSE_SOCKET(((LogUdpListener*)(listener->logUdpListener))->udpSocket);
#endif
        RvMemoryFree(listener->logUdpListener,NULL);
        listener->logUdpListener = NULL;
    }
    return RvLogUnregisterListener(listener->logMgr, logPrintUdp);
#else
    RV_UNUSED_ARG(listener);
    return RvLogListenerErrorCode(RV_ERROR_NOTSUPPORTED);
#endif
}


#endif  /* (RV_LOGLISTENER_TYPE != RV_LOGLISTENER_NONE) */
