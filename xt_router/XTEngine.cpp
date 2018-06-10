#include "XTEngine.h"
#include <iostream>
#include "media_device.h"
#include "InfoMgr.h"
#include "router_task.h"
#include "center_common_types.h"
#include "Router_config.h"
#include "XTRouter.h"
#include "share_type_def.h"
#include "xt_regist_server.h"
#include "XTRouterLog.h"
#include "break_monitor.h"
#include "pri_jk_engine.h"

#define SDP_TEMPLATE "v=0\no=- 1430622498429749 1 IN IP4 0.0.0.0\ns=PLAY stream from IPNC\nb=AS:12000\nt=0 0\na=tool:XTRouter Media v2015.05.20\na=rtcp-fb:* ccm fir\n"

#define	 FTypeSize sizeof(long)
#define KEY_FRAME_TYPE 128 // 系统头帧类型

#define MEDIA_TYPE_AUDIO 1 //音频
#define MEDIA_TYPE_VIDEO 0 //视频
#define MEDIA_TYPE_NA -1

XTEngine XTEngine::self;
XTEngine::XTEngine(void)
:m_nCopy(-1)
,m_strmids(0)
,m_srcs(0)
{
}

XTEngine::~XTEngine(void)
{
}

void XTEngine::media_server_log_cb(char* logname,log_level_type level,char* log_ctx,uint32_t log_ctx_len)
{
    severity_level ll_level = ll_off;
    switch(level)
    {
    case log_info:
        {
            ll_level = ll_info;

            break;
        }

    case log_warn:
        {
            ll_level = ll_warn;
            break;
        }

    case log_err:
        {
            ll_level = ll_error;
            break;
        }

    default:
        {
            break;
        }

    }

    WRITE_LOG(logname, ll_level, "%s", log_ctx);
}

//RTSP流化控制
int XTEngine::ctrl_rtsp_play_cb(int srcno, int trackid,long chid, double npt, float scale, uint32_t *rtp_pkt_seq, uint32_t *rtp_pkt_timestamp)
{
    int ret = 0;
    do 
    {
        src_info src;

        int srcno = 0;

        ret = instance()->get_src_no(srcno, src);
        if (srcno < 0)
        {
            ret = -1;
            break;
        }

        if (src.device.db_type != DEV_STREAM)
        {
            ret = 2;
            break;
        }

        switch(src.device.link_type)
        {
        case LINK_TCP_RTP_PRI:
            {
                ret = media_device::tcp_play_ctrl(src.device.dev_handle,npt,scale,rtp_pkt_timestamp);
                break;
            }

        case LINK_RTSP_RTP_STD:
            {
                ret = media_device::rtsp_play_ctrl(src.device.dev_handle,npt,scale,rtp_pkt_timestamp);
                break;
            }
        default:
            {
                break;
            }
        }

    } while (false);

    return ret;
}
int XTEngine::ctrl_rtsp_pause_cb(int srcno, int trackid, long chid)
{
    int ret = 0;
    do 
    {
        src_info src;
        int srcno = 0;

        ret = instance()->get_src_no(srcno, src);
        if (ret < 0)
        {
            ret = -1;
            break;
        }

        if (src.device.db_type != DEV_STREAM)
        {
            ret = 2;
            break;
        }

        switch(src.device.link_type)
        {
        case LINK_TCP_RTP_PRI:
            {
                ret = media_device::tcp_pause_ctrl(src.device.dev_handle);
                break;
            }

        case LINK_RTSP_RTP_STD:
            {
                ret = media_device::rtsp_pause_ctrl(src.device.dev_handle);
                break;
            }
        default:
            {
                break;
            }
        }


    } while (false);

    return ret;

}

//TCP流化控制
int XTEngine::ctrl_tcp_play_cb(int srcno,long chid,double npt,float scale,uint32_t *rtp_pkt_timestamp)
{
    int ret = 0;
    do 
    {
        src_info src;
        ret = instance()->get_src_no(srcno,src);
        if (ret < 0)
        {
            break;
        }
        if (src.device.db_type != DEV_STREAM)
        {
            ret = 2;
            break;
        }

        switch(src.device.link_type)
        {
        case LINK_TCP_RTP_PRI:
            {
                ret = media_device::tcp_play_ctrl(src.device.dev_handle,npt,scale,rtp_pkt_timestamp);
                break;
            }

        case LINK_RTSP_RTP_STD:
            {
                ret = media_device::rtsp_play_ctrl(src.device.dev_handle,npt,scale,rtp_pkt_timestamp);
                break;
            }
        default:
            {
                break;
            }
        }

    } while (false);

    return ret;

}
int XTEngine::ctrl_tcp_pause_cb(int srcno,long chid)
{
    int ret = 0;
    do 
    {
        src_info src;
        ret = instance()->get_src_no(srcno,src);
        if (ret < 0)
        {
            break;
        }

        if (src.device.db_type != DEV_STREAM)
        {
            ret = 2;
            break;
        }

        switch(src.device.link_type)
        {
        case LINK_TCP_RTP_PRI:
            {
                ret = media_device::tcp_pause_ctrl(src.device.dev_handle);
                break;
            }

        case LINK_RTSP_RTP_STD:
            {
                ret = media_device::rtsp_pause_ctrl(src.device.dev_handle);
                break;
            }
        default:
            {
                break;
            }
        }


    } while (false);

    return ret;

}

void XTEngine::rtcp_force_iframe_cb (const int srcno)
{
    dev_handle_t link_handle = instance()->get_dev_link_handle_src(srcno);
    if (link_handle<0)
    {
        return;
    }

    long ret_code =  media_device::request_iframe(link_handle);
    DEBUG_LOG(DBLOGINFO,ll_info,"rtcp_force_iframe_cb:media_device::request_iframe ret_code[%d] srcno[%d]",ret_code,srcno);
}

//模拟DRV模式
long XTEngine::copy_send(long strmid, unsigned char* data, unsigned int len,unsigned int frame_type,int data_type, unsigned int time_stamp)
{
    int media_type = MEDIA_TYPE_NA;
    _()->get_media_type(media_type,frame_type);

    std::vector<src_info> lst_src;
    _()->get_src_all(lst_src);
    for (std::vector<src_info>::iterator itr = lst_src.begin();itr != lst_src.end();++itr)
    {
        src_info &src = *itr;
        _()->upate_frame_state(src.srcno);
        if (KEY_FRAME_TYPE==frame_type && len<MAX_KEY_SIZE)(void)_()->update_sdp(src.srcno,(char*)data,len,data_type);
        int trackid = _()->get_trackid(src.srcno,media_type);
        media_server::send_frame_stamp(src.srcno, trackid, (char*)data, len, frame_type, data_type, time_stamp);
    }
    return 0;
}

boost::threadpool::pool* XTEngine::m_tp = new boost::threadpool::pool(1);
void XTEngine::post_one_frame(int strmid,
							  int frame_type,
							  int media_type,
							  int data_type, 
							  void *data,
							  int len)
{
	rtp_block *rtp2 = (rtp_block *)(data);

	std::vector<src_info> srcs;
    _()->get_src_strmid(strmid, srcs);//for loop 512x(512+1)/2=13w 次
    for(std::vector<src_info>::iterator itr = srcs.begin();srcs.end() != itr; ++itr)
    {
        if (itr->srcno<0 || !itr->active ) continue;
        _()->upate_frame_state(itr->srcno);

		//当私点播私有流时 有可能过来的是音视频流 但转发源只接收音频
		if (DEV_SIP == itr->device.db_type && CODEC_VIDEO == itr->device.dev_strmtype && MEDIA_TYPE_VIDEO == media_type) continue;

		if (KEY_FRAME_TYPE==frame_type && len < MAX_KEY_SIZE)(void)_()->update_sdp(itr->srcno,(char*)data,len,data_type);
        int trackid = _()->get_trackid(itr->srcno,media_type);
		long ret_code = media_server::send_rtp_stamp(itr->srcno, trackid, (char*)rtp2, 0, frame_type, data_type, 0,false,0);
        if (ret_code < 0)
		{
            //DEBUG_LOG(DBLOGINFO,ll_error,"send_data_stamp fail! srcno[%d] dev_handle[%d] frame_type[%d] trackid[%d]",itr->srcno,handle,frame_type,trackid);
            continue;
        }
    }

	rtp2->release();
}

inner::rtp_pool XTEngine::m_rtp_pool;
//数据抛出回调函数
long XTEngine::data_out_cb(long handle,unsigned char* data,long len, long frame_type,long data_type,void* user_data,long time_stamp, unsigned long nSSRC)

{
    int ret_code =0;
    do 
    {
        long strmid = (long)user_data;

        //断线监测
        break_monitor_mgr::_()->update_stream_state(strmid);

        //数据检测
        if (!data || MAX_FRAME_SIZE < len)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"data_out_cb data size error dev_handle[%d] frame_type[%d]",handle,frame_type);
            break;
        }

        int media_type = MEDIA_TYPE_NA;
        _()->get_media_type(media_type,frame_type);
        if (MEDIA_TYPE_NA==media_type && KEY_FRAME_TYPE!=frame_type)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"data_out_cb media data is na dev_handle[%d] frame_type[%d]",handle,frame_type);
            break;
        }
		/*rtp_block *block = (rtp_block *)(data);
		rtp_block *rtp2 = m_rtp_pool.force_alloc_any();
		if (rtp2)
		{
			rtp2->m_bFrameInfo = false;
			rtp2->copy(block);
			rtp2->set_rtp_param(&(block->m_rtp_param));
		}

		rtp2->assign();
		m_tp->schedule( boost::bind(post_one_frame, strmid,frame_type,media_type,data_type,rtp2,len));
        */
		std::vector<src_info> srcs;
        _()->get_src_strmid(strmid, srcs);//for loop 512x(512+1)/2=13w 次
        for(std::vector<src_info>::iterator itr = srcs.begin();srcs.end() != itr; ++itr)
        {
            if (itr->srcno<0 || !itr->active ) continue;
            _()->upate_frame_state(itr->srcno);

            //当私点播私有流时 有可能过来的是音视频流 但转发源只接收音频
            if (DEV_SIP == itr->device.db_type && CODEC_VIDEO == itr->device.dev_strmtype && MEDIA_TYPE_VIDEO == media_type) continue;

            if (KEY_FRAME_TYPE==frame_type && len < MAX_KEY_SIZE)(void)_()->update_sdp(itr->srcno,(char*)data,len,data_type);

            int trackid = _()->get_trackid(itr->srcno,media_type);
			ret_code = media_server::send_rtp_stamp(itr->srcno, trackid, (char*)data, len, frame_type, data_type, time_stamp,false,0);
            if (ret_code < 0)
			{
                DEBUG_LOG(DBLOGINFO,ll_error,"send_data_stamp fail! srcno[%d] dev_handle[%d] frame_type[%d] trackid[%d]",itr->srcno,handle,frame_type,trackid);
                continue;
            }
        }
    } while (false);
    return ret_code;
}
////////////////////////////////////////////////////////////////////////////////////

void XTEngine::get_media_type(int& media_type,const unsigned int frame_type)
{
    media_type = MEDIA_TYPE_NA;
    switch (frame_type)
    {
    case OV_VIDEO_I:
    case OV_VIDEO_P:
    case OV_VIDEO_B:
    case OV_H264:
    case OV_H264_I:
    case OV_H264_P:
    case OV_H264_B:
    case OV_H264_SEI:
    case OV_H264_SPS:
    case OV_H264_PPS:
    case OV_H265:
        {
            media_type = MEDIA_TYPE_VIDEO;
            break;
        }
    case OV_AUDIO:
    case OV_G711:
    case OV_AAC:
        {
            media_type = MEDIA_TYPE_AUDIO;
            break;
        }
    default:
        break;
    }

}

int XTEngine::start(unsigned long num_chan, const std::string& ip,unsigned short start_port,bool multiplex,const std::string& mul_start_ip, unsigned short msg_liten_port,
                    unsigned short rtsp_listen_port, unsigned short tcp_listen_port,unsigned short udp_listen_port,unsigned short regist_listen_port,bool snd_std_rtp)
{
    // read config
    std::string val = config::instance()->xt_dbtype("9|174|3");
    set_xttype(val.c_str());

    m_nCopy = config::instance()->copy_send(-1); 

    // init media server
    std::cout<<"init mediaserver..."<<std::endl;

    m_msCfg;
    ::memset(&m_msCfg, 0, sizeof(MS_CFG));
    m_msCfg.num_chan = num_chan;
    ::memcpy(m_msCfg.ip, ip.c_str(), ip.length());
    m_msCfg.snd_start_port = start_port;
    m_msCfg.demux = multiplex;
    ::memcpy(m_msCfg.mul_start_ip, mul_start_ip.c_str(), mul_start_ip.length());
    m_msCfg.msg_liten_port = msg_liten_port;
    m_msCfg.rtsp_listen_port = rtsp_listen_port;
    m_msCfg.tcp_listen_port = tcp_listen_port;
    m_msCfg.snd_std_rtp = snd_std_rtp;
    m_msCfg.sink_single = false;
    m_msCfg.udp_listen_port = udp_listen_port;

    // one2one transmit
    int sink = config::instance()->sink_perch(0);
    if (sink == 1)
    {
        m_msCfg.sink_single = true;
    }

    // strm control callback
    m_msCfg.rtsp_play_cb = ctrl_rtsp_play_cb;
    m_msCfg.rtsp_pause_cb = ctrl_rtsp_pause_cb;
    m_msCfg.tcp_play_cb = ctrl_tcp_play_cb;
    m_msCfg.tcp_pause_cb = ctrl_tcp_pause_cb;
    m_msCfg.xt_media_server_log_cb = media_server_log_cb;
    m_msCfg.rtcp_force_iframe_cb = rtcp_force_iframe_cb;

    // create strmids
    init_strmid(num_chan);

    // create srcs
    init_src(num_chan);

    int ret = 0;
    ret = media_server::init(m_msCfg);

    return ret;
}

int XTEngine::stop()
{
    uninit_strmid();

    uninit_src();

    return media_server::uninit();
}

void XTEngine::set_xttype(const std::string& types)
{
    std::string xttypes = types;

    std::string::size_type offset = 0;
    while (true)
    {
        std::string::size_type offset2 = xttypes.find('|', offset);
        if (std::string::npos == offset2)
        {
            std::string type = xttypes.substr(offset);
            long nT = atol(type.c_str());
            m_xtdbtypes[nT] = nT;

            break;
        }

        std::string type = xttypes.substr(offset, (offset2-offset));
        long nT = atol(type.c_str());
        m_xtdbtypes[nT] = nT;

        offset = offset2+1;
    }	
}

bool XTEngine::is_xttype(long type)
{
    if (m_xtdbtypes.find(type) != m_xtdbtypes.end())
    {
        return true;
    }

    return false;
}

int XTEngine::is_exist_src( const std::string &ids,const int dev_chid,const int dev_strmtype )
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].active
            && m_srcs[u].device.dev_ids == ids
            &&m_srcs[u].device.dev_chanid ==dev_chid
            &&m_srcs[u].device.dev_strmtype == dev_strmtype)
        {
            return u;
        }
    }

    return -1;
}

//获得登陆信息
int  XTEngine::get_login_info(const std::string &dev_ids, long db_type, std::string &name, std::string &pwd, long &port)
{
    //公司设备不用读取
    if (CXTRouter::_()->use_jk() && !is_xttype(db_type))
    {
        std::string res1;
        long sub_type = 0;
        if (!pri_jk_engine::_()->sync_get_logininfo_from_center(dev_ids, name, pwd, port, res1, sub_type))
        {
            WRITE_LOG(DBLOGINFO,ll_error,"get_login_info:fail dev_ids[%s] db_type[%d]",dev_ids.c_str(),db_type);

            return -1;
        }
    }

    WRITE_LOG(DBLOGINFO,ll_error,"get_login_info: dev_ids[%s] db_type[%d] name[%s] pwd[%s] port[%d]",dev_ids.c_str(),db_type,name.c_str(),pwd.c_str(),port);

    return 0;
}

int XTEngine::start_play_v1(const std::string& on_db_sn,const std::string& dev_ids,const long dev_chanid,
                            const long dev_strmtype,const std::string& db_url,const long db_chanid,const long db_type,
                            const long transmit_ch,long link_type,const std::string& login_name /*= "admin"*/,const std::string& login_password /*="12345"*/,const long login_port /*= -1*/)
{
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);
    WRITE_LOG(DBLOGINFO,ll_info,
        "start_play_v1:dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]db_url[%s]db_chanid[%d]db_type[%d]transmit_ch[%d]link_type[%d]on_db_sn[%s] login_name[%s]login_password[%s]login_port[%d]",
        dev_ids.c_str(),dev_chanid,dev_strmtype,db_url.c_str(),db_chanid,db_type,transmit_ch,link_type,on_db_sn.c_str(),login_name.c_str(),login_password.c_str(),login_port);

    int iRet = -1;
    do
    {
        if (is_exist_src(dev_ids, dev_chanid, dev_strmtype) != -1)
        {
            WRITE_LOG(DBLOGINFO,ll_info,"existed play response center play fail|:dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]", dev_ids.c_str(),dev_chanid,dev_strmtype);
            iRet = -1;
            break;
        }

        //CCS 由中心控制点播方式
        if (!CXTRouter::_()->use_ccs())
        {
            //特殊设备类型处理
            if (is_xttype(db_type))
            {
                int xt_linktype = config::_()->get_link_type(3);
                link_type = xt_linktype;
            }
            //手机使用udp + rtp私有
            if (313 == db_type)
            {
                link_type = 13;
            }
        }

        //反向注册
        ///////////////////////////////////////////////////////////////////////////////
        bool regist = false;
        xt_regist_server::xt_regist_client_t regist_client; 
        iRet = xt_regist_server::instance()->get_regist_client_by_ids(dev_ids.c_str(), &regist_client);
        if (iRet != 1)
        {
            iRet = xt_regist_server::instance()->get_regist_client_by_ip(db_url.c_str(), &regist_client);
        }

        if (iRet == 1)
        {
            regist = true;
        }
        //////////////////////////////////////////////////////////////////////////

        device_info device;
        device.dev_handle = -1;
        device.db_type = db_type;
        device.dev_ids = dev_ids;
        device.dev_chanid = dev_chanid;
        device.dev_strmtype = dev_strmtype;
        device.db_url = db_url;
        device.db_chanid = db_chanid;
        device.strmid = -1;
        device.tracknum = 0;
        device.db_port = login_port;
        device.link_type = link_type;
        device.transmit_ch = transmit_ch;

        if (!regist)
        {
            iRet = start_capture_v1(device.dev_handle, device.strmid, dev_ids, dev_chanid,
                dev_strmtype, db_type, db_url, db_chanid, link_type, login_port, login_name, login_password);
        }
        else
        {
            iRet = start_capture_v1(device.dev_handle, device.strmid, regist_client.ids, dev_chanid, 
                dev_strmtype, db_type, regist_client.ip, db_chanid,link_type, regist_client.port, login_name, login_password);
        }
        if (iRet < 0)
        {
            break;
        }
        else
        {
            create_transmit_src_task* ptask = new create_transmit_src_task(on_db_sn,device);
            if (NULL != ptask)
            {
                ptask->process_event();
            }
        }

        iRet = 1;
    } while (0);

    DEBUG_LOG(DBLOGINFO,ll_info,
        "start_play:ret[%d] dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]db_url[%s]db_chanid[%d]db_type[%d]transmit_ch[%d]link_type[%d]on_db_sn[%s] login_name[%s]login_password[%s]login_port[%d]",
        iRet,dev_ids.c_str(),dev_chanid,dev_strmtype,db_url.c_str(),db_chanid,db_type,transmit_ch,link_type,on_db_sn.c_str(),login_name.c_str(),login_password.c_str(),login_port);

    printf("start_play:ret[%d] dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]db_url[%s]db_chanid[%d]db_type[%d]transmit_ch[%d]link_type[%d]on_db_sn[%s] login_name[%s]login_password[%s]login_port[%d]\n",
        iRet,dev_ids.c_str(),dev_chanid,dev_strmtype,db_url.c_str(),db_chanid,db_type,transmit_ch,link_type,on_db_sn.c_str(),login_name.c_str(),login_password.c_str(),login_port);

    return iRet;

}

int XTEngine::start_play(const std::string& on_db_sn,const std::string& dev_ids, long dev_chanid,long dev_strmtype,
						 const std::string &localip,
						 const std::string& db_url,long db_chanid,long db_type, long &transmit_ch,long link_type,
                         const std::string login_name /*= "admin"*/, const std::string login_password /*="12345"*/,const long login_port /*= -1*/)
{
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);
    WRITE_LOG(DBLOGINFO,ll_info,
        "start_play:dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]db_url[%s]db_chanid[%d]db_type[%d]transmit_ch[%d]link_type[%d]on_db_sn[%s] login_name[%s]login_password[%s]login_port[%d]",
        dev_ids.c_str(),dev_chanid,dev_strmtype,db_url.c_str(),db_chanid,db_type,transmit_ch,link_type,on_db_sn.c_str(),login_name.c_str(),login_password.c_str(),login_port);

    int iRet = -1;
    int srcno = -1;
    device_info device;
    do
    {
        if (is_exist_src(dev_ids, dev_chanid, dev_strmtype) != -1)
        {
            WRITE_LOG(DBLOGINFO,ll_info,"existed play response center play fail|:dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]", dev_ids.c_str(),dev_chanid,dev_strmtype);
            iRet = -1;
            break;
        }

        //CCS 由中心控制点播方式
        if (!CXTRouter::_()->use_ccs())
        {
            //特殊设备类型处理
            if (is_xttype(db_type))
            {
                int xt_linktype = config::_()->get_link_type(3);
                link_type = xt_linktype;
            }
            //手机使用udp + rtp私有
            if (DEV_MOBELE_PHONE == db_type)
            {
                link_type = 13;
            }
        }

        //反向注册
        ///////////////////////////////////////////////////////////////////////////////
        bool regist = false;
        xt_regist_server::xt_regist_client_t regist_client = {0};
        iRet = xt_regist_server::instance()->get_regist_client_by_ids(dev_ids.c_str(), &regist_client);
        if (iRet == 1)
        {
            regist = true;
        }
        //////////////////////////////////////////////////////////////////////////
		void *ptr_bc_mp = NULL;
        if (!regist)
        {
            iRet = start_capture(device, dev_ids, dev_chanid, dev_strmtype, db_type, "0.0.0.0",db_url.c_str(),db_chanid,login_port, login_name, login_password, link_type,ptr_bc_mp);
        }
        else
        {
            iRet = start_capture(device, regist_client.ids, dev_chanid, dev_strmtype, db_type, "0.0.0.0",regist_client.ip, db_chanid,regist_client.port, login_name, login_password, link_type);
        }
		//模拟DRV模式
		if (m_nCopy > 1)
		{
			iRet = 0;
			for (int nC = 0;nC < m_nCopy;++nC)
			{
				iRet = create_src(device, srcno, transmit_ch);
				if (iRet < 0)
				{
					continue;
				}
				regist_src(srcno,on_db_sn);
			}
			break;
		}
		else
		{
			iRet = create_src(device, srcno, transmit_ch);
			if (iRet < 0)
			{
				break;
			}
			regist_src(srcno,on_db_sn);
		}
        if (iRet < 0)
        {
            break;
        }

        iRet = 1;
    } while (0);

    //解决中心点播失败停点网关创建转的的Bug
    if (iRet < 0)
    {
        if (!media_device::_()->is_md_handle(device.dev_handle))
        {
            (void)stop_capture(device);
        }
        (void)destroy_src(srcno);
    }

    DEBUG_LOG(DBLOGINFO,ll_info,
        "start_play:ret[%d] dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]db_url[%s]db_chanid[%d]db_type[%d]transmit_ch[%d]link_type[%d]on_db_sn[%s] login_name[%s]login_password[%s]login_port[%d]",
        iRet,dev_ids.c_str(),dev_chanid,dev_strmtype,db_url.c_str(),db_chanid,db_type,transmit_ch,link_type,on_db_sn.c_str(),login_name.c_str(),login_password.c_str(),login_port);

    printf("start_play:ret[%d] dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]db_url[%s]db_chanid[%d]db_type[%d]transmit_ch[%d]link_type[%d]on_db_sn[%s] login_name[%s]login_password[%s]login_port[%d]\n",
        iRet,dev_ids.c_str(),dev_chanid,dev_strmtype,db_url.c_str(),db_chanid,db_type,transmit_ch,link_type,on_db_sn.c_str(),login_name.c_str(),login_password.c_str(),login_port);

    return iRet;
}

int XTEngine::stop_play_trans(const src_info& src)
{
    if (!src.active)
    {
        return 0;
    }

    //查询是否SIP设备发过来的流已点播
    int ret = 0;
    if (!media_device::_()->is_md_handle(src.device.dev_handle))
    {
        ret = stop_capture(src.device);
    }
    ret = destroy_src(src.srcno);
    DEBUG_LOG(DBLOGINFO,ll_info,"stop_play_trans| stop filish! srcno[%d] dev_handle[%d] ret[%d]",src.srcno,src.device.dev_handle,ret);
    return ret;
}

int XTEngine::stop_play_by_srcno(const int srcno)
{
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);
    src_info src;
    int ret = get_src_no(srcno,src);
    if (ret < 0)
    {
        DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::stop_play_by_srcno | get_src_no fail! ret[5] srcno[%d]",ret,srcno);
        return ret;
    }
    return stop_play_trans(src);
}

int XTEngine::stop_play(const std::string& dev_ids, long dev_chanid, long dev_strmtype)
{
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);
    DEBUG_LOG(DBLOGINFO,ll_info,"执行停点 XTEngine::stop_play:dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]",dev_ids.c_str(),dev_chanid,dev_strmtype);
    src_info src;
    int srcno = get_src_ids(dev_ids, dev_chanid, dev_strmtype, src);
    if (srcno < 0)
    {
        DEBUG_LOG(DBLOGINFO,ll_error,"XTEngine::stop_play-ERR:get_src_ids fail! ids[%s] chid[%d] dev_strmtype[%d]",dev_ids.c_str(),dev_chanid, dev_strmtype);
        return 0;
    }

    return stop_play_trans(src);
}

int XTEngine::pro_dev_logout(const std::string& dev_ids)
{
    WRITE_LOG(DBLOGINFO,ll_info,"处理设备下线:dev_ids[%s]",dev_ids.c_str());
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);
    std::vector<src_info> srcs;
    get_src_ids2(dev_ids, srcs);

    for (std::size_t i=0;i < srcs.size();++i)
    {
        src_info &src = srcs[i];
        if (src.device.dev_ids == dev_ids 
            && src.active)
        {
            stop_play_trans(src);
            rtp_close_recv(src.device.dev_handle);

        }
    }
    return 0;

}

int XTEngine::stop_play_ids(const std::string& dev_ids)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"执行由IDS停点:dev_ids[%s]",dev_ids.c_str());
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);

    std::vector<src_info> srcs;
    get_src_ids2(dev_ids, srcs);

    for (std::size_t i=0;i < srcs.size();++i)
    {
        src_info &src = srcs[i];
        if (src.device.dev_ids == dev_ids 
            && src.active)
        {
            stop_play_trans(src);
        }
    }
    return 0;
}

int XTEngine::stop_play_stramid(const long lstramid)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"执行由stramid停点:lstramid[%d]",lstramid);

    std::vector<src_info> srcs;
    get_src_strmid(lstramid, srcs);

    for (std::size_t i=0;i < srcs.size();++i)
    {
        src_info &src = srcs[i];
        stop_play_trans(src);
    }

    return 0;
}

int XTEngine::stop_play_trans_chid(int srcno)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"执行由trans_chid停点:ltranschid[%d]",srcno);
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex); 	

    src_info src;
    int ret = get_src_no(srcno, src);
    if (ret<0 || !src.active)
    {
        return -5;
    }

    return stop_play_trans(src);
}

int XTEngine::stop_play_ip(const std::string& strip)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"执行由ip停点:strip[%d]",strip.c_str());

    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);

    std::vector<src_info> srcs;
    get_src_url(strip, srcs);

    for (std::size_t i=0;i < srcs.size();++i)
    {
        src_info &src = srcs[i];
        stop_play_trans(src);
    }

    return 0;
}
int XTEngine::stop_allplay()
{
    DEBUG_LOG(DBLOGINFO,ll_info,"执行停点所有 XTEngine::stop_allplay start...");
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);

#ifdef USE_INFO_MGR_
    //收集执行点播信息
    info_mgr::INFO_SIGNALINGEXECRESULT info;
    info.m_strTime = info_mgr::GetCurTime();
    info.m_lCtrlID = CT_STOPALLDB;
    info.m_strCtrlName = "执行停点所有点播";
    CInfoMgr::instance()->PostEventInfo(info_mgr::INFO_SIGNALING_EXEC_RESULT_ID,&info);
#endif //#ifdef USE_INFO_MGR_

    std::vector<src_info> srcs;
    get_src_all(srcs);

    for (std::size_t i=0;i < srcs.size();++i)
    {
        src_info &src = srcs[i];
        stop_play_trans(src);
    }

    DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::stop_allplay end...");

    //清理未完成的recv
    std::map<long,session_inf_t> sessions;
    gw_join_sip_session_mgr::_()->get_session_all(sessions);
    for (std::map<long,session_inf_t>::iterator itrs = sessions.begin() ;
        sessions.end() != itrs; ++itrs)
    {
        if (MSG_OPEN_RECV == itrs->second.type)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::stop_allplay clear recv of on srcno handle[%d]",itrs->second.dev_handle);
            rtp_close_recv(itrs->second.dev_handle);
        }
    }

    //清理所有对接sip会话
    DEBUG_LOG(DBLOGINFO,ll_info,"clear_session_all start!");
    gw_join_sip_session_mgr::_()->clear_session_all();
    DEBUG_LOG(DBLOGINFO,ll_info,"clear_session_all end!");

    return 0;
}

int XTEngine::update_ids(const std::string& dev_ids, long dev_chanid,long dev_strmtype,const std::string& dev_ids_new,long dev_chid_new)
{
    DEBUG_LOG(DBLOGINFO,ll_info,
        "XTEngine::update_idsdev_ids[%s]dev_chanid[%d]dev_strmtype[%d]dev_ids_new[%s]dev_chid_new[%d]",
        dev_ids.c_str(),dev_chanid,dev_strmtype,dev_ids_new.c_str(),dev_chid_new);
    int ret_code = update_src(dev_ids, dev_chanid, dev_strmtype, dev_ids_new, dev_chid_new);
    return ret_code;
}

int XTEngine::start_capture_v1(dev_handle_t& out_handle,long& out_strmid,const std::string& dev_ids,const long dev_chid,const long dev_strmtype,const int db_type,
                               const std::string& db_url,const long db_chanid,const int link_type,long port/* = 8000*/,const std::string& login_name /*= ""*/, const std::string& login_pwd/* = ""*/)
{
    int ret_code = -1;
    do
    {

        device_link_t dev_link_inf;
        long find_handle_ret = media_device::_()->find_link(dev_ids,dev_chid,dev_strmtype,dev_link_inf);
        if (find_handle_ret < 0)
        {
            long strmid = -1;
            std::string name(login_name); 
            std::string pwd(login_pwd);
            strmid = get_free_strmid();
            DEBUG_LOG(DBLOGINFO,ll_info,"start_capture_v1| 传统中心ONDB点播 strmid[%d] dev_ids[%s] dev_chid[%d] dev_strmtype[%d] db_type[%d] db_url[%s] db_chanid[%d] port[%d] szUser[%s] szPassword[%s] link_type[%d]",
                strmid, dev_ids.c_str(),dev_chid,dev_strmtype,db_type,db_url.c_str(),db_chanid,port,name.c_str(),pwd.c_str(),link_type);
            if (strmid < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,"start_capture_v1| 分配流ID失败 strmid[%d] dev_ids[%s] dev_chid[%d] dev_strmtype[%d] db_type[%d] db_url[%s] db_chanid[%d] port[%d] szUser[%s] szPassword[%s] link_type[%d]",
                    strmid, dev_ids.c_str(),dev_chid,dev_strmtype,db_type,db_url.c_str(),db_chanid,port,name.c_str(),pwd.c_str(),link_type);
                ret_code =  -1;
                break;
            }
            get_login_info(dev_ids, db_type, name, pwd, port);
            out_handle = media_device::start_capture(db_type, "0.0.0.0",(char*)db_url.c_str(), db_chanid, dev_strmtype, (void*)strmid, data_out_cb, port, (char*)name.c_str(), (char*)pwd.c_str(), link_type);
            if (out_handle < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,"start_capture_v1| fail! handle[%d]dev_ids[%s]dev_chid[%d]dev_strmtype[%d]",out_handle,dev_ids.c_str(),dev_chid,dev_strmtype);
                free_strmid(strmid);
                ret_code = -2;
                break;
            }
            out_strmid = strmid;
            DEBUG_LOG(DBLOGINFO,ll_info,"start_capture_v1| success handle[%d]dev_ids[%s]dev_chid[%d]dev_strmtype[%d]",out_handle,dev_ids.c_str(),dev_chid,dev_strmtype);
        }
        else
        {
            out_handle = dev_link_inf.link;
            out_strmid = dev_link_inf.strmid;
            DEBUG_LOG(DBLOGINFO,ll_info,"start_capture| 接收已由信令控制服务创建 strmid[%d] dev_ids[%s] dev_chid[%d] dev_strmtype[%d] db_type[%d] db_url[%s] db_chanid[%d]",
                out_strmid, dev_ids.c_str(),dev_chid,dev_strmtype,db_type,db_url.c_str(),db_chanid);
        }
        ret_code = 1;
    } while (0);
    return ret_code;
}

int XTEngine::start_capture(device_info &device, const std::string& dev_ids, long dev_chid,
                            long dev_strmtype, int db_type, 
							const std::string &localip,
							const std::string& db_url,
                            long db_chanid,int login_port, const std::string& login_name, const std::string& login_pwd, int link_type,
							void *bc_mp)
{
    int ret_code = -1;
    do 
    {
        long port = login_port;
        long strmid = -1;
        long handle = -1;
        device_link_t dev_link_inf;

        long find_handle_ret = media_device::_()->find_link(dev_ids,dev_chid,dev_strmtype,dev_link_inf);
        if (find_handle_ret < 0)
        {
            std::string name(login_name); 
            std::string pwd(login_pwd);
            strmid = get_free_strmid();
            if (strmid < 0)
            {
                ret_code = -1;
                break;
            }

            get_login_info(dev_ids, db_type, name, pwd, port);
            DEBUG_LOG(DBLOGINFO,ll_info,"start_capture| 传统中心ONDB点播 strmid[%d] dev_ids[%s] dev_chid[%d] dev_strmtype[%d] db_type[%d] db_url[%s] db_chanid[%d] port[%d] szUser[%s] szPassword[%s] link_type[%d]",
                strmid, dev_ids.c_str(),dev_chid,dev_strmtype,db_type,db_url.c_str(),db_chanid,port,name.c_str(),pwd.c_str(),link_type);

            handle = media_device::start_capture(db_type, localip.c_str(),(char*)db_url.c_str(), db_chanid, dev_strmtype, (void*)strmid, data_out_cb, port, (char*)name.c_str(), (char*)pwd.c_str(), link_type);
            if (handle < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_info,"start_capture| fail! handle[%d]dev_ids[%s]dev_chid[%d]dev_strmtype[%d]",handle,dev_ids.c_str(),dev_chid,dev_strmtype);
                free_strmid(strmid);
                ret_code = -2;
                break;
            }
            device.db_type = db_type;
            DEBUG_LOG(DBLOGINFO,ll_info,"start_capture| success handle[%d]dev_ids[%s]dev_chid[%d]dev_strmtype[%d]",handle,dev_ids.c_str(),dev_chid,dev_strmtype);
        }
        else
        {
            device.db_type = DEV_SIP;//SIP点播设备类型统一
            handle = dev_link_inf.link;
            strmid = dev_link_inf.strmid;
            DEBUG_LOG(DBLOGINFO,ll_info,"start_capture| 接收已由信令控制服务创建 strmid[%d] dev_ids[%s] dev_chid[%d] dev_strmtype[%d] db_type[%d] db_url[%s] db_chanid[%d]",
                strmid, dev_ids.c_str(),dev_chid,dev_strmtype,db_type,db_url.c_str(),db_chanid);
        }
        int track_num = 0;
        if (m_msCfg.snd_std_rtp)
        {
            char key[MAX_KEY_SIZE] = {0};
            long len_key = MAX_KEY_SIZE;
            long data_type = -1;
            media_device::_()->get_sdp_by_handle(handle, key, len_key,data_type);
            //点播纯音频
            if (2 == dev_strmtype)
            {
                std::string sdp;
                del_m_of_sdp(key,len_key,"video",sdp);
                if (!sdp.empty())
                {
                    ::memset(key,0,len_key);
                    ::strncpy(key,sdp.c_str(),sdp.length());
                }
            }
            track_num = parse_tracks_ex(key,len_key,device.track_infos);
            if (track_num == 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,"start_capture | parse_tracks_ex fail! |dev_ids[%s]dev_chid[%d]dev_strmtype[%d] key[%s]",dev_ids.c_str(),dev_chid,dev_strmtype,key);
            }

            if (track_num < 0)
            {
                DEBUG_LOG(DBLOGINFO, ll_error,"start_capture | parse_tracks_ex fail! | dev_ids[%s]dev_chid[%d]dev_strmtype[%d] track_num[%d] key[%s] len_key[%d]",dev_ids.c_str(),dev_chid,dev_strmtype,track_num,key,len_key);
                track_num = 0;
            }

            if (track_num >= MAX_TRACK)
            {
                WRITE_LOG(DBLOGINFO,ll_error, "start_capture| get sdp fail! |media_device::get_track失败 track_num[%d]!", track_num);
                media_device::stop_capture(handle);
                free_strmid(strmid);
                ret_code = -3;
                break;
            }
        }

        // 设备信息
        device.dev_handle = handle;
        device.dev_ids = dev_ids;
        device.dev_chanid = dev_chid;
        device.dev_strmtype = dev_strmtype;
        device.db_url = db_url;
        device.db_chanid = db_chanid;
        device.strmid = strmid;
        device.tracknum = track_num;
        device.db_port = port;
        device.link_type = link_type;
        ret_code = 0;
    } while (0);

    return ret_code;
}

//add by songlei 20150720
int XTEngine::parse_tracks(const char* sdp,const int sdp_len,track_ctx_t* track_info)
{
    if (track_info == NULL)
    {
        return -1;
    }
    int track_num = 0;
    xt_sdp::parse_buffer_t pb(sdp, sdp_len);
    xt_sdp::sdp_session_t xsdp;
    try
    {
        xsdp.parse(pb);
    }
    catch(...)
    {
        return -2;
    }

    std::map<std::string, std::list<std::string> >::iterator itr_ctrl;
    int ctrl_id=-1;
    bool is_exists_ctrl = false;
    xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
    for(track_num=0; xsdp.media_.end() != itr;++track_num,++itr)
    {
        if (itr->attribute_helper_.exists("control"))
        {
            itr_ctrl = itr->attribute_helper_.attributes_.find("control");
            if (itr->attribute_helper_.attributes_.end() != itr_ctrl)
            {
                if (!itr_ctrl->second.empty())
                {
                    int trackid = find_trackid(itr_ctrl->second.front());
                    if (-1 != trackid)
                    {
                        track_info->trackId = trackid;
                        is_exists_ctrl = true;
                    }
                }
            }
        }
        if ( 0 == itr->name_.compare("video"))
        {
            if (!is_exists_ctrl)
            {
                track_info->trackId = 1;
            }
            track_info->trackType = 0; 
            ::strncpy(track_info->trackname,"video",MAX_TRACKNAME_LEN);
        }
        else if ( 0 == itr->name_.compare("audio"))
        {
            if (!is_exists_ctrl)
            {
                track_info->trackId = 2;
            }

            track_info->trackType = 1;
            ::strncpy(track_info->trackname,"audio",MAX_TRACKNAME_LEN);
        }
        else
        {
            track_info->trackId = -1;
            track_info->trackType = -1;
            ::strncpy(track_info->trackname,itr->name_.c_str(),MAX_TRACKNAME_LEN);
        }

        ++track_info;
    }

    return track_num;
}

void XTEngine::del_m_of_sdp(const char* in_sdp,const int in_sdp_len,const std::string& m_name,std::string& out_sdp)
{
    do
    {
        out_sdp.clear();
        xt_sdp::parse_buffer_t pb(in_sdp,in_sdp_len);
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            break;
        }
        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for(; xsdp.media_.end() != itr;++itr)
        {
            if ( m_name == itr->name_)
            {
                itr = xsdp.media_.erase(itr);
                break;
            }
        }
        try
        {
            std::ostringstream oss;
            xsdp.encode(oss);
            out_sdp = oss.str();
        }
        catch(...)
        {
            break;
        }
    } while (0);
}

int XTEngine::parse_tracks_ex(const char* sdp,const int sdp_len,std::vector<track_ctx_t>& track_infos)
{
    track_infos.clear();
    xt_sdp::parse_buffer_t pb(sdp, sdp_len);
    xt_sdp::sdp_session_t xsdp;
    try
    {
        xsdp.parse(pb);
    }
    catch(...)
    {
        return -1;
    }

    std::map<std::string, std::list<std::string> >::iterator itr_ctrl;
    int ctrl_id=-1;
    bool is_exists_ctrl = false;
    track_ctx_t track_info;
    xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
    for(; xsdp.media_.end() != itr;++itr)
    {
        if (itr->attribute_helper_.exists("control"))
        {
            itr_ctrl = itr->attribute_helper_.attributes_.find("control");
            if (itr->attribute_helper_.attributes_.end() != itr_ctrl)
            {
                if (!itr_ctrl->second.empty())
                {
                    int trackid = find_trackid(itr_ctrl->second.front());
                    if (-1 != trackid)
                    {
                        track_info.trackId = trackid;
                        is_exists_ctrl = true;
                    }
                }
            }
        }
        if ( 0 == itr->name_.compare("video"))
        {
            if (!is_exists_ctrl)
            {
                track_info.trackId = 1;
            }
            track_info.trackType = 0; 
            ::strncpy(track_info.trackname,"video",MAX_TRACKNAME_LEN);
        }
        else if ( 0 == itr->name_.compare("audio"))
        {
            if (!is_exists_ctrl)
            {
                track_info.trackId = 2;
            }

            track_info.trackType = 1;
            ::strncpy(track_info.trackname,"audio",MAX_TRACKNAME_LEN);
        }
        else
        {
            track_info.trackId = -1;
            track_info.trackType = -1;
            ::strncpy(track_info.trackname,itr->name_.c_str(),MAX_TRACKNAME_LEN);
        }

        track_infos.push_back(track_info);
    }

    return track_infos.size();

}

void XTEngine::construct_video_for_sdp_default(xt_sdp::sdp_session_t::medium_t& m)
{
    m.name_ = "video";
    xt_sdp::sdp_session_t::codec_t  h264("H264", 96, 90000); 
    m.codecs_.push_back(h264);
    m.attribute_helper_.add_attribute("fmtp","96 profile-level-id=640020;packetization-mode=1;max-mbps=243000;max-fs=8100");

    xt_sdp::sdp_session_t::codec_t  h265("H265", 97, 90000);
    m.codecs_.push_back(h265);
    m.attribute_helper_.add_attribute("fmtp","97 profile-level-id=640020;packetization-mode=1;max-mbps=243000;max-fs=8100");
}
void XTEngine::construct_audio_for_sdp_default(xt_sdp::sdp_session_t::medium_t& m)
{
    m.name_ = "audio";
    //pcmu 0
    xt_sdp::sdp_session_t::codec_t g711_pcmu_8k_1("PCMU",0,8000);
    m.codecs_.push_back(g711_pcmu_8k_1);
    xt_sdp::sdp_session_t::codec_t g711_pcmu_8k_2("PCMU",0,8000,"","2");
    m.codecs_.push_back(g711_pcmu_8k_2);
    xt_sdp::sdp_session_t::codec_t g711_pcmu_16k_1("PCMU",0,16000);
    m.codecs_.push_back(g711_pcmu_16k_1);
    xt_sdp::sdp_session_t::codec_t g711_pcmu_16k_2("PCMU",0,16000,"","2");
    m.codecs_.push_back(g711_pcmu_16k_2);
    xt_sdp::sdp_session_t::codec_t g711_pcmu_32k_1("PCMU",0,32000);
    m.codecs_.push_back(g711_pcmu_32k_1);
    xt_sdp::sdp_session_t::codec_t g711_pcmu_32k_2("PCMU",0,32000,"","2");
    m.codecs_.push_back(g711_pcmu_32k_2);
    xt_sdp::sdp_session_t::codec_t g711_pcmu_44_1k_1("PCMU",0,44100);
    m.codecs_.push_back(g711_pcmu_44_1k_1);
    xt_sdp::sdp_session_t::codec_t g711_pcmu_44_1k_2("PCMU",0,44100,"","2");
    m.codecs_.push_back(g711_pcmu_44_1k_2);

    //puma 规定是8
    xt_sdp::sdp_session_t::codec_t g711_pcma_8k_1("PCMA",8,8000);
    m.codecs_.push_back(g711_pcma_8k_1);
    xt_sdp::sdp_session_t::codec_t g711_pcma_8k_2("PCMA",8,8000,"","2");
    m.codecs_.push_back(g711_pcma_8k_2);
    xt_sdp::sdp_session_t::codec_t g711_pcma_16k_1("PCMA",8,16000);
    m.codecs_.push_back(g711_pcma_16k_1);
    xt_sdp::sdp_session_t::codec_t g711_pcma_16k_2("PCMA",8,16000,"","2");
    m.codecs_.push_back(g711_pcma_16k_2);
    xt_sdp::sdp_session_t::codec_t g711_pcma_32k_1("PCMA",8,32000);
    m.codecs_.push_back(g711_pcma_32k_1);
    xt_sdp::sdp_session_t::codec_t g711_pcma_32k_2("PCMA",8,32000,"","2");
    m.codecs_.push_back(g711_pcma_32k_2);
    xt_sdp::sdp_session_t::codec_t g711_pcma_44_1k_1("PCMA",8,44100);
    m.codecs_.push_back(g711_pcma_44_1k_1);
    xt_sdp::sdp_session_t::codec_t g711_pcma_44_1k_2("PCMA",8,44100,"","2");
    m.codecs_.push_back(g711_pcma_44_1k_2);

    //aac 96以上  rfc1890
    xt_sdp::sdp_session_t::codec_t  aac_8k_1("MPEG4-GENERIC", 98, 8000);
    m.codecs_.push_back(aac_8k_1);
    xt_sdp::sdp_session_t::codec_t  aac_8k_2("MPEG4-GENERIC", 98, 8000,"","2"); 
    m.codecs_.push_back(aac_8k_2);
    xt_sdp::sdp_session_t::codec_t  aac_16k_1("MPEG4-GENERIC", 98, 16000); 
    m.codecs_.push_back(aac_16k_1);
    xt_sdp::sdp_session_t::codec_t  aac_16k_2("MPEG4-GENERIC", 98, 16000,"","2"); 
    m.codecs_.push_back(aac_16k_2);
    xt_sdp::sdp_session_t::codec_t  aac_32k_1("MPEG4-GENERIC", 98, 32000); 
    m.codecs_.push_back(aac_32k_1);
    xt_sdp::sdp_session_t::codec_t  aac_32k_2("MPEG4-GENERIC", 98, 32000,"","2"); 
    m.codecs_.push_back(aac_32k_2);
    xt_sdp::sdp_session_t::codec_t  aac_44_1k_1("MPEG4-GENERIC", 98, 44100); 
    m.codecs_.push_back(aac_44_1k_1);
    xt_sdp::sdp_session_t::codec_t  aac_44_1k_2("MPEG4-GENERIC", 98, 44100,"","2"); 
    m.codecs_.push_back(aac_44_1k_2);

    //aac_lc rfc 3016
    //a=fmtp:100 profile-level-id=24;object=23;bitrate=64000
    xt_sdp::sdp_session_t::codec_t aac_mpega_1("MP4A-LATM",100,90000);
    m.codecs_.push_back(aac_mpega_1); 
    xt_sdp::sdp_session_t::codec_t aac_mpega_2("MP4A-LATM",100,90000,"","2");
    m.codecs_.push_back(aac_mpega_2);

}

long XTEngine::rtp_get_rcv_inf(dev_handle_t link_handle, _RCVINFO *infos, int &num)
{
    return media_device::rtp_get_rcvinfo(link_handle,infos,num);
}

int XTEngine::stop_capture(const device_info &device)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::stop_capture | ids[%s] dev_ch[%d] dev_streamtype[%d] handle[%d]",
        device.dev_ids.c_str(),device.dev_chanid,device.dev_strmtype,device.dev_handle);
    int ret = 0;
    if (device.dev_handle >= 0)
    {
        ret = media_device::stop_capture(device.dev_handle);
    }
    if (0 <= ret)
    {
        free_strmid(device.strmid);
    }
    return ret;
}
int XTEngine::create_src_v1(device_info &device,int &srcno)
{
    return create_src(device,srcno,device.transmit_ch);
}
int XTEngine::create_src(device_info &device,int &srcno,long chanid)
{
    int ret_code = 0;
    do 
    {
        //转发通道被占用则返回 失败!
        if (get_src_status(chanid))
        {
            ret_code = -1;
            break;
        }

        src_info src;
        src.create_time = info_mgr::GetCurTimeMicrosecValue();
        src.device = device;
        src.regist = false;
        int trackids[MAX_TRACK];
        char* tracknames[MAX_TRACK];

        int tindex = 0;
        device_info::track_info_container_handle_t itr = device.track_infos.begin();
        for (; device.track_infos.end() != itr; ++itr)
        {
            trackids[tindex] = itr->trackId;
            tracknames[tindex] = itr->trackname;
            ++tindex;
        }
        int ret = media_server::create_src(device.tracknum, trackids,tracknames,srcno, chanid);
        if (ret < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"create_src fail:srcno[%d]ids[%s]ip[%s]ret[%d]",srcno,device.dev_ids.c_str(),device.db_url.c_str(),ret);
            ret_code = -1;
            break;
        }

        break_monitor_mgr::_()->update_stream_state(device.strmid);

        src.srcno = srcno;
        ret = active_src(srcno, src);
        if (ret < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"active_src fail:srcno[%d]ids[%s]ip[%s] ret[%d]",srcno,device.dev_ids.c_str(),device.db_url.c_str(),ret); 
            ret_code = -1;
            break;
        }
        DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::create_src ids[%s]chanid[%d] srcno[%d]tracknum[%d] end",device.dev_ids.c_str(),chanid,srcno,device.tracknum);

    } while (0);
    return ret_code;
}

int XTEngine::destroy_src(int srcno)
{
    if (is_active_src(srcno))
    {
        DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::destroy_src |srcno[%d]",srcno);
        inactive_src(srcno);
        int ret = media_server::destroy_src(srcno);
        if (ret < 0)
        {
            return -1;
        }
        free_src(srcno);
    }
    return 0;
}

int XTEngine::destroy_src(const std::string& dev_ids,const long dev_chanid,const long dev_strmtype)
{
    src_info src;
    int srcno = get_src_ids(dev_ids,dev_chanid,dev_strmtype,src);
    if (srcno < 0)
    {
        return 0;
    }

    return destroy_src(srcno);
}

int XTEngine::regist_src(int srcno,const std::string& ondb_sn)
{
    std::cout<<"center response"<<std::endl;
    src_info s;
    get_src_no(srcno,s);
    long chanid = -1;
    int ret = get_main_chanid(srcno, chanid);
    if (ret == 0)
    {
        s.regist = true;
        DEBUG_LOG(DBLOGINFO,ll_info,
            "向中心反馈开始:ondb_sn[%s] dev_ids[%s] dev_chanid[%d] dev_strmtype[%d] srcno[%d] chanid[%d] db_url[%s] device_db_chanid[%d] link_type[%d] dev_handle[%d]",
            ondb_sn.c_str(),s.device.dev_ids.c_str(),s.device.dev_chanid,s.device.dev_strmtype,srcno,chanid,s.device.db_url.c_str(),s.device.db_chanid,s.device.link_type,s.device.dev_handle);

        router_task_request_mgr::response_to_center(ondb_sn,s.device.dev_handle, srcno, s.device.dev_ids, s.device.dev_chanid,
            s.device.dev_strmtype, chanid, s.device.db_url, s.device.db_chanid, s.device.db_type, s.device.link_type, 9);
    }

    return ret;
}


int XTEngine::get_chanid(int srcno, int trackid, long &chanid)
{
    return media_server::get_chanid(srcno, trackid, chanid);
}

//获取转发服务通道
int XTEngine::get_chanid_by_srcno(std::list<long>& lst_chanid,const src_info& src)
{
    int i_ret_code = 0;
    lst_chanid.clear();
    int itrack = 0;
    long l_chanid = -1;
    for (; itrack < src.device.tracknum; ++itrack)
    {
        i_ret_code = get_chanid(src.srcno,itrack,l_chanid);
        if (i_ret_code < 0)
        {
            continue;
        }
        lst_chanid.push_back(l_chanid);
    }

    return i_ret_code;
}

unsigned long XTEngine::get_max_trans_ch()
{
    return m_msCfg.num_chan;
}

//内部管理部分
//////////////////////////////////////////////////////////////////////////
long XTEngine::init_strmid(long num)
{
    boost::unique_lock<boost::shared_mutex> lock(strmid_mutex_);
    if (!m_strmids)
    {
        m_strmids = new strmid_info[num];
    }

    if (!m_strmids)
    {
        return -1;
    }

    for (long u = 0;u < num;++u)
    {
        m_strmids[u].strmid = u;
        m_strmids[u].use = false;
    }

    return 0;
}
long XTEngine::uninit_strmid()
{
    boost::unique_lock<boost::shared_mutex> lock(strmid_mutex_);
    if (!m_strmids)
    {
        delete[] m_strmids;
        m_strmids = NULL;
    }
    return 0;
}
long XTEngine::get_free_strmid()
{
    boost::unique_lock<boost::shared_mutex> lock(strmid_mutex_);
    long strmid = -1;
    for (long u = 0;u < m_msCfg.num_chan ;++u)
    {
        //检验是否已分配
        if (!m_strmids[u].use)
        {
            m_strmids[u].use = true;
            strmid = m_strmids[u].strmid;
            break;
        }
    }
    return strmid;
}
void XTEngine::free_strmid(long userid)
{
    boost::unique_lock<boost::shared_mutex> lock(strmid_mutex_);
    for (long u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_strmids[u].strmid == userid)
        {
            m_strmids[u].use = false;
            break;
        }
    }
}

int XTEngine::init_src(int num)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    if (!m_srcs)
    {
        m_srcs = new src_info[num];
        DEBUG_LOG(NULL,ll_info,"XTEngine::init_src m_srcs[%p] num[%d]\n",m_srcs,num);
    }

    if (!m_srcs)
    {
        return -1;
    }

    return 0;
}
int XTEngine::uninit_src()
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    if (m_srcs)
    {
        delete[] m_srcs;
        m_srcs = NULL;
    }

    return 0;
}
int XTEngine::upate_src(const int srcno,const src_info& new_src)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return -1;
    }
    m_srcs[srcno] = new_src;

    return 0;
}

int XTEngine::upate_src(const src_info &new_src_info)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    if (new_src_info.srcno<0 || new_src_info.srcno >= m_msCfg.num_chan)
    {
        return -1;
    }
    m_srcs[new_src_info.srcno] = new_src_info;
    return 0;
}

int XTEngine::update_strmid_of_src(const int srcno,const long strmid)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return -1;
    }
    m_srcs[srcno].device.strmid = strmid;
    return 0;
}
int XTEngine::update_dev_handle_of_src(const int srcno,dev_handle_t handle)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return -1;
    }

    m_srcs[srcno].device.dev_handle = handle;
    return 0;
}

int XTEngine::update_access_info_of_src(const int srcno,const long strmid,const dev_handle_t h)
{
    int ret_code = 0;
    do 
    {
        ret_code = update_strmid_of_src(srcno,strmid);
        if (ret_code < 0)
        {
            break;
        }

        ret_code = update_dev_handle_of_src(srcno,h);
        if (ret_code < 0)
        {
            break;
        }

        ret_code = 1;
    } while (0);
    return ret_code;
}
bool XTEngine::get_src_status(const int srcno)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return false;
    }
    return m_srcs[srcno].active;
}
int XTEngine::active_src(int srcno, const src_info &info,const bool active_state/*=true*/)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return -1;
    }

    m_srcs[srcno] = info;
    m_srcs[srcno].active = active_state;

    return 0;
}

int XTEngine::update_src(const std::string& dev_ids, long dev_chanid, long dev_strmtype, const std::string& dev_ids_new, long dev_chid_new)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].device.dev_ids == dev_ids&&
            m_srcs[u].device.dev_chanid == dev_chanid&&
            m_srcs[u].device.dev_strmtype == dev_strmtype)
        {
            m_srcs[u].device.dev_ids = dev_ids_new;
            m_srcs[u].device.dev_chanid = dev_chid_new;
            return u;
        }
    }

    return 0;
}
int XTEngine::get_src_ids(const std::string& dev_ids, long dev_chanid, long dev_strmtype, src_info &info)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].device.dev_ids == dev_ids&&
            m_srcs[u].device.dev_chanid == dev_chanid&&
            m_srcs[u].device.dev_strmtype == dev_strmtype)
        {
            info = m_srcs[u];
            return info.srcno;
        }
    }

    return -1;
}
int XTEngine::get_src_no(int srcno,src_info &info)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return -1;
    }

    info = m_srcs[srcno];

    return 0;
}
int XTEngine::mod_src_ids(int srcno,std::string ids)
{
	boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

	if (srcno<0 || srcno >= m_msCfg.num_chan)
	{
		return -1;
	}

	m_srcs[srcno].device.dev_ids = ids;

	return 0;
}
int XTEngine::get_src_ids2(const std::string& dev_ids,std::vector<src_info> &srcs)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].device.dev_ids == dev_ids)
        {
            srcs.push_back(m_srcs[u]);
        }
    }

    return 0;
}
int XTEngine::get_src_strmid(long strmid,std::vector<src_info> &srcs)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].device.strmid == strmid && m_srcs[u].active)
        {
            srcs.push_back(m_srcs[u]);
        }
    }

    return 0;
}
int XTEngine::get_src_url(const std::string &db_url,std::vector<src_info> &srcs)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].device.db_url == db_url)
        {
            srcs.push_back(m_srcs[u]);
        }
    }

    return 0;
}
dev_handle_t XTEngine::get_dev_link_handle_src(const int srcno)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return -1;
    }

    return m_srcs[srcno].device.dev_handle;
}
int XTEngine::get_src_all(std::vector<src_info> &srcs)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].active)
        {
            srcs.push_back(m_srcs[u]);
        }
    }

    return 0;
}
int XTEngine::free_src(int srcno)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].srcno == srcno)
        {
            //modify  by songlei 20150626
            m_srcs[u].reset();
            m_srcs[u].device.strmid = -1;
            return 0;
        }
    }

    return -1;
}
int XTEngine::inactive_src(int srcno)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].srcno == srcno)
        {
            m_srcs[u].active = false;
            return 0;
        }
    }

    return -1;
}

bool XTEngine::is_active_src(const int srcno)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].srcno == srcno)
        {
            return m_srcs[u].active;
        }
    }
    //不存在即没激活
    return false;
}

long XTEngine::get_dev_stremtype_by_srcno(const int srcno)
{
    boost::unique_lock<boost::shared_mutex> lock2(m_mSrc);
    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].srcno == srcno)
        {
            return m_srcs[u].device.dev_strmtype;
        }
    }
    return -1;
}

void XTEngine::upate_frame_state(const int srcno)
{
    //boost::shared_lock<boost::shared_mutex> lock(m_mSrc);

    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return;
    }
    if (m_srcs[srcno].active)
    {
        m_srcs[srcno].frames += 1;
    }
}
int XTEngine::update_sdp(const int srcno,char *key, long len,long data_type)
{
    do 
    { 
        //boost::shared_lock<boost::shared_mutex> lock(m_mSrc); 
        if (srcno<0 || srcno >= m_msCfg.num_chan)
        {
            return -1;
        }
        if (m_srcs[srcno].active)
        {
            ::memcpy(m_srcs[srcno].device.key,key,len);
            m_srcs[srcno].device.key_len = len;
        } 
    } while (false);
    return media_server::set_key_data(srcno,key, len, data_type);
}
long  XTEngine::get_sdp_size(const int srcno)
{
    //boost::shared_lock<boost::shared_mutex> lock(m_mSrc);

    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return -1;
    }

    return m_srcs[srcno].device.key_len;
}
long XTEngine::save_sdp_srcno_to_srv_and_access(const int srcno,const long recv_dev_handle,const std::string& sdp)
{
    long ret_code = 0;
    long dev_handle = -1;
    char _sdp[MAX_KEY_SIZE] ={0};
    uint32_t _sdp_len = MAX_KEY_SIZE;
    _sdp_len = sdp.length();
    ::strncpy(_sdp,sdp.c_str(),_sdp_len);
    do 
    {
        if (srcno<0 || srcno >= m_msCfg.num_chan)
        {
            ret_code = -1;
            break;
        }
        DEBUG_LOG(DBLOGINFO,ll_info,"save_sdp_srcno_to_srv_and_access srcno[%d] sdp[%s] _sdp_len[%d]",srcno,_sdp,_sdp_len);
        ret_code = media_server::set_key_data(srcno, _sdp, _sdp_len,172);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"save_sdp_srcno_to_srv_and_access |media_server::set_key_data fail! srcno[%d] sdp[%s] _sdp_len[%d] recv_dev_handle[%d]",srcno,_sdp,_sdp_len,recv_dev_handle);
            ret_code = -2;
            break;
        }

        {
            boost::shared_lock<boost::shared_mutex> lock(m_mSrc);
            ::strncpy(m_srcs[srcno].device.key,_sdp,_sdp_len);
            m_srcs[srcno].device.key_len = _sdp_len;
        }

        ret_code = save_sdp_to_access(recv_dev_handle,_sdp,_sdp_len);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"save_sdp_srcno_to_srv_and_access | save_sdp_to_access fail! srcno[%d] sdp[%s] _sdp_len[%d] recv_dev_handle[%d]",srcno,_sdp,_sdp_len,recv_dev_handle);
            ret_code = -3;
            break;
        }

        ret_code = 0;
    } while (0);

    return ret_code;

}

long XTEngine::save_sdp_to_access(const dev_handle_t recv_dev_handle,const char* recv_sdp,const long sdp_len)
{
    int ret_code = 0;
    do 
    {
        xt_sdp::parse_buffer_t pb(recv_sdp,sdp_len);
        xt_sdp::sdp_session_t xsdp;

        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            ret_code = -1;
            break;
        }

        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for (;xsdp.media_.end() != itr;)
        {
            if (itr->attribute_helper_.exists("inactive") 
                && !itr->attribute_helper_.exists("recvonly"))
            {
                xsdp.media_.erase(itr++);
            }
            else
            {
                ++itr;
            }
        }

        try
        {
            std::ostringstream oss;
            xsdp.encode(oss);
            std::string ret_sdp = oss.str();
            long ret_sdp_len = ret_sdp.length();
            if(media_device::instance()->rtp_save_sdp(recv_dev_handle,ret_sdp.c_str(),ret_sdp_len) < 0)
            {
                ret_code = -2;
                break;
            }
            DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::save_sdp_to_access |recv_dev_handle[%d] ret_code[%d] sdp[%s]",recv_dev_handle,ret_code,ret_sdp.c_str());
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }
    } while (0);
    return ret_code; 
}

long XTEngine::save_sdp_to_srv(const int srcno,const char* sdp,const long sdp_len,const long data_type)
{
    long ret_code = 0;
    do 
    {
        if (srcno<0 || srcno >= m_msCfg.num_chan)
        {
            ret_code = -1;
            break;
        }
        boost::shared_lock<boost::shared_mutex> lock(m_mSrc);
        ::memcpy(m_srcs[srcno].device.key,sdp,sdp_len);
        m_srcs[srcno].device.key_len = sdp_len;
        ret_code = media_server::set_key_data(srcno, m_srcs[srcno].device.key, sdp_len,data_type);
        if (ret_code < 0)
        {
            ret_code = -2;
            break;
        }
        ret_code=1;
    } while (0);
    return ret_code;

}

long XTEngine::save_sdp_to_srv(const int srcno,std::string& recv_sdp)
{
    long ret_code = 0;
    do 
    {
        if (srcno<0 || srcno >= m_msCfg.num_chan)
        {
            ret_code = -1;
            break;
        }
        char sdp[MAX_SDP_LEN] ={0};
        long data_type = -1;
        long sdp_len = recv_sdp.length();
        ::strncpy(sdp,recv_sdp.c_str(),sdp_len);
        {
            boost::shared_lock<boost::shared_mutex> lock(m_mSrc);
            ::memcpy(m_srcs[srcno].device.key,sdp,sdp_len);
            m_srcs[srcno].device.key_len = sdp_len;
        }
        ret_code = media_server::set_key_data(srcno, sdp, sdp_len, 172);
        if (ret_code < 0)
        {
            ret_code = -2;
            break;
        }
        ret_code=1;
    } while (0);
    return ret_code;
}

int XTEngine::get_key(long strmid, char *key, long &len)
{
    boost::shared_lock<boost::shared_mutex> lock(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs[u].device.strmid == strmid)
        {
            len = m_srcs[u].device.key_len;
            ::memcpy(key, m_srcs[u].device.key, len);

            return 0;
        }
    }

    return -1;
}
int XTEngine::get_trackid(const int srcno,const int media_type)
{
    //boost::shared_lock<boost::shared_mutex> lock(m_mSrc);

    if (srcno<0 || srcno >= m_msCfg.num_chan)
    {
        return -1;
    }
    device_info::track_info_container_handle_t itr = m_srcs[srcno].device.track_infos.begin();
    for (;m_srcs[srcno].device.track_infos.end() != itr; ++itr)
    {
        if (itr->trackType == media_type)
        {
            return itr->trackId;
        }
    }
    return -1; 
}

void XTEngine::transform_media_type_to_name(const int media_type,std::string& trackname)const
{
    switch(media_type)
    {
    case MEDIA_TYPE_VIDEO:
        {
            trackname.assign("video");
            break; 
        }

    case MEDIA_TYPE_AUDIO:
        {
            trackname.assign("audio");
            break;
        }
    default:
        {
            break;
        }

    }

}


void XTEngine::get_all_src(std::list<src_info>& lst_src)
{	
    boost::shared_lock<boost::shared_mutex> lock(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if (m_srcs && m_srcs[u].active)
        {
            lst_src.push_back(m_srcs[u]);
        }
    }
}
//Get all src including stopped src by wluo
void XTEngine::get_all_src_1(std::list<src_info>& lst_src)
{	
    boost::shared_lock<boost::shared_mutex> lock(m_mSrc);

    for (int u = 0;u < m_msCfg.num_chan ;++u)
    {
        if ( !m_srcs[u].device.dev_ids.empty())
        {
            lst_src.push_back(m_srcs[u]);
        }
    }
}
//////////////////////////////////////////////////////////////////////////

//SIP部分
//////////////////////////////////////////////////////////////////////////////////////////////////////
dev_handle_t XTEngine::rtp_create_recv(const std::string& dev_ids, long dev_chanid, long dev_strmtype,int track_num, bool demux)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"Create Recv:dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]track_num[%d] demux[%d]",
        dev_ids.c_str(),dev_chanid,dev_strmtype,track_num, demux);
    dev_handle_t link_handle = -1;
    do
    {
        boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);
        long strmid = get_free_strmid();
        if (strmid < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"XTEngine::rtp_create_recv | get_free_strmid fail ids[%s] dev_strmtype[%d]",dev_ids.c_str(),dev_strmtype);
            break;
        }
        link_handle = media_device::instance()->find_link(dev_ids, dev_chanid, dev_strmtype);
        if (link_handle > 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,"接收机制已创建 Recv is Created |return current dev_handle  ids[%s] dev_ch[%d] dev_strmtype[%d] handle[%d]",dev_ids.c_str(),dev_chanid,dev_strmtype,link_handle);
            break;
        }

        link_handle = media_device::rtp_create_recv(track_num, demux);
        if (link_handle < 0)
        {
            return -1;
        }

        media_device::instance()->add_link(link_handle, dev_ids, dev_chanid, dev_strmtype);


        long ret_code = start_link_capture(link_handle, strmid);
        if (ret_code < 0)
        {
            WRITE_LOG(DBLOGINFO,ll_error,"rtp_create_recv|start_link_capture fail! ret_code[%d] handle[%d] strmid[%d]",ret_code,link_handle,strmid);
            rtp_close_recv(link_handle);
            return -2;
        }
        DEBUG_LOG(DBLOGINFO,ll_info,"rtp_create_recv|start_link_capture handle[%d] strmid[%d]",link_handle,strmid);
    } while (false);

    return link_handle;
}

long XTEngine::rtp_close_recv(const dev_handle_t dev_handle)
{
    long ret_code = 0;
    do 
    {
        DEBUG_LOG(DBLOGINFO,ll_info,"rtp_close_recv | dev_handle[%d] ",dev_handle);
        device_link_t dev_link_inf;
        ret_code = media_device::instance()->find_link(dev_handle,dev_link_inf);
        if (ret_code < 0)
        {
            //未找到说明已经被删除
            ret_code = 0;
            break;
        }
        free_strmid(dev_link_inf.strmid);
        ret_code = media_device::instance()->stop_capture(dev_handle);
        if (ret_code < 0)
        {
            ret_code = -2;
            break;
        }
        ret_code = 1;
    } while (0);
    return ret_code;
}
long XTEngine::rtp_close_recv(const std::string& dev_ids, const long dev_chanid, const long dev_strmtype)
{
    long ret_code = 0;
    do 
    {
        DEBUG_LOG(DBLOGINFO,ll_info,"rtp_close_recv| dev_ids[%s] dev_chanid[%d] dev_strmtype[%d]",dev_ids.c_str(),dev_chanid,dev_strmtype);
        device_link_t dev_link_inf;
        ret_code = media_device::instance()->find_link(dev_ids,dev_chanid,dev_strmtype,dev_link_inf);
        if (ret_code < 0)
        {
            //未找到说明已经被删除
            ret_code = 0;
            break;
        }
        free_strmid(dev_link_inf.strmid);
        ret_code = media_device::instance()->stop_capture(dev_link_inf.link);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"rtp_close_recv| stop_capture fail! handle[%d] dev_ids[%s] dev_chanid[%d] dev_strmtype[%d]",
                dev_link_inf.link,dev_ids.c_str(),dev_chanid,dev_strmtype);
            ret_code = -2;
            break;
        }
        ret_code = 1;
    } while (0);
    return ret_code;
}

void XTEngine::clear_recv_link_mgr_buf(const long dev_handle)
{
    media_device::instance()->del_link_by_handle(dev_handle);
}

long XTEngine::clear_recv_link_mgr_buf(const std::string& dev_ids, long dev_chanid, long dev_strmtype)
{
    DEBUG_LOG(DBLOGINFO,ll_info,
        "clear_recv_link_mgr_buf:dev_ids[%s]dev_chanid[%d]dev_strmtype[%d]",dev_ids.c_str(),dev_chanid,dev_strmtype);
    long ret_code = 0;
    do 
    {
        boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);
        long link = media_device::instance()->find_link(dev_ids, dev_chanid, dev_strmtype);
        if (link < 0)
        {
            ret_code = 0;
            break;
        }

        long ret2 = media_device::instance()->del_link(dev_ids, dev_chanid, dev_strmtype);
        if (ret2 < 0)
        {
            ret_code = -2;
            break;
        }

    } while (false);

    return ret_code;
}

int XTEngine::save_sdp_to_access(const std::string& dev_ids,long dev_chanid,long dev_strmtype,const char *sdp,long len_sdp)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"保存SDP:dev_ids[%s] dev_chanid[%d] dev_strmtype[%d] sdp[%s]",dev_ids.c_str(),dev_chanid,dev_strmtype,sdp);
    boost::unique_lock<boost::shared_mutex> lock(m_global_mutex);
    long link = media_device::instance()->find_link(dev_ids, dev_chanid, dev_strmtype);
    if (link < 0)
    {
        return -1;
    }

    long ret = media_device::instance()->rtp_save_sdp(link, sdp, len_sdp);
    if (ret < 0)
    {
        return -2;
    }

    return 0;
}

long XTEngine::start_link_capture(const dev_handle_t link_handle,long strmid)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"Gateway Join sip Start Capture data:link_handle[%d] strmid[%d]",link_handle,strmid);
    return media_device::instance()->start_link_captuer(link_handle,data_out_cb,strmid);
}

//sip对接操作接口
long XTEngine::gw_join_sip_create_rs(const std::string& dev_ids_r,const long dev_chid_r,
                                     const std::string& dev_ids_s,const long dev_chid_s, const long dev_strmtype,std::string& sdp,const bool demux_r/*=false*/)
{
    long ret_code = 0;
    do 
    {
        //打开接收
        std::string sdp_r;
        dev_handle_t dev_handle_r=-1;
        ret_code = sip_create_r(dev_ids_r,dev_chid_r,dev_strmtype,demux_r,sdp_r,dev_handle_r);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_create_rs |ret_code[%d] sip_create_r fail",ret_code);
            break;
        }
        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_create_rs:生成接收sdp[%s]",sdp_r.c_str());

        //打开发送
        std::string sdp_s;
        dev_handle_t dev_handle_s=-1;
        int srcno = -1;
        ret_code = gw_join_sip_create_s(dev_ids_s,dev_chid_s,dev_strmtype,sdp_s,srcno,dev_handle_s);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_create_rs |ret_code[%d] gw_join_sip_create_s fail",ret_code);
            break;
        }
        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_create_rs:生成发送sdp_s[%s]\n",sdp_s.c_str());

        //合并sdp
        size_t len = ::strlen(SDP_TEMPLATE);
        xt_sdp::parse_buffer_t pb(SDP_TEMPLATE, len);
        xt_sdp::parse_buffer_t pb_r(sdp_r.c_str(), sdp_r.length());
        xt_sdp::parse_buffer_t pb_s(sdp_s.c_str(), sdp_s.length());
        xt_sdp::sdp_session_t xsdp;
        xt_sdp::sdp_session_t xsdp_r;
        xt_sdp::sdp_session_t xsdp_s;

        try
        {
            xsdp.parse(pb);
            xsdp_r.parse(pb_r);
            xsdp_s.parse(pb_s);
        }
        catch(...)
        {
            return 0;
        }

        std::string local_ip = CXTRouter::Instance()->get_local_ip();
        xsdp.origin().set_address(local_ip.c_str());

        xsdp.media_.clear();

        xt_sdp::sdp_session_t::medium_container_t::iterator itr;
        for(itr = xsdp_s.media_.begin(); xsdp_s.media_.end() != itr; ++itr)
        {
            xsdp.media_.push_back(*itr);
        }

        for(itr = xsdp_r.media_.begin(); xsdp_r.media_.end() != itr; ++itr)
        {
            xsdp.media_.push_back(*itr);
        }

        try
        {
            std::ostringstream oss;
            xsdp.encode(oss);
            sdp = oss.str();
        }
        catch(...)
        {
            ret_code = -4;
            break;
        }	

    } while (false);

    return ret_code;
}

long XTEngine::gw_join_sip_clear_trans_ch(const long sessionid)
{
    long ret_code = 0;
    do 
    {
        session_inf_t session;
        long ret = gw_join_sip_session_mgr::_()->get_session(sessionid,session);
        if (ret < 0 /*|| MSG_OPEN_SEND == session.type*/)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_close_r|会话已经被清理或会话不存在get_session not find!sessionid[%d],type[%d],ret[%d]",sessionid,session.type,ret);
            ret_code = 0;
            break;
        }

        //接已机制清理后理应清理转发机制
        ret_code = destroy_src(session.send_ids,session.send_chid,session.stremtype);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_close_r | destroy_src fail sessionid[%d] ret_code[%d] recv_ids[%s] recv_chid[%d] stremtype[%d]",
                sessionid,ret_code,session.recv_ids.c_str(),session.recv_chid,session.stremtype);
            ret_code = -2;
            break;
        }
        ret_code = 1;
    } while (false);
    return ret_code;

}
long XTEngine::gw_join_sip_close_r(const long sessionid)
{
    long ret_code = 0;
    do 
    {
        session_inf_t session;
        long ret = gw_join_sip_session_mgr::_()->get_session(sessionid,session);
        if (ret < 0 || MSG_OPEN_SEND == session.type)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_close_r|会话已经被清理或会话不存在get_session not find!sessionid[%d]",sessionid);
            ret_code = 0;
            break;
        }

        //清理接收机制
        ret_code = rtp_close_recv(session.recv_ids,session.recv_chid,session.stremtype);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_close_r | rtp_close_recv fail sessionid[%d] ret_code[%d] recv_ids[%s] recv_chid[%d] stremtype[%d]",
                sessionid,ret_code,session.recv_ids.c_str(),session.recv_chid,session.stremtype);
        }
        ret_code = 1;
    } while (false);
    return ret_code;
}

long XTEngine::sip_create_r(const std::string& dev_ids, const long dev_chanid,const long dev_strmtype,const bool demux,std::string& sdp,dev_handle_t& out_dev_handle)
{
    long ret_code = 0;

    do 
    {
        int track_num = get_tracknum(dev_strmtype);
        if (track_num <= 0)
        {
            ret_code = -1;
            DEBUG_LOG(DBLOGINFO,ll_error,
                "sip_create_r |get_tracknum fail! dev_ids[%s] dev_chanid[%d] dev_strmtype[%d] track_num[%d]",dev_ids.c_str(),dev_chanid,dev_strmtype,track_num);
            break;
        }

        //打开接收端
        out_dev_handle = rtp_create_recv(dev_ids,dev_chanid,dev_strmtype,track_num,demux);
        if (out_dev_handle < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,
                "sip_create_r |create_recv fail link_handle[%d] dev_ids[%s] dev_chanid[%d] dev_strmtype[%d]",out_dev_handle,dev_ids.c_str(),dev_chanid,dev_strmtype);
            ret_code = -2;
            break;
        }

        ret_code = sip_create_sdp_r(out_dev_handle,dev_strmtype,track_num,sdp);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"sip_create_r |create_sdp_r fale ret_code[%d] dev_ids[%s] dev_chanid[%d] dev_strmtype[%d]",ret_code,dev_ids.c_str(),dev_chanid,dev_strmtype);
            break;
        }
        ret_code = 1;
    } while (0);

    return ret_code;
}

//生成接收端sdp
long XTEngine::sip_create_sdp_r(long link_handle,const long dev_strmtype,int& track_num,std::string&sdp)
{
    if (CXTRouter::_()->get_default_sdp().empty())
    {
        return sip_create_sdp_r_impl_v1(link_handle,dev_strmtype,track_num,sdp);
    }
    return sip_create_sdp_r_impl_v2(link_handle,dev_strmtype,track_num,sdp);
}

long XTEngine::sip_create_sdp_r_impl_v1(long link_handle,const long dev_strmtype,int& track_num,std::string&sdp)
{
    long ret_code = 0;
    do 
    {
        long sdp_len = ::strlen(SDP_TEMPLATE);
        xt_sdp::parse_buffer_t pb(SDP_TEMPLATE, sdp_len); 
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }

        //获取接收端信息 生成接收端sdp
        std::string rev_ip = CXTRouter::_()->get_local_ip();
        xsdp.origin().set_address(rev_ip);

        if (xsdp.attribute_helper_.exists("streamid"))
        {
            xsdp.attribute_helper_.clear_attribute("streamid");
        }

        if (0 == dev_strmtype)
        {
            xsdp.attribute_helper_.add_attribute("streamid","0");
        }
        if (1 == dev_strmtype)
        {
            xsdp.attribute_helper_.add_attribute("streamid","1");
        }
        std::string control_vaule;
        xsdp.media_.clear();
        _RCVINFO rcv_inf[MAX_TRACK];
        ret_code = rtp_get_rcv_inf(link_handle,rcv_inf,track_num);
        for (int tracktype=0; tracktype<track_num; ++tracktype)
        {
            xt_sdp::sdp_session_t::medium_t m;
            if (dev_strmtype < 2)
            {
                switch(tracktype)
                {
                case 0:
                    {
                        construct_video_for_sdp_default(m);
                        control_vaule.assign("track1");
                        break;
                    }
                case 1:
                    {
                        construct_audio_for_sdp_default(m);
                        control_vaule.assign("track2");
                        break;
                    }
                default:
                    break;
                }
            }
            else if ( dev_strmtype == 2 && track_num == 1)
            {
                construct_audio_for_sdp_default(m);
                control_vaule.assign("track2");
            }

            //fmtp:
            xt_sdp::sdp_session_t::connection_t cvalue(xt_sdp::ipv4,rev_ip);
            m.connections_.push_back(cvalue);
            m.protocol_ = "RTP/AVP";
            m.port_ = rcv_inf[tracktype].port_rtp; 
            if (rcv_inf[tracktype].demux)
            { 
                if (!xsdp.attribute_helper_.exists("rtpport-mux"))
                {
                    xsdp.add_attribute("rtpport-mux");
                }
                std::ostringstream demuxid;
                demuxid<<rcv_inf[tracktype].demuxid;
                m.add_attribute("muxid",demuxid.str());
            }
            m.add_attribute("control",control_vaule.c_str());
            m.add_attribute("recvonly");
            xsdp.media_.push_back(m);
        }
        try
        {
            std::ostringstream oss;
            xsdp.encode(oss);
            sdp = oss.str();
        }
        catch(...)
        {
            ret_code = -4;
            break;
        }
        ret_code=1;
    } while (0);
    return ret_code;
}
long XTEngine::sip_create_sdp_r_impl_v2(long link_handle,const long dev_strmtype,int& track_num,std::string&sdp)
{
    long ret_code = 0;
    do
    {
        sdp.clear();
        std::string default_sdp = CXTRouter::_()->get_default_sdp();
        if (default_sdp.empty())
        {
            ret_code = -1;
            break;
        }
        xt_sdp::parse_buffer_t pb(default_sdp.c_str(),default_sdp.length());
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            ret_code = -2;
            break;
        }

        //session 部分
        std::string rev_ip = CXTRouter::_()->get_local_ip();
        xsdp.origin().set_address(rev_ip);

        if (xsdp.attribute_helper_.exists("streamid"))
        {
            xsdp.attribute_helper_.clear_attribute("streamid");
        }
        if (0 == dev_strmtype)
        {
            xsdp.attribute_helper_.add_attribute("streamid","0");
        }
        if (1 == dev_strmtype)
        {
            xsdp.attribute_helper_.add_attribute("streamid","1");
        }

        //m部分
        _RCVINFO rcv_inf[MAX_TRACK];        
        ret_code = rtp_get_rcv_inf(link_handle,rcv_inf,track_num);
        if (ret_code < 0 || track_num < 1)
        {
            ret_code = -3;
            break;
        }
        xt_sdp::sdp_session_t::medium_container_t::iterator itrm;
        for (itrm = xsdp.media_.begin();xsdp.media_.end() != itrm;)
        {
            int tracktype=-1;
            if (0 == itrm->name_.compare("video"))
            {
                tracktype = 0;
                if (1 == track_num)
                {//纯音频
                    xsdp.media_.erase(itrm++);
                    continue;
                }
            }
            if (0 == itrm->name_.compare("audio"))
            {
                if (1 == track_num) tracktype = 0;
                else tracktype = 1;
            }
            if (tracktype < 0)
            {
                ret_code = -4;
                break;
            }
            //修改IP 端口 复用参数
            xt_sdp::sdp_session_t::connection_t cvalue(xt_sdp::ipv4,rev_ip);
            itrm->connections_.clear();
            itrm->connections_.push_back(cvalue);
            itrm->port_ = rcv_inf[tracktype].port_rtp; 
            if (rcv_inf[tracktype].demux)
            {
                if (!xsdp.attribute_helper_.exists("rtpport-mux"))
                {
                    xsdp.add_attribute("rtpport-mux");
                }
                std::ostringstream demuxid;
                demuxid<<rcv_inf[tracktype].demuxid;
                itrm->add_attribute("muxid",demuxid.str());
            }
            if (itrm->attribute_helper_.exists("recvonly"))
            {
                itrm->attribute_helper_.clear_attribute("recvonly");
            }
            itrm->add_attribute("recvonly");
            ++itrm;
        }// end for ( itrm = xsdp.media_.begin();xsdp.media_.end() != itrm;)

        if (ret_code < 0) break;

        //生成结果sdp
        try
        {
            std::ostringstream oss;
            xsdp.encode(oss);
            sdp = oss.str();
        }
        catch(...)
        {
            ret_code = -4;
            break;
        }
        ret_code=1;
    } while (0);
    return ret_code;
}

long XTEngine::gw_join_sip_close_s(const long sessionid)
{
    long ret_code = 0;
    do 
    {
        session_inf_t session;
        ret_code = gw_join_sip_session_mgr::_()->get_session(sessionid,session);
        if (ret_code < 0 || MSG_OPEN_RECV == session.type)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_close_s|get_session fail sessionid[%d]",sessionid);
            ret_code = 0;
            break;
        }

        src_info src;
        ret_code = get_src_ids(session.send_ids,session.send_chid,session.stremtype,src);
        if (ret_code < 0 )
        {
            //外部系统点播内部系统ondb提前已清理转发源故不用清理发送
            ret_code = 1;
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_close_s |get_src_ids fail sessionid[%d] ret_code[%d] ids[%s] chid[%d] stremtype[%d]",sessionid,ret_code,session.send_ids.c_str(),session.send_chid,session.stremtype);
            break;
        }

        session_inf_t::rtp_dst_container_handle_t itr = session.send_dsts.begin();
        for (; session.send_dsts.end() != itr; ++itr)
        {
            ret_code = media_server::_()->del_send(src.srcno,itr->trackid,itr->dst_ip.c_str(),itr->dst_port,itr->dst_demux,itr->dst_demuxid);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_close_s |del_send_src fail sessionid[%d] ret_code[%d] dst_ip[%s]",sessionid,ret_code,itr->dst_ip.c_str());
                continue;
            }
        }
    } while (0);

    return ret_code;
}

long XTEngine::gw_join_sip_create_s(const std::string& dev_ids, const long dev_chanid,const long dev_strmtype,std::string& sdp,int&srcno,dev_handle_t& dev_handle)
{
    long ret_code = 0;
    do 
    {
        ret_code = sip_create_sdp_s(dev_ids,dev_chanid,dev_strmtype,sdp,srcno,dev_handle);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"sip_crete_s |sip_create_sdp_s fail ret_code[%d]",ret_code);
            break;
        }

    } while (0);

    return ret_code;
}
long XTEngine::create_sdp_s(const int srcno,std::string &sdp)
{
    long ret_code=0;
    src_info info;
    ret_code = get_src_no(srcno,info);
    if (ret_code > -1)
    {
        dev_handle_t dev_handle = -1; 
        int srcno = -1;
        ret_code = sip_create_sdp_s(info.device.dev_ids,info.device.dev_chanid,info.device.dev_strmtype,sdp,srcno,dev_handle);
    }
    else
    {
        DEBUG_LOG(DBLOGINFO,ll_error,"create_sdp_s |get_src_no fail srcno[%d] ret_code[%d] dev_ids[%s] dev_ch[%d] dev_strmtype[%d]",
            ret_code,srcno,info.device.dev_ids.c_str(),info.device.dev_chanid,info.device.dev_strmtype);
    }

    return ret_code;
}

long XTEngine::sip_create_sdp_s(const int srcno,const long dev_strmtype,const char* in_sdp, const int in_sdp_len,std::string &out_sdp)
{
    return create_sdp_s_impl(srcno,dev_strmtype,in_sdp,in_sdp_len,out_sdp);
}

long XTEngine::sip_create_sdp_s(const std::string& dev_ids, const long dev_chanid,const long dev_strmtype,std::string& sdp,int&srcno,dev_handle_t& dev_handle)
{
    long ret_code = 0;
    do 
    {
        dev_handle = -1;
        src_info src;
        ret_code = get_src_ids(dev_ids,dev_chanid,dev_strmtype,src);
        if (ret_code < 0 )
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"sip_create_sdp_s |get_src_ids fail ret_code[%d] dev_ids[%s] dev_chanid[%d] dev_strmtype[%d]",
                ret_code,dev_ids.c_str(),dev_chanid,dev_strmtype);
            break;
        }
        dev_handle = src.device.dev_handle;

        char src_sdp[MAX_KEY_SIZE]={0};
        long sdp_len = MAX_KEY_SIZE;
        long data_type = -1;
        ret_code = media_device::_()->get_sdp_by_handle(src.device.dev_handle, src_sdp, sdp_len,data_type);
        if (sdp_len <= 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"sip_creare_sdp_s |get_sdp_by_handle fail ret_code[%d] dev_handle[%d] dev_ids[%s] dev_chanid[%d] dev_strmtype[%d]",
                ret_code,src.device.dev_handle,dev_ids.c_str(),dev_chanid,dev_strmtype);
            break;
        }

        //生成交换推送sdp
        ret_code = create_sdp_s_impl(src.srcno,dev_strmtype,src_sdp,sdp_len,sdp);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"sip_creare_sdp_s |create_sdp_s_impl fail ret_code[%d] dev_ids[%s] dev_chanid[%d] dev_strmtype[%d] srcno[%d]",
                ret_code,dev_ids.c_str(),dev_chanid,dev_strmtype,src.srcno);
            break;
        }
        ret_code = 1;
    } while (0);

    return ret_code;
}

long XTEngine::create_sdp_s_impl(const int srcno,const long dev_strmtype,const char* in_sdp, const int in_sdp_len,std::string &out_sdp)
{
    long ret_code = 0;
    do 
    {
        if (0 == std::strlen(in_sdp) || in_sdp_len < 1)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"create_sdp_s_impl insdp is empty! srcno[%d]",srcno);
            ret_code = -1;
            break;
        }
        xt_sdp::parse_buffer_t pb(in_sdp,in_sdp_len); 
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }

        std::string send_ip = CXTRouter::Instance()->get_local_ip();
        xsdp.origin().set_address(send_ip);

        int track_num = get_tracknum(dev_strmtype);
        if (track_num <= 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"sip_create_sdp_s |get_tracknum fail track_num[%d] srcno[%d]",track_num,srcno);
            ret_code = -2;
            break;
        }

        std::string track_name;
        svr_info svr[MAX_TRACK_NUM]={0}; 
        int ret_code = media_server::get_svr_info(svr,track_num,srcno);
        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for (;xsdp.media_.end() != itr; ++itr)
        {

            for (int index=0; index<track_num; ++index)
            {
                if (0 != itr->name_.compare(svr[index].trackname))
                {
                    continue;
                }

                itr->connections_.clear();
                xt_sdp::sdp_session_t::connection_t c(xt_sdp::ipv4,send_ip);
                itr->connections_.push_back(c);

                itr->port_ = svr[index].rtp_send_port;

                if (svr[index].multiplex_s)
                {
                    if (!itr->attribute_helper_.exists("rtpport-mux"))
                    {
                        itr->add_attribute("rtpport-mux");
                    }

                    if (itr->attribute_helper_.exists("muxid"))
                    {
                        itr->clear_attribute("muxid");

                    }

                    std::ostringstream demuxid;
                    demuxid<<svr[index].multid_s;
                    itr->add_attribute("muxid",demuxid.str());
                }

                if (!itr->attribute_helper_.exists("sendonly"))
                {
                    itr->add_attribute("sendonly");
                }

            }
        }

        try
        {
            std::ostringstream oss;
            xsdp.encode(oss);	
            out_sdp = oss.str();
        }
        catch(...)
        {
            ret_code = -4;
            break;
        }
        ret_code=1;
    } while (0);
    return ret_code;
}

int XTEngine::gw_join_sip_save_sdp_to_access(const std::string& dev_ids, long dev_chanid, long dev_strmtype, const char *sdp, long len_sdp)
{
    int ret_code = 0;
    do 
    {
        xt_sdp::parse_buffer_t pb(sdp,len_sdp);
        xt_sdp::sdp_session_t xsdp;

        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            ret_code = -1;
            break;
        }

        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for (;xsdp.media_.end() != itr;)
        {
            if (itr->attribute_helper_.exists("inactive") 
                && !itr->attribute_helper_.exists("sendonly"))
            {
                xsdp.media_.erase(itr++);
            }
            else
            {
                ++itr;
            }
        }

        try
        {
            std::ostringstream oss;
            xsdp.encode(oss);
            std::string ret_sdp = oss.str();
            long ret_sdp_len = ret_sdp.length();
            if (save_sdp_to_access(dev_ids,dev_chanid,dev_strmtype,ret_sdp.c_str(),ret_sdp_len) < 0)
            {
                ret_code = -2;
                break;
            }
            DEBUG_LOG(DBLOGINFO,ll_info,"sip_interior_play_sip_save_sdp |dev_ids[%s] dev_chanid[%d] dev_strmtype[%d] |保存交换接收端sdp ret_code[%d] sdp[%s]",
                dev_ids.c_str(),dev_chanid,dev_strmtype,ret_code,ret_sdp.c_str());
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }
    } while (0);

    return ret_code;
}

int XTEngine::gw_join_sip_save_sdp_rs(const long sessionid,const char *sdp, long len_sdp)
{
    int ret_code = 0;
    do 
    {
        session_inf_t session;
        ret_code = gw_join_sip_session_mgr::_()->get_session(sessionid,session);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_save_sdp_rs |get_seesion ret_code[%d] sessionid[%d]",ret_code,sessionid);
            break;
        }
        //生成接收接收sdp
        std::string sdp_r;
        ret_code = gw_join_sip_transform_call_sdp(sdp_r,sdp,"sendonly");
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_save_sdp_rs |sipo_transform_call_sdp fail ret_code[%d] sessionid[%d]",ret_code,sessionid);
            break;
        }

        long len_sdp_r=sdp_r.size();
        ret_code = XTEngine::instance()->gw_join_sip_save_sdp_to_access(session.recv_ids,session.recv_chid,session.stremtype,sdp_r.c_str(),len_sdp_r);

        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_save_sdp_rs |保存接收SDP[%s]",sdp_r.c_str());

        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_save_sdp_rs |gw_join_sip_save_sdp_to_access fail! ret_code[%d] sessionid[%d]",ret_code,sessionid);
            break;
        }

        ret_code = gw_join_sip_session_mgr::_()->save_sdp_to_session(sessionid,sdp,len_sdp);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_save_sdp_rs |save_sdp_to_session!ret_code[%d] sessionid[%d]",ret_code,sessionid);
            break;
        }

    } while (false);

    return ret_code;
}

int XTEngine::sip_transform_call_sdp_ex(std::string& recv_sdp,std::string& send_sdp,const std::string& call_sdp)
{
    int ret_code = 0;
    do 
    {
        //解析sdp
        xt_sdp::parse_buffer_t pb(call_sdp.c_str(),call_sdp.size());
        xt_sdp::sdp_session_t xsdp; 
        xt_sdp::parse_buffer_t recv_pb(call_sdp.c_str(),call_sdp.size());
        xt_sdp::sdp_session_t recv_xsdp;
        xt_sdp::parse_buffer_t send_pb(call_sdp.c_str(),call_sdp.size());
        xt_sdp::sdp_session_t send_xsdp;
        try
        {
            xsdp.parse(pb);
            recv_xsdp.parse(recv_pb);
            send_xsdp.parse(send_pb);
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }

        recv_xsdp.media_.clear();
        send_xsdp.media_.clear();

        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for (;xsdp.media_.end() != itr; ++itr)
        {
            if (itr->attribute_helper_.exists("recvonly"))
            {
                recv_xsdp.media_.push_back(*itr);
            }
            else if (itr->attribute_helper_.exists("sendrecv"))
            {
                recv_xsdp.media_.push_back(*itr);
                send_xsdp.media_.push_back(*itr);
            }
            else if (itr->attribute_helper_.exists("sendonly"))
            {
                send_xsdp.media_.push_back(*itr);
            }
        }
        try
        {
            std::ostringstream oss_recv;
            recv_xsdp.encode(oss_recv);
            recv_sdp = oss_recv.str();

            std::ostringstream oss_send;
            send_xsdp.encode(oss_send);
            send_sdp = oss_send.str();
        }
        catch(...)
        {
            ret_code = -4;
            break;
        }
    } while (0);
    return ret_code;
}

int XTEngine::gw_join_sip_transform_call_sdp(std::string& out_sdp,const std::string& call_sdp,const std::string& condition /*= "recvonly"*/)
{
    int ret_code = 0;
    do 
    {
        //解析sdp
        xt_sdp::parse_buffer_t pb(call_sdp.c_str(),call_sdp.size());

        char temp_sdp1[4096];
        long sdp_len = call_sdp.length();
        ::memcpy(temp_sdp1,call_sdp.c_str(),sdp_len);

        xt_sdp::parse_buffer_t tmp_pb(temp_sdp1,sdp_len);
        xt_sdp::sdp_session_t xsdp;
        xt_sdp::sdp_session_t tmp_sdp;
        try
        {
            xsdp.parse(pb);
            tmp_sdp.parse(tmp_pb);
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }

        tmp_sdp.media_.clear();

        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for (;xsdp.media_.end() != itr; ++itr)
        {
            if (itr->attribute_helper_.exists(condition.c_str()))
            {
                tmp_sdp.media_.push_back(*itr);
            }
            else if (itr->attribute_helper_.exists("sendrecv"))
            {
                tmp_sdp.media_.push_back(*itr);
            }
        }

        try
        {
            std::ostringstream oss;
            tmp_sdp.encode(oss);
            out_sdp = oss.str();
        }
        catch(...)
        {
            ret_code = -4;
            break;
        }

    } while (0);

    return ret_code;
}

int XTEngine::get_tracknum(const long stream_type) const
{
    int track_num = -1;
    switch(stream_type)
    {
    case STREAM_MAIN:
    case STREAM_SUB:
        {
            track_num=2;
            break;
        }

    case STREAM_AUDIO:
        {
            track_num=1;
            break;
        }

    default:
        {
            break;
        }

    }

    return track_num;

}

long XTEngine::gw_join_sip_add_send(const long sessionid)
{
    long ret_code=0;
    do 
    {
        session_inf_t session;
        ret_code = gw_join_sip_session_mgr::_()->get_session(sessionid,session);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send |get_session ret_code[%d] sessionid[%d]",ret_code,sessionid);
            break;
        }

        //解析sdp
        std::string send_sdp;
        long send_sdp_len;
        switch(session.type)
        {
        case MSG_SIP_2_SIP_OPEN_CALL:
            {
                ret_code = gw_join_sip_2_sip_call_switch_set_data(session,send_sdp);
                send_sdp_len = send_sdp.length();
                if (ret_code < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_2_sip_call_switch_set_data fail recvids[%s] sendids[%s] sessionid[%d]",session.recv_ids.c_str(),session.send_ids.c_str(),session.sessionid);
                    ret_code = -1;
                    break;
                }
                break;
            }
        case MSG_OPEN_CALL:
            {
                ret_code = gw_join_sip_transform_call_sdp(send_sdp,session.sdp);
                send_sdp_len = send_sdp.length();
                if (ret_code < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send |gw_join_sip_transform_call_sdp fail ret_code[%d] sessionid[%d]",ret_code,sessionid);
                    break;
                }
                break;
            }
        default:
            {
                send_sdp = session.sdp;
                send_sdp_len = send_sdp.length();
                break;
            }
        }
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send |sessionid[%d] 推流sdp[%s] 解析sdp失败!",sessionid,session.sdp.c_str());
            break;
        }

        if (send_sdp.empty())
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send |sessionid[%d] 目标推流sdp为空!",sessionid,session.sdp.c_str());
            ret_code = -8;
            break;
        }

        //根据sdp增加发送
        src_info src;
        ret_code = get_src_ids(session.send_ids,session.send_chid,session.stremtype,src);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send |get_src_ids sessionid[%d] ret_code[%d] send_ids[%s] send_chid[%d] stremtype[%d]",sessionid,ret_code,session.send_ids.c_str(),session.send_chid,session.stremtype);
            break;
        }
        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_add_send |start -- | srcno[%d] sessionid[%d] send_ids[%s] send_chid[%d] stremtype[%d]",src.srcno,sessionid,session.send_ids.c_str(),session.send_chid,session.stremtype);

        int srcno = src.srcno;

        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_add_send |sessionid[%d] 推流sdp[%s]",sessionid,send_sdp.c_str());
        xt_sdp::parse_buffer_t pb(send_sdp.c_str(),send_sdp_len);
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send |解析sdp 发生异常 |sessionid[%d]",sessionid);
            ret_code = -3;
            break;
        }

        svr_info svr[MAX_TRACK];
        int tracknum = xsdp.media_.size();
        int ret_code = media_server::get_svr_info(svr,tracknum,srcno);
        if (ret_code < 0 || tracknum <=0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send | get_svr_info fail! srcno[%d] sessionid[%d] send_ids[%s] send_chid[%d] stremtype[%d] tracknum[%d] ret_code[%d]",
                src.srcno,sessionid,session.send_ids.c_str(),session.send_chid,session.stremtype,tracknum,ret_code);
            break;
        }

        bool is_send = false;
        std::string trakname;
        std::list<std::string> lsta;
        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for (;xsdp.media_.end() != itr; ++itr)
        {
            rtp_dst_info_t send_dst;

            if (itr->connections_.empty())
            {
                send_dst.dst_ip = xsdp.connection_.address_;

            }
            else
            {
                xt_sdp::sdp_session_t::connection_t c = itr->connections_.front();
                send_dst.dst_ip = c.address();
            }

            send_dst.dst_port = (unsigned short)itr->port_;
            if (0 == send_dst.dst_port)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,"send dst port is empty! sessionid[%d] ip[%s] send_ids[%s]",sessionid,send_dst.dst_ip.c_str(),session.send_ids.c_str());
                continue;
            }

            for(int i=0; i<tracknum; ++i)
            {

                if (0 != itr->name_.compare(svr[i].trackname))
                {
                    continue;
                }

                send_dst.trackid = svr[i].trackid;

                if (itr->attribute_helper_.exists("rtpport-mux"))
                {
                    send_dst.dst_demux = true;
                    lsta = itr->attribute_helper_.get_values("muxid");
                    if (!lsta.empty())
                    {
                        send_dst.dst_demuxid = ::str_to_num<int>(lsta.front().c_str());
                    }
                    lsta.clear();
                }
                else
                {
                    send_dst.dst_demux = false;
                    send_dst.dst_demuxid = 0;
                }

                DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_add_send | media_server::add_send start |sessionid[%d] srcno[%d] trackid[%d] send_ip[%s] sendport[%d] demux[%d] muxid[%d]",
                    sessionid,srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);

                ret_code = media_server::add_send(srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);
                if (ret_code < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send | add_send fail! | sessionid[%d] srcno[%d] trackid[%d] send_ip[%s] sendport[%d] demux[%d] muxid[%d] ret_code[%d]",
                        sessionid,srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid,ret_code);
                    continue;
                }
                is_send = true;
                ret_code = gw_join_sip_session_mgr::_()->save_send_dst(sessionid,send_dst);
                if (ret_code < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send | sessionid[%d] save_send_dst fail! srcno[%d] trackid[%d] send_ip[%s] sendport[%d] demux[%d] muxid[%d]",
                        sessionid,srcno,send_dst.trackid,send_dst.dst_ip.c_str(),send_dst.dst_port,send_dst.dst_demux,send_dst.dst_demuxid);
                }
            }
        }

        //没有任何流被推送出去
        if (!is_send)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send |没有任何流被推送出去|sessionid[%d]",sessionid);
            ret_code = -5;
            break;
        }

        ret_code = 1;
    } while (false);
    return ret_code;
}

int XTEngine::gw_join_sip_create_transmit_machine_made(const std::string& dev_ids,const long dev_ch,const long dev_strmtype,
                                                       const long transmit_ch,const dev_handle_t dev_handle,const long strmid)
{
    int ret_srcno = -1;
    int oper_code = -1;
    int ret_code = 0;
    do 
    {
        device_info device;
        device.db_type = DEV_SIP;
        device.dev_handle = dev_handle;
        device.dev_ids = dev_ids;
        device.dev_chanid = dev_ch;
        device.dev_strmtype = dev_strmtype;
        device.db_chanid = dev_ch;
        device.strmid = strmid;
        device.tracknum = 0;
        if (m_msCfg.snd_std_rtp)
        {
            char key[MAX_KEY_SIZE] = {0};
            long len_key = MAX_KEY_SIZE;
            long data_type = -1;
            media_device::_()->get_sdp_by_handle(dev_handle, key, len_key,data_type);

            device.key_len = len_key;
            ::strncpy(device.key,key,len_key);
            device.tracknum = parse_tracks_ex(key,len_key,device.track_infos);
            if (device.tracknum < 0)
            {
                device.tracknum = 0;
            }
            if (device.tracknum >= MAX_TRACK)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_create_transmit_machine_made | media_device::get_track fail! track_num[%d] dev_ids[%s]dev_chid[%d]dev_strmtype[%d]",
                    device.tracknum,dev_ids.c_str(),dev_ch,dev_strmtype);
                return -1;
            }
        }
        ret_code = create_src(device, ret_srcno, transmit_ch);
        if (ret_code < 0)
        {
            oper_code = -1;
            break;
        }
        ret_code = media_server::set_key_data(ret_srcno, device.key, device.key_len,172);
        if (ret_code < 0)
        {
            oper_code = -2;
            break;
        }

        oper_code = 0;
    } while (0);
    if (oper_code < 0)
    {
        ret_code = destroy_src(ret_srcno);
        ret_srcno = -1;
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_create_transmit_machine_made destroy_src fail ids[%s] devch[%d] dev_stremtype[%d] srcno[%d] ret_code[%d]",
                dev_ids.c_str(),dev_ch,dev_strmtype,ret_srcno,ret_code);
        }
    }
    return ret_srcno;
}

long XTEngine::sip_2_sip_create_free_transmit_channel(const std::string& recvids,const std::string& sendids,const long transmit_ch,const long dev_chanid, const long dev_strmtype,const bool dumx_flags,std::string& sdp_two_way,int& out_srcno,long& recv_dev_handle)
{
    long ret_code = 0;
    do 
    {
        //创建接收机制
        std::string sdp_r;
        ret_code = sip_create_r(recvids,dev_chanid,dev_strmtype,dumx_flags,sdp_r,recv_dev_handle);
        if (ret_code < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,
                "sip_2_sip_create_free_transmit_channel | sip_create_r fail! ret_code[%d] recvids[%s] dev_chanid[%d] dev_strmtype[%d] dumx_flags[%d]",
                ret_code,recvids.c_str(),dev_chanid,dev_strmtype,dumx_flags);
            break;
        }

        //创建转发源
        int srcno = -1;
        char sdp_s[MAX_KEY_SIZE] = {0};
        int sdp_len_s = -1;

        int check_srcno = -1;
        check_srcno = is_exist_src(sendids,dev_chanid,dev_strmtype);
        if (0 <= check_srcno)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,
                "sip_2_sip_create_free_transmit_channel |转发源已创建 create transmit src ids[%s] dev_chanid[%d] dev_strmtype[%d] transmit src is exits srcno[%d]",
                sendids.c_str(),dev_chanid,dev_strmtype,check_srcno);
            out_srcno = check_srcno;
            std::string send_sdp;
            ret_code = create_sdp_s(out_srcno,send_sdp);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "sip_2_sip_create_free_transmit_channel create_sdp_s fail ret_code[%d] ids[%s] dev_chanid[%d] dev_strmtype[%d]",
                    ret_code,sendids.c_str(),dev_chanid,dev_strmtype);
                break;
            }

            sdp_len_s = send_sdp.length();
            if (sdp_len_s > MAX_KEY_SIZE)
            {
                ret_code = -1;
                break;
            }
            ::strncpy(sdp_s,send_sdp.c_str(),sdp_len_s);

        }
        else
        {
            char local_ip[64] = {0};
            ::strncpy(local_ip,CXTRouter::Instance()->get_local_ip().c_str(),64);
            ret_code = media_server::create_src_defult(&srcno,sdp_s,&sdp_len_s,transmit_ch,local_ip);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "sip_2_sip_create_free_transmit_channel | create_src_defult fail ids[%s] dev_chanid[%d] dev_strmtype[%d] ret_code[%d] transmit_ch[%d] local_ip[%s]",
                    sendids.c_str(),dev_chanid,dev_strmtype,ret_code,transmit_ch,local_ip);
                break;
            }
            if (srcno < 0)
            {
                break;
            }

            DEBUG_LOG(DBLOGINFO,ll_info,
                "sip_2_sip_create_free_transmit_channel | create transmit src ids[%s] dev_chanid[%d] dev_strmtype[%d]  srcno[%d]",
                sendids.c_str(),dev_chanid,dev_strmtype,srcno);

            src_info src;
            src.create_time = boost::posix_time::microsec_clock::local_time();
            device_info device;
            device.db_chanid = 0;
            device.db_type = DEV_SIP;
            device.db_url = "sip";
            device.dev_chanid = 0;
            device.dev_handle = -1;
            device.dev_ids = sendids;
            device.dev_strmtype = 0;
            device.key_len = -1;
            device.strmid = -1;

            if (m_msCfg.snd_std_rtp)
            {
                device.tracknum = parse_tracks_ex(sdp_s,sdp_len_s,device.track_infos);
                if (device.tracknum < 0)
                {
                    device.tracknum = 0;
                }
            }

            src.device = device;
            src.regist = false; 
            src.srcno = srcno;
            out_srcno = srcno;
            ret_code = active_src(srcno,src,true);
            if (ret_code < 0)
            {
                break;
            }
        }

        xt_sdp::parse_buffer_t pb_r(sdp_r.c_str(),sdp_r.length());
        xt_sdp::sdp_session_t xsdp_r;
        xt_sdp::parse_buffer_t pb_s(sdp_s,sdp_len_s);
        xt_sdp::sdp_session_t xsdp_s;
        size_t len = ::strlen(SDP_TEMPLATE);
        xt_sdp::parse_buffer_t pb_rs(SDP_TEMPLATE,len);
        xt_sdp::sdp_session_t xsdp_rs;
        try
        {
            xsdp_r.parse(pb_r);
            xsdp_s.parse(pb_s);
            xsdp_rs.parse(pb_rs);
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }

        xsdp_rs.origin().address_ = CXTRouter::Instance()->get_local_ip();

        //xsdp_rs.bandwidths_.push_back(xt_sdp::sdp_session_t::bandwidth_t("AS",2048));
        if (xsdp_r.attribute_helper_.exists("rtpport-mux")||xsdp_s.attribute_helper_.exists("rtpport-mux"))
        {
            xsdp_rs.add_attribute("rtpport-mux");
        }

        xsdp_rs.media_.clear();

        //recvonly
        xt_sdp::sdp_session_t::medium_container_t::iterator itr;
        for(itr = xsdp_r.media_.begin(); xsdp_r.media_.end() != itr; ++itr)
        {
            xsdp_rs.media_.push_back(*itr);
        }

        //sendonly
        for(itr = xsdp_s.media_.begin(); xsdp_s.media_.end() != itr; ++itr)
        {
            xsdp_rs.media_.push_back(*itr);
        }

        try
        {
            std::ostringstream oss;
            xsdp_rs.encode(oss);
            sdp_two_way = oss.str();
        }
        catch(...)
        {
            ret_code = -4;
            break;
        }

        ret_code = 1;
    } while (0);
    return ret_code;
}

long XTEngine::gw_join_sip_2_sip_call_switch_set_data(const session_inf_t& ssession_1,std::string& send_sdp)
{
    long ret_code = 0;
    do 
    {
        std::string s1_send_sdp;
        std::string s1_recv_sdp;
        ret_code = sip_transform_call_sdp_ex(s1_recv_sdp,s1_send_sdp,ssession_1.sdp);
        if (ret_code < 0)
        {
            ret_code = -2;
            break;
        }

        //推流无端接收sdp
        send_sdp = s1_recv_sdp;

        //转发源标识
        int trans_srcno = ssession_1.srcno;
        //检验srcno是否已关联接入句柄
        dev_handle_t link_handle = get_dev_link_handle_src(trans_srcno);
        if (link_handle < 0)
        {
            session_inf_t session_2;
            ret_code = gw_join_sip_session_mgr::_()->get_session(ssession_1.send_ids,ssession_1.recv_ids,session_2);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_2_sip_call_switch_set_data| get_session fail! sendids[%s] recvids[%s]",ssession_1.send_ids.c_str(),ssession_1.recv_ids.c_str());
                ret_code = -3;
                break;
            }

            std::string s2_send_sdp;
            std::string s2_recv_sdp;
            ret_code = sip_transform_call_sdp_ex(s2_recv_sdp,s2_send_sdp,session_2.sdp);
            if (ret_code < 0)
            {
                ret_code = -5;
                break;
            }

            //组帧sdp
            std::string send_acees_sdp(s2_send_sdp);

            //接入设备句柄
            long access_dev_handle = session_2.dev_handle;
            //保存sdp
            DEBUG_LOG(DBLOGINFO,ll_info,
                "swich: | save_sdp_srcno_to_srv_and_access | sessionid[%d] srcno[%d] dev_handle[%d]",ssession_1.sessionid,trans_srcno,access_dev_handle);
            ret_code = save_sdp_srcno_to_srv_and_access(trans_srcno,access_dev_handle,send_acees_sdp);
            if (ret_code < 0)
            {
                ret_code = -4;
                break;
            }

            DEBUG_LOG(DBLOGINFO,ll_info,"swich: | start_link_capture | sessionid[%d] srcno[%d] dev_handle[%d]",session_2.sessionid,trans_srcno,access_dev_handle);
            long strmid  = media_device::_()->get_strmid_by_dev_handle(access_dev_handle);
            src_info src;
            ret_code = get_src_no(trans_srcno,src);
            if (ret_code < 0)
            {
                ret_code = -6;
                break;
            }
            src.device.strmid = strmid;
            src.device.dev_handle = access_dev_handle;
            src.active = true;//激活

            DEBUG_LOG(DBLOGINFO,ll_info,
                "swich: | upate_src | sessionid1[%d] sessionid2[%d] srcno[%d] dev_handle[%d] streamid[%d]",
                ssession_1.sessionid,session_2.sessionid,trans_srcno,access_dev_handle,strmid);
            ret_code = upate_src(trans_srcno,src);
            if (ret_code < 0)
            {
                ret_code = -7;
                break;
            }
        }
        else
        {
            DEBUG_LOG(DBLOGINFO,ll_info,"swich: |sessionid1[%d] srcno[%d] 已关联设备激活 不用进行交换操作!",ssession_1.sessionid,trans_srcno);
        }
        ret_code = 1;
    } while (0);
    return ret_code;
}
