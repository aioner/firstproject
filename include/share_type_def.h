#ifndef SHARE_TYPE_DEF_H__
#define  SHARE_TYPE_DEF_H__
// 帧数据类型
enum ov_frame_type
{
    OV_FRAME_TYPE_UNKNOWN = 0xffffffff,
    OV_VIDEO_I  = 0x00000000,
    OV_AUDIO    = 0x00000001,
    HC_HEADE    = 0x68,
    HC_AUDIO    = 0x69,
    OV_HEADE    = 0x80,
    OV_VIDEO_P  = 0x00010000,
    OV_VIDEO_B  = 0x00020000,
    OV_VIDEO_SEI= OV_VIDEO_P,
    OV_VIDEO_SPS= OV_VIDEO_I,
    OV_VIDEO_PPS= OV_VIDEO_I,

    OV_H264			= 0x00100000,
    OV_H264_I		= OV_H264+1,
    OV_H264_P		= OV_H264+2,
    OV_H264_B		= OV_H264+3,
    OV_H264_SEI		= OV_H264+4,
    OV_H264_SPS		= OV_H264+5,
    OV_H264_PPS		= OV_H264+6,

    OV_G711			= 0x00110000,

    OV_AAC			= 0x00120000,

    OV_H265			= 0x00130000,
};
#endif //#ifndef SHARE_TYPE_DEF_H__
