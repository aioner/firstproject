#ifndef _XT_SESSION_H
#define _XT_SESSION_H

#include <map>
#include <boost/thread/recursive_mutex.hpp>

using namespace std;

#define LEN_SDP	4096	//SDP��Ϣ����
#define LEN_IP	256		//IP 

// SDP
struct xt_sdp 
{
    char	sdp[LEN_SDP];
    int 	len;
};

// �˿���Ϣ
struct xt_track_port
{
    int track_id;
    unsigned short snd_port;
    bool demux;
    unsigned int demuxid;
};

namespace XT_RTSP
{

    class rtsp_src
    {
    public:
        rtsp_src(void);
        ~rtsp_src(void);

    public:
        // ����SDP
        int set_sdp(const char *sdp, int len);

        // ���SDP
        int get_sdp(char *sdp, int &len);

        // ����track���Ͷ˿�
        int set_snd_port(int trackid, unsigned short port, bool demux,unsigned int demuxid);

        // ���track���Ͷ˿�
        int get_snd_port(int trackid, unsigned short &port, bool &demux,unsigned int &demuxid);

    private:
        boost::recursive_mutex	m_mutex;		//mutex
        xt_sdp						m_sSDP;			//SDP
        map<int,xt_track_port>		m_mTrkPort;		//track port
    };

}
#endif//end #ifndef _XT_SESSION_H
