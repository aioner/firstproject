#include "XTSrc.h"
#include "rvsdp.h"

extern int sdp_erase_control_full_uri(char *sdp, int len);

namespace XT_RTSP
{

    rtsp_src::rtsp_src(void)
    {
        ::memset(&m_sSDP, 0, sizeof(xt_sdp));
    }

    rtsp_src::~rtsp_src(void)
    {
    }

    int rtsp_src::set_sdp(const char *sdp, int len)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        int ret = -1;
        if (!sdp || len<=0)
        {
            return -1;
        }

        if (len <= LEN_SDP)
        {
            ret = 0;
            ::memcpy(m_sSDP.sdp, sdp, len);
            m_sSDP.len = len;

            int new_sdp_len = sdp_erase_control_full_uri(m_sSDP.sdp, len);
            if (new_sdp_len > 0)
            {
                m_sSDP.len = new_sdp_len;
            }
        }

        return ret;
    }

    int rtsp_src::get_sdp(char *sdp, int &len)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        int ret = -1;
        if (!sdp || len<=0)
        {
            return -1;
        }

        if ((m_sSDP.len > 0) && (len >= m_sSDP.len))
        {
            ret = 0;
            ::memcpy(sdp, m_sSDP.sdp, m_sSDP.len);
            len = m_sSDP.len;
        }

        return ret;
    }

    int rtsp_src::set_snd_port(int trackid, unsigned short port, bool demux,unsigned int demuxid)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        m_mTrkPort[trackid].track_id = trackid;
        m_mTrkPort[trackid].snd_port = port;
        m_mTrkPort[trackid].demux = demux;
        m_mTrkPort[trackid].demuxid = demuxid;

        return 0;
    }

    int rtsp_src::get_snd_port(int trackid, unsigned short &port, bool &demux,unsigned int &demuxid)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        if (m_mTrkPort.find(trackid) == m_mTrkPort.end())
        {
            return -1;
        }

        port = m_mTrkPort[trackid].snd_port;
        demux = m_mTrkPort[trackid].demux;
        demuxid = m_mTrkPort[trackid].demuxid;
        return 0;
    }

}
