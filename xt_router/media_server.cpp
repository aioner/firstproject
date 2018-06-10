#include "media_server.h"
#include <string.h>
#include "XTRouterLog.h"

//#define SIP_TEST_CODE

media_server media_server::self;
media_server::media_server(void)
{
}

media_server::~media_server(void)
{
}

int media_server::init(MS_CFG &cfg)
{
    int ret = ::xt_init_server(cfg);

#ifdef SIP_TEST_CODE
    start_sip_port_t sip_port;
    sip_port.protocol = TRANS_PROTOCOL_TCP | TRANS_PROTOCOL_UDP;
    sip_port.transport = 5060;
    sip_port.tls_port = 0;
    sip_port.dtls_port = 0;

    start_timer_t start_timer;
    start_timer.default_session_time_interval = 10000;

    ::xt_start_sip(sip_port,start_timer,"11.55.10.232", "songlei01", "123456");

    regist_timer_t regist_timer;
    regist_timer.expires = 300;
    regist_timer.link_time_interval = 10000;
    regist_timer.regist_retry_time_interval = 60;

    sip_channel_t chtype;
    chtype.mtu = 1500;
    chtype.bandwidth = 4096;
    chtype.delay = 0;
    chtype.packetloss = 0;
    chtype.type = XT_CHAN_FIXED;

    ::xt_regist_sip(regist_timer,chtype,"sip:songlei01@11.55.10.232",NULL, 0);

#endif //#ifdef SIP_TEST_CODE

    return ret;
}

int media_server::uninit()
{
#ifdef SIP_TEST_CODE
    ::xt_unregist_sip();
    ::xt_stop_sip();
#endif//#ifdef SIP_TEST_CODE
    return ::xt_uninit_server();
}

int media_server::regist(const std::string ids, const std::string local_ip, unsigned short local_port,
                         const std::string server_ip, unsigned short server_port)
{
    return 0;
    //return ::xt_regist(ids.c_str(), server_ip.c_str(), server_port);
}

int media_server::stop_regist(const char* sz_server_ip,unsigned short server_port)
{
    return 0;
    //return ::xt_stop_regist(sz_server_ip,server_port);
}

int media_server::regist2(const std::string ids, const std::string server_ip, unsigned short server_port, unsigned int millisec)
{
	return ::xt_regist(ids.c_str(), server_ip.c_str(), server_port, millisec);
}

int media_server::stop_regist2(const char* sz_server_ip,unsigned short server_port, unsigned int millisec)
{
	return ::xt_stop_regist(sz_server_ip,server_port, millisec);
}

int media_server::create_src(int tracknum,
                             int *trackids,
                             char *tracknames[],
                             int &srcno,				
                             long chanid)
{
    /*char tracknames2[2][128] = {"video", "audio"};

    src_track_info_t tracks;
    tracks.tracknum = tracknum;
    ::memcpy(tracks.trackids, trackids, sizeof(int)*tracknum);
    for (int ii=0; ii< tracknum; ++ii)
    {
    ::memcpy(tracks.tracknames[ii],tracknames[ii], 64);
    }

    return ::xt_create_src(tracks, srcno, chanid);//*/
    return ::xt_create_src(tracknum, trackids, tracknames, srcno, chanid);
}

int media_server::destroy_src(int srcno)
{
    return ::xt_destroy_src(srcno);
}

int media_server::set_key_data(int srcno, char *keydata, long len, long datatype)
{
    return ::xt_set_key(srcno, keydata, len, datatype);
}

int media_server::send_frame_stamp(int srcno,				// 源id
                                  int trackid,			// trackid
                                  char *buff,				// 发送数据
                                  unsigned long len,		// 数据长度
                                  int frame_type,			// 帧类型
                                  long device_type,		// 设备类型
                                  uint32_t in_time_stamp,
								  bool use_ssrc,
								  uint32_t ssrc)// 外部输入时戳
{
    return ::xt_send_data_in_stamp(srcno,trackid,buff,len,frame_type,device_type,true,in_time_stamp,use_ssrc,ssrc);
}

int media_server::send_rtp_stamp(int srcno,				// 源id
								   int trackid,			// trackid
								   char *buff,				// 发送数据
								   unsigned long len,		// 数据长度
								   int frame_type,			// 帧类型
								   long device_type,		// 设备类型
								   uint32_t in_time_stamp,
								   bool use_ssrc,
								   uint32_t ssrc)// 外部输入时戳
{
	return ::xt_send_rtp_in_stamp(srcno,trackid,buff,len,frame_type,device_type,true,in_time_stamp,use_ssrc,ssrc);
}

int media_server::get_chanid(int srcno, int trackid, long &chanid)
{
    return ::xt_get_chanid(srcno, trackid, chanid);
}

int media_server::add_send(xmpp_cfg_t& cfg)
{
    return ::add_trans_sever(cfg);
}

//增加发一个发送
int media_server::add_send(int srcno,              // 源id
                           int trackid,            //track id
                           const char *ip,           // 目标ip
                           unsigned short port,      // 目标port
                           bool demux /*= false*/,       // 复用
                           unsigned int demuxid /*= 0*/) // 复用id
{
    return ::xt_add_send(srcno,trackid,ip,port,demux,demuxid);
}

// 删除转发
int media_server::del_send(int srcno,              // 源id
                           int trackid,             //track id
                           const char *ip,          // 目标ip
                           unsigned short port,      // 目标port
                           bool demux /*= false*/,        // 复用
                           unsigned int demuxid/* = 0*/)  // 复用id
{
    return ::xt_del_send(srcno,trackid,ip,port,demux,demuxid);

}

// 删除转发(srcno 源id)
int media_server::del_send_src(int srcno)
{
    return ::xt_del_send_src(srcno);

}

// 删除所有转发
int media_server::del_send_all()
{
    return ::xt_del_send_all();
}

int media_server::get_svr_info(svr_info info[],int& tracknum,const int srcno)
{
    return ::xt_get_svr_info(info,tracknum,srcno); 
}

int media_server::create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const char* local_bind_ip)
{
    return ::xt_create_src_defult(srcno,sdp,sdp_len,chanid,local_bind_ip);
}

int media_server::set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval)
{
    return ::xt_ms_set_rtsp_heartbit_time(check_timer_interval,time_out_interval);
}
