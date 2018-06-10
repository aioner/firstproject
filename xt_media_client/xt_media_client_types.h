#ifndef _XT_MEDIA_CLIENT_TYPES_H_INCLUDED
#define _XT_MEDIA_CLIENT_TYPES_H_INCLUDED

#ifdef _WIN32
#ifdef XT_MEDIA_CLIENT_EXPORTS
#define XT_MEDIA_CLIENT_API __declspec(dllexport)
#else
#define XT_MEDIA_CLIENT_API __declspec(dllimport)
#endif

#define XT_MEDIA_CLIENT_STDCALL  __stdcall
#else  //else linux
#define XT_MEDIA_CLIENT_API __attribute__((visibility("default")))
#define XT_MEDIA_CLIENT_STDCALL 
#endif
#include<stdint.h>

#define MKBETAG(a,b,c,d)                        ((d) | ((c) << 8) | ((b) << 16) | ((unsigned)(a) << 24))
#define MEDIA_CLIENT_URI_LEN                    128
#define MEDIA_CLIENT_IP_LEN                     32
#define MEDIA_CLIENT_STREAM_NUM                 4

#pragma pack(push, 8)

#ifdef __cplusplus
extern "C"
{
#endif
    //数据块类型
    typedef enum _ov_frame_type
    {
        OV_FRAME_TYPE_UNKNOWN  = 0xffffffff,
        OV_VIDEO_I            = 0x00000000,
        OV_AUDIO              = 0x00000001,
        HC_HEADE              = 0x68,
        HC_AUDIO              = 0x69,
        OV_HEADE              = 0x80,
        OV_VIDEO_P            = 0x00010000,
        OV_VIDEO_B            = 0x00020000,
        OV_VIDEO_SEI          = OV_VIDEO_P,
        OV_VIDEO_SPS          = OV_VIDEO_I,
        OV_VIDEO_PPS          = OV_VIDEO_I,
        OV_AAC                = 0x00120000,
        OV_H264              = 0x00100000,
        OV_H264_I          = OV_H264+1,
        OV_H264_P          = OV_H264+2,
        OV_H264_B         = OV_H264+3,
        OV_H264_SEI        = OV_H264+4,
        OV_H264_SPS            = OV_H264+5,
        OV_H264_PPS        = OV_H264+6,
        OV_G711           = 0x00110000,
        OV_H265           = 0x00130000,
    } ov_frame_type;

    typedef void *xt_media_link_handle_t;

    typedef enum _xt_media_client_status_t
    {
        MEDIA_CLIENT_STATUS_OK = 0,
        MEDIA_CLIENT_STATUS_UNKNOWN,
        MEDIA_CLIENT_STATUS_NULLPTR,
        MEDIA_CLIENT_STATUS_BADPARAM,
        MEDIA_CLIENT_STATUS_ENV_INIT_FAIL,
        MEDIA_CLIENT_STATUS_TIMEOUT,
        MEDIA_CLIENT_STATUS_LINK_NOT_EXISTS,
        MEDIA_CLIENT_STATUS_NOT_SUPPORTED,
        MEDIA_CLIENT_STATUS_BUF_NOT_ENOUGH,
        MEDIA_CLIENT_STATUS_CONNECT_FAIL,
        MEDIA_CLIENT_STATUS_DESCRIBE_FAIL,
        MEDIA_CLIENT_STATUS_SETUP_FAIL,
        MEDIA_CLIENT_STATUS_PLAY_FAIL,
        MEDIA_CLIENT_STATUS_PAUSE_FAIL,
        MEDIA_CLIENT_STATUS_TEARDOWN_FAIL,
        MEDIA_CLIENT_STATUS_BAD_SDP_CTX,
        MEDIA_CLIENT_STATUS_BAD_ADDR,
        MEDIA_CLIENT_STATUS_OPEN_RTP_FAIL,
        MEDIA_CLIENT_STATUS_BAD_LINK_MODE,
        MEDIA_CLIENT_STATUS_QUERY_RTCP_FAIL,
        MEDIA_CLIENT_STATUS_LOGIN_FAIL,
        MEDIA_CLIENT_RTCP_SEND_FIR_FAIL,
        MEDIA_CLIENT_STATUS_SDP_EXIST
    } xt_media_client_status_t;

    typedef enum _xt_media_client_link_mode
    {
        MEDIA_CLIENT_LKMODE_STD,		
        MEDIA_CLIENT_LKMODE_STD_MULTI,
        MEDIA_CLIENT_LKMODE_PRIV,
        MEDIA_CLIENT_LKMODE_PRIV_FAST,
        MEDIA_CLIENT_LKMODE_PUSH			//push stdard stream
    } xt_media_client_link_mode;

    //目前支持的连接类型
    typedef enum _xt_media_client_link_t
    {
        LT_TCP_TCP_PRI = 0,          //私有tcp会话以及私有tcp流
        LT_TCP_UDP_PRI = 1,          //私有tcp会话以及私有udp流
        LT_TCP_UDP_MULTI = 2,        //私有tcp会话以及私有udp流多播
        LT_TCP_RTP_PRI = 3,          //私有tcp会话以及私有rtp混合流
        LT_TCP_RTP_MULTI = 4,        //私有tcp会话以及私有rtp混合流多播
        LT_TCP_RTP_DEMUX_PRI = 5,    //私有tcp会话以及私有rtp混合端口复用流

        LT_RTSP_RTP_STD = 6,         //标准rtsp会话以及标准rtp流
        LT_RTSP_RTP_DEMUX = 7,       //标准rtsp会话以及复用rtp流
        LT_RTSP_RTP_PRI = 8,         //标准rtsp会话以及私有rtp流

        LT_TCP_RTP_STD = 9,          //私有tcp会话以及标准rtp流

        LT_XMPP_RTP_STD =10,         //私有XMPP会话以及标准rtp流
        LT_XMPP_RTP_PRI = 11,        //私有XMPP会话以及私有rtp流
        LT_XMPP_RTP_DEMUX_PRI =12,   //私有XMPP会话以及私有rtp混合端口复用流

        LT_UDP_RTP_PRI  = 13,        //私有udp会话以及私有rtp混合流
        LT_UDP_RTP_DEMUX_PRI = 14,    //私有UDP会话 私有RTP流
        LT_UDP_RTP_MULTIPRI  = 15,    //私有有UDP协商 RTP多播
        LT_UDP_RTP_STD = 16,          //私有有UDP协商 标准RTP流
        LT_UDP_RTP_DEMUX_STD = 17,     //私有有UDP协商 标准RTP 复用流
        LT_UDP_RTP_MULTI_STD = 18,     //私有有UDP协商 标准RTP流 多播
    } xt_media_client_link_t;

    typedef enum _xt_av_media_t
    {
        XT_AVMEDIA_TYPE_UNKNOWN = -1,
        XT_AVMEDIA_TYPE_DEMUX = 0,
        XT_AVMEDIA_TYPE_VIDEO = 0x1,
        XT_AVMEDIA_TYPE_AUDIO = 0x2,
        XT_AVMEDIA_TYPE_SUBTITLE = 0x4,
        XT_AVMEDIA_TYPE_ATTACHMENT = 0x8,
        XT_AVMEDIA_TYPE_NB = 0x10
    } xt_av_media_t;

    typedef enum _xt_av_codec_id_t
    {
        XT_AV_CODEC_ID_NONE,

        /* video codecs */
        XT_AV_CODEC_ID_MPEG1VIDEO,
        XT_AV_CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
        XT_AV_CODEC_ID_MPEG2VIDEO_XVMC,
        XT_AV_CODEC_ID_H261,
        XT_AV_CODEC_ID_H263,
        XT_AV_CODEC_ID_RV10,
        XT_AV_CODEC_ID_RV20,
        XT_AV_CODEC_ID_MJPEG,
        XT_AV_CODEC_ID_MJPEGB,
        XT_AV_CODEC_ID_LJPEG,
        XT_AV_CODEC_ID_SP5X,
        XT_AV_CODEC_ID_JPEGLS,
        XT_AV_CODEC_ID_MPEG4,
        XT_AV_CODEC_ID_RAWVIDEO,
        XT_AV_CODEC_ID_MSMPEG4V1,
        XT_AV_CODEC_ID_MSMPEG4V2,
        XT_AV_CODEC_ID_MSMPEG4V3,
        XT_AV_CODEC_ID_WMV1,
        XT_AV_CODEC_ID_WMV2,
        XT_AV_CODEC_ID_H263P,
        XT_AV_CODEC_ID_H263I,
        XT_AV_CODEC_ID_FLV1,
        XT_AV_CODEC_ID_SVQ1,
        XT_AV_CODEC_ID_SVQ3,
        XT_AV_CODEC_ID_DVVIDEO,
        XT_AV_CODEC_ID_HUFFYUV,
        XT_AV_CODEC_ID_CYUV,
        XT_AV_CODEC_ID_H264,
        XT_AV_CODEC_ID_INDEO3,
        XT_AV_CODEC_ID_VP3,
        XT_AV_CODEC_ID_THEORA,
        XT_AV_CODEC_ID_ASV1,
        XT_AV_CODEC_ID_ASV2,
        XT_AV_CODEC_ID_FFV1,
        XT_AV_CODEC_ID_4XM,
        XT_AV_CODEC_ID_VCR1,
        XT_AV_CODEC_ID_CLJR,
        XT_AV_CODEC_ID_MDEC,
        XT_AV_CODEC_ID_ROQ,
        XT_AV_CODEC_ID_INTERPLAY_VIDEO,
        XT_AV_CODEC_ID_XAN_WC3,
        XT_AV_CODEC_ID_XAN_WC4,
        XT_AV_CODEC_ID_RPZA,
        XT_AV_CODEC_ID_CINEPAK,
        XT_AV_CODEC_ID_WS_VQA,
        XT_AV_CODEC_ID_MSRLE,
        XT_AV_CODEC_ID_MSVIDEO1,
        XT_AV_CODEC_ID_IDCIN,
        XT_AV_CODEC_ID_8BPS,
        XT_AV_CODEC_ID_SMC,
        XT_AV_CODEC_ID_FLIC,
        XT_AV_CODEC_ID_TRUEMOTION1,
        XT_AV_CODEC_ID_VMDVIDEO,
        XT_AV_CODEC_ID_MSZH,
        XT_AV_CODEC_ID_ZLIB,
        XT_AV_CODEC_ID_QTRLE,
        XT_AV_CODEC_ID_TSCC,
        XT_AV_CODEC_ID_ULTI,
        XT_AV_CODEC_ID_QDRAW,
        XT_AV_CODEC_ID_VIXL,
        XT_AV_CODEC_ID_QPEG,
        XT_AV_CODEC_ID_PNG,
        XT_AV_CODEC_ID_PPM,
        XT_AV_CODEC_ID_PBM,
        XT_AV_CODEC_ID_PGM,
        XT_AV_CODEC_ID_PGMYUV,
        XT_AV_CODEC_ID_PAM,
        XT_AV_CODEC_ID_FFVHUFF,
        XT_AV_CODEC_ID_RV30,
        XT_AV_CODEC_ID_RV40,
        XT_AV_CODEC_ID_VC1,
        XT_AV_CODEC_ID_WMV3,
        XT_AV_CODEC_ID_LOCO,
        XT_AV_CODEC_ID_WNV1,
        XT_AV_CODEC_ID_AASC,
        XT_AV_CODEC_ID_INDEO2,
        XT_AV_CODEC_ID_FRAPS,
        XT_AV_CODEC_ID_TRUEMOTION2,
        XT_AV_CODEC_ID_BMP,
        XT_AV_CODEC_ID_CSCD,
        XT_AV_CODEC_ID_MMVIDEO,
        XT_AV_CODEC_ID_ZMBV,
        XT_AV_CODEC_ID_AVS,
        XT_AV_CODEC_ID_SMACKVIDEO,
        XT_AV_CODEC_ID_NUV,
        XT_AV_CODEC_ID_KMVC,
        XT_AV_CODEC_ID_FLASHSV,
        XT_AV_CODEC_ID_CAVS,
        XT_AV_CODEC_ID_JPEG2000,
        XT_AV_CODEC_ID_VMNC,
        XT_AV_CODEC_ID_VP5,
        XT_AV_CODEC_ID_VP6,
        XT_AV_CODEC_ID_VP6F,
        XT_AV_CODEC_ID_TARGA,
        XT_AV_CODEC_ID_DSICINVIDEO,
        XT_AV_CODEC_ID_TIERTEXSEQVIDEO,
        XT_AV_CODEC_ID_TIFF,
        XT_AV_CODEC_ID_GIF,
        XT_AV_CODEC_ID_DXA,
        XT_AV_CODEC_ID_DNXHD,
        XT_AV_CODEC_ID_THP,
        XT_AV_CODEC_ID_SGI,
        XT_AV_CODEC_ID_C93,
        XT_AV_CODEC_ID_BETHSOFTVID,
        XT_AV_CODEC_ID_PTX,
        XT_AV_CODEC_ID_TXD,
        XT_AV_CODEC_ID_VP6A,
        XT_AV_CODEC_ID_AMV,
        XT_AV_CODEC_ID_VB,
        XT_AV_CODEC_ID_PCX,
        XT_AV_CODEC_ID_SUNRAST,
        XT_AV_CODEC_ID_INDEO4,
        XT_AV_CODEC_ID_INDEO5,
        XT_AV_CODEC_ID_MIMIC,
        XT_AV_CODEC_ID_RL2,
        XT_AV_CODEC_ID_ESCAPE124,
        XT_AV_CODEC_ID_DIRAC,
        XT_AV_CODEC_ID_BFI,
        XT_AV_CODEC_ID_CMV,
        XT_AV_CODEC_ID_MOTIONPIXELS,
        XT_AV_CODEC_ID_TGV,
        XT_AV_CODEC_ID_TGQ,
        XT_AV_CODEC_ID_TQI,
        XT_AV_CODEC_ID_AURA,
        XT_AV_CODEC_ID_AURA2,
        XT_AV_CODEC_ID_V210X,
        XT_AV_CODEC_ID_TMV,
        XT_AV_CODEC_ID_V210,
        XT_AV_CODEC_ID_DPX,
        XT_AV_CODEC_ID_MAD,
        XT_AV_CODEC_ID_FRWU,
        XT_AV_CODEC_ID_FLASHSV2,
        XT_AV_CODEC_ID_CDGRAPHICS,
        XT_AV_CODEC_ID_R210,
        XT_AV_CODEC_ID_ANM,
        XT_AV_CODEC_ID_BINKVIDEO,
        XT_AV_CODEC_ID_IFF_ILBM,
        XT_AV_CODEC_ID_IFF_BYTERUN1,
        XT_AV_CODEC_ID_KGV1,
        XT_AV_CODEC_ID_YOP,
        XT_AV_CODEC_ID_VP8,
        XT_AV_CODEC_ID_PICTOR,
        XT_AV_CODEC_ID_ANSI,
        XT_AV_CODEC_ID_A64_MULTI,
        XT_AV_CODEC_ID_A64_MULTI5,
        XT_AV_CODEC_ID_R10K,
        XT_AV_CODEC_ID_MXPEG,
        XT_AV_CODEC_ID_LAGARITH,
        XT_AV_CODEC_ID_PRORES,
        XT_AV_CODEC_ID_JV,
        XT_AV_CODEC_ID_DFA,
        XT_AV_CODEC_ID_WMV3IMAGE,
        XT_AV_CODEC_ID_VC1IMAGE,
        XT_AV_CODEC_ID_UTVIDEO,
        XT_AV_CODEC_ID_BMV_VIDEO,
        XT_AV_CODEC_ID_VBLE,
        XT_AV_CODEC_ID_DXTORY,
        XT_AV_CODEC_ID_V410,
        XT_AV_CODEC_ID_XWD,
        XT_AV_CODEC_ID_CDXL,
        XT_AV_CODEC_ID_XBM,
        XT_AV_CODEC_ID_ZEROCODEC,
        XT_AV_CODEC_ID_MSS1,
        XT_AV_CODEC_ID_MSA1,
        XT_AV_CODEC_ID_TSCC2,
        XT_AV_CODEC_ID_MTS2,
        XT_AV_CODEC_ID_CLLC,
        XT_AV_CODEC_ID_MSS2,
        XT_AV_CODEC_ID_VP9,
        XT_AV_CODEC_ID_AIC,
        XT_AV_CODEC_ID_ESCAPE130_DEPRECATED,
        XT_AV_CODEC_ID_G2M_DEPRECATED,
        XT_AV_CODEC_ID_WEBP_DEPRECATED,

        XT_AV_CODEC_ID_BRENDER_PIX= MKBETAG('B','P','I','X'),
        XT_AV_CODEC_ID_Y41P       = MKBETAG('Y','4','1','P'),
        XT_AV_CODEC_ID_ESCAPE130  = MKBETAG('E','1','3','0'),
        XT_AV_CODEC_ID_EXR        = MKBETAG('0','E','X','R'),
        XT_AV_CODEC_ID_AVRP       = MKBETAG('A','V','R','P'),

        XT_AV_CODEC_ID_012V       = MKBETAG('0','1','2','V'),
        XT_AV_CODEC_ID_G2M        = MKBETAG( 0 ,'G','2','M'),
        XT_AV_CODEC_ID_AVUI       = MKBETAG('A','V','U','I'),
        XT_AV_CODEC_ID_AYUV       = MKBETAG('A','Y','U','V'),
        XT_AV_CODEC_ID_TARGA_Y216 = MKBETAG('T','2','1','6'),
        XT_AV_CODEC_ID_V308       = MKBETAG('V','3','0','8'),
        XT_AV_CODEC_ID_V408       = MKBETAG('V','4','0','8'),
        XT_AV_CODEC_ID_YUV4       = MKBETAG('Y','U','V','4'),
        XT_AV_CODEC_ID_SANM       = MKBETAG('S','A','N','M'),
        XT_AV_CODEC_ID_PAF_VIDEO  = MKBETAG('P','A','F','V'),
        XT_AV_CODEC_ID_AVRN       = MKBETAG('A','V','R','n'),
        XT_AV_CODEC_ID_CPIA       = MKBETAG('C','P','I','A'),
        XT_AV_CODEC_ID_XFACE      = MKBETAG('X','F','A','C'),
        XT_AV_CODEC_ID_SGIRLE     = MKBETAG('S','G','I','R'),
        XT_AV_CODEC_ID_MVC1       = MKBETAG('M','V','C','1'),
        XT_AV_CODEC_ID_MVC2       = MKBETAG('M','V','C','2'),
        XT_AV_CODEC_ID_SNOW       = MKBETAG('S','N','O','W'),
        XT_AV_CODEC_ID_WEBP       = MKBETAG('W','E','B','P'),
        XT_AV_CODEC_ID_SMVJPEG    = MKBETAG('S','M','V','J'),
        XT_AV_CODEC_ID_HEVC       = MKBETAG('H','2','6','5'),
#define XT_AV_CODEC_ID_H265 XT_AV_CODEC_ID_HEVC

        /* various PCM "codecs" */
        XT_AV_CODEC_ID_FIRST_AUDIO = 0x10000,     ///< A dummy id pointing at the start of audio codecs
        XT_AV_CODEC_ID_PCM_S16LE = 0x10000,
        XT_AV_CODEC_ID_PCM_S16BE,
        XT_AV_CODEC_ID_PCM_U16LE,
        XT_AV_CODEC_ID_PCM_U16BE,
        XT_AV_CODEC_ID_PCM_S8,
        XT_AV_CODEC_ID_PCM_U8,
        XT_AV_CODEC_ID_PCM_MULAW,
        XT_AV_CODEC_ID_PCM_ALAW,
        XT_AV_CODEC_ID_PCM_S32LE,
        XT_AV_CODEC_ID_PCM_S32BE,
        XT_AV_CODEC_ID_PCM_U32LE,
        XT_AV_CODEC_ID_PCM_U32BE,
        XT_AV_CODEC_ID_PCM_S24LE,
        XT_AV_CODEC_ID_PCM_S24BE,
        XT_AV_CODEC_ID_PCM_U24LE,
        XT_AV_CODEC_ID_PCM_U24BE,
        XT_AV_CODEC_ID_PCM_S24DAUD,
        XT_AV_CODEC_ID_PCM_ZORK,
        XT_AV_CODEC_ID_PCM_S16LE_PLANAR,
        XT_AV_CODEC_ID_PCM_DVD,
        XT_AV_CODEC_ID_PCM_F32BE,
        XT_AV_CODEC_ID_PCM_F32LE,
        XT_AV_CODEC_ID_PCM_F64BE,
        XT_AV_CODEC_ID_PCM_F64LE,
        XT_AV_CODEC_ID_PCM_BLURAY,
        XT_AV_CODEC_ID_PCM_LXF,
        XT_AV_CODEC_ID_S302M,
        XT_AV_CODEC_ID_PCM_S8_PLANAR,
        XT_AV_CODEC_ID_PCM_S24LE_PLANAR_DEPRECATED,
        XT_AV_CODEC_ID_PCM_S32LE_PLANAR_DEPRECATED,
        XT_AV_CODEC_ID_PCM_S24LE_PLANAR = MKBETAG(24,'P','S','P'),
        XT_AV_CODEC_ID_PCM_S32LE_PLANAR = MKBETAG(32,'P','S','P'),
        XT_AV_CODEC_ID_PCM_S16BE_PLANAR = MKBETAG('P','S','P',16),

        /* various ADPCM codecs */
        XT_AV_CODEC_ID_ADPCM_IMA_QT = 0x11000,
        XT_AV_CODEC_ID_ADPCM_IMA_WAV,
        XT_AV_CODEC_ID_ADPCM_IMA_DK3,
        XT_AV_CODEC_ID_ADPCM_IMA_DK4,
        XT_AV_CODEC_ID_ADPCM_IMA_WS,
        XT_AV_CODEC_ID_ADPCM_IMA_SMJPEG,
        XT_AV_CODEC_ID_ADPCM_MS,
        XT_AV_CODEC_ID_ADPCM_4XM,
        XT_AV_CODEC_ID_ADPCM_XA,
        XT_AV_CODEC_ID_ADPCM_ADX,
        XT_AV_CODEC_ID_ADPCM_EA,
        XT_AV_CODEC_ID_ADPCM_G726,
        XT_AV_CODEC_ID_ADPCM_CT,
        XT_AV_CODEC_ID_ADPCM_SWF,
        XT_AV_CODEC_ID_ADPCM_YAMAHA,
        XT_AV_CODEC_ID_ADPCM_SBPRO_4,
        XT_AV_CODEC_ID_ADPCM_SBPRO_3,
        XT_AV_CODEC_ID_ADPCM_SBPRO_2,
        XT_AV_CODEC_ID_ADPCM_THP,
        XT_AV_CODEC_ID_ADPCM_IMA_AMV,
        XT_AV_CODEC_ID_ADPCM_EA_R1,
        XT_AV_CODEC_ID_ADPCM_EA_R3,
        XT_AV_CODEC_ID_ADPCM_EA_R2,
        XT_AV_CODEC_ID_ADPCM_IMA_EA_SEAD,
        XT_AV_CODEC_ID_ADPCM_IMA_EA_EACS,
        XT_AV_CODEC_ID_ADPCM_EA_XAS,
        XT_AV_CODEC_ID_ADPCM_EA_MAXIS_XA,
        XT_AV_CODEC_ID_ADPCM_IMA_ISS,
        XT_AV_CODEC_ID_ADPCM_G722,
        XT_AV_CODEC_ID_ADPCM_IMA_APC,
        XT_AV_CODEC_ID_VIMA       = MKBETAG('V','I','M','A'),
        XT_AV_CODEC_ID_ADPCM_AFC  = MKBETAG('A','F','C',' '),
        XT_AV_CODEC_ID_ADPCM_IMA_OKI = MKBETAG('O','K','I',' '),
        XT_AV_CODEC_ID_ADPCM_DTK  = MKBETAG('D','T','K',' '),
        XT_AV_CODEC_ID_ADPCM_IMA_RAD = MKBETAG('R','A','D',' '),
        XT_AV_CODEC_ID_ADPCM_G726LE = MKBETAG('6','2','7','G'),

        /* AMR */
        XT_AV_CODEC_ID_AMR_NB = 0x12000,
        XT_AV_CODEC_ID_AMR_WB,

        /* RealAudio codecs*/
        XT_AV_CODEC_ID_RA_144 = 0x13000,
        XT_AV_CODEC_ID_RA_288,

        /* various DPCM codecs */
        XT_AV_CODEC_ID_ROQ_DPCM = 0x14000,
        XT_AV_CODEC_ID_INTERPLAY_DPCM,
        XT_AV_CODEC_ID_XAN_DPCM,
        XT_AV_CODEC_ID_SOL_DPCM,

        /* audio codecs */
        XT_AV_CODEC_ID_MP2 = 0x15000,
        XT_AV_CODEC_ID_MP3, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
        XT_AV_CODEC_ID_AAC,
        XT_AV_CODEC_ID_AC3,
        XT_AV_CODEC_ID_DTS,
        XT_AV_CODEC_ID_VORBIS,
        XT_AV_CODEC_ID_DVAUDIO,
        XT_AV_CODEC_ID_WMAV1,
        XT_AV_CODEC_ID_WMAV2,
        XT_AV_CODEC_ID_MACE3,
        XT_AV_CODEC_ID_MACE6,
        XT_AV_CODEC_ID_VMDAUDIO,
        XT_AV_CODEC_ID_FLAC,
        XT_AV_CODEC_ID_MP3ADU,
        XT_AV_CODEC_ID_MP3ON4,
        XT_AV_CODEC_ID_SHORTEN,
        XT_AV_CODEC_ID_ALAC,
        XT_AV_CODEC_ID_WESTWOOD_SND1,
        XT_AV_CODEC_ID_GSM, ///< as in Berlin toast format
        XT_AV_CODEC_ID_QDM2,
        XT_AV_CODEC_ID_COOK,
        XT_AV_CODEC_ID_TRUESPEECH,
        XT_AV_CODEC_ID_TTA,
        XT_AV_CODEC_ID_SMACKAUDIO,
        XT_AV_CODEC_ID_QCELP,
        XT_AV_CODEC_ID_WAVPACK,
        XT_AV_CODEC_ID_DSICINAUDIO,
        XT_AV_CODEC_ID_IMC,
        XT_AV_CODEC_ID_MUSEPACK7,
        XT_AV_CODEC_ID_MLP,
        XT_AV_CODEC_ID_GSM_MS, /* as found in WAV */
        XT_AV_CODEC_ID_ATRAC3,
#if FF_API_VOXWARE
        XT_AV_CODEC_ID_VOXWARE,
#endif
        XT_AV_CODEC_ID_APE,
        XT_AV_CODEC_ID_NELLYMOSER,
        XT_AV_CODEC_ID_MUSEPACK8,
        XT_AV_CODEC_ID_SPEEX,
        XT_AV_CODEC_ID_WMAVOICE,
        XT_AV_CODEC_ID_WMAPRO,
        XT_AV_CODEC_ID_WMALOSSLESS,
        XT_AV_CODEC_ID_ATRAC3P,
        XT_AV_CODEC_ID_EAC3,
        XT_AV_CODEC_ID_SIPR,
        XT_AV_CODEC_ID_MP1,
        XT_AV_CODEC_ID_TWINVQ,
        XT_AV_CODEC_ID_TRUEHD,
        XT_AV_CODEC_ID_MP4ALS,
        XT_AV_CODEC_ID_ATRAC1,
        XT_AV_CODEC_ID_BINKAUDIO_RDFT,
        XT_AV_CODEC_ID_BINKAUDIO_DCT,
        XT_AV_CODEC_ID_AAC_LATM,
        XT_AV_CODEC_ID_QDMC,
        XT_AV_CODEC_ID_CELT,
        XT_AV_CODEC_ID_G723_1,
        XT_AV_CODEC_ID_G729,
        XT_AV_CODEC_ID_8SVX_EXP,
        XT_AV_CODEC_ID_8SVX_FIB,
        XT_AV_CODEC_ID_BMV_AUDIO,
        XT_AV_CODEC_ID_RALF,
        XT_AV_CODEC_ID_IAC,
        XT_AV_CODEC_ID_ILBC,
        XT_AV_CODEC_ID_OPUS_DEPRECATED,
        XT_AV_CODEC_ID_COMFORT_NOISE,
        XT_AV_CODEC_ID_TAK_DEPRECATED,
        XT_AV_CODEC_ID_METASOUND,
        XT_AV_CODEC_ID_FFWAVESYNTH = MKBETAG('F','F','W','S'),
        XT_AV_CODEC_ID_SONIC       = MKBETAG('S','O','N','C'),
        XT_AV_CODEC_ID_SONIC_LS    = MKBETAG('S','O','N','L'),
        XT_AV_CODEC_ID_PAF_AUDIO   = MKBETAG('P','A','F','A'),
        XT_AV_CODEC_ID_OPUS        = MKBETAG('O','P','U','S'),
        XT_AV_CODEC_ID_TAK         = MKBETAG('t','B','a','K'),
        XT_AV_CODEC_ID_EVRC        = MKBETAG('s','e','v','c'),
        XT_AV_CODEC_ID_SMV         = MKBETAG('s','s','m','v'),

        /* subtitle codecs */
        XT_AV_CODEC_ID_FIRST_SUBTITLE = 0x17000,          ///< A dummy ID pointing at the start of subtitle codecs.
        XT_AV_CODEC_ID_DVD_SUBTITLE = 0x17000,
        XT_AV_CODEC_ID_DVB_SUBTITLE,
        XT_AV_CODEC_ID_TEXT,  ///< raw UTF-8 text
        XT_AV_CODEC_ID_XSUB,
        XT_AV_CODEC_ID_SSA,
        XT_AV_CODEC_ID_MOV_TEXT,
        XT_AV_CODEC_ID_HDMV_PGS_SUBTITLE,
        XT_AV_CODEC_ID_DVB_TELETEXT,
        XT_AV_CODEC_ID_SRT,
        XT_AV_CODEC_ID_MICRODVD   = MKBETAG('m','D','V','D'),
        XT_AV_CODEC_ID_EIA_608    = MKBETAG('c','6','0','8'),
        XT_AV_CODEC_ID_JACOSUB    = MKBETAG('J','S','U','B'),
        XT_AV_CODEC_ID_SAMI       = MKBETAG('S','A','M','I'),
        XT_AV_CODEC_ID_REALTEXT   = MKBETAG('R','T','X','T'),
        XT_AV_CODEC_ID_SUBVIEWER1 = MKBETAG('S','b','V','1'),
        XT_AV_CODEC_ID_SUBVIEWER  = MKBETAG('S','u','b','V'),
        XT_AV_CODEC_ID_SUBRIP     = MKBETAG('S','R','i','p'),
        XT_AV_CODEC_ID_WEBVTT     = MKBETAG('W','V','T','T'),
        XT_AV_CODEC_ID_MPL2       = MKBETAG('M','P','L','2'),
        XT_AV_CODEC_ID_VPLAYER    = MKBETAG('V','P','l','r'),
        XT_AV_CODEC_ID_PJS        = MKBETAG('P','h','J','S'),
        XT_AV_CODEC_ID_ASS        = MKBETAG('A','S','S',' '),  ///< ASS as defined in Matroska

        /* other specific kind of codecs (generally used for attachments) */
        XT_AV_CODEC_ID_FIRST_UNKNOWN = 0x18000,           ///< A dummy ID pointing at the start of various fake codecs.
        XT_AV_CODEC_ID_TTF = 0x18000,
        XT_AV_CODEC_ID_BINTEXT    = MKBETAG('B','T','X','T'),
        XT_AV_CODEC_ID_XBIN       = MKBETAG('X','B','I','N'),
        XT_AV_CODEC_ID_IDF        = MKBETAG( 0 ,'I','D','F'),
        XT_AV_CODEC_ID_OTF        = MKBETAG( 0 ,'O','T','F'),
        XT_AV_CODEC_ID_SMPTE_KLV  = MKBETAG('K','L','V','A'),
        XT_AV_CODEC_ID_DVD_NAV    = MKBETAG('D','N','A','V'),


        XT_AV_CODEC_ID_PROBE = 0x19000, ///< codec_id is not known (like XT_AV_CODEC_ID_NONE) but lavf should attempt to identify it

        XT_AV_CODEC_ID_MPEG2TS = 0x20000, /**< _FAKE_ codec to indicate a raw MPEG-2 TS
                                          * stream (only used by libavformat) */
                                          XT_AV_CODEC_ID_MPEG4SYSTEMS = 0x20001, /**< _FAKE_ codec to indicate a MPEG-4 Systems
                                                                                 * stream (only used by libavformat) */
                                                                                 XT_AV_CODEC_ID_FFMETADATA = 0x21000,   ///< Dummy codec for streams containing only metadata information.
    } xt_av_codec_id_t;

    typedef enum _xt_media_client_session_flag
    {
        MEDIA_CLIENT_USE_NONE_SESSION       = 0,
        MEDIA_CLIENT_USE_RTSP_SESSION       = 1<<0,
        MEDIA_CLIENT_USE_UDP_SESSION        = 1<<1,
        MEDIA_CLIENT_USE_TCP_SESSION        = 1<<2,
        MEDIA_CLIENT_USE_ALL_SESSION        = 0xfffffff
    } xt_media_client_session_flag;

    typedef struct _xt_sdp_media_info_t
    {
        char				uri[MEDIA_CLIENT_URI_LEN];
        xt_av_media_t		media_type;
        xt_av_codec_id_t	cid;
        uint16_t			port_rtp;
        uint16_t			port_rtcp;
        bool				demux;
        uint32_t			demux_id;
		int payload;
    } xt_sdp_media_info_t;

    typedef struct _xt_session_context_t
    {
        uint32_t    ssrc;
        uint32_t    demuxid;
        uint16_t    rtp_port;
        uint16_t    rtcp_port;
        uint8_t     mode;
        uint8_t     demux;
    } xt_session_context_t;

    typedef struct _xt_session_param_t
    {
        xt_av_media_t stream_type;
        xt_session_context_t client_ctx;
        xt_session_context_t server_ctx;
		char destination[MEDIA_CLIENT_IP_LEN];
		uint8_t is_unicast;
        uint32_t code;//码流类型
    } xt_session_param_t;

    typedef struct _xt_session_server_info_t
    {
        char ip[MEDIA_CLIENT_IP_LEN];
        uint32_t channel;
        uint16_t port;
    } xt_session_server_info_t;

    typedef void (XT_MEDIA_CLIENT_STDCALL *xt_media_client_done_callback_t)(void *ctx, xt_media_client_status_t stat);

    typedef struct _xt_custom_session_callback_t
    {
        xt_media_client_status_t (XT_MEDIA_CLIENT_STDCALL *get_server_info_callback)(void *ctx, xt_session_server_info_t* server_info);
        xt_media_client_status_t (XT_MEDIA_CLIENT_STDCALL *parse_sdp_callback)(void *ctx, const char *sdp, uint32_t length, xt_sdp_media_info_t sdp_media_infos[MEDIA_CLIENT_STREAM_NUM], uint16_t *num);
        xt_media_client_status_t (XT_MEDIA_CLIENT_STDCALL *describe_callback)(void *ctx, char *sdp, uint32_t *length);
        xt_media_client_status_t (XT_MEDIA_CLIENT_STDCALL *setup_callback)(void *ctx, xt_session_param_t params[MEDIA_CLIENT_STREAM_NUM], uint16_t num);
        xt_media_client_status_t (XT_MEDIA_CLIENT_STDCALL *play_callback)(void *ctx, double npt, float scale, uint32_t *seq, uint32_t *timestamp);
        xt_media_client_status_t (XT_MEDIA_CLIENT_STDCALL *pause_callback)(void *ctx);
        xt_media_client_status_t (XT_MEDIA_CLIENT_STDCALL *describe_and_setup_callback)(void *ctx, xt_session_param_t params[MEDIA_CLIENT_STREAM_NUM], uint16_t num, char *sdp, uint32_t *length);
        xt_media_client_status_t (XT_MEDIA_CLIENT_STDCALL *teardown_callback)(void *ctx);
    } xt_custom_session_callback_t;

    typedef struct _xt_rtp_prof_info_t
    {
        uint32_t recv_Bps;
        uint32_t recv_packet_fraction_lost;
        uint32_t recv_packet_cumulative_lost;
        uint32_t recv_jitter;
    } xt_rtp_prof_info_t;

    typedef int32_t (XT_MEDIA_CLIENT_STDCALL *xt_ports_mgr_get_callback_t)(void *ctx, uint32_t num, uint16_t *ports, int32_t opt);
    typedef void (XT_MEDIA_CLIENT_STDCALL *xt_ports_mgr_free_callback_t)(void *ctx, uint32_t num, uint16_t *ports);

    typedef enum 
    {
        md_log_info=0,
        md_log_warn,
        md_log_error,
        md_log_debug
    }media_client_log_level_t;

    typedef void (XT_MEDIA_CLIENT_STDCALL *xt_media_client_log_cb_t)(const char* log_name,const media_client_log_level_t log_level,const char* log_ctx);
    typedef struct _xt_media_client_cfg_t
    {
        char udp_session_bind_ip[MEDIA_CLIENT_IP_LEN];
        uint16_t udp_session_bind_port;
        uint16_t udp_session_heartbit_proid;
        uint16_t udp_session_request_try_count;
        uint16_t udp_session_request_one_timeout;

        char tcp_session_bind_ip[MEDIA_CLIENT_IP_LEN];
        uint16_t tcp_session_bind_port;
        uint16_t tcp_session_connect_timeout;
        uint16_t tcp_session_login_timeout;
        uint16_t tcp_session_play_timeout;
        uint16_t tcp_session_stop_timeout;

        uint16_t rtsp_session_connect_timeout;
        uint16_t rtsp_session_describe_timeout;
        uint16_t rtsp_session_setup_timeout;
        uint16_t rtsp_session_play_timeout;
        uint16_t rtsp_session_pause_timeout;
        uint16_t rtsp_session_teardown_timeout;

        uint16_t rtp_rv_thread_num;
        uint16_t rtp_sink_thread_num;
        uint16_t rtp_post_thread_num;

        xt_ports_mgr_get_callback_t ports_mgr_get_callback;
        xt_ports_mgr_free_callback_t ports_mgr_free_callback;
        xt_media_client_log_cb_t media_log_callback;
        void *ports_mgr_ctx;
        uint32_t session_enable_flags;

    } xt_media_client_cfg_t;


    typedef void (XT_MEDIA_CLIENT_STDCALL *xt_media_client_frame_callback_t)(void *ctx, xt_media_link_handle_t link, void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc);

    typedef long (XT_MEDIA_CLIENT_STDCALL *regist_call_back_t)(const char *ip, uint16_t port, const uint8_t *data, uint32_t length);

    typedef struct _xt_sink_info_t
    {
        int index;
        uint16_t port_rtp;
        uint16_t port_rtcp;
        bool demux;
        uint32_t demuxid;
    }xt_sink_info_t;

	typedef struct _xt_media_client_rtcp_sr_t
    {
        uint32_t    mNTPtimestamp;  /* Most significant 32bit of NTP timestamp */
        uint32_t    lNTPtimestamp;  /* Least significant 32bit of NTP timestamp */
        uint32_t    timestamp;      /* RTP timestamp */
        uint32_t    packets;        /* Total number of RTP data packets transmitted by the sender
                                       since transmission started and up until the time this SR packet
                                     was generated. */
        uint32_t    octets;     /* The total number of payload octets (not including header or padding */
    } xt_media_client_rtcp_sr_t;

    typedef struct _xt_media_client_rtcp_rr_t
    {
        uint32_t    fractionLost;   /* The fraction of RTP data packets from source specified by
                                       SSRC that were lost since previous SR/RR packet was sent. */
        uint32_t    cumulativeLost; /* Total number of RTP data packets from source specified by
                                       SSRC that have been lost since the beginning of reception. */
        uint32_t    sequenceNumber; /* Sequence number that was received from the source specified
                                       by SSRC. */
        uint32_t    jitter;         /* Estimate of the statistical variance of the RTP data packet inter
                                       arrival time. */
        uint32_t    lSR;            /* The middle 32 bits of the NTP timestamp received. */
        uint32_t    dlSR;           /* Delay since the last SR. */
    } xt_media_client_rtcp_rr_t;

    typedef void (XT_MEDIA_CLIENT_STDCALL *xt_media_client_rtcp_report_callback_t)(void *ctx, uint32_t ssrc, const xt_media_client_rtcp_sr_t *sr, const xt_media_client_rtcp_rr_t *rr);
	typedef uint8_t (XT_MEDIA_CLIENT_STDCALL *xt_media_client_no_frame_arrived_callback_t)(void *ctx);

#ifdef __cplusplus
}
#endif

#pragma pack(pop)


#endif //_XT_MEDIA_CLIENT_TYPES_H_INCLUDED
