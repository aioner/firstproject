/************************************************************************
 File Name     : rvrtpencryptionmode.h
 Description   :
*************************************************************************
 Copyright (c)  2002 , RADVision, Inc. All rights reserved.
*************************************************************************
 NOTICE:
 This document contains information that is proprietary to RADVISION Inc. 
 No part of this publication may be reproduced in any form whatsoever
 without written prior approval by RADVISION Inc. 
 
 RADVISION Inc. reserves the right to revise this publication and make
 changes without obligation to notify any person of such revisions or
 changes.
*************************************************************************
 $Revision: $
 $Date:   05/09/2002 $
 $Author: Scott K. Eaton $
************************************************************************/

#if !defined(RVRTPENCRYPTIONMODE_H)
#define RVRTPENCRYPTIONMODE_H

#include "rvtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************
   This is an enumeration of encryption modes in which the stack can run.
   enumeration:
   value: RV_RTPENCRYPTIONMODE_RFC1889 or RV_RTPENCRYPTIONMODE_RFC3550
         Follow the RFC 1889 mode of encryption where
         the entire RTP and RTCP packet is encrypted. This is the only mode
         currently fully supported by the supplied encryption plug-ins.In this mode 
         the RvRtpEncryptionId is not used.
   value: RV_RTPENCRYPTIONMODE_H235_PADDING
         Follow H.235 annex D7, Do not encrypt the RTP header. Allow 
         padding to be added to the data to fill it out to the encryption
         algorithm's block size. Do not encrypt RTCP packets. In this mode the 
         RvRtpEncryptionId will be the RTP packet's payload type. The suppled 
         encryption plug-ins support this mode in a limited manner since they only
         use one key and will not switch keys based on payload type.
   value: RV_RTPENCRYPTIONMODE_H235_CIPHERTEXTSTEALING
         Follow H.235 annex D7, Do not encrypt the RTP header. Do not 
         pad the data, instead use ciphertext stealing to encrypt 
         the last block if it is not a complete block. Do not encrypt 
         RTCP packets.In this mode the RvRtpEncryptionId will be the RTP 
         packet's payload type. To support this mode you would need to write an
         encryption plug-in that supports ciphertext stealing, and key selection
         based on the payload type. 
**************************************************************************************/

typedef RvUint32 RvRtpEncryptionMode;

#define RV_RTPENCRYPTIONMODE_FLAG_PLAINNTEXTRTPHEADER  0x00000001
#define RV_RTPENCRYPTIONMODE_FLAG_NOPADDING            0x00000002
#define RV_RTPENCRYPTIONMODE_FLAG_PLAINNTEXTRTCP       0x00000004
#define RV_RTPENCRYPTIONMODE_FLAG_ID_PAYLOADTYPE       0x00000008
#define RV_RTPENCRYPTIONMODE_FLAG_ID_MKI               0x00000010

#define RvRtpEncryptionModeIsPlainTextRtcp(t)      (((t)&RV_RTPENCRYPTIONMODE_FLAG_PLAINNTEXTRTCP) > 0)
#define RvRtpEncryptionModeIsNoPadding(t)          (((t)&RV_RTPENCRYPTIONMODE_FLAG_NOPADDING) > 0)
#define RvRtpEncryptionModeIsPlainTextRtpHeader(t) (((t)&RV_RTPENCRYPTIONMODE_FLAG_PLAINNTEXTRTPHEADER) > 0)
#define RvRtpEncryptionModeIsPayloadTypeId(t)      (((t)&RV_RTPENCRYPTIONMODE_FLAG_ID_PAYLOADTYPE) > 0)
#define RvRtpEncryptionModeIsMkiId(t)              (((t)&RV_RTPENCRYPTIONMODE_FLAG_ID_MKI) > 0)
#define RvRtpEncryptionModeHasId(t)                (((t)&(RV_RTPENCRYPTIONMODE_FLAG_ID_PAYLOADTYPE|RV_RTPENCRYPTIONMODE_FLAG_ID_MKI)) > 0)

#define RV_RTPENCRYPTIONMODE_RFC1889                 (0)  
#define RV_RTPENCRYPTIONMODE_RFC3550                 (RV_RTPENCRYPTIONMODE_RFC1889)
    
#define RV_RTPENCRYPTIONMODE_H235_PADDING            (RV_RTPENCRYPTIONMODE_FLAG_PLAINNTEXTRTCP|RV_RTPENCRYPTIONMODE_FLAG_PLAINNTEXTRTPHEADER|RV_RTPENCRYPTIONMODE_FLAG_ID_PAYLOADTYPE)
#define RV_RTPENCRYPTIONMODE_H235_CIPHERTEXTSTEALING (RV_RTPENCRYPTIONMODE_H235_PADDING|RV_RTPENCRYPTIONMODE_FLAG_NOPADDING)


#ifdef __cplusplus
}
#endif

#endif  /* Include guard */

