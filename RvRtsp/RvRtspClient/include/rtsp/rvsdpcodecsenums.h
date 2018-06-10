/******************************************************************************
Filename    :rvsdpcodecsenums.h
Description : differents enumerations

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

#ifndef _file_rvsdpcodecsenums_h_
#define _file_rvsdpcodecsenums_h_

#ifdef RV_SDP_CODECS_SUPPORTED

/* An attribute that represent a codec data looks like:
  "a = fmtp XX name:value ;name:value ;name:value ;name ;name" 
  for example: a=fmtp:98 profile-level-id=1;config=000001B001000001B509" 
  (in this example the codec's parameters names are :"profile-level-id" & "config")
  parameter's name can appear with no value.
   
  In a codec line there is a list of parameters of "name:value" or only "name"
  Here are all the names that can appear in this list:*/


/* Some codecs retreived data from other attributes in the media level
   such as 'a=ptime' or 'a=maxptime'.these attributes are common to some codecs*/ 
#define    RV_SDPCODECS_ATTR_DATA_PTIME               "ptime"
#define    RV_SDPCODECS_ATTR_DATA_MAXPTIME            "maxptime" /*used by AMR*/
#define    RV_SDP_CODEC_ATTR_DATA_DTMF_OUT_OF_BAND              "OutOfBandDtmf"  /*used by DTMF ?????????*/



/*Most of the codecs retreives their data from the 'a=fmtp' attribute.
Here are the legal parameters name for each codec */

/*legal names for G_7231*/
#define    RV_SDPCODECS_FMTP_G7231_AUDIO_FRAMES		 "audioFrames"
#define    RV_SDPCODECS_FMTP_G7231_SILENCE_SUPPORT   "SilenceSupp"

/*legal names for G_729*/
#define    RV_SDPCODECS_FMTP_G729_ANNEXA          "annexa"          
#define    RV_SDPCODECS_FMTP_G729_ANNEXB          "annexb"      
#define    RV_SDPCODECS_FMTP_G729_SILENCE_SUPPORT "SilenceSupp"               

/*legal names for H_263*/
#define    RV_SDPCODECS_FMTP_H263_SQCIF_V        "SQCIF"/*sqcifMPI	INTEGER (1..32) OPTIONAL,	-- units 1/29.97 Hz*/
#define    RV_SDPCODECS_FMTP_H263_QCIF_V         "QCIF"	/*qcifMPI	INTEGER (1..32) OPTIONAL,	-- units 1/29.97 Hz*/
#define    RV_SDPCODECS_FMTP_H263_CIF_V          "CIF"	/*cifMPI		INTEGER (1..32) OPTIONAL,	-- units 1/29.97 Hz*/
#define    RV_SDPCODECS_FMTP_H263_CIF4_V         "CIF4"	/*cif4MPI	INTEGER (1..32) OPTIONAL,	-- units 1/29.97 Hz*/
#define    RV_SDPCODECS_FMTP_H263_CIF16_V        "CIF16"/*cif16MPI	INTEGER (1..32) OPTIONAL,	-- units 1/29.97 Hz*/
#define    RV_SDPCODECS_FMTP_H263_XMAX_V         "XMAX"
#define    RV_SDPCODECS_FMTP_H263_YMAX_V         "YMAX"
#define    RV_SDPCODECS_FMTP_H263_MPI_V          "MPI"
#define    RV_SDPCODECS_FMTP_H263_D_V            "D"
#define    RV_SDPCODECS_FMTP_H263_E_V            "E"
#define    RV_SDPCODECS_FMTP_H263_F_V            "F"
#define    RV_SDPCODECS_FMTP_H263_G_V            "G"
#define    RV_SDPCODECS_FMTP_H263_I_V            "I"
#define    RV_SDPCODECS_FMTP_H263_J_V            "J"
#define    RV_SDPCODECS_FMTP_H263_K_V            "K"
#define    RV_SDPCODECS_FMTP_H263_L_V            "L"
#define    RV_SDPCODECS_FMTP_H263_M_V            "M"
#define    RV_SDPCODECS_FMTP_H263_N_V            "N"
#define    RV_SDPCODECS_FMTP_H263_O_V            "O"
#define    RV_SDPCODECS_FMTP_H263_P_V            "P"
#define    RV_SDPCODECS_FMTP_H263_Q_V            "Q"
#define    RV_SDPCODECS_FMTP_H263_R_V            "R"
#define    RV_SDPCODECS_FMTP_H263_S_V            "S"
#define    RV_SDPCODECS_FMTP_H263_T_V            "T"
#define    RV_SDPCODECS_FMTP_H263_PAR_V         "PAR"
#define    RV_SDPCODECS_FMTP_H263_CPCF_V         "CPCF" 
#define    RV_SDPCODECS_FMTP_H263_MAXBR_V        "MAXBR"/*maxBitRate	INTEGER (1..192400),	-- units 100 bit/s*/
#define    RV_SDPCODECS_FMTP_H263_BPP_V          "BPP"	/*bppMaxKb	INTEGER (0..65535) OPTIONAL,	-- units 1024 bits*/
#define    RV_SDPCODECS_FMTP_H263_HRD_V          "HRD"	/*hrd-B		INTEGER (0..524287) OPTIONAL,	-- units 128 bits*/
#define    RV_SDPCODECS_FMTP_H263_INTERLACED_V   "INTERLACED"

/*legal names for MP4V_ES*/
#define    RV_SDPCODECS_FMTP_MP4V_ES_PROFILE_LEVEL_ID   "profile-level-id"
#define    RV_SDPCODECS_FMTP_MP4V_ES_CONFIG             "config"

/*legal names for MP4A_LATM*/
#define    RV_SDPCODECS_FMTP_MP4A_LATM_PROFILE_LEVEL_ID   "profile-level-id"
#define    RV_SDPCODECS_FMTP_MP4A_LATM_OBJECT             "object"
#define    RV_SDPCODECS_FMTP_MP4A_LATM_BITRATE            "bitrate"
#define    RV_SDPCODECS_FMTP_MP4A_LATM_CPRESENT           "cpresent"
#define    RV_SDPCODECS_FMTP_MP4A_LATM_CONFIG             "config"      

/*legal names for AMR*/
#define    RV_SDPCODECS_FMTP_AMR_OCTET_ALIGN         "octet-align" 
#define    RV_SDPCODECS_FMTP_AMR_MODE_SET            "mode-set"
#define    RV_SDPCODECS_FMTP_AMR_MODE_CHANGE_PERIOD  "mode-change-period"
#define    RV_SDPCODECS_FMTP_AMR_MODE_CHANGE_NEGHBOR "mode-change-neighbor"
#define    RV_SDPCODECS_FMTP_AMR_CRC                 "crc"
#define    RV_SDPCODECS_FMTP_AMR_ROBUST_SORTING      "robust-sorting"
#define    RV_SDPCODECS_FMTP_AMR_INTERLEAVING        "interleaving"

/*legal names for H_261*/
#define    RV_SDPCODECS_FMTP_H261_QCIF_V             "QCIF"
#define    RV_SDPCODECS_FMTP_H261_CIF_V              "CIF"
#define    RV_SDPCODECS_FMTP_H261_D_V                "D"

/* XXX - Add New Codec parameters name */


#endif /* RV_SDP_CODECS_SUPPORTED */

#endif /*_file_rvsdpcodecsenums_h_*/


