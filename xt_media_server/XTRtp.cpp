#include "XTRtp.h"
#include "rv_api.h"
#include "RunInfoMgr.h"
#include "xtXml.h"

#ifdef _OS_WINDOWS
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifndef CLOSE_SESSION 
#include "h_xtsessionserver.h"
#endif

extern void MEDIA_SVR_PRINT(const xt_log_level ll,
                            const char* format,
                            ...);

void xt_fir_cb(unsigned long chanid);

uint32_t bitfieldSet(
                     uint32_t    value,
                     uint32_t    bitfield,
                     int32_t         nStartBit,
                     int32_t         nBits)
{
    uint32_t mask = (1 << nBits) - 1;

    return (value & ~(mask << nStartBit)) +
        ((bitfield & mask) << nStartBit);
}

XTRtp XTRtp::self;
XTRtp::XTRtp(void)
{
}

XTRtp::~XTRtp(void)
{
}

void XTRtp::_raddr_cb(void *hmp, rv_net_address *addr)
{
    if (!_()->m_sink_single || !hmp || !addr)
    {
        return;
    }

    std::string ip;
    rv_net_ipv4 ipv4;
    convert_rvnet_to_ipv4(&ipv4, addr);
    unsigned short port = ipv4.port;

#ifdef _OS_WINDOWS
    in_addr a;
    a.S_un.S_addr = ipv4.ip;
#else
    struct in_addr a;
    a.s_addr = ipv4.ip;;
#endif
    char* ip0 =  ::inet_ntoa(a);
    if (ip0)
    {
        ip = ip0;
    }

    unsigned long chid = -1;
    Rtp_Handle rtp;
    int ret = _()->get_rtp(hmp, chid, rtp);
    if (ret < 0)
    {
        return;
    }

    Rtp_Sink sink;
    ret = _()->get_sink(chid, sink);
    if (ret < 0)
    {
        return;
    }

    if (ip==sink.ip && port==sink.port)
    {
        return;
    }

    MEDIA_SVR_PRINT(level_info, "XTRtp::_raddr_cb: addr[%s]chid[%d] port[%d]",addr,chid,port);

    _()->del_send_chan(chid);

    _()->add_send(chid, ip, port, sink.linkid, sink.mode, sink.ssrc, sink.multiplex, sink.multid);
}

void XTRtp::sink_rtcp_cb(unsigned int ssrc, 
                         rv_rtcp_info &send, 
                         rv_rtcp_info &recieve,
						 unsigned char *ip,
						 unsigned short port,
						 int multiplex,
						 unsigned int multid)
{
    if (0 != ssrc)
    {
		_()->set_rtcp_rport(ssrc,send,recieve,(char *)ip, port,multiplex?true:false,multid);
    }
    return;
}

rv_bool XTRtp::rtcpapp_msg_cb(unsigned int		chan_id,
                              unsigned char		subtype,
                              unsigned int		ssrc,
                              unsigned char*		name,
                              unsigned char*		userData, 
                              unsigned int       userDataLen)
{
    return true;
}

rv_bool XTRtp::rtcp_rawdata_cb(
                               mp_handle sink,
                               uint8_t *buffer,
                               uint32_t buffLen,
                               rv_net_address *remoteAddress,
                               rv_bool *pbDiscardBuffer)
{
    const int len_fic = 12;
    unsigned char FIC[len_fic] = {0};
    if (buffLen<len_fic || !buffer)
    {
        return false;
    }

    ::memcpy(FIC, buffer, len_fic);

    unsigned char FMT = 0;
    unsigned char PT = 0;
    FMT = bitfieldSet(FMT, FIC[0], 0, 5);
    PT = bitfieldSet(PT, FIC[1], 0, 8);
    if (FMT == 4 && 
        PT == 206)//FIR
    {
        unsigned int chanid = 0;
        do 
        {
            std::map<unsigned int, Rtp_Handle>::iterator itr	= instance()->m_rtpHandles.begin();
            for (;itr != instance()->m_rtpHandles.end();++itr)
            {
                Rtp_Handle &r = itr->second;
                if (r.hmsink.hmsink == sink)
                {
                    chanid = itr->first;
                    xt_fir_cb(chanid);
                    break;
                }
            }

        } while (false);
    }

    return true;
}

void XTRtp::set_rtcp_rport(unsigned int ssrc, const rv_rtcp_info &send,const rv_rtcp_info &recieve,
						   char *ip, unsigned short port, bool multiplex, unsigned int multid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex, boost::defer_lock);
    if (lock.try_lock())
    {
        std::list<Rtp_Sink>::iterator itr_sink = m_rtpSinks.begin();
        for (; m_rtpSinks.end() != itr_sink; ++itr_sink)
        {
            //if (itr_sink->ssrc == ssrc)//port is rtcp port that is rtp port+1
			if (strcmp(ip,itr_sink->ip.c_str()) == 0&& port == (itr_sink->port+1)&&
				multiplex == itr_sink->local_multiplex && multid == itr_sink->local_multid)
            {
                ::memcpy(&(itr_sink->send),&send,sizeof(rv_rtcp_info));
                ::memcpy(&(itr_sink->recieve),&recieve,sizeof(rv_rtcp_info));
            }
        }

        lock.unlock();
    } 
}

bool XTRtp::is_same_sink(Rtp_Sink &s1, Rtp_Sink &s2)
{
    if(s1.chanid == s2.chanid &&
        s1.ip == s2.ip &&
        s1.port == s2.port &&
        s1.multiplex == s2.multiplex && 
        s1.multid == s2.multid)
    {
        return true;
    }

    return false;
}

int XTRtp::init(unsigned int num_chan, 
                std::string ip, 
                unsigned short start_port, 
                bool multiplex,
                bool sink_single,
                bool use_traffic_shapping)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex); 
    m_sink_single = sink_single;

    //得到系统的CPU数量
    caster_descriptor descriptor;
    descriptor.rv_adapter_thread_nums = boost::thread::hardware_concurrency();
    descriptor.thread_nums = boost::thread::hardware_concurrency();

    descriptor.numberOfSessions = 1024;
    descriptor.use_traffic_shapping = use_traffic_shapping;

    bool ret = init_mp_caster(&descriptor);
    if (!ret)
    {
        std::cout << "init_mp_caster fail" << std::endl;
        return -1;
    }

    std::string ips[2];
    int num[2];

    ips[0] = ip;
    ips[1] = ip;
    num[0] = num_chan;
    num[1] = num_chan;

    xtXml xml;
#ifdef _OS_WINDOWS
    xml.open("D:\\NetMCSet\\xtrouter_config.xml");
#else
    xml.open("/etc/xtconfig/d/netmcset/xtrouter_config.xml");
#endif//#ifdef _OS_WINDOWS	

    xtXmlNodePtr root = xml.getRoot();

    xtXmlNodePtr router = xml.getNode(root, "router");

    xtXmlNodePtr ip_node = xml.getNode(router,"local_sndip");
    const char *ip1 = xml.getValue(ip_node, "ip1");
    if (NULL != ip1)
    {
        ips[0] = ip1;
        std::string num1 = xml.getValue(ip_node, "num1");

        num[0] = ::atoi(num1.c_str());
    }
    const char *ip2 = xml.getValue(ip_node, "ip2");
    if (NULL != ip2)
    {
        ips[1] = ip2;
        std::string num2 = xml.getValue(ip_node, "num2");

        num[1] = ::atoi(num2.c_str());
    }

    //创建发送服务
    for (unsigned long i=0; i<num_chan; ++i)	
    {
        bc_mp_descriptor bcmp_descriptor;
        rv_net_ipv4 address;

        rv_inet_pton4(&address, ip.c_str());
        if (i < num[0])
        {
            rv_inet_pton4(&address, ips[0].c_str());
        }
        else
        {
            rv_inet_pton4(&address, ips[1].c_str());
        }
        if (multiplex)
        {
            address.port = start_port;
        }
        else
        {
            address.port = start_port + i*2;
        }

        convert_ipv4_to_rvnet(&bcmp_descriptor.local_address, &address);

        address.port = 0;
        convert_rvnet_to_ipv4(&address, &bcmp_descriptor.local_address);

        bcmp_descriptor.manual_rtcp = MP_FALSE;	
        bcmp_descriptor.max_bandwidth = 0;
        bcmp_descriptor.ssrc_type = FRAME_MSSRC;
        bcmp_descriptor.active_now = MP_TRUE;
        bcmp_descriptor.msink_multicast_rtp_ttl = 128;	
        bcmp_descriptor.msink_multicast_rtcp_ttl = 0;
        bcmp_descriptor.multiplex = multiplex;

        Rtp_Handle rtp;	
        rtp.multiplex = multiplex;
        rtp.port = address.port;
        rtp.payload = 96;
		rtp.multid = 0;
		//rtp.hmp就是bc_mp对象实体指针
        ret = open_bc_mp(&bcmp_descriptor, &rtp.hmp, &rtp.hmssrc, &rtp.hmsink, &rtp.multid);
        if(!ret)	
        {
            std::cout << "open_bc_mp fail:num="  << i << std::endl;
            return i;
        }

        set_raddr_cb(&rtp.hmp, _raddr_cb);
        setSink_RtcpCB(rtp.hmsink.hmsink, sink_rtcp_cb);
        setSink_SID(rtp.hmsink.hmsink, i);
        setSink_AppMsgCB(rtp.hmsink.hmsink, rtcpapp_msg_cb);
        mp_rtcp_set_rawdata_cb(rtp.hmsink.hmsink, rtcp_rawdata_cb);

#ifndef CLOSE_SESSION 
        xtm_set_snd_port(i, address.port, multiplex, rtp.multid);
#endif

        m_rtpHandles[i] = rtp;
    }

    return num_chan;
}

int XTRtp::uninit()
{
    bool ret = false;
    boost::unique_lock<boost::shared_mutex> lock(m_mutex); 

    std::map<unsigned int, Rtp_Handle>::iterator itr = m_rtpHandles.begin();
    for (;itr != m_rtpHandles.end(); ++itr)
    {
        Rtp_Handle &rtp = itr->second;
        ret = close_mp(&rtp.hmp);
    }

    m_rtpHandles.clear();

    end_mp_caster();

    return ((ret==true) ? (int)0 : (int)-1);
}

void XTRtp::set_payload(unsigned long chanid, int payload, bool update)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
    if(it == m_rtpHandles.end())
    {
        return;
    }

    Rtp_Handle &rtp = it->second;

    if (update)
    {
        rtp.payload = payload;
    }
    else
    {
        rtp.payload_old = payload;
        rtp.payload = payload;
    }
}

int XTRtp::update_resend_flag(const int flag)
{
    int ret_code = 0;
    do 
    {
        mp_h_s hmp={0};
        std::map<unsigned int, Rtp_Handle>::iterator itr = m_rtpHandles.begin();
        for(; m_rtpHandles.end() != itr; ++itr)
        {
            hmp.hmp = itr->second.hmp.hmp;
            ::update_resend_flag(&hmp,flag);
        }

    } while (0);

    return ret_code;
}

#ifndef _OS_WINDOWS
#define MAX_PATH 256

#endif//#ifndef _OS_WINDOWS
void XTRtp::set_file_patha(const char * file)
{
    do 
    {
        mp_h_s hmp={0};
        std::map<unsigned int, Rtp_Handle>::iterator itr = m_rtpHandles.begin();
        for(int i=0; m_rtpHandles.end() != itr; ++i,++itr)
        {
            hmp.hmp = itr->second.hmp.hmp;

            char file_path[MAX_PATH] = "";
            ::sprintf(file_path,"%s_%d", file, i);
            ::mp_set_file_path(&hmp, file_path);
        }

    } while (0);
}

void XTRtp::reset_payload(unsigned long chanid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex);

    std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
    if(it == m_rtpHandles.end())
    {
        return;
    }
    it->second.payload = it->second.payload_old;	
}

int XTRtp::get_payload(unsigned long chanid)
{
	boost::unique_lock<boost::shared_mutex> lock(m_mutex);

	std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
	if(it == m_rtpHandles.end())
	{
		return 96;
	}
	
	return it->second.payload;
}

int XTRtp::add_send(long chanid,
                    std::string ip,						
                    unsigned short port,			
                    void* linkid,					
                    long mode,					
                    unsigned int ssrc,			
                    bool multiplex,		
                    unsigned int multid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex); 

    MEDIA_SVR_PRINT(level_info, "add_send:chanid[%d] ip[%s] port[%d] linkid[%d] mode[%d] ssrc[%d] demux[%d] demuxid[%d]",
        chanid,ip.c_str(),port,linkid,mode,ssrc,multiplex,multid);

    std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
    if(it == m_rtpHandles.end())
    {
        MEDIA_SVR_PRINT(level_info, "add_send fail:chanid[%d] rtp_num[%d]",chanid,m_rtpHandles.size());
        return -1;
    }

    Rtp_Handle &rtp = it->second;

    rtp_sink_descriptor sink_desc;	
    sink_desc.rtcp_opt = MP_TRUE;
    sink_desc.multiplex = multiplex;
    sink_desc.multiplexID = multid;

    rv_net_ipv4 address;
    ::rv_inet_pton4(&address, ip.c_str());

    address.port = port;
    ::convert_ipv4_to_rvnet(&sink_desc.rtp_address, &address);	

    address.port = port + 1;
    ::convert_ipv4_to_rvnet(&sink_desc.rtcp_address, &address);	

    msink_h_s hmsink;
    mp_bool bRet = add_rtp_sink(&rtp.hmp, &sink_desc, &hmsink);
    if(!bRet)
    {
        MEDIA_SVR_PRINT(level_info, "%s", "增加一个发送_add_rtp_sink失败");
        return -1;
    }

    Rtp_Sink sink;
    sink.chanid = chanid;
    sink.hmsink = hmsink;
    sink.ip = ip;
    sink.port = port;
    sink.linkid = linkid;
    sink.mode = mode;
    sink.ssrc = ssrc;
    sink.multiplex = multiplex;
    sink.multid = multid;
	//赋值发送端复用信息
	sink.local_multiplex = rtp.multiplex;
	sink.local_multid = rtp.multid;

    sink.createtime = common::GetCurTimeMicrosecValue();

    add_sink(sink);

    return 0;
}

void XTRtp::get_rtp_sink(std::list<Rtp_Sink>& lst_rtp_Sink)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex); 
    lst_rtp_Sink = m_rtpSinks;
}

int XTRtp::del_send(long chanid,			
                    std::string ip,						
                    unsigned short port,			
                    long mode,					
                    bool multiplex,		
                    unsigned int multid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex); 

    return _del_send(chanid, ip, port, mode, multiplex, multid);
}
int XTRtp::_del_send(unsigned long chanid,			
                     std::string ip,						
                     unsigned short port,			
                     short mode,					
                     bool multiplex,		
                     unsigned int multid)
{
    MEDIA_SVR_PRINT(level_info, "del send:chanid[%d] ip[%s] port[%d] mode[%d] demux[%d] demuxid[%d]",
        chanid,ip.c_str(),port,mode,multiplex,multid);

    std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
    if(it == m_rtpHandles.end())
    {
        MEDIA_SVR_PRINT(level_info, "del send fail(not find):chanid[%d] ip[%s] port[%d] mode[%d] demux[%d] demuxid[%d]",
            chanid,ip.c_str(),port,mode,multiplex,multid);
        return -1;
    }

    Rtp_Handle &rtp = it->second;

    Rtp_Sink sink;
    sink.chanid = chanid;
    sink.ip = ip;
    sink.port = port;
    sink.mode = mode;
    sink.multiplex = multiplex;
    sink.multid = multid;
    if (get_sink(sink) < 0)
    {
        MEDIA_SVR_PRINT(level_info, "del send fial(not find):chanid[%d] ip[%s] port[%d] mode[%d] demux[%d] demuxid[%d]",
            chanid,ip.c_str(),port,mode,multiplex,multid);
        return -1;
    }

    delete_sink(sink);

    mp_bool bRet = del_sink(&rtp.hmp, &sink.hmsink);

    if(!bRet)
    {
        MEDIA_SVR_PRINT(level_info, "del send fail:chanid[%d] ip[%s] port[%d] mode[%d] demux[%d] demuxid[%d]",
            chanid,ip.c_str(),port,mode,multiplex,multid);
        return -1;
    }

    return 0;
}

int XTRtp::del_send(void *linkid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex); 

    while (true)
    {
        Rtp_Sink sink;
        if (get_sink(linkid, sink) < 0)
        {
            break;
        }

        _del_send(sink.chanid, sink.ip, sink.port, sink.mode, sink.multiplex, sink.multid);
    }

    return 0;
}

int XTRtp::del_send_all()
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex); 

    while (m_rtpSinks.size() > 0)
    {
        Rtp_Sink &sink = *m_rtpSinks.begin();

        _del_send(sink.chanid, sink.ip, sink.port, sink.mode, sink.multiplex, sink.multid);
    }

    return 0;
}

int XTRtp::del_send_chan(unsigned long chanid)
{
    boost::unique_lock<boost::shared_mutex> lock(m_mutex); 

    while (true)
    {
        Rtp_Sink sink;
        if (get_sink(chanid, sink) < 0)
        {
            break;
        }

        _del_send(sink.chanid, sink.ip, sink.port, sink.mode, sink.multiplex, sink.multid);
    }

    return 0;
}

int XTRtp::add_sink(Rtp_Sink &sink)
{
    bool exist = false;
    std::list<Rtp_Sink>::iterator itr = m_rtpSinks.begin();
    for (;itr != m_rtpSinks.end();++itr)
    {
        Rtp_Sink &s = *itr;
        if (is_same_sink(sink, s))
        {
            exist = true;
            break;
        }
    }

    if (exist)
    {
        return -1;
    }

    m_rtpSinks.push_back(sink);

    return 0;
}

int XTRtp::delete_sink(Rtp_Sink &sink)
{
    std::list<Rtp_Sink>::iterator itr = m_rtpSinks.begin();
    for (;itr != m_rtpSinks.end();++itr)
    {
        Rtp_Sink &s = *itr;
        if (is_same_sink(sink, s))
        {
            m_rtpSinks.erase(itr);
            return 0;
        }
    }

    return -1;
}

int XTRtp::get_sink_by_comdition(Rtp_Sink& out_sink,const rtp_sink_comdition_t& comdition)
{
    bool exist = false;
    std::list<Rtp_Sink>::iterator itr = m_rtpSinks.begin();
    for (;itr != m_rtpSinks.end();++itr)
    {
        if (0 ==itr->ip.compare(comdition.ip)
            && comdition.port == itr->port
            && comdition.multiplex_flag == itr->multiplex
            && comdition.muxid == itr->multid
            && comdition.chanid == itr->chanid)
        {
            exist = true;
            out_sink = *itr;
            break;
        }
    }

    if (exist)
    {
        return 0;
    }

    return -1;

}

int XTRtp::get_sink(Rtp_Sink &sink)
{
    bool exist = false;
    std::list<Rtp_Sink>::iterator itr = m_rtpSinks.begin();
    for (;itr != m_rtpSinks.end();++itr)
    {
        Rtp_Sink &s = *itr;
        if (is_same_sink(sink, s))
        {
            exist = true;

            sink.hmsink = s.hmsink;
            sink.linkid = s.linkid;
            sink.ssrc = s.ssrc;

            break;
        }
    }

    if (exist)
    {
        return 0;
    }

    return -1;
}
int XTRtp::get_sink(void *linkid, Rtp_Sink &sink)
{
    bool exist = false;
    std::list<Rtp_Sink>::iterator itr = m_rtpSinks.begin();
    for (;itr != m_rtpSinks.end();++itr)
    {
        Rtp_Sink &s = *itr;
        if (s.linkid == linkid)
        {
            exist = true;
            sink = s;
            break;
        }
    }

    if (exist)
    {
        return 0;
    }

    return -1;
}

int XTRtp::get_cur_sink_num()
{
    return m_rtpSinks.size();
}
int XTRtp::get_sink(unsigned long chanid, Rtp_Sink &sink)
{
    bool exist = false;
    std::list<Rtp_Sink>::iterator itr = m_rtpSinks.begin();
    for (;itr != m_rtpSinks.end();++itr)
    {
        Rtp_Sink &s = *itr;
        if (s.chanid == chanid)
        {
            exist = true;

            sink = s;

            break;
        }
    }

    if (exist)
    {
        return 0;
    }

    return -1;
}

int XTRtp::get_rtp(void *hmp, unsigned long &chanid, Rtp_Handle &rtp)
{
    std::map<unsigned int, Rtp_Handle>::iterator itr	= m_rtpHandles.begin();
    for (;itr != m_rtpHandles.end();++itr)
    {
        Rtp_Handle &r = itr->second;
        if (r.hmp.hmp == hmp)
        {
            chanid = itr->first;
            rtp = r;
            return 0;
        }
    }

    return -1;
}

//20160224 解决跨中心不能点转发hevc的Bug modify by songlei
int XTRtp::send_data(const xt_track_t& track, char *buff, unsigned long len, long device_type, bool is_std)
{
    int ret_code = -1;
    do
    {
        boost::shared_lock<boost::shared_mutex> lock(m_mutex);
        std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(track.chanid);
        if(m_rtpHandles.end() == it) break;
        XTFrameInfo FrameInfo;
        FrameInfo.verify = 0xA1A2A3A4;        //校验位
        FrameInfo.frametype = track.frametype;
        FrameInfo.datatype = device_type;
        FrameInfo.streamtype = STD_RTP_STREAM;

        mp_bool pump_ret = MP_FALSE;
        if (is_std)
        {
            pump_ret = ::pump_frame_in(&it->second.hmp, &it->second.hmssrc, (uint8_t*)buff, len, MP_FALSE, 0, it->second.payload, FrameInfo,0,true,0,0);
        }
        else
        {
            pump_ret = ::pump_frame_in(&it->second.hmp, &it->second.hmssrc, (uint8_t*)buff, len, MP_FALSE, 0, it->second.payload,FrameInfo,0,false,0,0);
        }

        if (MP_FALSE == pump_ret)
        {
            ret_code = -2;
            break;
        }
        ret_code = 1;
    } while (0);
    return ret_code;
}

int XTRtp::send_data(unsigned long chanid, char *buff, unsigned long len, int frame_type, long device_type, bool is_std)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
    if(it == m_rtpHandles.end())
    {
        return -1;
    }

    Rtp_Handle &rtp = it->second;

    XTFrameInfo FrameInfo;	
    FrameInfo.verify = 0xA1A2A3A4;		//校验位
    FrameInfo.frametype = frame_type;
    FrameInfo.datatype = device_type;
    FrameInfo.streamtype = STD_RTP_STREAM;

    if (is_std)
    {
        pump_frame_in(&rtp.hmp, 
            &rtp.hmssrc, 
            (uint8_t*)buff, 
            len, 
            MP_FALSE, 
            0, 
            rtp.payload,
            FrameInfo,0,true,0,0);
    }
    else
    {
        pump_frame_in(&rtp.hmp, 
            &rtp.hmssrc, 
            (uint8_t*)buff, 
            len, 
            MP_FALSE, 
            0, 
            rtp.payload,
            FrameInfo,0,false,0,0);
    }

    return 0;
}

//20160224 解决跨中心不能点转发hevc的Bug modify by songlei
int XTRtp::send_data_in_stamp(const xt_track_t& track,           // 转发track
                       char *buff,                               // 发送数据
                       unsigned long len,                        // 数据长度
                       long device_type,                         // 设备类型
                       bool frame_ts_flg,                        // 是否外部传入时戳
                       uint32_t in_time_stamp,                   // 外部输入时戳
                       uint8_t priority,                         // 优先级
                       bool is_std /*= false*/,
					   bool use_ssrc,
					   uint32_t ssrc)                  // 标准流开关
{
    int ret_code = -1;
    do 
    {
        boost::shared_lock<boost::shared_mutex> lock(m_mutex);
        std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(track.chanid);
        if(m_rtpHandles.end() == it) break;

        XTFrameInfo FrameInfo;
        FrameInfo.verify = 0xA1A2A3A4;        // 校验位
        FrameInfo.frametype = track.frametype;
        FrameInfo.datatype = device_type;
        FrameInfo.streamtype = STD_RTP_STREAM;

        mp_bool frameTS_opt = frame_ts_flg ? MP_TRUE : MP_FALSE;
        mp_bool pump_ret = MP_FALSE;
        if (is_std)
        {
            pump_ret = ::pump_frame_in(&it->second.hmp, &it->second.hmssrc, (uint8_t*)buff, len, frameTS_opt, in_time_stamp, it->second.payload, FrameInfo,priority,true,use_ssrc,ssrc);
        }
        else
        {
            pump_ret = ::pump_frame_in(&it->second.hmp, &it->second.hmssrc, (uint8_t*)buff, len, frameTS_opt, in_time_stamp, it->second.payload, FrameInfo,priority,false,use_ssrc,ssrc);
        }

        if (MP_FALSE == pump_ret)
        {
            ret_code = -2;
            break;
        }

        ret_code = 1;
    } while (0);
    return ret_code;
}

int XTRtp::send_rtp_in_stamp(const xt_track_t& track,           // 转发track
							  char *buff,                               // 发送数据
							  unsigned long len,                        // 数据长度
							  long device_type,                         // 设备类型
							  bool frame_ts_flg,                        // 是否外部传入时戳
							  uint32_t in_time_stamp,                   // 外部输入时戳
							  uint8_t priority,                         // 优先级
							  bool is_std /*= false*/,
							  bool use_ssrc,
							  uint32_t ssrc)                  // 标准流开关
{
	int ret_code = -1;
	do 
	{
		boost::shared_lock<boost::shared_mutex> lock(m_mutex);
		std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(track.chanid);
		if(m_rtpHandles.end() == it) break;

		XTFrameInfo FrameInfo;
		FrameInfo.verify = 0xA1A2A3A4;        // 校验位
		FrameInfo.frametype = track.frametype;
		FrameInfo.datatype = device_type;
		FrameInfo.streamtype = STD_RTP_STREAM;

		mp_bool frameTS_opt = frame_ts_flg ? MP_TRUE : MP_FALSE;
		mp_bool pump_ret = MP_FALSE;
		if (is_std)//buff实际上为block指针
		{
			pump_ret = ::pump_rtp_in2(&it->second.hmp, &it->second.hmssrc, buff);
		}
		else
		{
			pump_ret = ::pump_rtp_in2(&it->second.hmp, &it->second.hmssrc, buff);
		}

		if (MP_FALSE == pump_ret)
		{
			ret_code = -2;
			break;
		}

		ret_code = 1;
	} while (0);
	return ret_code;
}

int XTRtp::send_data_in_stamp(unsigned long chanid,              // 通道号
                              char *buff,                        // 发送数据
                              unsigned long len,                 // 数据长度
                              int frame_type,                    // 帧类型
                              long device_type,                  // 设备类型
                              bool frame_ts_flg,                 // 是否外部传入时戳
                              uint32_t in_time_stamp,            // 外部输入时戳
                              uint8_t priority,                  // 发送数据优先级
                              bool is_std /*= false*/)           // 标准rtp
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex); 

    std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
    if(it == m_rtpHandles.end())
    {
        return -1;
    }

    Rtp_Handle &rtp = it->second;

    XTFrameInfo FrameInfo;	
    FrameInfo.verify = 0xA1A2A3A4;		//校验位
    FrameInfo.frametype = frame_type;
    FrameInfo.datatype = device_type;
    FrameInfo.streamtype = STD_RTP_STREAM;

    mp_bool frameTS_opt = MP_FALSE;
    if (frame_ts_flg)
    {
        frameTS_opt = MP_TRUE;
    }

    if (is_std)
    {
        pump_frame_in(&rtp.hmp, 
            &rtp.hmssrc, 
            (uint8_t*)buff, 
            len, 
            frameTS_opt, 
            in_time_stamp, 
            rtp.payload,
            FrameInfo,priority,true,0,0);
    }
    else
    {
        pump_frame_in(&rtp.hmp, 
            &rtp.hmssrc, 
            (uint8_t*)buff, 
            len, 
            frameTS_opt, 
            in_time_stamp, 
            rtp.payload,
            FrameInfo,priority,false,0,0);
    }

    return 0;

}

//20160405 增加国标PS流RTP发送接口
int XTRtp::send_data_in_stamp_ps(const xt_track_t& track,           // 转发track
                              char *buff,                               // 发送数据
                              unsigned long len,                        // 数据长度
                              long device_type,                         // 设备类型
                              bool frame_ts_flg,                        // 是否外部传入时戳
                              uint32_t in_time_stamp,                   // 外部输入时戳
                              uint8_t priority,                         // 优先级
                              bool is_std,                                // 标准流开关
                              bool usr_ssrc,
                              uint32_t ssrc)
{
    int ret_code = -1;
    do 
    {
        boost::shared_lock<boost::shared_mutex> lock(m_mutex);
        std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(track.chanid);
        if(m_rtpHandles.end() == it) break;

        XTFrameInfo FrameInfo;
        FrameInfo.verify = 0xA1A2A3A4;        // 校验位
        FrameInfo.frametype = track.frametype;
        FrameInfo.datatype = device_type;
        FrameInfo.streamtype = PS_RTP_STREAM;

        mp_bool frameTS_opt = frame_ts_flg ? MP_TRUE : MP_FALSE;
        mp_bool pump_ret = MP_FALSE;
        if (is_std)
        {
            pump_ret = ::pump_frame_in(&it->second.hmp, &it->second.hmssrc, (uint8_t*)buff, len, frameTS_opt, in_time_stamp, it->second.payload, FrameInfo,priority,true,usr_ssrc,ssrc);
        }
        else
        {
            pump_ret = ::pump_frame_in(&it->second.hmp, &it->second.hmssrc, (uint8_t*)buff, len, frameTS_opt, in_time_stamp, it->second.payload, FrameInfo,priority,false,0,0);
        }

        if (MP_FALSE == pump_ret)
        {
            ret_code = -2;
            break;
        }

        ret_code = 1;
    } while (0);
    return ret_code;
}

int XTRtp::get_sink_sn(long chanid, unsigned short *sn)
{
	boost::shared_lock<boost::shared_mutex> lock(m_mutex); 

	std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
	if (it == m_rtpHandles.end())
	{
		return -1;
	}

	Rtp_Handle rtp = it->second;

	int ret = ::get_sink_sn(&rtp.hmp, sn);
	if (ret < 0)
	{
		return -1;
	}

	return 0;
}

int XTRtp::get_rtp(unsigned long chanid, Rtp_Handle &rtp)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex); 

    std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chanid);
    if (it == m_rtpHandles.end())
    {
        return -1;
    }

    rtp = it->second;
    return 0;
}

#ifdef _USE_RTP_SEND_CONTROLLER
static void mp_network_changed_cb(void *ctx, mp_handle hmp, uint32_t bitrate, uint32_t fraction_lost, uint32_t rtt)
{
    Rtp_Handle *h = static_cast<Rtp_Handle *>(ctx);
    if (NULL != h)
    {
        h->cb(h->ctx, bitrate, fraction_lost, rtt);
    }
}

int XTRtp::register_network_changed_callback(int chan, xt_network_changed_callback_t cb, void *ctx)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mutex);
    std::map<unsigned int, Rtp_Handle>::iterator it = m_rtpHandles.find(chan);
    if (m_rtpHandles.end() == it)
    {
        return -1;
    }

    it->second.cb = cb;
    it->second.ctx = ctx;

    mp_register_network_changed_callback(it->second.hmp.hmp, mp_network_changed_cb, &(it->second));
    return 0;
}
#endif
