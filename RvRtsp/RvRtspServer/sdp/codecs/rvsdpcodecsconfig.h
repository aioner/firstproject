/******************************************************************************
Filename    :rvsdpcodecsconfig.h
Description : definitions regarding use of codecs configurations

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

#ifndef _rvsdpcodecsconfig_h_
#define _rvsdpcodecsconfig_h_

#include "rvsdpconfig.h"

#ifdef RV_SDP_CODECS_SUPPORTED

/*
 *	Defines the supported codecs
 */

/* XXX - Add New Codec */
#define RV_SDP_CODEC_H263

#define RV_SDP_CODEC_H261

#define RV_SDP_CODEC_MP4A_LATM

#define RV_SDP_CODEC_MP4V_ES

#define RV_SDP_CODEC_AMR



#define RV_SDP_CODEC_DTMF

#define RV_SDP_CODEC_G726

#define RV_SDP_CODEC_G7231

#define RV_SDP_CODEC_G729

#define RV_SDP_CODEC_G722

#define RV_SDP_CODEC_PCMU

#define RV_SDP_CODEC_PCMA


#endif /*RV_SDP_CODECS_SUPPORTED*/

 #endif /* #ifndef _rvsdpcodecsconfig_h_ */
