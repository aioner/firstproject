//
//create by songlei 20160316
//
#include "dps_data_send_mgr.h"
#include "dps_cfg_mgr.h"
#include "dps_ch_mgr.h"

#ifdef _WIN32
#ifdef _DEBUG
# pragma comment(lib,"XTMediaServer_d.lib")
#pragma message("Auto Link XTMediaServer_d.lib")
#else
# pragma comment(lib,"XTMediaServer.lib")
#pragma message("Auto Link XTMediaServer.lib")
#endif // end #ifdef _DEBUG
#endif // end #ifdef _WIN32

dps_data_send_mgr dps_data_send_mgr::my_;

int MEDIASERVER_STDCALL ms_multi_code_query_cb(const int prime_srcno,const ms_code_t code,int* srcno)
{
    int ret_code = -1;
    do 
    {
        if (NULL == srcno) break;
        srcno_t tmp_srcno = -1;
        long stream_type = dps_data_send_mgr::_()->ms_code_stream_type_cast(code);
        if ( -1 == stream_type)
        {//此时按主、子、音的顺序进行转发
           tmp_srcno = dps_ch_mgr::_()->get_srcno_by_ch_and_streamtype(prime_srcno,0);
           if (tmp_srcno < 0)
           {
              tmp_srcno = dps_ch_mgr::_()->get_srcno_by_ch_and_streamtype(prime_srcno,1);
              if (tmp_srcno < 0)
              {
                  tmp_srcno = dps_ch_mgr::_()->get_srcno_by_ch_and_streamtype(prime_srcno,2);
              }
           }
        }
        else
        {
            tmp_srcno = dps_ch_mgr::_()->get_srcno_by_ch_and_streamtype(prime_srcno,stream_type);
        }
        *srcno = tmp_srcno;
        ret_code = 1;
    } while (0);
    return ret_code;
}

long dps_data_send_mgr::ms_code_stream_type_cast(ms_code_t code) const
{
    long stream_type = -1;
    switch (code)
    {
    case ms_code_main:
        {
            stream_type = 0;
            break;
        }
    case ms_code_sub:
        {
            stream_type=1;
            break;
        }
    case ms_code_audio:
        {
            stream_type=2;
            break;
        }
    default:
        {
            stream_type = -1;
            break;
        }
    }
    return stream_type;
}

int dps_data_send_mgr::start()
{
    cfg_.num_chan = dps_cfg_mgr::_()->chan_num(64);
    std::memcpy(cfg_.ip, dps_cfg_mgr::_()->local_bind_ip("0.0.0.0").c_str(), DPS_MAX_IP_LEN);
    cfg_.snd_start_port = dps_cfg_mgr::_()->send_port(20010) ;
    cfg_.demux = dps_cfg_mgr::_()->is_demux_s(false);
    std::memcpy(cfg_.mul_start_ip, dps_cfg_mgr::_()->mul_ip("239.0.0.1").c_str(), DPS_MAX_IP_LEN);
    cfg_.msg_liten_port = dps_cfg_mgr::_()->xtmsg_listenport(20001);
    cfg_.rtsp_listen_port = dps_cfg_mgr::_()->rtsp_listenport(1554);
    cfg_.tcp_listen_port = 0;
    cfg_.snd_std_rtp = dps_cfg_mgr::_()->is_std_rtp(true);
    cfg_.sink_single = dps_cfg_mgr::_()->is_sink_perch(false);
    cfg_.udp_listen_port = dps_cfg_mgr::_()->udp_listenport(19900);
    cfg_.use_traffic_shaping = false;

    // strm control callback
    cfg_.rtsp_play_cb = NULL;
    cfg_.rtsp_pause_cb = NULL;
    cfg_.tcp_play_cb = NULL;
    cfg_.tcp_pause_cb = NULL;
    cfg_.xt_media_server_log_cb = NULL;
    cfg_.rtcp_force_iframe_cb = NULL;
    ::xt_register_multi_code_query_callback(ms_multi_code_query_cb);
    return init(cfg_);
}
int dps_data_send_mgr::init(ms_cfg &cfg)
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

int dps_data_send_mgr::uninit()
{
#ifdef SIP_TEST_CODE
    ::xt_unregist_sip();
    ::xt_stop_sip();
#endif//#ifdef SIP_TEST_CODE
    return ::xt_uninit_server();
}

srcno_t dps_data_send_mgr::create_src_ex(src_info_t& src,bool is_auto_malloc_srno/* = true*/)
{
    src_info_t::read_lock_t lock(src.mutex_track_infos_);
    int trackids[MAX_TRACK]= {0};
    char* tracknames[MAX_TRACK] = {0};
    int tindex = 0;
    for (src_info_t::track_infos_container_itr_t itr = src.track_infos_.begin(); src.track_infos_.end() != itr; ++itr)
    {
        trackids[tindex] = itr->track_id;
        tracknames[tindex] = itr->track_name;
        ++tindex;
    }
    int specify_channle = -1;
    if (is_auto_malloc_srno)
    {
        specify_channle = -1;
    }
    else
    {
        specify_channle = src.transmit_ch_;
    }
    int srcno = -1;
    int ret = create_src(src.track_num_, trackids,tracknames,srcno,specify_channle);
    if (ret < 0 ) srcno = -2;
    src.srcno_ = srcno;
    return srcno;
}

int dps_data_send_mgr::regist(const std::string ids, const std::string server_ip, unsigned short server_port, unsigned int millisec)
{
    return ::xt_regist(ids.c_str(), server_ip.c_str(), server_port, millisec);
}

int dps_data_send_mgr::stop_regist(const char* sz_server_ip,unsigned short server_port, unsigned int millisec)
{
    return ::xt_stop_regist(sz_server_ip,server_port, millisec);
}

int dps_data_send_mgr::create_src(int tracknum, int *trackids, char *tracknames[], int &srcno, long chanid)
{
    return ::xt_create_src(tracknum, trackids, tracknames, srcno, chanid);
}

int dps_data_send_mgr::destroy_src(int srcno)
{
    return ::xt_destroy_src(srcno);
}

int dps_data_send_mgr::set_key_data(int srcno, char *keydata, long len, long datatype)
{
    return ::xt_set_key(srcno, keydata, len, datatype);
}

int dps_data_send_mgr::send_data_stamp(int srcno, int trackid, char *buff, unsigned long len, int frame_type, long device_type, uint32_t in_time_stamp)
{
    return ::xt_send_data_in_stamp(srcno,trackid,buff,len,frame_type,device_type,true,in_time_stamp);
}

int dps_data_send_mgr::get_chanid(int srcno, int trackid, long &chanid)
{
    return ::xt_get_chanid(srcno, trackid, chanid);
}

int dps_data_send_mgr::add_send(xmpp_cfg_t& cfg)
{
    return ::add_trans_sever(cfg);
}

//增加发一个发送
int dps_data_send_mgr::add_send(int srcno, int trackid, const char *ip, unsigned short port,  bool demux /*= false*/, unsigned int demuxid /*= 0*/) 
{
    return ::xt_add_send(srcno,trackid,ip,port,demux,demuxid);
}

// 删除转发
int dps_data_send_mgr::del_send(int srcno, int trackid, const char *ip, unsigned short port, bool demux /*= false*/, unsigned int demuxid/* = 0*/)
{
    return ::xt_del_send(srcno,trackid,ip,port,demux,demuxid);
}

// 删除转发(srcno 源id)
int dps_data_send_mgr::del_send_src(int srcno)
{
    return ::xt_del_send_src(srcno);
}

// 删除所有转发
int dps_data_send_mgr::del_send_all()
{
    return ::xt_del_send_all();
}

int dps_data_send_mgr::get_svr_info(svr_info info[],int& tracknum,const int srcno)
{
    return ::xt_get_svr_info(info,tracknum,srcno); 
}

int dps_data_send_mgr::create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const char* local_bind_ip)
{
    return ::xt_create_src_defult(srcno,sdp,sdp_len,chanid,local_bind_ip);
}

int dps_data_send_mgr::set_rtsp_heartbit_time(const unsigned int check_timer_interval,const unsigned int time_out_interval)
{
    return ::xt_ms_set_rtsp_heartbit_time(check_timer_interval,time_out_interval);
}

void const dps_data_send_mgr::get_connect_info(connect_info_container_t& connect_infos)const
{
    connect_infos.clear();
    do 
    {
        uint32_t connect_num=xt_get_cur_connect_num();
        if (connect_num < 1) break;
        
        connect_info_t*  ptr_info = new connect_info_t[connect_num];
        if ( NULL == ptr_info) break;

        int ret = ::xt_get_connect_info(ptr_info,connect_num);
        if (ret < 0 ) break;
        for(uint32_t index=0; index < connect_num; ++index)
        {
            connect_infos.push_back(ptr_info[index]);
        }

        if (NULL != ptr_info)
        {
            delete [] ptr_info;
            ptr_info = NULL;
        }
    } while (0);
}
