#include "XTRouter.h"
#include "pri_jk_engine.h"
#include "framework/task.h"
#include "XTEngine.h"
#include "media_device.h"
#include "media_server.h"
#include "InfoMgr.h"
#include "FuncEx.h"
#include "media_server.h"
#include "router_task.h"
#include "xt_regist_server.h"
#include "XTRouterLog.h"
#include "SlaveIPC.h"
//#include "std_sip_engine.h"
#include "sip_svr_engine.h"
//#include "common_ctrl_msg.h"
#include "xmpp_client.h"

#include <functional>
#include <stdlib.h>
#include <boost/filesystem.hpp>

CXTRouter CXTRouter::m_obj;
extern bool router_engine_init();
extern bool reouter_engine_uninit();
int g_ids_index=-1;
//命令定义
/////////////////////////////////////////////////////////////////////////////////
#define  COMMAND_HELP "help"                  //帮助
#define COMMAND_CLEAR "cls"
#define DES_COMMAND_CLEAR "clear"

#define TAB_1 " -"
#define TAB_2 ":"
#define OPTION "[option]" 
#define VALUE "[value]"
#define FLG_1 "----------------------------------------------------------------------------\n"

#define COMMAND_PLAY "play"                   
#define DES_COMMAND_PLAY "device "

#define COMMAND_STOP "stop"                    
#define DES_COMMAND_STOP "stop device"

#define COMMAND_STOP_All "stopall"             
#define DES_COMMAND_STOP_ALL "device"

#define COMMAND_SHOW_PLAY_INFO "spi"           
#define DES_COMMAND_SHOW_PLAY_INFO "display_current_play_info"

#define COMMAND_SHOW_KEY_INFO "ski"           
#define DES_COMMAND_SHOW_KEY_INFO "display sdp info"

#define COMMAND_HIDE_PLAY_INFO "hpi"          
#define DES_COMMAND_HIDE_PLAY_INFO "hide_play_info"

#define COMMAND_SHOW_CONNECT_INFO "scoi"       
#define DES_COMMAND_SHOW_CONNECT_INFO "display_connect_info"

#define COMMAND_HIDE_CONNECT_INFO "hcoi"      
#define DES_COMMAND_HIDE_CONNECT_INFO "hide_connect_info"

#define COMMAND_SHOW_LINK_SERVER_INFO "slsi"   
#define DES_COMMAND_SHOW_LINK_SERVER_INFO "display_link_server_info"

#define COMMAND_SHOW_LOGIN_LONGOUT_INFO "sli"  
#define DES_COMMAND_SHOW_LOGIN_LONGOUT_INFO "display_device_online"

#define COMMAND_SHOW_BASE_INFO "swi"  
#define DES_COMMAND_SHOW_BASE_INFO "display_work_info"


#define COMMAND_LOG_ON_OFF "log"
#define DES_COMMAND_LOG_ON_OFF "log open/off：0-off 1-all 4-info 5-warning 6-error"


/////////////////////////////////////////////////////////////////////////////////

//参数定义
/////////////////////////////////////////////////////////////////////////////////
//play and stop
#define ARG_IDS "ids"             
#define DES_ARG_IDS "device_IDS ids1)"

#define ARG_DEV_CH_ID "devch"     
#define DES_ARG_DEV_CH_ID "device_channel 0)"

#define ARG_STRM_TYPE "stype"     
#define DES_ARG_STRM_TYPE "device_stream 0)"

#define ARG_DB_IP "ip"            
#define DES_ARG_DB_IP "paly_IP"

#define ARG_DB_CH_ID "dbch"       
#define DES_ARG_DB_CH_ID "play_channel" 

#define ARG_DB_TYPE "dbtype"      
#define DES_ARG_DB_TYPE "play_type" 

#define ARG_LINK_TYPE "linktype"      
#define DES_ARG_LINK_TYPE "link_type 0)"

#define ARG_SEND_CH_ID "ch"   
#define DES_ARG_SEND_CH_ID "send_channel -1"

#define ARG_DB_NUM "dbnum"   
#define DES_ARG_DB_NUM "play_num -1"

#define ARG_STRM_ID "strmid"      
#define DES_ARG_STRM_ID "stream_id" 

#define ARG_REAL "r"
#define DES_ARG_REAL "check_real_info"

#define ARG_HISTORY "h"
#define DES_ARG_HISTORY "check_histroy_info"

#define ARG_CENTER "center"
#define DES_ARG_CENTER " check_center_command"

#define ARG_EXEC "exec"
#define DES_ARG_EXEC " get_exec_result"

#define ARG_RESP "resp"
#define DES_ARG_RESP "get_center_response"

#define ARG_ALL "level"
#define DES_ARG_ALL "leval_log"

/////////////////////////////////////////////////////////////////////////////////

#define LOG_SIZE 300*1024*1024

void CXTRouter::regist_log_sys()
{
    try 
    {
        //添加交换日志文件夹
        boost::filesystem::create_directory(LOG_PATH);
    }
    catch(...)
    {
        std::cerr<<"create log directory fail!"<<"please check:"<<LOG_PATH<<std::endl;
    }

	int level = config::instance()->log_level(0);

    init_log_target(LOG_PATH"db_info");
    init_log_target(LOG_PATH"xmpp");
    init_log_target(LOG_PATH"zl");
    init_log_target(LOG_PATH"media_svr");
    init_log_target(LOG_PATH"rtsp_svr");
    init_log_target(LOG_PATH"tcp_svr");

    if (level <= 0)
    {
        set_log_on_off(false);
    }
    else
    {
        set_log_on_off(true);
    }
}

void CXTRouter::init_cfg()
{
    //初始化交换配置
    //////////////////////////////////////////////////////////////////////////
    m_config.chan_num = config::_()->chan_num(128);
    m_config.local_ip = config::_()->local_ip("0.0.0.0");
    m_config.local_snd_ip = config::_()->local_sndip("0.0.0.0");
    m_config.start_sndport = config::_()->snd_port(20011);
    m_config.demux_flg_s = (0 != config::_()->demux_s(0));
    m_config.mul_ip = config::_()->mul_ip("239.0.0.1");
    m_config.xtmsg_listenport = config::_()->xtmsg_listenport(20001);
    m_config.rtsp_listenport = config::_()->rtsp_listenport(1554);
    m_config.tcp_listenport = config::_()->tcp_listenport(20000);
    m_config.regist_listenport = config::_()->regist_listenprt(20002);
    m_config.udp_listenport = config::_()->udp_listenport(19900);
    m_config.regist = config::_()->regist(false);
    m_config.regist_bind = config::_()->regist_bind(19935);
    m_config.regist_ip = config::_()->regist_ip("127.0.0.1");
    m_config.regist_port = config::_()->regist_port(20002);
    m_config.std_rtp = config::_()->std_rtp(false);
    m_config.use_strmtype_param = 0 < config::_()->use_strmtype_param(0) ?  true : false;
    m_config.link_center_type = config::instance()->link_center(config_type::LINK_ON);;

    int link_type = config::_()->get_link_type(3);
    switch (link_type)
    {
    case 5: case 8: case 12: case 14: case 17:
        {
            m_config.demux_r = true;
            break;
        }
    default:
        {
            m_config.demux_r = false;
            break;
        }
    }
    //中心配置部分
    /////////////////////////////
    m_config.log_usre_name = config::_()->local_name("");
    m_config.log_usre_password = config::_()->login_password("");
    m_config.log_usre_res_id = config::_()->login_res_id("");
    m_config.log_center_name = config::_()->cneter_host("");
    m_config.log_center_port = config::_()->center_port(0);
    /////////////////////////////
    m_config.default_sdp = config::_()->default_sdp();
}

int CXTRouter::StartXTRouter()
{
    std::cout<<"init cfg"<<std::endl;
    init_cfg();
    //added by lichao, 20150408 增加media_device的初始化和反初始化
    long ret_code = media_device::init();
    if (ret_code < 0)
    {
        std::cout<<"media_device::init fail!"<<std::endl;
        return -1;
    }
    std::cout<<"media_device::init success!"<<std::endl;

    //注册命令
    std::cout<<"register command..."<<std::endl;
    ResCommand();

    std::cout<<"xt_regist_server init..."<<std::endl;
    xt_regist_server::instance()->init();

    int iRet = 0;

    do 
    {
        //扩展功能
        iRet = CFuncEx::instance()->InitFuncEx();
        if (iRet < 0)
        {
            break;
        }
#ifdef USE_SNMP_
        //启动SNMP上报服务 BY wluo
        std::cout<<"Start SNMP slave report server..."<<std::endl;
        iRet =SlaveIPC::instance()->EstablishSlaveIPC(5800);
        if (iRet < 0)
        {
            break;
        }
        SlaveIPC::instance()->SendNotification(notification_serveronline); //added by wluo
#endif //#ifdef USE_SNMP_

        //启动服务器
        iRet = XTEngine::_()->start(m_config.chan_num, m_config.local_snd_ip, m_config.start_sndport, m_config.demux_flg_s,
            m_config.mul_ip, m_config.xtmsg_listenport, m_config.rtsp_listenport, m_config.tcp_listenport,
            m_config.udp_listenport,m_config.regist_listenport, SndStdRtp());
        if (iRet< 0)
        {
            break;
        }

        //反向注册
        if (m_config.regist)
        {
            media_server::regist2("ids0000", m_config.regist_ip, m_config.regist_port, 1000);
        }

        //设置rtsp srv 心跳
        unsigned int check_timer_interval = config::_()->rtsp_srv_check_timer_interval(0);;
        unsigned int time_out_interval = config::_()->rtsp_srv_time_out_interval(0);
        media_server::set_rtsp_heartbit_time(check_timer_interval,time_out_interval);

#ifdef _USE_WEB_SRV_
        std::cout<<"Start Web log server..."<<std::endl;
        web_srv_mgr::_()->init(config::instance()->get_web_server_port(8140));
#endif// #ifdef _USE_WEB_SRV_

        //模块初始化
        std::cout << "modules init..." << std::endl;
        CtrlMsgInit();

    } while (0);

    return iRet;
}

int CXTRouter::StopXTRouter()
{
    int ret = 0;

    std::cout<<"stop center link..."<<std::endl;
    CtrlMsgUninit();

#ifdef USE_SNMP_
    //stop SNMP report server by wluo
    std::cout<<"stop SNMP slave report server..."<<std::endl;
    ret = SlaveIPC::instance()->Exit();
    if ( ret < 0)
    {
        std::cout<<"stop SNMP slave report server fail!"<<std::endl;
    }
    SlaveIPC::instance()->SendNotification(notification_serveroffline); //added by wluo
#endif //#ifdef USE_SNMP_

    std::cout<<"stop all play..."<<std::endl;
    ret = XTEngine::instance()->stop_allplay();
    if ( ret < 0)
    {
        std::cout<<"stop all play fail..."<<std::endl;
    }

    std::cout<<"stop func_ex..."<<std::endl;
    ret = CFuncEx::instance()->UnitFuncEx();
    if (ret < 0)
    {
        std::cout<<"stop func_ex fail..."<<std::endl;
    }

    std::cout<<"stop xt_engine..."<<std::endl;
    ret = XTEngine::instance()->stop();
    if ( ret < 0)
    {
        std::cout<<"stop xt_engine fail..."<<std::endl;
    }

    //added by lichao, 20150408 增加media_device的初始化和反初始化
    media_device::term();

#ifdef _USE_WEB_SRV_
    std::cout<<"stop Web log server..."<<std::endl;
    web_srv_mgr::_()->uninit();
#endif// #ifdef _USE_WEB_SRV_

    return ret;
}

void CXTRouter::GetConfig(router_config &config)
{
    config = m_config;
}

void CXTRouter::ResCommand()
{
    command_manager_t::instance()->register_exit_cmd(
        "exit", 
        boost::bind(&CXTRouter::AppExit,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_PLAY, 
        boost::bind(&CXTRouter::Play,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_STOP, 
        boost::bind(&CXTRouter::Stop,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_STOP_All, 
        boost::bind(&CXTRouter::StopAll,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_SHOW_CONNECT_INFO, 
        boost::bind(&CXTRouter::ShowConnectInfo,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_HIDE_CONNECT_INFO, 
        boost::bind(&CXTRouter::HideConnectInfo,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_SHOW_PLAY_INFO,
        boost::bind(&CXTRouter::ShowPlayInfo,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_SHOW_KEY_INFO,
        boost::bind(&CXTRouter::ShowKeyInfo,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_HIDE_PLAY_INFO,
        boost::bind(&CXTRouter::HidePlayInfo,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_HELP,
        boost::bind(&CXTRouter::Help,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_SHOW_LOGIN_LONGOUT_INFO,
        boost::bind(&CXTRouter::ShowlogInfo,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_SHOW_LINK_SERVER_INFO,
        boost::bind(&CXTRouter::ShowLinkSeverEventInfo,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_CLEAR,
        boost::bind(&CXTRouter::Clear,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_SHOW_BASE_INFO,
        boost::bind(&CXTRouter::ShowBaseInfo,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "framework",
        boost::bind(&CXTRouter::QueryFrameworkTaskScheduler,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        COMMAND_LOG_ON_OFF,boost::bind(&CXTRouter::LogOnOff,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "xplay",boost::bind(&CXTRouter::XmppPlay,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "xstop",boost::bind(&CXTRouter::XmppStop,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "xreg",boost::bind(&CXTRouter::xregister,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "xregstop",boost::bind(&CXTRouter::xstop_register,this,_1, _2));

    command_manager_t::instance()->register_cmd(
        "ccs",boost::bind(&CXTRouter::get_center_name,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "loginccs",boost::bind(&CXTRouter::log_ccs,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "openr",boost::bind(&CXTRouter::openr,this,_1,_2));
    command_manager_t::instance()->register_cmd(
        "setsdpr",boost::bind(&CXTRouter::setsdpr,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "addsend",boost::bind(&CXTRouter::add_send,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "delsend",boost::bind(&CXTRouter::del_send,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "delsendall",boost::bind(&CXTRouter::del_send_all,this,_1, _2));

    command_manager_t::instance()->register_cmd(
        "closer",boost::bind(&CXTRouter::close_recv,this,_1,_2));

    command_manager_t::instance()->register_cmd(
        "getsndinf",boost::bind(&CXTRouter::getsndinf,this,_1,_2));

     command_manager_t::instance()->register_cmd(
         "getrecvinf",boost::bind(&CXTRouter::getrecvinf,this,_1,_2));

     command_manager_t::instance()->register_cmd(
         "gws",boost::bind(&CXTRouter::gws,this,_1,_2));

     command_manager_t::instance()->register_cmd(
         "recv",boost::bind(&CXTRouter::recv,this,_1,_2));

}

COMMAND_DISPATTCH_FUNCTION CXTRouter::close_recv(const command_argument_t& Args,std::string &result)
{
    dev_handle_t handle = Args.get<dev_handle_t>("handle",-1);
    media_device::stop_capture(handle);

//     char tids[16];
//     ::sprintf(tids,"ids%d",g_ids_index);
//      std::string dev_ids = Args.get< std::string>("ids",tids);
//      long dev_chanid = Args.get<long>("dbch",0);
//      long dev_strmtype = Args.get<long>("dbstream",0);
// 
//     //发送center停止指令
//    router_task_request_mgr::stop_play("cmd",dev_ids, dev_chanid,dev_strmtype);
// 
//     //关闭接收
//     XTEngine::instance()->clear_recv_link_mgr_buf(dev_ids,dev_chanid,dev_strmtype);
    std::ostringstream oss;
    oss<<"close_recv sucess!!"<<"dev_handle:"<<handle<< std::endl;
    result.clear();
    result.append(oss.str());
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::del_send(const command_argument_t& Args,std::string &result)
{
    int srcno             = Args.get<int>("srcno",0);
    int trackid           = Args.get<int>("trackid",0);
    std::string ip        = Args.get<std::string>("ip","172.16.5.231");
    unsigned short port    = Args.get<unsigned short>("port",16000);
    bool demux            = Args.get<bool>("demux",false); 
    unsigned int demuxid   = Args.get<unsigned int>("demuxid",0);

    int ret = media_server::del_send(srcno,trackid,ip.c_str(),port,demux,demuxid);
    std::cout<<"del_send_ret:"<<ret<<std::endl;
    std::ostringstream oss;
    oss<<"del_send_ret:" << ret << std::endl;
    result.clear();
    result.append(oss.str());
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::del_send_all(const command_argument_t& Args,std::string &result)
{
    media_server::del_send_all();
    std::ostringstream oss;
    oss<<"del_send_all sucess!!" << std::endl;
    result.clear();
    result.append(oss.str());
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::recv(const command_argument_t& Args,std::string&result)
{
    std::vector<_device_link> handles;
    media_device::_()->get_handle_all(handles);
    std::cout<<"--------------------------Recv Inf--------------------------------"<<std::endl;
    std::cout<<"Current Recv Total:"<<handles.size()<<std::endl;
    std::vector<_device_link>::iterator itr = handles.begin();
    for (;handles.end() != itr; ++itr)
    {
        std::cout<<"handle:"<<itr->link
        <<" |ids:"<<itr->ids
        <<" |dev_chanid:"<<itr->dev_chanid
        <<" |dev_strmtype:"<<itr->dev_strmtype
        <<" |active:"<<itr->active
        <<" |strmid:"<<itr->strmid<<std::endl;
    }
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::gws(const command_argument_t& Args,std::string&result)
{
    std::map<long,session_inf_t> session;
    gw_join_sip_session_mgr::_()->get_session_all(session);
    std::cout<<"--------------------------GW Session Inf--------------------------------"<<std::endl;
    std::cout<<"Current session Total:"<<session.size()<<std::endl;
    std::map<long,session_inf_t>::iterator itr = session.begin();
    for(;session.end() != itr; ++itr)
    {
        std::cout<<"sessionid:"<<itr->second.sessionid
            <<" |type:"<<itr->second.type
            <<" |recv_ids:"<<itr->second.recv_ids
            <<" |send_ids:"<<itr->second.send_ids
            <<" |stremtype:"<<itr->second.stremtype
            <<" |recv_chid:"<<itr->second.recv_chid
            <<" |send_chid:"<<itr->second.send_chid
            <<" |sdp_len:"<<itr->second.sdp_len
            <<" |transmitchannel:"<<itr->second.transmitchannel
            <<" |dev_handle:"<<itr->second.dev_handle
            <<" |srcno:"<<itr->second.srcno<<std::endl;
    }
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::getrecvinf(const command_argument_t&Args ,std::string&result)
{
    long ret_code = -1;
    char tids[16];
    sprintf(tids,"ids%d",g_ids_index);
    std::string dev_ids = Args.get< std::string>("ids",tids);
    long dev_chanid = Args.get<long>("dbch",0);
    long dev_strmtype = Args.get<long>("dbstream",0);
    int transch = Args.get<int>("ch",-1);

    src_info src;
    std::ostringstream oss;
    if (transch > -1)
    {
        ret_code = XTEngine::instance()->get_src_no(transch,src);
        if (ret_code < 0)
        {
            result = "XTRouter getrevinf get_src_no fail!";
            std::cerr<<"CXTRouter::getinfr get_src_no fail!"<<std::endl;
            return true;
        }

        dev_ids = src.device.dev_ids;
        dev_chanid = src.device.db_chanid;
        dev_strmtype = src.device.dev_strmtype;
        transch = src.srcno;
    }

    dev_handle_t link_handle = media_device::_()->find_link(dev_ids,dev_chanid,dev_strmtype);
    if (link_handle < 0)
    {
        link_handle = src.device.dev_handle;
    }
    if (link_handle < 0)
    {
        result = "find link_handle fail!";
        std::cout<<"未找到对应的设备句柄"<<std::endl;
        return true;
    }

    int track_num=MAX_TRACK;
    _RCVINFO rcv_inf[MAX_TRACK];
    std::string track_name;
    ret_code = XTEngine::instance()->rtp_get_rcv_inf(link_handle,rcv_inf,track_num);
    for (int tracktype=0; tracktype<track_num; ++tracktype)
    {
        switch(tracktype)
        {
        case 0:
            track_name = "video";
            break;

        case 1:
            track_name = "audio";
            break;
        default:
            break;
        }

        std::cout<<"srcno:"<<transch
            <<" track_name:"<<track_name
            <<" index:"<<rcv_inf[tracktype].index
            <<" rtp_port:"<<rcv_inf[tracktype].port_rtp
            <<" rtcp_port:"<<rcv_inf[tracktype].port_rtcp
            <<" demux:"<<rcv_inf[tracktype].demux
            <<" demuxid:"<<rcv_inf[tracktype].demuxid<<std::endl;
        

            oss<<"srcno:"<<transch
                <<" ,track_name:"<<track_name
                <<" ,index:"<<rcv_inf[tracktype].index
                <<" ,rtp_port:"<<rcv_inf[tracktype].port_rtp
                <<" ,rtcp_port:"<<rcv_inf[tracktype].port_rtcp
                <<" ,demux:"<<rcv_inf[tracktype].demux
                <<" ,demuxid:"<<rcv_inf[tracktype].demuxid<<std::endl;
            result.append(oss.str());
            oss.str("");
    }
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::getsndinf(const command_argument_t& Args,std::string&result)
{
    char tids[16];
    sprintf(tids,"ids%d",g_ids_index);
    std::string dev_ids = Args.get< std::string>("ids",tids);	
    long dev_chanid = Args.get<long>("dbch",0);		
    long dev_strmtype = Args.get<long>("dbstream",0);
    int transch = Args.get<int>("transch",-1);

    src_info src;
    long ret_code;
    if (transch < 0)
    {
        ret_code = XTEngine::instance()->get_src_ids(dev_ids,dev_chanid,dev_strmtype,src);
        if (ret_code < 0)
        {
            result = "get_src_ids fail";
            ERR_OUT<<"get_src_ids fail!"<<std::endl;
            return false;
        }
    }
    else
    {
        src.srcno = transch;
    }

    svr_info info[9]={0};
    int tracknum;
    ret_code = media_server::get_svr_info(info,tracknum,src.srcno);
    std::ostringstream oss;
    for (int index=0; index<tracknum; ++index)
    {
        //std::string trackname;
        //XTEngine::instance()->get_trank_name(src.srcno,info[index].trackid,trackname);

        oss.str("");
        std::cout<<" srcno:"<<src.srcno
            <<" trackid:"<<info[index].trackid
            <<" tranckname:"<<info[index].trackname
            <<" rtp_send_port:"<<info[index].rtp_send_port
            <<" rtcp_send_port:"<<info[index].rtcp_send_port
            <<" multiplex_s:"<<info[index].multiplex_s
            <<" multid_s:"<<info[index].multid_s
            <<std::endl;
        oss<<" ,srcno:"<<src.srcno
            <<", trackid:"<<info[index].trackid
            <<", tranckname:"<<info[index].trackname
            <<", rtp_send_port:"<<info[index].rtp_send_port
            <<", rtcp_send_port:"<<info[index].rtcp_send_port
            <<", multiplex_s:"<<info[index].multiplex_s
            <<", multid_s:"<<info[index].multid_s
            <<std::endl;
        result.append(oss.str());
    }

    std::string send_sdp;
    XTEngine::instance()->create_sdp_s(src.srcno,send_sdp);
    std::cout<<"send_sdp:\n"
        <<send_sdp<<std::endl;

    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::add_send(const command_argument_t& Args,std::string &result)
{
    //解析请求者SDP得到trackid
    int srcno = Args.get<int>("srcno",0);

    int trackid = Args.get<int>("trackid",0);

    std::string ip = Args.get<std::string>("ip","172.16.5.231");
    unsigned short port= Args.get<unsigned short>("port",16000);

    bool demux = Args.get<bool>("demux",false) ;
    unsigned int demuxid = Args.get<unsigned int>("demuxid",0);

    svr_info info[9]={0};
    int tracknum;
    int ret_code = media_server::get_svr_info(info,tracknum,srcno);

    std::ostringstream oss;
    for (int index=0; index<tracknum; ++index)
    {
        oss.str("");
        std::cout<<"trackid:"<<info[index].trackid
            <<" tranckname:"<<info[index].trackname
            <<" rtp_send_port:"<<info[index].rtp_send_port
            <<" rtcp_send_port:"<<info[index].rtcp_send_port
            <<" multiplex_s:"<<info[index].multiplex_s
            <<" multid_s:"<<info[index].multid_s
            <<std::endl;

        oss<<"trackid:"<<info[index].trackid
            <<" tranckname:"<<info[index].trackname
            <<" rtp_send_port:"<<info[index].rtp_send_port
            <<" rtcp_send_port:"<<info[index].rtcp_send_port
            <<" multiplex_s:"<<info[index].multiplex_s
            <<" multid_s:"<<info[index].multid_s
            <<std::endl;
        result.append(oss.str());
    }

    ret_code = media_server::add_send(srcno,trackid,ip.c_str(),port,demux,demuxid);

    return true;
} 

COMMAND_DISPATTCH_FUNCTION CXTRouter::openr(const command_argument_t& Args,std::string &result)
{
    long ret_code = -1;
    char tids[16];
    sprintf(tids,"ids%d",++g_ids_index);
    std::string dev_ids = Args.get< std::string>("ids",tids);
    long dev_chanid = Args.get<long>("dbch",0);
    long dev_strmtype = Args.get<long>("dbstream",0);
    bool demux = Args.get<bool>("demux",false);

    std::string sdp;
    dev_handle_t handle;
   ret_code = XTEngine::_()->sip_create_r(dev_ids,dev_chanid,dev_strmtype,demux,sdp,handle);
   if (ret_code < 0)
   {
       std::cout<<"sip_create_r fail! retcode:"<<ret_code<<std::endl;
   }
   else
   {
       std::cout<<"handle:"<<handle<<std::endl<<"recv sdp:"<<std::endl<<sdp<<std::endl;
   }
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::setsdpr(const command_argument_t& Args,std::string &result)
{
    long ret_code = -1;
    char tids[16];
    sprintf(tids,"ids%d",g_ids_index);
    std::string dev_ids = Args.get< std::string>("ids",tids);	
    long dev_chanid = Args.get<long>("dbch",0);		
    long dev_strmtype = Args.get<long>("dbstream",0);
    int track_num = Args.get<int>("track",2);
    bool demux = Args.get<bool>("demux",false);	
    std::string sdp = Args.get<std::string>("sdp",
        "v=0\n" 
        "o=center_sip_4_244 1 1 IN IP4 172.16.5.209\n" 
        "s=-\n" 
        "c=IN IP4 172.16.5.19\n"
        "b=AS:2048\n" 
        "t=0 0\n" 
        "a=range:npt=0-\n"
        "a=rtpport-mux\n" 
        "m=audio 20000 RTP/AVP 0\n"
        "a=muxid:7777\n"
        "c=IN IP4 172.16.5.19\n" 
        "a=rtpmap:0 PCMU/8000\n" 
        "a=sendonly\n"
        "m=video 20000 RTP/AVP 98\n"
        "a=muxid:7777\n"
        "c=IN IP4 172.16.5.19\n" 
        "b=TIAS:2048000\n" 
        "a=rtpmap:98 H264/90000\n" 
        "a=fmtp:98 profile-level-id=64001e;max-mbps=92160;max-fs=3072\n"
        "a=sendonly\n");

    /*
    "v=0\n"
    "o=- 1430622498429749 1 IN IP4 172.16.5.159\n"
    "s=RTP/RTCP stream from IPNC\n"
    "i=2?videoCodecType=H.264\n"
    "t=0 0\n"
    "a=tool:XTRouter Media v2015.05.20\n"
    "a=type:broadcast\n"
    "a=range:npt=0-\n"
    "a=x-qt-text-nam:RTP/RTCP stream from IPNC\n"
    "a=x-qt-text-inf:2?videoCodecType=H.264\n"
    "m=video 40008 RTP/AVP 96\n"
    "c=IN IP4 172.16.9.114\n"
    "a=rtpmap:96 H264/90000\n"
    "a=fmtp:96 profile-level-id=42e01e; packetization-mode=1\n"
    "a=TIAS:2000000\n"
    "a=sendonly"
    */

    int sdp_size=sdp.length();
    ret_code = XTEngine::instance()->save_sdp_to_access(dev_ids,dev_chanid,dev_strmtype,sdp.c_str(),sdp_size);

    //解析sdp
    xt_sdp::parse_buffer_t pb(sdp.c_str(), sdp_size); 
    xt_sdp::sdp_session_t xsdp;	
    try
    {
        xsdp.parse(pb);
    }
    catch(...)
    {
        ret_code = -3;
    }
    //启动center点播
    std::string play_ip = xsdp.origin().address();

    router_task_request_mgr::start_play("cmd",dev_ids,dev_chanid,dev_strmtype,"0.0.0.0",play_ip,
        dev_chanid,9,-1,6,"admin","12345",-1);

    std::ostringstream oss;
    result.clear();
    oss<<"parse command sucess" << std::endl;
    result.append(oss.str());
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::log_ccs(const command_argument_t& Args,std::string &result)
{
#ifdef _USE_XMPP_FUNC_
    if (xmpp_client::instance()->is_xmpp())
    {
        std::cerr<<"ccs:"<<xmpp_client::instance()->get_ccs_name()
            <<"\n login jid:"<<xmpp_client::instance()->get_local_jid()<<std::endl;
        
    }
    else
    {
        std::cout<<"log ccs..."<<std::endl;
        if (xmpp_ret_code::LONGIN_CENTER_SUCCESS 
            == xmpp_client::instance()->start_xmpp_client())
        {
            std::cout<<"log ccs sussces"<<std::endl;
        }
        else
        {
            std::cerr<<"log ccs fail!"<<std::endl;
        }

    }
#endif //#ifdef _USE_XMPP_FUNC_
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::get_center_name(const command_argument_t& Args,std::string &result)
{
#ifdef _USE_XMPP_FUNC_
    if (xmpp_client::instance()->is_xmpp())
    {
        std::cout<<"ccs:"<<xmpp_client::instance()->get_ccs_name()
            <<"\n login jid:"<<xmpp_client::instance()->get_local_jid()<<std::endl;
    }
    else
#endif //#ifdef _USE_XMPP_FUNC_
    {
        std::cout<<"no log in ccs!"<<std::endl;
    }

    return true;
}


void CXTRouter::CtrlMsgInit()
{
    if (use_jk())
    {
        pri_jk_engine::_()->init();
    }

#ifdef _USE_XMPP_FUNC_
    if (use_ccs())
    {
        //登录ccs
        printf("start xmpp...\n");
        //执行登录中心操作
        if (xmpp_ret_code::LONGIN_CENTER_SUCCESS 
            != xmpp_client::instance()->start_xmpp_client())
        {
            printf("start xmpp fail!\n");
        }
        else
        {
            printf("start xmpp success!\n");
        }
    }
#endif//#ifdef _USE_XMPP_FUNC_

#ifdef _USE_COMMON_CTRL_MSG_
    if (use_sc())
    {
        common_ctrl_msg_mgr::_()->init_ctrl_msg();
    }
#else 
    /*if (use_sc())
    {
        //std sip 注册
        std::cout<<"start sip sip..."<<std::endl;
        long iRet = std_sip_engine::instance()->std_sip_start();
        if (iRet< 0)
        {
            std::cout<<"start sip link srv fail ret_code:"<<iRet<<"!"<<std::endl;
        }
    }
	if (use_sc2())
	{
		std::cout<<"start sip svr"<<std::endl;
		long ret = sip_svr_engine::instance()->std_sip_start();
		if (ret<0)
		{
			std::cout<<"start sip svr fail:%d"<<ret<<std::endl;
		}
	}*/
#endif //#ifdef _USE_COMMON_CTRL_MSG_
}

void CXTRouter::CtrlMsgUninit()
{
    if (use_jk())
    {
        pri_jk_engine::_()->uninit();
    }

#ifdef _USE_XMPP_FUNC_
    if (use_ccs())
    {
        xmpp_client::instance()->stop_xmpp_client();
    }
#endif//#ifdef _USE_XMPP_FUNC_

#ifdef _USE_COMMON_CTRL_MSG_
    if (use_sc())
    {
        common_ctrl_msg_mgr::_()->uninit_ctrl_msg();
    }
#else 
    /*if (use_sc())
    {
        std::cout<<"stop std sip..."<<std::endl;
        std_sip_engine::_()->std_sip_stop();
    }*/
	if (use_sc2())
	{
		std::cout<<"stop sip svr"<<std::endl;
		sip_svr_engine::instance()->std_sip_stop();
	}
#endif //#ifdef _USE_COMMON_CTRL_MSG_

}

COMMAND_DISPATTCH_FUNCTION CXTRouter::LogOnOff(const command_argument_t& Args,std::string&result)
{
    bool bRet = true;

    int level =  Args.get< int>(ARG_ALL,0);

    if (level < 0)
    {
        set_log_on_off(false);
    }
    else
    {
        set_log_on_off(true);
		set_log_level((severity_level)level);
    }

    result = "log operator sucess";
    return bRet;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowBaseInfo(const command_argument_t& Args,std::string &result)
{
    bool bRet = true;

    std::string arg = Args.get<std::string>(0);
    if (0 == arg.compare(ARG_CENTER))
    {
        ShowCenterCommand(Args,result);
    }
    else if (0 == arg.compare(ARG_EXEC))
    {
        ShowSigalingExecRet(Args,result);
    }
    else if (0 == arg.compare(ARG_EXEC))
    {
        ShowResponseInfo(Args,result);
    }
    //收到中心信令
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT> listRealPlayEvent;
    CRealInfo::instance()->GetCenterDbCommandEventInfo(listRealPlayEvent);
    std::cout<<"收到中心信令 总量:"<<listRealPlayEvent.size()<<std::endl;

    std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>::iterator itr1 = listRealPlayEvent.begin();
    for(;listRealPlayEvent.end() != itr1; ++itr1)
    {
        std::cout<<itr1->GetStr()<<std::endl;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////

    //信令执行结果
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::list<info_mgr::INFO_SIGNALINGEXECRESULT> lstSigalingExecRuslut;
    CRealInfo::instance()->GetRealSigalingExecRet(lstSigalingExecRuslut);
    std::cout<<"信令执行结果 总量:"<<lstSigalingExecRuslut.size()<<std::endl;

    std::list<info_mgr::INFO_SIGNALINGEXECRESULT>::iterator itr2 = lstSigalingExecRuslut.begin();
    for(;lstSigalingExecRuslut.end() != itr2; ++itr2)
    {
        std::cout<<itr2->GetStr()<<std::endl;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////

    //向中心反馈
    ////////////////////////////////////////////////////////////////////////////////////////////////
    std::list<info_mgr::INFO_RESPONSETOCENTER> listRealResponseToCenterEvent;
    CRealInfo::instance()->GetRealResponseToCenterEventInfo(listRealResponseToCenterEvent);
    std::cout<<"向中心反馈 总量:"<<listRealResponseToCenterEvent.size()<<std::endl;

    std::list<info_mgr::INFO_RESPONSETOCENTER>::iterator itr = listRealResponseToCenterEvent.begin();
    for(;listRealResponseToCenterEvent.end() != itr; ++itr)
    {
        std::cout<<itr->GetStr()<<std::endl;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////

    return bRet;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowCenterCommand(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;

    do
    { 
        size_t uiCount = Args.count();
        if ( 0 == uiCount || 2 < uiCount )
        {
            break;
        }

        std::string strCommad;
        strCommad = Args.get<std::string>(1);


        if ( 0 == strCommad.compare(ARG_HISTORY))
        {
            std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT> lstSignalling;
            CHistoryInfo::instance()->GetPlayEventInfo(lstSignalling);
            std::cout<<"***********************最近"<<lstSignalling.size()
                <<"条中心信令***********************"<<std::endl;

            std::list<info_mgr::INFO_CENTERDBCOMMANDEVENT>::iterator itr = lstSignalling.begin();
            for (; lstSignalling.end() != itr; ++itr)
            {
                std::cout<<itr->GetStr()<<std::endl;
            }

        }
        else if ( 0 == strCommad.compare(ARG_REAL))
        {
        }
        else
        {
            break;
        }
        return bIsRet;

    } while(0);

    std::cout<<"命令参数错误:\n-h:查看历史信息"<<" -r:查看实时信息"<<std::endl;

    return bIsRet;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowSigalingExecRet(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;
    do
    { 
        size_t uiCount = Args.count();
        if ( 0 == uiCount || 2 < uiCount )
        {
            break;
        }

        std::string strCommad;
        strCommad = Args.get<std::string>(1);

        if ( 0 == strCommad.compare(ARG_HISTORY))
        {
            std::list<info_mgr::INFO_SIGNALINGEXECRESULT> lstSigalingExecRuslut;
            CHistoryInfo::instance()->GetSigalingExecRet(lstSigalingExecRuslut);

            std::cout<<"***********************最近"<<lstSigalingExecRuslut.size()
                <<"条中心信令执行结果***********************"<<std::endl;

            std::list<info_mgr::INFO_SIGNALINGEXECRESULT>::iterator itr = lstSigalingExecRuslut.begin();
            for (;lstSigalingExecRuslut.end() != itr; ++itr)
            {
                std::cout<<itr->GetStr()<<std::endl;
            }

        }
        else if ( 0 == strCommad.compare(ARG_REAL))
        {
        }
        else
        {
            break;
        }

        return bIsRet;

    } while(0);

    std::cout<<"命令参数错误:\n-h:查看历史信息"<<" -r:查看实时信息"<<std::endl;

    return bIsRet;

}

COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowResponseInfo(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;

    do
    { 
        size_t uiCount = Args.count();
        if ( 0 == uiCount || 2 < uiCount )
        {
            break;
        }

        std::string strCommad;
        strCommad = Args.get<std::string>(1);

        if ( 0 == strCommad.compare(ARG_HISTORY))
        {
            std::list<info_mgr::INFO_RESPONSETOCENTER> listResponseToCenterEvent;
            CHistoryInfo::instance()->GetResponseToCenterEvent(listResponseToCenterEvent);

            std::cout<<"***********************最近"<<listResponseToCenterEvent.size()
                <<"条反馈中心信息***********************"<<std::endl;

            std::list<info_mgr::INFO_RESPONSETOCENTER>::iterator itr = listResponseToCenterEvent.begin();
            for (; listResponseToCenterEvent.end() != itr; ++itr)
            {
                std::cout<<itr->GetStr()<<std::endl;
            }
        }
        else
        {
            break;
        }

        return bIsRet;

    } while(0);

    std::cout<<"命令参数错误:\n-h:查看历史信息"<<" -r:查看实时信息"<<std::endl;

    return bIsRet;

}

void CXTRouter::PrintSkiCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss <<COMMAND_SHOW_KEY_INFO << OPTION<<" = "<<VALUE <<"\n"
        << OPTION <<"\n"
        <<ARG_STRM_ID <<"         " << "stream_id of play channel " <<std::endl;
    oss<<"for example :ski strmid=0"<<"\n"<<std::endl;
    result.append(oss.str());
}

void CXTRouter::PrintSwiCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss <<COMMAND_SHOW_BASE_INFO << "   " << OPTION<<" = "<<VALUE <<"\n"
        << OPTION <<"\n"
        <<ARG_CENTER <<"        " <<DES_ARG_CENTER <<"\n"
        <<ARG_REAL <<"              " << DES_ARG_REAL <<"\n"
        <<ARG_HISTORY <<"              " << DES_ARG_HISTORY <<"\n"
        <<ARG_EXEC <<"          " << DES_ARG_EXEC <<"\n"
        <<ARG_REAL<<"              "<<DES_ARG_REAL<<"\n"
        <<ARG_HISTORY <<"              " << DES_ARG_HISTORY <<"\n"
        << ARG_RESP << "           " << DES_ARG_RESP << "\n"
        <<ARG_REAL<<"              "<<DES_ARG_REAL<<"\n"
        <<ARG_HISTORY<<"              "<<DES_ARG_HISTORY<<"\n"
        <<ARG_ALL<<"            " <<"all information"<<"\n" << std::endl;
    result.append(oss.str());
}

void CXTRouter::PrintGersndinfCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss <<"getsndinf" << "   " <<OPTION<<" = "<<VALUE <<std::endl;
    oss << OPTION <<std::endl;
    oss <<ARG_SEND_CH_ID <<"             " << "XTRouter play the channel " << std::endl;
    oss << std::endl;
    result.append(oss.str());

}

void CXTRouter::PrintGetrevinfCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss << "getrcevinf"<<"   " << OPTION<<" = "<<VALUE <<"\n"
        << OPTION <<"\n"
        <<ARG_SEND_CH_ID <<"             " << "XTRouter play the channel " << "\n"
        << std::endl;
    result.append(oss.str());
}

void CXTRouter::PrintStopCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss<<COMMAND_STOP_All<<std::endl;
    oss<<std::endl;
    oss <<COMMAND_STOP <<OPTION<<" = "<<VALUE<<std::endl;
    oss <<OPTION <<std::endl;
    oss <<ARG_SEND_CH_ID <<  "             " << "the channel of play device channel"<<std::endl;
    oss <<std::endl;
    oss << "for example: stop ch=0" <<std::endl;
    result.append(oss.str());

}

void CXTRouter::PrintSpiCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss <<COMMAND_SHOW_PLAY_INFO<<std::endl;
    oss << std::endl;
    result.append(oss.str());

}

void CXTRouter::PrintslsiCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss<<COMMAND_SHOW_LINK_SERVER_INFO<<"\n" << std::endl;
    result.append(oss.str());

}

void CXTRouter::PrintsliCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss<<COMMAND_SHOW_LOGIN_LONGOUT_INFO<<"\n" << std::endl;
    result.append(oss.str());
}

void CXTRouter::PrintclsComamndHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss<<COMMAND_CLEAR<<"\n" << std::endl;
    result.append(oss.str());

}

void CXTRouter::PrintScoiCommandHelp( std::string &result )
{
    std::ostringstream oss;
    oss.str("");
    oss<<FLG_1;
    oss<<COMMAND_SHOW_CONNECT_INFO<<std::endl;
    oss<<std::endl;
    result.append(oss.str());
}

void CXTRouter::PrintlogCommandHelp( std::string &result )
{
	std::ostringstream oss;
	oss.str("");
	oss<<FLG_1;
	oss<<COMMAND_LOG_ON_OFF<<"   " <<OPTION<<" = "<<VALUE<<"\n"
		<< OPTION << "\n"
		<< "1"<<"              "<<"the XTRouter log open"<<"\n"
		<< "0"<<"              "<<"the XTRouter log close"<<"\n"
		<< "4" << "              "<< "open log_info_level"<<"\n"
		<< "5" << "              "<< "open log_waring_level"<<"\n"
		<< "6" << "              "<< "open log_error_level"<<"\n"
		<<std::endl;
	result.append(oss.str());


}

void CXTRouter::PrintPlayCommandHelp(std::string &result)
{
	std::ostringstream oss;
	oss.str("");
	oss<<FLG_1;
	oss <<COMMAND_PLAY<<"   " << OPTION<<" = "<<VALUE<<"\n"
		<<OPTION<<"\n"
		<<ARG_IDS << "            " << "the ids of play device"<<"\n"
		<<ARG_DEV_CH_ID << "          " << "the devch of the device" << "\n"
		<<ARG_STRM_TYPE<< "          "<<"the device_stream of play device"<<"\n"
		<<ARG_DB_IP<< "             "<<"the ip of play device"<<"\n"
		<<ARG_DB_CH_ID<<"           " << "the channel of play device channel"<< "\n"
		<<ARG_DB_TYPE<<"         " << "the type of the device" <<"\n"
		<<ARG_LINK_TYPE <<"       " <<"link_type of the play device"<< "\n"
		<<ARG_DB_NUM <<"          " <<"play_num of the play device"<< "\n"
		<<ARG_SEND_CH_ID <<"             " <<"transmit channel of xtrouter"
        <<"port"<<"              "<<"play dst port init -1"
		<< std::endl;
	oss <<"for example:play ids=001 devch=0 stype=0 ip=172.16.2.113 dbch=0 dbtype=2 linktype=0 dbnum=1 sendch=-1"<<"\n"<<std::endl;

	result.append(oss.str());
}

COMMAND_DISPATTCH_FUNCTION  CXTRouter::Help(const command_argument_t& Args,std::string &result)
{
    bool bRet = true;
    std::cout<<"----------显示当前帮助信息----------"<<std::endl;
    result.clear();
    if (0 == Args.count())
    {
        PrintPlayCommandHelp(result);
        PrintStopCommandHelp(result);
        PrintSkiCommandHelp(result);
        PrintSpiCommandHelp(result);
        PrintlogCommandHelp(result);
        PrintGetrevinfCommandHelp(result);
        PrintGersndinfCommandHelp(result);
        PrintScoiCommandHelp(result);
        PrintSwiCommandHelp(result);
        PrintclsComamndHelp(result);
        PrintslsiCommandHelp(result);
        PrintsliCommandHelp(result);
        result.append("recv            show create recv info\n");
        result.append("gws             show gw session info\n");
    }
    else
    {
        std::string strCommad;
        strCommad = Args.get<std::string>(0);

        if (0 == strCommad.compare(COMMAND_PLAY))
        {
            PrintPlayCommandHelp(result);
        }
        else if (0 == strCommad.compare(COMMAND_STOP))
        {
            PrintStopCommandHelp(result);
        }
        else if (0 == strCommad.compare(COMMAND_LOG_ON_OFF))
        {
            PrintlogCommandHelp(result);
        }
    }
    std::cout<<result<<std::endl;
    return bRet;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::XmppPlay(const command_argument_t& Args,std::string &result)
{
#ifdef _USE_XMPP_FUNC_
    std::string ip = Args.get<std::string>("ip","172.16.3.228"); 
    int play_type = Args.get<int>("dbtype",172);
    int play_ch = Args.get<int>("dbch",0);
    int strame = Args.get<int>("stype",0);
    int trans_ch = Args.get<int>("sendch",-1);

    xmpp_client::instance()->test_play(ip,play_type,play_ch,strame,trans_ch);
#endif //#ifdef _USE_XMPP_FUNC_
    return true;
}
COMMAND_DISPATTCH_FUNCTION CXTRouter::xregister(const command_argument_t& Args,std::string &result)
{
#ifdef _USE_XMPP_FUNC_
    xmpp_client::instance()->test_play_inform_request();
#endif //#ifdef _USE_XMPP_FUNC_
    return true;
}
COMMAND_DISPATTCH_FUNCTION CXTRouter::xstop_register(const command_argument_t& Args,std::string &result)
{
#ifdef _USE_XMPP_FUNC_
    xmpp_client::instance()->test_stop_inform_request();
#endif //#ifdef _USE_XMPP_FUNC_
    return true;
}
COMMAND_DISPATTCH_FUNCTION CXTRouter::XmppStop(const command_argument_t& Args,std::string &result)
{
#ifdef _USE_XMPP_FUNC_
    std::string token = Args.get<std::string>("dbjid","src");
    long transmitChannel = Args.get<int>("dbch",0);;
    xmpp_client::instance()->test_stop(token,transmitChannel);
#endif //#ifdef _USE_XMPP_FUNC_
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::Play(const command_argument_t& Args,std::string &result)
{
    //设备IDS
    char tids[16];
    sprintf(tids,"ids%d",++g_ids_index);
    std::string dev_ids = Args.get<std::string>(ARG_IDS,tids); 

    //设备通道号
    long dev_chanid = Args.get<long>(ARG_DEV_CH_ID,0); 

    //设备码流类型
    long dev_strmtype = Args.get<long>(ARG_STRM_TYPE,0);

	//localip
	std::string localip = Args.get<std::string>("localip","0.0.0.0");

    //点播IP
    std::string db_url = Args.get<std::string>(ARG_DB_IP);

    //点播通道	
    long db_chanid = Args.get<long>(ARG_DB_CH_ID,0);

    //点播设备类型
    long db_type = Args.get<long>(ARG_DB_TYPE,9);

    //连接类型
    long link_type = Args.get<long>(ARG_LINK_TYPE,3);

    //批量点播数
    unsigned long db_num = Args.get<unsigned long>(ARG_DB_NUM,1);

    //指定转发服务通道号
    long trans_chanid = Args.get<long>(ARG_SEND_CH_ID,-1);

    long port = Args.get<long>("port",-1);

    if (db_num <= 1)
    {
        db_num = 1;
    }
    else if (db_num > m_config.chan_num)
    {
        db_num = m_config.chan_num;
    }

    int ret = 0;
    for (unsigned long num = 0;num < db_num;++num)
    {
        std::cout << "start play"<<" "
            << "devids:" << dev_ids.c_str() 
            << " devch:" << dev_chanid 
            << " devstrmtype:" << dev_strmtype 
            << " dbtype:" << db_type 
            << " linktype:" << link_type 
            << " dburl:" << db_url.c_str() 
            << " dbch:" << db_chanid 
            << " dbnum:" << db_num
            << " chanid:" << trans_chanid 
            << " port:"<<port<< std::endl;

        router_task_request_mgr::start_play("cmd",dev_ids,dev_chanid,dev_strmtype,localip,db_url,db_chanid,db_type,trans_chanid,link_type,"admin","12345",port);
		//boost::this_thread::sleep(boost::posix_time::millisec(1000));
        //还原转发服务通道
        trans_chanid=-1;
        ++db_chanid;
        ++dev_chanid;
    }

#ifdef USE_INFO_MGR_
    info_mgr::INFO_CENTERDBCOMMANDEVENT infoEvet;
    infoEvet.m_strTime = info_mgr::GetCurTime();
    infoEvet.m_DevIDS = dev_ids;
    infoEvet.m_lDevChid = dev_chanid;
    infoEvet.m_lDevStrmtype = dev_strmtype;
    infoEvet.m_DbIp = db_url;
    infoEvet.m_lDbChanid = db_chanid;
    infoEvet.m_lDevType = db_type;
    infoEvet.m_lChanid = trans_chanid;
    infoEvet.m_strDes = "点播指定源";
    CInfoMgr::instance()->PostEventInfo(info_mgr::INFO_CENTER_DB_COMMAND_EVENT,&infoEvet);
#endif //#ifdef USE_INFO_MGR_

    std::ostringstream oss;
    oss<<"parse command sucess" << std::endl;
    result.append(oss.str());
    return true;
}

COMMAND_DISPATTCH_FUNCTION  CXTRouter::StopAll(const command_argument_t& Args,std::string &result)
{
    std::ostringstream oss;
    router_task_request_mgr::stop_all();
#ifdef USE_INFO_MGR_
    info_mgr::INFO_CENTERDBCOMMANDEVENT infoEvet;
    infoEvet.m_strTime = info_mgr::GetCurTime();
    infoEvet.m_strDes = "停点所有";
    CInfoMgr::instance()->PostEventInfo(info_mgr::INFO_CENTER_DB_COMMAND_EVENT,&infoEvet);
#endif //#ifdef USE_INFO_MGR_

    result.clear();
    oss << "stopall success "<< std::endl;
    result.append(oss.str());
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::Stop(const command_argument_t& Args,std::string &result )
{
    //转发服务通道号
    long ch = Args.get<long>(ARG_SEND_CH_ID,-1);

    std::cout << "停止点播" << " chanid:" << ch << std::endl;

    router_task_request_mgr::stop_play(ch);

#ifdef USE_INFO_MGR_
    info_mgr::INFO_CENTERDBCOMMANDEVENT infoEvet;
    infoEvet.m_strTime = info_mgr::GetCurTime();
    infoEvet.m_DevIDS = "";
    infoEvet.m_lDevChid = 0;
    infoEvet.m_lDevStrmtype = 0;
    infoEvet.m_DbIp = "";
    infoEvet.m_lDbChanid = 0;
    infoEvet.m_lDevType = 0;
    infoEvet.m_lChanid = ch;
    infoEvet.m_strDes = "停点指定源";
    CInfoMgr::instance()->PostEventInfo(info_mgr::INFO_CENTER_DB_COMMAND_EVENT,&infoEvet);
#endif //#ifdef USE_INFO_MGR_

    result.clear();
    std::ostringstream oss;
    oss << "stop ch " << ch<< "success" <<std::endl;
    result.append(oss.str());
    return true;

}

COMMAND_DISPATTCH_FUNCTION  CXTRouter::HideConnectInfo(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;
    return bIsRet;

}

COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowConnectInfo(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;
    result.clear();
    std::list<info_mgr::INFO_TRANS> lstConnectInfo;
    CInfoMgr::instance()->GetConnectInfo(lstConnectInfo);
    std::cout<<"Current ConnectInfo Total:"<<lstConnectInfo.size()<<std::endl;
    std::list<info_mgr::INFO_TRANS>::iterator itr;
    std::ostringstream oss;
    oss.str("");
    oss<<"Current ConnectInfo Total:"<<lstConnectInfo.size()<<std::endl;
    result.append(oss.str());
    for(itr = lstConnectInfo.begin(); lstConnectInfo.end() != itr; ++itr)
    {
        oss.str("");
        std::cout<<"Srcno:"<<itr->srcno
            <<" |TransCh:"<<itr->m_lChID
            <<" |SendPort:"<<itr->m_lSendPort
            <<" |DstIP:"<<itr->m_pszDestIP
            <<" |DstPort:"<<itr->m_lDestPort
            <<" |Protocol:"<<itr->m_usProtocol
            /*<<" |SendPack:"<<itr->m_uiPackets*/
            <<" |SendBytes:"<<itr->m_uiOctets<<"(KB)"
            <<" |DestMuxid:"<<itr->m_uiDestMultid
            <<" |SendMuxid:"<<itr->m_uiSendMultid
			<<" |TotalLost:"<<itr->m_uiCumulativeLost
            <<" |Lost:"<<itr->m_uiFractionLost<<"%"
			<<" |rtt:"<<itr->m_urtt
            <<" |Jitter:"<<itr->m_uiJitter
            <<" |Time:"<<itr->m_pszCreateTime<<std::endl;
			
		 oss<<"Time:"<<itr->m_pszCreateTime 
			<<",Protocol:"<< itr->m_usProtocol
			<<",Srcno:"<< itr->srcno 
            <<",TransCh:"<< itr->m_lChID 
			<<",SendPort:"<< itr->m_lSendPort 
			<<",DstIP:"<< itr->m_pszDestIP
            <<",DstPort:"<< itr->m_lDestPort 
			/*<< ",SendPack:" << itr->m_uiPackets*/  
			<<",SendBytes:"<< itr->m_uiOctets << "(KB)" 
			<<",DestMuxid:"<<itr->m_uiDestMultid
            <<",SendMuxid:"<< itr->m_uiSendMultid 
			<<",TotalLost:"<<itr->m_uiCumulativeLost
			<<",rtt:"<<itr->m_urtt
			<<",Lost:"<< itr->m_uiFractionLost<< "%" 
			<<",Jitter:"<<itr->m_uiJitter<<std::endl;
        result.append(oss.str());
    }
    return bIsRet;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowPlayInfo(const command_argument_t& Args, std::string &result)
{
    bool bIsRet = true;
    std::list<info_mgr::INFO_PLAY> lstPlayInfo;
    CInfoMgr::instance()->GetPlayInfo(lstPlayInfo);
    result.clear();
    std::ostringstream oss;
    oss.str("");
    oss<<"Current PlayInfo Total:"<<lstPlayInfo.size() << std::endl;
    result.append(oss.str());

    for (std::list<info_mgr::INFO_PLAY>::iterator itr = lstPlayInfo.begin(); lstPlayInfo.end() != itr; ++itr)
    {
        oss.str("");
        oss<<"FN:"<<itr->m_lFrames
            <<" |TransCh:"<<itr->m_iTransChannel
            <<" |Srcno:"<< itr->m_Srcno
            <<" |StreamId:"<<itr->m_lStreamId
            <<" |Handle:"<<itr->m_Dev_handle
            <<" |IP:"<<itr->m_sDBUrl
            <<" |IDS:"<<itr->m_sDevIDS 
            <<" |DevCh:"<<itr->m_lDevCh
            <<" |DevType:"<<itr->m_iType
            <<" |StreamType:"<<itr->m_lDevStrmTyp
            <<" |Port:"<<itr->m_lPort
            <<" |PlayCh:"<<itr->m_lDBCh
            <<" |StreamNum:"<<itr->m_iTracknum
            <<" |SdpSize:"<<itr->m_lKey
            <<" |Time:"<<itr->m_strBiuldTime<<std::endl;
        result.append(oss.str());
    }
    std::cout<<result;
    return bIsRet;
}
COMMAND_DISPATTCH_FUNCTION  CXTRouter::HidePlayInfo(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;
    return bIsRet;

}

COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowKeyInfo(const command_argument_t& Args, std::string& result)
{
    bool bIsRet = true;
    result.clear();
    std::ostringstream oss;
    oss.str("");
    do 
    {
        long strmid = Args.get<long>(ARG_STRM_ID, -1);
        if (strmid < 0)
        {
            oss<<  "error arg  " << std::endl;
            result.append(oss.str());
            break;
        }

        char key[MAX_KEY_SIZE] = "";
        long len = MAX_KEY_SIZE;
        int ret = XTEngine::instance()->get_key(strmid, key, len);
        if (ret<0)
        {
            oss <<  "error streamflg " << std::endl;
             result.append(oss.str());
            break;
        }
        oss<<"***********************"<<DES_COMMAND_SHOW_KEY_INFO<<"***********************"<<std::endl;
        oss << key << std::endl;
        result.append(oss.str());
        std::cout<<result;
    } while (0);

    return bIsRet;
}

//登录登出中心 信息查看
COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowlogInfo(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;
    do 
    {
        std::cout<<"***********************"<<DES_COMMAND_SHOW_LOGIN_LONGOUT_INFO<<"***********************"<<std::endl;
        std::list<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT> lstLoginOrLogoutCenterEvent;
        CHistoryInfo::instance()->GetLoginOrLogoutCenterEvent(lstLoginOrLogoutCenterEvent);
        std::list<info_mgr::INFO_LOGINORLOGOUTCENTEREVNT>::iterator itrinfo = lstLoginOrLogoutCenterEvent.begin();
        for (; lstLoginOrLogoutCenterEvent.end() != itrinfo; ++itrinfo)
        {
            std::cout<<itrinfo->GetStr()<<std::endl;
        }
    } while (0);

    return bIsRet;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::ShowLinkSeverEventInfo(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;

    std::list<info_mgr::INFO_LINKSERVEREVENT> listLinkSeverEvent;
    CHistoryInfo::instance()->GetLinkSeverEventInfo(listLinkSeverEvent);

    std::cout<<"***********************最近"<<listLinkSeverEvent.size()
        <<"条LinkServer信息***********************"<<std::endl;

    std::list<info_mgr::INFO_LINKSERVEREVENT>::iterator itr = listLinkSeverEvent.begin();
    for (; listLinkSeverEvent.end() != itr;++itr)
    {
        std::cout<<itr->GetStr()<<std::endl;
    }
    return bIsRet;
}


COMMAND_DISPATTCH_FUNCTION CXTRouter::Clear(const command_argument_t& Args,std::string &result)
{
    bool bIsRet = true;
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif //#ifdef _WIN32
    return bIsRet;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::AppExit(const command_argument_t&,std::string &result)
{
    return true;
}

COMMAND_DISPATTCH_FUNCTION CXTRouter::QueryFrameworkTaskScheduler(const command_argument_t& args, std::string& result)
{
    //without arguments
    if (0 != args.count())
    {
        return false;
    }

    std::ostringstream os;
    os.str("");
    result.clear();
    framework::task_sched_info_type info;
    if (!framework::task_base::query_task_sched_info(info))
    {
        std::cout << "query task scheduler not supported" << std::endl;
        os << "query task scheduler not supported" << std::endl;
        result.append(os.str());
        return true;
    }

    static const char *const task_thread_status_strings[] = 
    {
        "valid",
        "new",
        "thread_begin",
        "waiting",
        "task_begin",
        "task_end",
        "thread_end",
        "destroy"
    };

    std::cout << "task scheduler size:" << info.threads_info.size() << std::endl;

    os << "task scheduler size:" << info.threads_info.size() << std::endl;
    result.append(os.str());
   
    typedef framework::task_sched_info_type::task_thread_info_container_type thread_info_container_type;
    thread_info_container_type &threads_info = info.threads_info;
    for (thread_info_container_type::size_type index = 0; index < info.threads_info.size(); ++index)
    {
         os.str("");
        std::ostringstream oss;  
#ifdef _WIN32
        std::tm t;
        ::localtime_s(&t, &threads_info[index].point);
        oss << "<" 
            << t.tm_mon << "-" << t.tm_mday << " " << t.tm_hour << ":" << t.tm_min << ":" << t.tm_sec
            << ">";
#else
        time_t timep;
        struct tm *p;
        time(&timep); 
        p = localtime(&timep); 

        oss << "<" 
            << 1 + p->tm_mon << "-" << p->tm_mday << " " << p->tm_hour << ":" << p->tm_min<< ":" << p->tm_sec
            << ">";
#endif

        std::cout << " [" << index << "]tid:" << threads_info[index].tid 
            << ",status:" << task_thread_status_strings[threads_info[index].status] 
        << ",point:" << oss.str()
            << ",deferred_task_num:" << threads_info[index].deferred_task_num
            << ",immediate_task_num:" << threads_info[index].immediate_task_num << std::endl;

        os << " [" << index << "]tid:" << threads_info[index].tid 
            << ",status:" << task_thread_status_strings[threads_info[index].status] 
        << ",point:" << oss.str()
            << ",deferred_task_num:" << threads_info[index].deferred_task_num
            << ",immediate_task_num:" << threads_info[index].immediate_task_num << std::endl;
        
        result.append(os.str());
    }
    return true;
}
