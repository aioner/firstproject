#include "sdp_parser.h"
#include "compat.h"
#include "rvsdp.h"
#include <string>
#include <boost/lexical_cast.hpp>

#define RTP_PT_PRIVATE          96
extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{
    bool sdp_parser_library::init()
    {
        return (RV_OK == ::RvSdpMgrConstruct());
    }

    void *sdp_parser_library::parse(const char *sdp, int len)
    {
        RvSdpParseStatus status = RV_SDPPARSER_STOP_ZERO;
        //modified by lichao, 20141117 防止sdp结尾没有'\0'结尾
        char txt[2048];
        if ((size_t)len >= sizeof(txt))
        {
            len = sizeof(txt) - 1;
        }
        (void)memcpy(txt, sdp, len);
        txt[len] = 0;
        len++;
        return ::rvSdpMsgConstructParse(NULL, txt, &len, &status);
    }

    bool sdp_parser_library::encode(void *msg, char *sdp, int& len)
    {
        RvSdpMsg *x = static_cast<RvSdpMsg *>(msg);
        if (NULL == x)
        {
            return false;
        }

        RvSdpStatus stat = RV_SDPSTATUS_ILLEGAL_SET;
        if (NULL == rvSdpMsgEncodeToBuf(x, sdp, len, &stat))
        {
            return false;
        }

        len = strlen(sdp);
        return (RV_SDPSTATUS_OK == stat);
    }

    void sdp_parser_library::term()
    {
        ::RvSdpMgrDestruct();
    }

    namespace detail
    {
        class sdp_parser_msg_helper_t
        {
        public:
            explicit sdp_parser_msg_helper_t(void *p)
                :msg_((RvSdpMsg *)p)
            {}

            ~sdp_parser_msg_helper_t()
            {
                if (NULL != msg_)
                {
                    rvSdpMsgDestruct(msg_);
                    msg_ = NULL;
                }
            }

            RvSdpMsg *operator->()
            {
                return msg_;
            }

            operator bool () const
            {
                return (NULL != msg_);
            }
        private:
            RvSdpMsg *msg_;
        };

        bool _copy_chars(const char *&cstr, char end_char, char *dest, uint32_t dest_len)
        {
            while (' ' == *cstr)
            {
                cstr++;
            }

            uint32_t ii = 0;
            while (('\0' != *cstr) && (end_char != *cstr) && (ii < dest_len))
            {
                dest[ii] = *cstr;

                cstr++;
                ii++;
            }

            if (ii >= dest_len)
            {
                return false;
            }

            dest[ii] = '\0';

            return true;
        }

        bool parse_media_type(const char *media_name, xt_av_media_t& media_type)
        {
            if (0 == strcasecmp("video", media_name))
            {
                media_type = XT_AVMEDIA_TYPE_VIDEO;
            }
            else if (0 == strcasecmp("audio", media_name))
            {
                media_type = XT_AVMEDIA_TYPE_AUDIO;
            }
            else if (0 == strcasecmp("text", media_name))
            {
                media_type = XT_AVMEDIA_TYPE_SUBTITLE;
            }
            else
            {
                return false;
            }

            return true;
        }

        bool parse_media_info(const char *rtpmap, xt_av_codec_id_t& cid, int32_t& payload, uint32_t& sample_rate, uint32_t& channel)
        {
            char cstr[64];

            if (!_copy_chars(rtpmap, ' ', cstr, sizeof(cstr)))
            {
                return false;
            }

            payload = ::atol(cstr);
            if (payload < 0)
            {
                return false;
            }

            if (!_copy_chars(rtpmap, '/', cstr, sizeof(cstr)))
            {
                return false;
            }

            if (0 == strcasecmp("H264", cstr))
            {
                cid = XT_AV_CODEC_ID_H264;
                rtpmap++;
                sample_rate = ::atol(rtpmap);
            }
            else if (0 == strcasecmp("H265", cstr))
            {
                cid = XT_AV_CODEC_ID_H265;
                rtpmap++;
                sample_rate = ::atol(rtpmap);
            }
            else if (0 == strcasecmp("H263-2000", cstr))
            {
                cid = XT_AV_CODEC_ID_H263;
            }
            else if (0 == strcasecmp("MP4V-ES", cstr))
            {
                cid = XT_AV_CODEC_ID_MPEG4;
            }
            else if ((0 == strcasecmp("MP4A-LATM", cstr)) || (0 == strcasecmp("MPEG4-GENERIC", cstr)))
            {
                cid = XT_AV_CODEC_ID_AAC;
            }
            else if (0 == strcasecmp("L16", cstr) /*&& payload >= RTP_PT_PRIVATE*/)//modify by snglei 20150917
            {
                cid = XT_AV_CODEC_ID_PCM_S16BE;
            }
            else if (0 == strcasecmp("PCMU", cstr)/* && payload >= RTP_PT_PRIVATE*/)//modify songlei 20150630 
            {
                cid = XT_AV_CODEC_ID_PCM_MULAW;
                rtpmap++;
                _copy_chars(rtpmap, '/', cstr, sizeof(cstr));
                sample_rate = ::atol(cstr);
                channel = ::atol(rtpmap + 1);
            }
            else if (0 == strcasecmp("PCMA", cstr) /*&& payload >= RTP_PT_PRIVATE*/)//modify by snglei 20150917
            {
                cid = XT_AV_CODEC_ID_PCM_ALAW;
            }
            else if (0 == strcasecmp("AMR", cstr))
            {
                cid = XT_AV_CODEC_ID_AMR_NB;
            }
            else if (0 == strcasecmp("AMR-WB", cstr))
            {
                cid = XT_AV_CODEC_ID_AMR_WB;
            }
            else if (0 == strcasecmp("vorbis", cstr))
            {
                cid = XT_AV_CODEC_ID_VORBIS;
            }
            else if (0 == strcasecmp("VP8", cstr))
            {
                cid = XT_AV_CODEC_ID_VP8;
            }
            else if (0 == strcasecmp("JPEG", cstr))
            {
                cid = XT_AV_CODEC_ID_MJPEG;
            }
            else if (0 == strcasecmp("G722", cstr))
            {
                cid = XT_AV_CODEC_ID_ADPCM_G722;
            }
            else if (0 == strcasecmp("G726-", cstr))
            {
                cid = XT_AV_CODEC_ID_ADPCM_G726;
            }
            else if (0 == strcasecmp("iLBC", cstr))
            {
                cid = XT_AV_CODEC_ID_ILBC;
            }
            else if (0 == strcasecmp("speex", cstr))
            {
                cid = XT_AV_CODEC_ID_SPEEX;
            }
            else if (0 == strcasecmp("opus", cstr))
            {
                cid = XT_AV_CODEC_ID_OPUS;
            }
            else if (0 == strcasecmp("MPA", cstr))
            {
                cid = XT_AV_CODEC_ID_MP3;
                rtpmap++;
                _copy_chars(rtpmap, '/', cstr, sizeof(cstr));
                sample_rate = ::atol(cstr);
                channel = ::atol(rtpmap + 1);
            }
            else
            {
                cid = XT_AV_CODEC_ID_NONE;
                return false;
            }

            return true;
        }
    }

    bool parse_send_info(const char *str, xt_sdp_media_info_t &info)
    {
        ::memset(&info, 0, sizeof(xt_sdp_media_info_t));

        std::string src(str);
        std::string tmp;

        std::string::size_type s1 = src.find("rtp-port");
        if (s1 != std::string::npos)
        {
            std::string::size_type s2 = src.find("=", s1);
            if (s2 != std::string::npos)
            {
                std::string::size_type s3 = src.find(";", s2);
                if (s3 != std::string::npos)
                {
                    tmp = src.substr(s2+1, (s3-s2));
                }
                else
                {
                    s3 = src.length();
                    tmp = src.substr(s2+1, (s3-s2));
                }
            }
        }
        if (!tmp.empty())
        {
            info.port_rtp = ::atoi(tmp.c_str());
            info.port_rtcp = info.port_rtp + 1;
        }

        s1 = src.find("rtpport-mux");
        if (s1 == std::string::npos)
        {
            return true;
        }
        info.demux = true;

        tmp.clear();
        s1 = src.find("muxid");
        if (s1 != std::string::npos)
        {
            std::string::size_type s2 = src.find("=", s1);
            if (s2 != std::string::npos)
            {
                std::string::size_type s3 = src.find(";", s2);
                if (s3 != std::string::npos)
                {
                    tmp = src.substr(s2+1, (s3-s2));
                }
                else
                {
                    s3 = src.length();
                    tmp = src.substr(s2+1, (s3-s2));
                }
            }
        }
        if (!tmp.empty())
        {
            info.demux_id = ::atoi(tmp.c_str());
        }

        return true;
    }

    //static 
    bool sdp_parser_t::parse(const char *uri, const char *sdp, uint32_t length, std::vector<xt_sdp_media_info_t>& media_infos)
    {
        media_infos.clear();

        detail::sdp_parser_msg_helper_t msg(sdp_parser_library::instance()->parse(sdp, length));
        if (!msg)
        {
            return false;
        }

        RvUint32 media_num = rvSdpMsgGetNumOfMediaDescr(msg);
        for (RvUint32 ii = 0; ii < media_num; ++ii)
        {
            RvSdpMediaDescr *media_descr = rvSdpMsgGetMediaDescr(msg, ii);

            RvSdpAttribute *control_attr = NULL;
            RvSdpAttribute *rtpmap_attr = NULL;

            RvUint32 attr_num = rvSdpMediaDescrGetNumOfAttr2(media_descr);
            for (RvUint32 jj = 0; jj < attr_num; ++jj)
            {
                RvSdpAttribute *att = rvSdpMediaDescrGetAttribute2(media_descr, jj);

                if (NULL != att->iAttrName)
                {
                    if ((0 == ::strcmp("control", att->iAttrName)) && (NULL != att->iAttrValue))
                    {
                        control_attr = att;
                    }
                    else if (0 == ::strcmp("rtpmap", att->iAttrName) && (NULL != att->iSpecAttrData))
                    {
                        rtpmap_attr = att;
                    }

                    if ((NULL != control_attr) && (NULL != rtpmap_attr))
                    {
                        break;
                    }
                }
            } // for (int32_t jj = 0; jj < attr_num; ++jj)

            if ((NULL != control_attr) && (NULL != rtpmap_attr))
            {
                xt_sdp_media_info_t sdp_media_info;

                if (!detail::parse_media_type(media_descr->iMediaTypeStr, sdp_media_info.media_type))
                {
                    continue;
                }

                int32_t payload = 0;
                uint32_t sample_rate = 0;
                uint32_t channel = 0;

                char rtpmap_txt[64];
                if (!detail::parse_media_info(rtpmap_attr->iSpecAttrData->iGetValueFunc(rtpmap_attr, rtpmap_txt), sdp_media_info.cid, payload, sample_rate, channel))
                {
                    continue;
                }

                if (0 == strncmp("rtsp", control_attr->iAttrValue, sizeof("rtsp") - 1))
                {
                    (void)strncpy_s(sdp_media_info.uri, control_attr->iAttrValue, MEDIA_CLIENT_URI_LEN);
                }
                else
                {
                    (void)snprintf_s(sdp_media_info.uri, MEDIA_CLIENT_URI_LEN, MEDIA_CLIENT_URI_LEN, "%s/%s", uri, control_attr->iAttrValue);
                }

                media_infos.push_back(sdp_media_info);
            }
            else
            {
                //todo ...
            }
        } //for (int32_t ii = 0; ii < media_num; ++ii)

        return !media_infos.empty();
    }

    bool sdp_parser_t::parse_sip(const char *uri, const char *sdp, uint32_t length, std::vector<xt_sdp_media_info_t>& media_infos)
    {
        media_infos.clear();

        detail::sdp_parser_msg_helper_t msg(sdp_parser_library::instance()->parse(sdp, length));
        if (!msg)
        {
            return false;
        }

        //解析session部分
        std::string ip;
        bool   demux = false;
        uint32_t demux_id = 0;
        xt_sdp_media_info_t video, audio;
        ::memset(&video, 0, sizeof(xt_sdp_media_info_t));
        ::memset(&audio, 0, sizeof(xt_sdp_media_info_t));

        RvSdpConnection* ptr_session_c = rvSdpMsgGetConnection(msg);
        if (NULL != ptr_session_c)
        {
            const char* uri1 = rvSdpConnectionGetAddress(ptr_session_c);
            if (NULL != uri1)
            {
                ip.assign(uri1);
            }
        }
        RvUint32 session_attr_num = rvSdpMsgGetNumOfAttr2(msg);
        for(RvUint32 i=0; i < session_attr_num; ++i)
        {
            RvSdpAttribute* att = rvSdpMsgGetAttribute2(msg,i);
            if (NULL != att->iAttrName)
            {
                if (0 == ::strcmp("rtpport-mux", att->iAttrName))
                {
                    demux = true;
                }
                if (0 == ::strcmp("muxid", att->iAttrName) && att->iAttrValue)
                {
                    demux_id = boost::lexical_cast<uint32_t>(att->iAttrValue);
                }
                if (0 == ::strcmp("send-video", att->iAttrName)  && (NULL != att->iAttrValue))
                {
                    parse_send_info(att->iAttrValue, video);					
                }
                if (0 == ::strcmp("send-audio", att->iAttrName)  && (NULL != att->iAttrValue))
                {
                    parse_send_info(att->iAttrValue, audio);					
                }
            }
        }

        //处理 m=
        RvUint32 media_num = rvSdpMsgGetNumOfMediaDescr(msg);
       md_log(md_log_info,"sdp_parser_t::parse_sip media_num[%d]",media_num);
        uint16_t rtp_port = 0;
        for (RvUint32 ii = 0; ii < media_num; ++ii)
        {
            RvSdpMediaDescr *media_descr = rvSdpMsgGetMediaDescr(msg, ii);

            RvSdpConnection*  ptrConnection = rvSdpMediaDescrGetConnection(media_descr);
            if (NULL != ptrConnection)
            {
                const char* ptruri = rvSdpConnectionGetAddress(ptrConnection);
                if (NULL != ptruri)
                {
                    ip.assign(ptruri);
                }
            }

            rtp_port = media_descr->iPort;

            RvSdpAttribute *rtpmap_attr = NULL;
            RvUint32 attr_num = rvSdpMediaDescrGetNumOfAttr2(media_descr);
            for (RvUint32 jj = 0; jj < attr_num; ++jj)
            {
                RvSdpAttribute *att = rvSdpMediaDescrGetAttribute2(media_descr, jj);

                if (NULL != att->iAttrName)
                {
                    if (0 == ::strcmp("inactive", att->iAttrName) && (NULL != att->iSpecAttrData))
                    {
                        rtpmap_attr = NULL;
                        break;
                    }
                    if (0 == ::strcmp("rtpmap", att->iAttrName) && (NULL != att->iSpecAttrData))
                    {
                        rtpmap_attr = att;
                    }
                    if (0 == ::strcmp("rtpport-mux", att->iAttrName))
                    {
                        demux = true;
                    }
                    if (0 == ::strcmp("muxid", att->iAttrName) && att->iAttrValue)
                    {
                        demux_id = boost::lexical_cast<uint32_t>(att->iAttrValue);
                    }
                }
            } // for (int32_t jj = 0; jj < attr_num; ++jj)

			//复用标识和复用id同时满足要求时为复用，否则不为复用并提示非法
			if ((demux && demux_id==0) || (!demux && demux_id > 0))
			{
				demux = false;
				demux_id = 0;
				md_log(md_log_info,"sdp_parser_t::parse_sip demux and demux_id illegal");
			}

            if (NULL != rtpmap_attr)
            {
                xt_sdp_media_info_t sdp_media_info;

                if (!detail::parse_media_type(media_descr->iMediaTypeStr, sdp_media_info.media_type))
                {
                    continue;
                }

                uint32_t sample_rate = 0;
                uint32_t channel = 0;

                char rtpmap_txt[64];
                if (!detail::parse_media_info(rtpmap_attr->iSpecAttrData->iGetValueFunc(rtpmap_attr, rtpmap_txt), sdp_media_info.cid, sdp_media_info.payload, sample_rate, channel))
                {
                    continue;
                }
                sdp_media_info.demux = demux;
                sdp_media_info.demux_id = demux_id;
                sdp_media_info.port_rtp = rtp_port;
                sdp_media_info.port_rtcp =rtp_port + 1;
                ::strncpy(sdp_media_info.uri,ip.c_str(),MEDIA_CLIENT_URI_LEN);

                if (XT_AVMEDIA_TYPE_VIDEO==sdp_media_info.media_type && video.port_rtp!=0)
                {
                    sdp_media_info.demux = video.demux;
                    sdp_media_info.demux_id = video.demux_id;
                    sdp_media_info.port_rtp = video.port_rtp;
                    sdp_media_info.port_rtcp = video.port_rtp + 1;
                }
                else if (XT_AVMEDIA_TYPE_AUDIO==sdp_media_info.media_type && audio.port_rtp!=0)
                {
                    sdp_media_info.demux = audio.demux;
                    sdp_media_info.demux_id = audio.demux_id;
                    sdp_media_info.port_rtp = audio.port_rtp;
                    sdp_media_info.port_rtcp = audio.port_rtp + 1;
                }

                media_infos.push_back(sdp_media_info);
               md_log(md_log_info,"sdp_parser_t::parse_sip add meida info -uri[%s] media_type[%d] demux[%d] demux_id[%d] rtp_port[%d]",
                    sdp_media_info.uri,sdp_media_info.media_type,demux,demux_id,rtp_port);
            }
            else
            {
                //todo ...
            }
        } //for (int32_t ii = 0; ii < media_num; ++ii)

        //将视频放到前面video audio ..
        std::vector<xt_sdp_media_info_t>::iterator itr  = media_infos.begin();
        for(; media_infos.end() != itr;)
        {
            if ( XT_AVMEDIA_TYPE_VIDEO == itr->media_type)
            {
                xt_sdp_media_info_t tmp = *itr;
                media_infos.erase(itr);
                media_infos.insert(media_infos.begin(),tmp);
                break;
            }
            else
            {
                ++itr;
            }
        }

        return !media_infos.empty();
    }

    //static 
    bool sdp_parser_t::remove_on_pure_audio(char *sdp, int& len)
    {
        detail::sdp_parser_msg_helper_t msg(sdp_parser_library::instance()->parse(sdp, len));
        if (!msg)
        {
            return false;
        }

        RvSdpListIter iter;
        RvSdpMediaDescr *descr = rvSdpMsgGetFirstMediaDescr(msg.operator->(), &iter);
        while (NULL != descr)
        {
            if (0 == strcasecmp(descr->iMediaTypeStr, "audio"))
            {
                descr = rvSdpMsgGetNextMediaDescr(&iter);
            }
            else
            {
                rvSdpMsgRemoveCurrentMediaDescr(&iter);
                descr = rvSdpMsgGetFirstMediaDescr(msg.operator->(), &iter);
            }
        }

        return sdp_parser_library::instance()->encode(msg.operator->(), sdp, len);
    }
}

