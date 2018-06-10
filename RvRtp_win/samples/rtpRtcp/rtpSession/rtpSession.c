/***********************************************************************
        Copyright (c) 2005 RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Ltd.. No part of this document may be reproduced in any
form whatsoever without written prior approval by RADVISION Ltd..

RADVISION Ltd. reserve the right to revise this publication and make
changes without obligation to notify any person of such revisions or
changes.
***********************************************************************/
/*********************************************************************
 *                       rtpH.c                                *
 *                                                                   *
 * The following program is a simple console application that        *
 * demonstrates how to implement RTP sending and                     *
 * receiving capabilities using the                                  *
 * RADVISION Advanced RTP Protocol Toolkit.                          *
 * The program does the following:                                   *
 * 1. Initializes the RTP and RTCP Stacks.                           *
 * 2. Opens RTP and RTCP sessions on the local address.              *
 * 3. Sets remote addresses for RTP and RTCP sessions.               *
 * 4  Sends "dummy" g.711 PCMU 20 ms packets.                        *
 * 5. Automatically sends and receives RTCP reports.                 *
 * 6. Closes RTP and RTCP sessions.                                  *
 * 7. Shuts down RTP and RTCP stacks.                                *
 * This program must be compiled with #define APP_1 and without one. *
 * Both applications must be launched at the same time on            *
 * the same platform for RTP demonstration.                          *
 * NOTE: On embedded operation systems this sample must be executed  *
 * on two different patforms. Pay attention, that this case requires *
 * remote IP address to be specified instead of 127.0.0.1.           *
 * On the first platform APP_1 has to be defined; on the second one  *
 * it has to be not defined.                                         *                                   *
 *********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdarg.h>
#include "rvansi.h"
#include "osDependency.h"
#include "rtp.h"
#include "rtcp.h"
#include "rvrtpseli.h"
#include "payload.h"

#define RTP_NUMBER_PACKETS_TO_SEND         (1000)

#if (RV_OS_TYPE != RV_OS_TYPE_IPHONE)
/*#define APP_1*/
#endif

#if defined(APP_1)
#define RTP_SESSION_RECEIVING_PORT         (5500)
#define RTP_SESSION_SENDING_PORT           (6500)
#define RTP_SESSION_LOCAL_ADDR              "10.0.2.15"
#define RTP_SESSION_REMOTE_ADDR             "10.0.2.15"
#else
#define RTP_SESSION_RECEIVING_PORT         (6500)
#define RTP_SESSION_SENDING_PORT           (5500)
#define RTP_SESSION_LOCAL_ADDR              "172.16.70.148"
#define RTP_SESSION_REMOTE_ADDR             "172.28.53.193"
#endif
/*-----------------------------------------------------------------------*/
/*                           MODULE VARIABLES                            */
/*-----------------------------------------------------------------------*/

static RvRtpSession  rtpH  = NULL;                       /* Stores RTP session handle  */
static RvRtcpSession rtcpH = NULL;                       /* Stores RTCP session handle */
static RvUint32 receivedPackets  = 0;                    /* Stores counter of received RTP packets */
static RvUint32 rtpTimestamp     = 12345;                /* Stores RTP sending timestamp */

/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS PROTOTYPES                 */
/*-----------------------------------------------------------------------*/

static int SendRTPPacket(int packetNumber);

static void rtpEventHandler(
        IN  RvRtpSession  hRTP,
        IN  void *       context);

static RvBool Rv_inet_pton4(const RvChar *src, RvUint8 *dst);

#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN && RV_OS_TYPE != RV_OS_TYPE_WINCE)
static int ScOSPrintf(IN const char *format,... );
#endif

/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/
#if RV_ANDROID
static int RtpSessionMain(void)
#else
/* Main application */
#if (RV_OS_TYPE == RV_OS_TYPE_NUCLEUS || RV_OS_TYPE == RV_OS_TYPE_PSOS)
int mainForEmbedded(void)
#elif (RV_OS_TYPE != RV_OS_TYPE_VXWORKS && RV_OS_TYPE != RV_OS_TYPE_INTEGRITY)
int RtpSessionMain(int argc, char* argv[])
#else
int RtpSessionMain(void)
#endif
#endif /* RV_ANDROID */
{
    RvStatus      status     =  RV_ERROR_UNKNOWN;
    RvNetAddress  rtpReceivingAddress;
    RvNetAddress  rtpSendingAddress;
    RvNetAddress  rtcpSendingAddress;
    RvInt32       pack       = 0;                /* packet number */
    RvNetIpv4     address;

#if (RV_OS_TYPE != RV_OS_TYPE_VXWORKS && RV_OS_TYPE != RV_OS_TYPE_PSOS && RV_OS_TYPE != RV_OS_TYPE_NUCLEUS && RV_OS_TYPE != RV_OS_TYPE_INTEGRITY && !RV_ANDROID)
    RV_UNUSED_ARG(argc); /* just to remove warning */
    RV_UNUSED_ARG(argv); /* just to remove warning */
#endif
#if defined(APP_1)
   ScOSPrintf("Sample compiled with APP_1\n");
#else
   ScOSPrintf("Sample compiled without APP_1\n");
#endif
    /* Initialization of RTP stack */
    status = RvRtpInit();

    if (status != RV_OK)
    {
        ScOSPrintf("ERROR: RvRtpInit() failed\n");
        return status;
    }

    /* Initialization of RTCP Stack (requires SELI interface for automatic RTCP reports) */
    status = RvRtcpInit();

    if (status != RV_OK)
    {
        ScOSPrintf("ERROR: RvRtcpInit() failed\n");
        return status;
    }

    /* Initialization of SELI interface */
    RvRtpSeliInit();

    /* Creating RTP listening address (ip:port [:scope - for IPV6] )*/
    address.port   = RTP_SESSION_RECEIVING_PORT;
    Rv_inet_pton4(RTP_SESSION_LOCAL_ADDR, (RvUint8 *)&address.ip); /* local address */

    /* Creating IPV4 RTP listening address */
    RvNetCreateIpv4(&rtpReceivingAddress, &address);

    /* Opening RTP and RTCP sessions */
    /* SSRC will be generated as pseudo random number */
    /* SDES must be unique for all RTP session participants */
    rtpH  = RvRtpOpenEx(&rtpReceivingAddress, 0, 0, "RTP sender 8273423");
    if (rtpH == NULL)
    {
        /* For example, it can occur when specified port is busy */
        ScOSPrintf("ERROR: RvRtpOpenEx() cannot open RTP session.\n");
        return -1;
    }
    ScOSPrintf("RTP session is opened on port %d\n", RTP_SESSION_RECEIVING_PORT);
    rtcpH = RvRtpGetRTCPSession(rtpH);

    address.port    = RTP_SESSION_SENDING_PORT;
    Rv_inet_pton4(RTP_SESSION_REMOTE_ADDR, (RvUint8 *)&address.ip); /* local address */

    /* Creating IPV4 RTP sending address */
    RvNetCreateIpv4(&rtpSendingAddress, &address);

    RvRtpAddRemoteAddress(rtpH, &rtpSendingAddress);

    address.port    = RTP_SESSION_SENDING_PORT + 1;

    /* Creating RTCP sending address (RTP port+1) */
    RvNetCreateIpv4(&rtcpSendingAddress, &address);

    if (rtcpH != NULL)
    {
        /* Remote RTCP address setting for RTCP session */
        RvRtcpAddRemoteAddress(rtcpH, &rtcpSendingAddress);
        /* RvRtcpSessionSetParam(rtcpH, RVRTCP_PARAMS_RTPCLOCKRATE, 8000);  clock rate - needed for dynamic payload         types */
    }

    RvRtpSetEventHandler(rtpH, (RvRtpEventHandler_CB)rtpEventHandler, NULL);

    ScOSPrintf("waiting for synchronization\n");
    RvRtpSeliSelectUntil(20000); /* just for synchronization between 2 applications */

    for (pack=0; pack < RTP_NUMBER_PACKETS_TO_SEND; pack++)
    {
      /*
         The following calls are responsible for activating the process that selects
         all registered file descriptors and calls to the appropriate callback
         function when an event occurs.
         Actually we have a number of cases when RvRtpSeliSelectUntil(20) exits without waiting
         20 ms for RTCP automatic periodic report processing (for sending and receiving).
       */
             RvRtpSeliSelectUntil(0);
             RvRtpSeliSelectUntil(0);
             RvRtpSeliSelectUntil(20);

        /********************************************************************************
         * IMPORTANT:                                                                   *
         * The following function SendRTPPacket() is usually called                     *
         * in a separate thread with delays of 20 ms,                                   *
         * since it is required for g.711 20 ms packets.                                *
         * It is called here just for demonstration of RvRtpWrite() usage.              *
         * We have no common interface for creating threads on many operating systems   *
         * that we support. Therefore we demonstrate RvRtpWrite usage in this way.      *
         * Pay attention that RvRtpSeliSelectUntil() also invokes RTCP periodic         *
         * sending and receiving. In case these events occur,                           *
         * the delay between RTP sending will be lower than 20 ms.                      *
         ********************************************************************************/
        SendRTPPacket(pack);
        RvRtpSeliSelectUntil(20);   /* just to receive packets if available */
    }

    /* Just to receive the remained packets */
    for (pack=0; pack < RTP_NUMBER_PACKETS_TO_SEND; pack++)
        RvRtpSeliSelectUntil(20);

    /* Closes RTP and RTCP sessions. */
    status = RvRtpClose(rtpH);
    if (status != RV_OK)
    {
        ScOSPrintf("ERROR: RvRtpClose() cannot close RTP session.\n");
        return -1;
    }

    /* Ends SELI interface*/
    status = RvRtpSeliEnd();
    if (status != RV_OK)
    {
        ScOSPrintf("ERROR: RvRtpSeliEnd() failed.\n");
        return -1;
    }

    /* Shuts down the RTCP module.  */
    status = RvRtcpEnd();
    if (status != RV_OK)
    {
        ScOSPrintf("ERROR: RvRtcpEnd() cannot finish to work with RTCP Stack.\n");
        return -1;
    }

    /* Shuts down the RTP module.  */
    RvRtpEnd();

    ScOSPrintf( "\nreceived %d packets\n", receivedPackets);

    return status;
}

/*-----------------------------------------------------------------------*/
/*                           STATIC FUNCTIONS                            */
/*-----------------------------------------------------------------------*/


/******************************************************************************
 * SendRTPPacket() sends dummy g.711 PCMU RTP packet to the remote RTP session *
 * packetNumber - packet number                                               *
 ******************************************************************************/

static int SendRTPPacket(int packetNumber)
{
    unsigned char       buffer2send[200];
    RvRtpParam p;

    /* Initializing RvRtpParam structure */
    RvRtpParamConstruct(&p);

    /* Initializing buffer */
    memset(buffer2send, 0, sizeof(buffer2send));

    /* Getting the RTP header size with payload header size (0 - in case of g.711) */
    p.sByte = RvRtpPCMUGetHeaderLength();

    /* Setting the payload header - no payload header in g.711 case */
    /* Putting RTP payload type into structure p (for RTP header)*/
    RvRtpPCMUPack(buffer2send, 160, &p, NULL);

    /* "Dummy RTP packet number %d" - Just to fill the buffer */
    /* Pay attention that the payload information begins after p.sByte bytes */
    sprintf((char*)(buffer2send + p.sByte), "Dummy RTP packet number %d", packetNumber);
    rtpTimestamp += 160;
    p.timestamp = rtpTimestamp; /* 20 ms packet, G.711  sample rate = 8000 samples/sec, 20ms packet => 160 */
    /* p.sequenceNumber will be incremented automatically */

    /* 160 bytes - packet size */
    /* Sequence number is calculated automatically */
    /* RvRtpWrite sends the RTP packet
       160+p.sByte is RTP header size (12) + payload header(0) +  payload(160) = 172 */
    return RvRtpWrite(rtpH, buffer2send, 160 + p.sByte, &p); /* 20 ms of g.711 PCMU packet sending */
}

/*****************************************************************
 * This function is the implementation of the
 * RTP event handler for the RTP session.
 * It is called when RTP packets are received from the network
 * Parameters:
 * hRTP - Handle of RTP session
 * context - Context that was set by RvRtpSetEventHandler().
 * returns: NONE.
 *****************************************************************/
static void rtpEventHandler(
        IN  RvRtpSession  hRTP,
        IN  void *       context)
{
    int status = -1;
    unsigned char bufferToreceive[200];
    RvRtpParam p;

    RV_UNUSED_ARG(context); /* just to remove warning */

    /* Reading the RTP message */
    status = RvRtpRead    (hRTP, bufferToreceive, sizeof(bufferToreceive), &p);
    if (status < 0)
        return; /* error */

    RvRtpPCMUUnpack(bufferToreceive,  sizeof(bufferToreceive), &p, NULL/* no payload header for g.711 */);

    /* Here, p contains all related to RTP header information.
       In this place actual procession of payload may be done */

    if (memcmp(bufferToreceive + p.sByte, "Dummy RTP packet number", strlen("Dummy RTP packet number"))==0)
    {
        ScOSPrintf ("*"); /* received right packet */
        receivedPackets++;
    }
    else
    {
        ScOSPrintf ("\nreceived wrong packet\n");
    }
    return;
}


 /* int
 * inet_pton4(src, dst)
 *  like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *  1 if `src' is a valid dotted quad, else 0.
 * notice:
 *  does not touch `dst' unless it's returning 1.
 * author:
 *  Paul Vixie, 1996.
 */
static RvBool Rv_inet_pton4(const RvChar *src, RvUint8 *dst)
{
    static RvChar digits[] = "0123456789";
    RvInt saw_digit, octets, ch;
    RvUint8 tmp[4], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0' && (ch != ':')) {
        RvChar *pch;

        if ((pch = strchr(digits, ch)) != NULL) {
            RvUint newValue = (RvUint)(*tp * 10 + (pch - digits));

            if (newValue > 255)
                return RV_FALSE;
            *tp = (RvUint8)newValue;
            if (! saw_digit) {
                if (++octets > 4)
                    return RV_FALSE;
                saw_digit = 1;
            }
        } else if (ch == '.' && saw_digit) {
            if (octets == 4)
                return RV_FALSE;
            *++tp = 0;
            saw_digit = 0;
        } else
            return RV_FALSE;
    }
    if (octets < 4)
        return RV_FALSE;
    memcpy(dst, tmp, 4);
    return RV_TRUE;
}

#if RV_ANDROID
/***************************************************************************
* SAMPLE_NAME
* ------------------------------------------------------------------------
* General: The main function for Android. Will be replaced at compilation time
*          with the project name itself.
***************************************************************************/
void rtpSession(int bStart)
{
	if(bStart)
	{
		RtpSessionMain();
	}
}
#endif

#if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN && RV_OS_TYPE != RV_OS_TYPE_WINCE)

#if (RV_OS_TYPE == RV_OS_TYPE_IPHONE)
int OsIphoneVPrintf(const char *format, va_list args);
#endif

/***************************************************************************
 * ScOSPrintf
 * ------------------------------------------------------------------------
 * General: Implementation of ScOSPrintf for different Operating Systems.
 *          (Print formatted output to the standard output stream.)
 * Return Value: The number of characters printed, or a negative value
 *               if an error occurs.
 *-------------------------------------------------------------------------
 * Arguments:
 * Input: format - Format control.
 *        There might be additional parameters according to the format.
 *-------------------------------------------------------------------------
 ***************************************************************************/
static int ScOSPrintf(IN const char *format,... )
{
    int      charsNo;
    va_list  ap;

	va_start(ap,format);

#if RV_ANDROID
	charsNo = OsAndroidVPrintf(format, ap);
#elif (RV_OS_TYPE == RV_OS_TYPE_IPHONE)
    charsNo = OsIphoneVPrintf(format, ap);
#else
    charsNo = vprintf(format,ap);
    fflush(stdout);
#endif
	va_end(ap);

    return charsNo;
}

#endif /* #if (RV_OS_TYPE != RV_OS_TYPE_SYMBIAN && RV_OS_TYPE != RV_OS_TYPE_WINCE) */

#ifdef __cplusplus
}
#endif

