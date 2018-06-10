#include "media_device.h"
#include <boost/thread/lock_guard.hpp>
#include "Router_config.h"
#include "XTRouterLog.h"
#include "XTRouter.h"
media_device media_device::self;
media_device::media_device(void)
{

}

media_device::~media_device(void)
{
}

long media_device::init()
{
    ::StartMediadevice();
    //::InitializeDevice(-1,NULL);
    //加载配置 
    xt_client_cfg_t cfg;
    std::string recv_ip = config::instance()->local_sndip("0.0.0.0");
    strcpy(cfg.udp_session_bind_ip,recv_ip.c_str());
    cfg.udp_session_bind_port = config::instance()->udp_session_bind_port(19001);
    cfg.udp_session_heartbit_proid = config::instance()->udp_session_heartbit_proid(20); //udp会话保活心跳周期 单位：毫秒 默认配置为:20
    cfg.udp_session_request_try_count = config::instance()->udp_session_request_try_count(4);  //udp会话操作失败后重试次数 默认配置为:4
    cfg.udp_session_request_one_timeout = config::instance()->udp_session_request_one_timeout(5000);  //udp会话操作等待时间 默认配置为:5000

    strcpy(cfg.tcp_session_bind_ip,recv_ip.c_str());
    cfg.tcp_session_bind_port = config::instance()->tcp_session_bind_port(0);
    cfg.tcp_session_connect_timeout = config::instance()->tcp_session_connect_timeout(3000); 	//tcp会话连接超时时间，单位：毫秒 默认配置为:10000
    cfg.tcp_session_login_timeout = config::instance()->tcp_session_login_timeout(3000);	   	//tcp会话登录超时时间，单位：毫秒 默认配置为:10000
    cfg.tcp_session_play_timeout = config::instance()->tcp_session_play_timeout(2000);		//tcp会话点播超时时间，单位：毫秒 默认配置为:2000
    cfg.tcp_session_stop_timeout = config::instance()->tcp_session_stop_timeout(2000);		//tcp会话停点超时时间，单位：毫秒 默认配置为:2000

    cfg.rtsp_session_connect_timeout = config::instance()->rtsp_session_connect_timeout(3000);   //rtsp会话连接超时时间，单位：毫秒 默认配置为:10000
    cfg.rtsp_session_describe_timeout = config::instance()->rtsp_session_describe_timeout(3000);		//rtsp会话describe超时时间，单位：毫秒 默认配置为:10000
    cfg.rtsp_session_setup_timeout = config::instance()->rtsp_session_setup_timeout(3000);		//rtsp会话setup超时时间，单位：毫秒 默认配置为:10000
    cfg.rtsp_session_play_timeout = config::instance()->rtsp_session_play_timeout(2000);         //rtsp会话play超时时间，单位：毫秒 默认配置为:10000
    cfg.rtsp_session_pause_timeout = config::instance()->rtsp_session_pause_timeout(2000);        //rtsp会话pause超时时间，单位：毫秒 默认配置为:10000
    cfg.rtsp_session_teardown_timeout = config::instance()->rtsp_session_teardown_timeout(1000);     //rtsp会话teardown超时时间，单位：毫秒 默认配置为:1000
    long start_port = config::instance()->rtp_recv_start_port(16000);
    long portNum = config::instance()->rtp_recv_port_num(1000);

    return ::InitializeDeviceEx(-1,cfg, start_port, portNum);
}

void media_device::term()
{
    ::UnInitializeDevice(-1,NULL);
    ::EndMediadevice();
}

long  media_device::start_capture(int device_type, const char *localip,char* url, long channel, int media_type, void* user_data, access_data_output_cb_t pfnDataCB, int port, char* szUser, char* szPassword,int link_type,void *bc_mp)
{
    WRITE_LOG(DBLOGINFO,ll_info,"media_device::start_capture.device_type[%d], url[%s], media_type[%d], port[%d], link_type[%d], user_data[%d]", device_type, url, media_type, port, link_type, user_data);
    return ::StartDeviceCapture(url, port, device_type, channel, user_data, pfnDataCB, szUser, szPassword, link_type, media_type, 0,NULL,0,localip,bc_mp);
}

int media_device::stop_capture(dev_handle_t handle)
{
    if (handle < 0)
    {
        return 0;
    }

    //删除上层协商时的句柄管理
    media_device::_()->del_link_by_handle(handle);

    return ::StopDeviceCapture(handle);
}

// long media_device::get_sdp_by_handle(dev_handle_t oper_handle,const char* sdp,long& length)
// {
//     if (oper_handle < 0)
//     {
//         return -1;
//     }
//     return ::GetSDP(oper_handle,(uint8_t*)sdp,(long&)length);
// }

long media_device::get_data_type_by_handle(dev_handle_t oper_handle)
{
    if (oper_handle < 0)
    {
        return -1;
    }

    return ::GetDataType(oper_handle);

}
long media_device::get_sdp_by_handle(dev_handle_t oper_handle,const char* sdp,long& length,long& data_type)
{
    long ret_code = -1;
    do
    {
        if (oper_handle < 0)
        {
            break;
        }

        ret_code = ::GetSDP(oper_handle,(uint8_t*)sdp,(long&)length);
        if (ret_code < 0 || length <= 0)
        {
            ret_code = -2;
            break;
        }

        data_type = ::GetDataType(oper_handle);
        if (data_type < 0)
        {
            ret_code = -3;
            break;
        }
        ret_code = 1;
    }while(0);
    return ret_code;
}

int media_device::tcp_play_ctrl(const long oper_handle,double npt,float scale,uint32_t *rtp_pkt_timestamp)
{
    return ::TcpPlayCtrl(oper_handle,npt,scale,(unsigned long*)rtp_pkt_timestamp);

}
int media_device::tcp_pause_ctrl(const long oper_handle)
{
    return ::TcpPauseCtrl(oper_handle);

}
int media_device::rtsp_play_ctrl(const long oper_handle,double npt,float scale,uint32_t *rtp_pkt_timestamp)
{
    return -1;

}
int media_device::rtsp_pause_ctrl(const long oper_handle)
{
    return -1;
}

long media_device::rtp_create_recv2(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports)
{
    return ::md_create_recvinfo(9,track_num,demux,multicast,multicastip,multiports);

}
long media_device::rtp_create_recv(int track_num, bool demux)
{
    WRITE_LOG(DBLOGINFO,ll_info, "media_device::rtp_create_recv.track_num[%d], demux[%d]", track_num, demux);
    return ::md_create_recv(track_num, demux);
}

long media_device::rtp_get_rcvinfo(long link, _RCVINFO *infos, int &num)
{
    return ::md_get_rcvinfo(link, infos, num);
}

long media_device::rtp_save_sdp(long link, const char *sdp, unsigned int len_sdp)
{
    return ::md_set_sdp(link, sdp, len_sdp);
}

long media_device::start_link_captuer(const long link_handel,access_data_output_cb_t data_out_cb,long strmid)
{
    long opr_code = -1;
    long ret_link_handle = -1;
    do 
    {
        bool check_link_state;
        ret_link_handle = instance()->link_active_state(link_handel,check_link_state);
        if (ret_link_handle < 0)
        {
            opr_code = -1;
            break;
        }
        //对于已经捕获数据的设备句柄重复捕获时直接返回设备句柄
        if (check_link_state)
        {
            opr_code=0;
            break;
        }

        ret_link_handle = ::md_start_link_captuer(link_handel,data_out_cb,(void*)strmid);
        if (ret_link_handle < 0)
        {
            opr_code = -2;
            break;
        }
        ret_link_handle = instance()->active_link(link_handel,strmid,true);
        if (ret_link_handle < 0)
        {
            opr_code = -3;
            break;
        }
        opr_code=1;
    } while (0);

    if (opr_code < 0)
    {
        return opr_code;
    }
    return ret_link_handle;
}

long media_device::request_iframe(long link)
{
    return ::md_request_iframe(link);
}
long media_device::add_link(dev_handle_t link, std::string ids, long dev_chanid, long dev_strmtype)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);

    _device_link dev;
    dev.link = link;
    dev.ids = ids;
    dev.dev_chanid = dev_chanid;
    dev.dev_strmtype = dev_strmtype;
    dev.active = false;
    m_vlink.push_back(dev);

    return 0;
}

long media_device::del_link(const std::string &ids, long dev_chanid, long dev_strmtype)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();)
    {
        _device_link &dev = *itr;
        if (dev.ids==ids&&
            dev.dev_chanid==dev_chanid)
        {
            if (CXTRouter::_()->get_router_cfg().use_strmtype_param)
            {
                if (dev.dev_strmtype==dev_strmtype)
                {
                    DEBUG_LOG(DBLOGINFO,ll_info,
                        "media_device::del_link_by_ids handle[%d] ids[%s] dev_chanid[%d] dev_strmtype[%d]",
                        dev.link,ids.c_str(),dev.dev_chanid,dev.dev_strmtype);
                    m_vlink.erase(itr++);
                    return 0;
                }
            }
            else
            {
                DEBUG_LOG(DBLOGINFO,ll_info,
                    "media_device::del_link_by_ids handle[%d] ids[%s] dev_chanid[%d] dev_strmtype[%d]",
                    dev.link,ids.c_str(),dev.dev_chanid,dev.dev_strmtype);
                m_vlink.erase(itr++);
                return 0;
            }
        }
        else
        {
            ++itr;
        }
    }
    return -1;
}

long media_device::find_link(const std::string &ids,long dev_chanid,long dev_strmtype,device_link_t& dev_link_inf)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();++itr)
    {
        _device_link &dev = *itr;
        if (dev.ids        == ids&&
            dev.dev_chanid  == dev_chanid)
        {
            if (CXTRouter::_()->get_router_cfg().use_strmtype_param)
            {
                if (dev.dev_strmtype == dev_strmtype)
                {
                    dev_link_inf.link = itr->link;
                    dev_link_inf.ids = itr->ids;
                    dev_link_inf.dev_chanid = itr->dev_chanid;
                    dev_link_inf.dev_strmtype = itr->dev_strmtype;
                    dev_link_inf.active = itr->active;
                    dev_link_inf.strmid = itr->strmid;
                    return 0;
                }
            }
            else
            {
                dev_link_inf.link = itr->link;
                dev_link_inf.ids = itr->ids;
                dev_link_inf.dev_chanid = itr->dev_chanid;
                dev_link_inf.dev_strmtype = itr->dev_strmtype;
                dev_link_inf.active = itr->active;
                dev_link_inf.strmid = itr->strmid;
                return 0;
            }

        }
    }

    return -1;

}

dev_handle_t media_device::find_link(const std::string &ids, long dev_chanid, long dev_strmtype)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();++itr)
    {
        _device_link &dev = *itr;
        if (dev.ids        == ids&&
            dev.dev_chanid  == dev_chanid)
        {
            if (CXTRouter::_()->get_router_cfg().use_strmtype_param)
            {
                if (dev.dev_strmtype == dev_strmtype)
                {
                    return dev.link;
                }
            }
            else
            {
                return dev.link;
            }

        }
    }

    return -1;
}

void media_device::del_link_by_ids(const std::string &ids)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();)
    {
        _device_link &dev = *itr;
        if (dev.ids==ids)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,
                "media_device::del_link_by_ids handle[%d] ids[%s] dev_chanid[%d] dev_strmtype[%d]",
                dev.link,dev.ids.c_str(),dev.dev_chanid,dev.dev_strmtype);
            m_vlink.erase(itr++);
            break;
        }
        else
        {
            ++itr;
        }
    }
}
void media_device::del_link_by_handle(const dev_handle_t link)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();)
    {
        _device_link &dev = *itr;
        if (dev.link ==link)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,
                "media_device::del_link_by_handle handle[%d] ids[%s] dev_chanid[%d] dev_strmtype[%d]",
                link,dev.ids.c_str(),dev.dev_chanid,dev.dev_strmtype);
            m_vlink.erase(itr++);
            break;
        }
        else
        {
            ++itr;
        }
    }

}

dev_handle_t media_device::link_active_state(const dev_handle_t link,bool& link_state)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();++itr)
    {
        if (link == itr->link)
        {
            link_state = itr->active;
            return itr->link;
        }
    }
    return -1;
}
dev_handle_t media_device::active_link(const dev_handle_t link,const long strmid/*=-1*/,const bool flg/*=false*/)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();++itr)
    {
        if (link == itr->link)
        {
            itr->active = true;
            itr->strmid = strmid;
            return itr->link;
        }
    }
    return -1;
}
long media_device::find_link(const dev_handle_t dev_handle,device_link_t& dev_link_inf)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();++itr)
    {
        if (itr->link == dev_handle)
        {
            dev_link_inf.link = itr->link;
            dev_link_inf.ids = itr->ids;
            dev_link_inf.dev_chanid = itr->dev_chanid;
            dev_link_inf.dev_strmtype = itr->dev_strmtype;
            dev_link_inf.active = itr->active;
            dev_link_inf.strmid = itr->strmid;
            return 0;
        }
    }

    return -1;

}

long media_device::mod_ids_by_handle(const dev_handle_t dev_handle,const std::string ids)
{
	boost::lock_guard<boost::detail::spinlock> lock(m_lock);
	std::vector<_device_link>::iterator itr = m_vlink.begin();
	for (;itr!=m_vlink.end();++itr)
	{
		if (itr->link == dev_handle)
		{
			itr->ids = ids;
			return 0;
		}
	}

	return -1;
}

long media_device::get_strmid_by_dev_handle(const dev_handle_t link)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();++itr)
    {
        if (itr->link == link)
        {
            return itr->strmid;
        }
    }
    return -1;
}

bool media_device::is_md_handle(const dev_handle_t link)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    std::vector<_device_link>::iterator itr = m_vlink.begin();
    for (;itr!=m_vlink.end();++itr)
    {
        if (itr->link == link)
        {
            return true;
        }
    }
    return false;
}

void media_device::get_handle_all(std::vector<_device_link>& handle)
{
    boost::lock_guard<boost::detail::spinlock> lock(m_lock);
    handle = m_vlink;
}

long media_device::set_regist_callback(regist_call_back_t func)
{
    ::md_set_regist_callback(9,func);
    return 0;
}
