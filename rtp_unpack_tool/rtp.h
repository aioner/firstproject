#ifndef _RTP_H_INCLUDED
#define _RTP_H_INCLUDED

#include "cli.h"
#include<stdint.h>


//数据块类型
typedef enum _ov_frame_type
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
    OV_AAC = 0x00120000,
} ov_frame_type;

#define RTP_FIXED_HEADER_LEN            12

struct rtp_fixed_header_t
{
    uint8_t version : 2;
    uint8_t padding : 1;
    uint8_t extension : 1;
    uint8_t CC : 4;
    uint8_t marker : 1;
    uint8_t payload : 7;
    uint16_t seq;
    uint32_t timestamp;
    uint32_t ssrc;
    uint32_t csrcs[15];

    friend std::ostream& operator<< (std::ostream& os, const rtp_fixed_header_t& h)
    {
        return os << "version:" << (uint32_t)h.version << std::endl
           << "padding:" << (uint32_t)h.padding << std::endl
           << "extension:" << (uint32_t)h.extension << std::endl
           << "CC:" << (uint32_t)h.CC << std::endl
           << "marker:" << (uint32_t)h.marker << std::endl
           << "payload:" << (uint32_t)h.payload << std::endl
           << "seq:" << h.seq << std::endl
           << "timestamp:" << h.timestamp << std::endl
            << "ssrc:" << h.ssrc << std::endl;
    }
};

struct rtp_ext_header_t
{
    uint16_t profile;
    uint16_t length;
    uint32_t data[1];
};


class rtp_unpack_parser_impl_t : public rtp_unpack_parser_t
{
public:
    bool parse(std::istream& input, std::ostream& output, const std::string &format, std::ostream& log);
};


#endif //_RTP_H_INCLUDED
