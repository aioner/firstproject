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
#include "rvansi.h"
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
            RvSprintf(c,"%d",(int)(RvTimestampGet(NULL)/1000) - startTime);
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
/*
#if defined(LOG_LISTENER_HIGH_PRECISION)
#define LOG_RECORD_HEADER_SIZE 53
#else
#define LOG_RECORD_HEADER_SIZE 49
#endif
*/

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
 * logFormatMessage2 - Format a log record into a printable string
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
RvChar* RVCALLCONV logFormatMessage2(
    IN  RvLogRecord*    logRecord,
    IN  RvLogListener*  listener,
    OUT RvUint32*       size)
{
    RvChar* buf;
    RvChar* threadName;
    RvChar  threadId[20];
    RvTime t;
    RvTm tm;
    RvUint32 len;
    RvInt logHeaderSize;
    RvBool printDate, printMilliSecs;
    RvChar dateTxt[10], millisecsTxt[5];

    /* LOG_RECORD_HEADER_SIZE = TaskName(8)+1+Date(2+1+2+1+2)+1+
                            Time(2+1+2+1+2+1+3)+1+ModuleName(12)+
                            2+LogType(5)+3*/

    logHeaderSize = 53;


    if (listener)
    {
        printDate = listener->printDate;
        printMilliSecs = listener->printMilliSecs;
    }
    else {
        printDate = RV_TRUE;
        printMilliSecs = RV_FALSE;
    }


    if (!printDate)
        logHeaderSize -= 9; /*Date(2+1+2+1+2)+1*/
    if (!printMilliSecs)
        logHeaderSize -= 4;

    /* Move back in the text - there's free space there for our formatting */
    buf = (RvChar *)RvLogRecordGetText(logRecord) - logHeaderSize;

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
        RvSprintf(threadId,"%p",(void*)threadName);
        threadName = threadId;
    }

    if (printDate)
        RvSprintf(dateTxt,"%2.2u/%2.2u/%2.2u ", RvTmGetMon(&tm), RvTmGetMday(&tm), RvTmGetYear(&tm) % 100);
    else
        dateTxt[0] = 0;

    if (printMilliSecs)
        RvSprintf(millisecsTxt,".%3.3u",((RvUint32)(RvTmGetNsec(&tm)/RV_TIME64_NSECPERMSEC)) % 1000);
    else
        millisecsTxt[0] = 0;

/*
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
*/
    RvSprintf(buf, "%8.8s %s%2.2u:%2.2u:%2.2u%s %-12.12s: %s -",
        threadName,
        dateTxt,
        RvTmGetHour(&tm), RvTmGetMin(&tm), RvTmGetSec(&tm),
        millisecsTxt,
        RvLogSourceGetName(RvLogRecordGetSource(logRecord)),
        msgTypeStr[RvLogRecordGetMessageType(logRecord)]);

    /* We're done with time information */
    RvTimeDestruct(&t);
    RvTmDestruct(&tm);

    /* This one is for the addition of the actual string after the formatted header */
    buf[logHeaderSize - 1] = ' ';

    /* Add a newline in the end and calculate the size of message */
    len = (RvUint32)strlen(buf);
    buf[len] = '\0';
    if (size != NULL)
        *size = len;

    /* Return the formatted buffer */
    return buf;
}

RVCOREAPI
RvChar* RVCALLCONV logFormatMessage(
    IN  RvLogRecord*    logRecord,
    OUT RvUint32*       size)
{
    return logFormatMessage2(logRecord,NULL,size);
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
#if (RV_LOGLISTENER_TYPE != RV_LOGLISTENER_UDP)

#if (RV_OS_TYPE == RV_OS_TYPE_OSE)
	RvPrintf("%s\r\n", logFormatMessage2(logRecord, NULL, NULL)); /* use dbgprintf, and it requires \r */
#else

    FILE* f = (FILE *)userData;

    /* We use %s so we don't get formatting characters by mistake into the message - this might
       cause us to crash */
    RvFprintf(f, "%s\n", logFormatMessage2(logRecord, NULL,NULL));
#endif
#endif

    RV_UNUSED_ARG(logRecord);
    RV_UNUSED_ARG(userData);
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
        logInfo->curFileNum++;
        if (logInfo->curFileNum > logInfo->numFiles)
            logInfo->curFileNum = 1;

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


#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI) && (RV_OS_VERSION < RV_OS_SYMBIAN_9)
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

        if (logInfo->logMessage == NULL)
            break;

        RvSemaphorePost(&logInfo->logIsReady, NULL);
    }

    if (openSts == RV_OK)
        fclose(logInfo->openedFile);

    RvSemaphorePost(&logInfo->logIsReady, NULL);
    
    return;
}

void logSendMessage(LogFileListener* logInfo, RvChar* logMessage, RvUint32 logMessageSize)
{
    if (logInfo->logSymbThread)
    {
        logInfo->logMessage     = logMessage;
        logInfo->logMessageSize = logMessageSize;
        RvSemaphorePost(&logInfo->message, NULL);
        RvSemaphoreWait(&logInfo->logIsReady, NULL);
        if (!logMessage)
        {
            RvThreadDestruct((RvThread*)logInfo->logSymbThread);
            RvSemaphoreDestruct(&logInfo->logIsReady,NULL);
            RvSemaphoreDestruct(&logInfo->message,NULL);
            free(logInfo->logSymbThread);                
            logInfo->logSymbThread = NULL;
        }
    }
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
    RvUint32 logMessageSize, logMessageSize2;
    RvLogListener* listener;

    RV_UNUSE_GLOBALS;

    /* Make sure we didn't encounter any errors while trying to open a logfile */
    if (logInfo->openError) return;

    /* See if we have to switch to another file */
    if (logInfo->openedFile == NULL ||
        (logInfo->numFiles > 1 && logInfo->curSize > logInfo->maxFileSize))
    {
#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE) || (RV_OS_VERSION >= RV_OS_SYMBIAN_9)
        /* We need to open the next file... */
        if (logPrintOpenFile(logInfo) != RV_OK)
            return;
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
    listener = (RvLogListener*)((RvChar*)logInfo - RV_OFFSETOF(RvLogListener,logFileListener));
    logMessage = logFormatMessage2(logRecord, listener, &logMessageSize);

    logMessageSize2 = logMessageSize;
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    logMessage[logMessageSize2++] = '\r';
#endif
    logMessage[logMessageSize2++] = '\n';

#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
    {
        DWORD bytesWritten;
        WriteFile(logInfo->openedFile, logMessage, logMessageSize, &bytesWritten, NULL);
        WriteFile(logInfo->openedFile, "\r\n", 2, &bytesWritten, NULL);
    }
#elif (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE) || (RV_OS_VERSION >= RV_OS_SYMBIAN_9)
    fwrite(logMessage, logMessageSize2, 1, logInfo->openedFile);
#else
    /* Send the message to the log thread */
    logSendMessage(logInfo, logMessage, logMessageSize2);
#endif

    logMessage[logMessageSize] = '\0';

    /* See if we have to flush the logfile after adding the message */
    if (logInfo->flushLines)
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
        FlushFileBuffers(logInfo->openedFile);
#elif (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE) || (RV_OS_VERSION >= RV_OS_SYMBIAN_9)
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

    logMessage = logFormatMessage2(logRecord, NULL, &logMessageSize);

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

    logMessage = logFormatMessage2(logRecord, NULL, &logMessageSize);
    logMessage[logMessageSize] = '\n';
    logMessage[logMessageSize+1] = '\0';
    RvOutputDebugPrintf(logMessage);
    logMessage[logMessageSize] = '\0';
}

#endif  /* (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) */



#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_UDP)
#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS)
#define SOCKET_OPEN NU_Socket
#define SOCKET_CLOSE NU_Close_Socket
#define SOCKET_SEND_TO NU_Send_To
#define RV_SOCKADDR_PTR struct sockaddr_in *
#else
#define SOCKET_OPEN socket
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#define SOCKET_CLOSE closesocket
#else
#define SOCKET_CLOSE close
#endif
#define SOCKET_SEND_TO sendto
#define RV_SOCKADDR_PTR struct sockaddr *
#endif


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
    RvInt len,ret;

    if (logInfo == NULL)
        return;

    if (logInfo->udpSocket == -1)
    {
#if (RV_SOCKET_USE_SHARER == RV_YES)
        RvSocketConstruct(RV_ADDRESS_TYPE_IPV4,RvSocketProtocolUdp,NULL,&(logInfo->udpSocket));
#else  /* #if (RV_NET_TYPE != RV_NET_NONE) */
        logInfo->udpSocket = SOCKET_OPEN(AF_INET, SOCK_DGRAM, 0);
#endif /* #if (RV_NET_TYPE != RV_NET_NONE) */

        memset(&logInfo->remoteAddress, 0, sizeof(logInfo->remoteAddress));
        logInfo->remoteAddress.sin_family = AF_INET;
        logInfo->remoteAddress.sin_port   = htons(logInfo->port);
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
        logText = logFormatMessage2(logRecord, NULL, NULL);
    else
        logText = "FILE CLOSE:";

#if 0
    printToMemory(logText,strlen(logText));
    return;
#endif


    /* Note: 10 is added to strlen of logText because
       of the extra data printed in the following sprintf (nnnnnnnn ...NL) */
    len = strlen(logText);
    if (!(logRecord == NULL ||
        (len + 10) >= (int)(size_t)(LOG_BUFF_SIZE - (logInfo->logBufPtr - logInfo->logBuffer))))
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

        ret = SOCKET_SEND_TO(sockToUse, logInfo->logBuffer, logInfo->logBufPtr-logInfo->logBuffer, 0,
            (RV_SOCKADDR_PTR)&logInfo->remoteAddress, sizeof(logInfo->remoteAddress));

        logInfo->logBufPtr = logInfo->logBuffer;

        if (logRecord == NULL)
            SOCKET_SEND_TO(sockToUse, logText, strlen(logText),0,(RV_SOCKADDR_PTR)&logInfo->remoteAddress, sizeof(logInfo->remoteAddress));
    }

    RvSprintf(logInfo->logBufPtr, "%8d %s\n", logInfo->serialNum++, logText);
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

    listener->printDate = RV_TRUE;
    listener->printMilliSecs = RV_FALSE;

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


#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI) && (RV_OS_VERSION < RV_OS_SYMBIAN_9)
void RvLogListenerStartSymbianLogThread(LogFileListener* logInfo)
{
    RvChar threadName[20];
    RvStatus status;
    RvUint done = 0;
    RV_USE_CCORE_GLOBALS;
    
#define MALLOC_DONE     0x01
#define SEM1_DONE       0x02
#define SEM2_DONE       0x04
#define THR_CONSTR_DONE 0x08
    
    /*      static int logThreadNum = 0; */
    
    /* Create log thread */
    
    status = RV_ERROR_UNKNOWN;
    
    logInfo->logSymbThread = malloc(sizeof(RvThread));
    if (logInfo == NULL)
        goto roll_down;
    done |= MALLOC_DONE;
    
    if (RvSemaphoreConstruct(0, NULL, &logInfo->message) != RV_OK)
        goto roll_down;
    done |= SEM1_DONE;
    if (RvSemaphoreConstruct(0, NULL, &logInfo->logIsReady) != RV_OK)
        goto roll_down;        
    done |= SEM2_DONE;
    
    
    status = RvThreadConstruct((RvThreadFunc)logPrintThread, logInfo, 
        NULL, (RvThread*)logInfo->logSymbThread);
    if (status != RV_OK)
        goto roll_down;
    done |= THR_CONSTR_DONE;
    
    RvSprintf(threadName, "LogThread%d", ++logThreadNum);
    RvThreadSetName((RvThread*)logInfo->logSymbThread, threadName);
    status = RvThreadCreate((RvThread*)logInfo->logSymbThread);
    if (status != RV_OK)
        goto roll_down;
    
    status = RvThreadStart((RvThread*)logInfo->logSymbThread);
    if (status != RV_OK)
        goto roll_down; 
    
roll_down:
    if (status != RV_OK)
    {
        if (done & THR_CONSTR_DONE)
            RvThreadDestruct((RvThread*)logInfo->logSymbThread);
        if (done & SEM2_DONE)
            RvSemaphoreDestruct(&logInfo->logIsReady,NULL);
        if (done & SEM1_DONE)
            RvSemaphoreDestruct(&logInfo->message,NULL);
        if (done & MALLOC_DONE)
            free(logInfo->logSymbThread);                
        logInfo->logSymbThread = NULL;
    }
}
#endif


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
        listener->logFileListener.curFileNum = 0;
        listener->logFileListener.curSize = 0;
        listener->logFileListener.openError = RV_FALSE;
        listener->logFileListener.openedFile = NULL;

#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI) && (RV_OS_VERSION < RV_OS_SYMBIAN_9)
        RvLogListenerStartSymbianLogThread(&listener->logFileListener);
#endif
        
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
* RvLogListenerFlushFiles
*
* Causes to flush the file based log listener.
*
* INPUT   : logMgr             - Log manager
* OUTPUT  : None.
* RETURN  : RV_OK on success, other values on failure
*/
RVCOREAPI
void RVCALLCONV RvLogListenerFlushFiles(
    IN  RvLogMgr*       logMgr)
{
#if ((RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || \
    (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL))
    RvStatus ret;
    RvInt cnt;
    LogFileListener *logFileListener = NULL;

    ret = RvLockGet(&logMgr->lock,NULL);
    if (ret != RV_OK)
        return;

    for (cnt = 0; cnt < logMgr->numListeners; cnt++)
    {
        if (logMgr->listener[cnt] == logPrintLogfile)
        {
            logFileListener = logMgr->listenerUserData[cnt];
            if (logFileListener && logFileListener->openedFile && !logFileListener->openError)
            {
#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
                FlushFileBuffers(logFileListener->openedFile);
#else
                fflush(logFileListener->openedFile);
#endif
            }
        }
    }

    RvLockRelease(&logMgr->lock,NULL);

#else
    RV_UNUSED_ARG(logMgr);
#endif
}


/********************************************************************************************
 * RvLogListenerGetLogfile
 *
 * Get the log file listener
 *
 * INPUT   : logMgr             - Log manager to listen to
 * OUTPUT  : None.
 * RETURN  : Log listener or NULL
 */
RVCOREAPI
RvLogListener * RVCALLCONV RvLogListenerGetLogfile(
    IN  RvLogMgr*       logMgr)
{
#if ((RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || \
    (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL))
    void * ctx;
    RvLogListener * fileListener;

    ctx = RvLogFindListenerCtxByCb(logMgr, logPrintLogfile);
    if (ctx == NULL) return NULL;

    fileListener = RV_GET_STRUCT(RvLogListener, logFileListener, ctx);
    return fileListener;
#else
    RV_UNUSED_ARG(logMgr);
    return NULL;
#endif  /* ((RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || \
    (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)) */
}


/********************************************************************************************
 * RvLogListenerReconfigureLogfile
 *
 * Reconfigure a log listener that sends log messages to a file
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
RvStatus RVCALLCONV RvLogListenerReconfigureLogfile(
    IN  RvLogListener*  listener,
    IN  RvLogMgr*       logMgr,
    IN  const RvChar*   fileName,
    IN  RvUint32        numFiles,
    IN  RvUint32        fileSize,
    IN  RvBool          flushEachMessage,
    IN  RvBool          printMilliSecs,
	IN	const RvChar*	extention )
{
#if ((RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32) || \
    (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL))
    RV_UNUSED_ARG(logMgr);

    if (numFiles == 0) numFiles = 1;
    if (((fileSize != 0) || (numFiles > 1)) && (fileSize < 100000)) fileSize = 100000;

    if (fileName != NULL)
    {
        strncpy(listener->logFileListener.baseFilename, fileName,
                sizeof(listener->logFileListener.baseFilename)-10);
    }

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
    listener->logFileListener.curFileNum = 0;
    listener->logFileListener.curSize = 0;
    listener->logFileListener.openError = RV_FALSE;
    listener->printMilliSecs = printMilliSecs;

#if (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_WIN32)
    if (listener->logFileListener.openedFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(listener->logFileListener.openedFile);
        listener->logFileListener.openedFile = NULL;
    }
#elif (RV_LOGLISTENER_TYPE == RV_LOGLISTENER_FILE_AND_TERMINAL)
#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE) || (RV_OS_VERSION >= RV_OS_SYMBIAN_9)
    if (listener->logFileListener.openedFile != NULL)
    {
        fclose(listener->logFileListener.openedFile);
        listener->logFileListener.openedFile = NULL;
    }
#else
    logSendMessage(&listener->logFileListener, NULL, 0);
#endif
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_SYMBIAN) && (RV_THREADNESS_TYPE == RV_THREADNESS_MULTI) && (RV_OS_VERSION < RV_OS_SYMBIAN_9)
        RvLogListenerStartSymbianLogThread(&listener->logFileListener);
#endif

    return  RV_OK;
#else
    RV_UNUSED_ARG(listener);
    RV_UNUSED_ARG(logMgr);
    RV_UNUSED_ARG(fileName);
    RV_UNUSED_ARG(numFiles);
    RV_UNUSED_ARG(fileSize);
    RV_UNUSED_ARG(printMilliSecs);
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
    {
        RvUint32 n = listener->logFileListener.curFileNum;
        if (n == 0)
            n++;
        RvSprintf(fileName, "%s%.4d", listener->logFileListener.baseFilename,n);
    }
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
#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN) || (RV_THREADNESS_TYPE == RV_THREADNESS_SINGLE) || (RV_OS_VERSION >= RV_OS_SYMBIAN_9)
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
    RvAddress rvaddr;

    if (RvMemoryAlloc(NULL, (RvSize_t)(sizeof(LogUdpListener)),NULL,(void**)&udpListener) != RV_OK)
        return RvLogListenerErrorCode(RV_ERROR_OUTOFRESOURCES);

    strncpy(udpListener->fileName, fileName,
        sizeof(udpListener->fileName)-10);

    RvAddressConstructIpv4(&rvaddr,0,0);
    RvAddressSetString(serverIpAddr,&rvaddr);

    udpListener->ipAddr = rvaddr.data.ipv4.ip;
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
        SOCKET_CLOSE(((LogUdpListener*)(listener->logUdpListener))->udpSocket);
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
