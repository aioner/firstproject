#include "router_task.h"
#include "media_server.h"
#include <stdarg.h>
#include "xtXml.h"
#include <base64/base64.h>
#include "XTRouter.h"
#include "gw_join_sip_session_mgr.h"
#include "XTRouterLog.h"
#include "pri_jk_engine.h"

#define ROUTER_TASK_LOG_BUF_LEN  1024
static void (*gs_pn_log_entry)(const char *) = NULL;
const unsigned char g_sdp_hc[] = {0x34, 0x48, 0x4b, 0x48, 0xfe, 0xb3, 0xd0, 0xd6, 0x08, 0x03, 
                                  0x04, 0x20, 0x00, 0x00, 0x00, 0x00, 0x03, 0x10, 0x01, 0x10, 
                                  0x01, 0x10, 0x10, 0x00, 0x80, 0x3e, 0x00, 0x00, 0xc0, 0x02, 
                                  0x40, 0x02, 0x11, 0x10, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00};


void router_task_request_mgr::start_play(const std::string& ondb_sn,const std::string &ids, long chanid, long strmtype,
										 const std::string &localip,
										 const std::string &db_url, long db_chanid, long db_type, long chanid2, long link_type,
                                         const std::string login_name /*="admin"*/ ,const std::string login_password /*="12345"*/, const long login_port /*= -1*/)
{
    start_play_router_task *request_task = new start_play_router_task(ondb_sn,ids, chanid, strmtype, 
		localip,db_url, db_chanid, 
        db_type, chanid2, link_type,login_name,login_password,login_port);
    request_task->request_event();
}

void router_task_request_mgr::update_ids(const std::string& ids, long chanid, long strmtype, 
                                         const std::string& new_ids, long new_chaid)
{
    update_ids_router_task *request_task = new update_ids_router_task(ids, chanid, strmtype, new_ids, new_chaid);
    request_task->request_event();
}

void router_task_request_mgr::stop_play(const std::string& ondb_sn,const std::string& ids, long chanid, long strmtype)
{
    stop_play_router_task *request_task = new stop_play_router_task(ondb_sn,ids, chanid, strmtype);
    request_task->request_event();
}

void router_task_request_mgr::stop_play(const std::string& ids)
{
    stop_play_router_task2 *request_task = new stop_play_router_task2(ids);
    request_task->request_event();
}

void router_task_request_mgr::stop_play(const long chanid)
{
    stop_play_router_task3 *request_task = new stop_play_router_task3(chanid);
    request_task->request_event();
}

void router_task_request_mgr::stop_all()
{
    stop_all_router_task *request_task = new stop_all_router_task;
    request_task->request_event();
}

// void router_task_request_mgr::error_to_center(const std::string& ondb_sn,const std::string&ids, long chanid,  long stream_type,
//                                               long ctl_id, long err_id)
// {
//     result_to_center_router_task *request_task = new result_to_center_router_task(ondb_sn,-1,-1,ids, chanid, stream_type,
//         -1, -1, "", "", -1, -1, -1, ctl_id, err_id);
//     request_task->request_event();
// }

void router_task_request_mgr::reponse_play_fail_to_center(const std::string& ondb_sn,const std::string&ids, long chanid, long stream_type)
{
    result_to_center_router_task *request_task = new result_to_center_router_task(ondb_sn,-1,-1,ids, chanid, stream_type,
        -1, -1, "", "", -1, -1, -1, OP_PLAY,ERR_PLAY_FAIL);
    if (NULL != request_task)
    {
        request_task->request_event();
    }
}
void router_task_request_mgr::reponse_stop_reuslt_to_center(const std::string& ondb_sn,const std::string&ids, long chanid, long stream_type)
{
    result_to_center_router_task *request_task = new result_to_center_router_task(ondb_sn,-1,-1,ids, chanid, stream_type,
        -1, -1, "", "", -1, -1, -1, OP_STOP,SUCCESS_STOP_OK);
    if (NULL != request_task)
    {
        request_task->request_event();
    }
}
void router_task_request_mgr::response_to_center(const std::string& ondb_sn,const dev_handle_t dev_handle,const int srcno,const std::string&ids, long chanid,  long stream_type, 
                                                 long server_id, const std::string&ip, long playid, long dvrtype,
                                                 long link_type, long server_type,const long ctl_id/* = OP_PLAY*/)
{
    result_to_center_router_task *request_task = new result_to_center_router_task(ondb_sn,dev_handle,srcno,ids, chanid, stream_type, 
        server_id, server_type, "", ip, playid, dvrtype, link_type, ctl_id, SUCCESS_PLAY_OK);
    request_task->request_event();
}
void router_task_request_mgr::response_to_center_v1(const std::string& ondb_sn,const dev_handle_t dev_handle,const int srcno,const std::string&ids, long chanid,  long stream_type, 
                                                 long transmit_ch, const std::string&ip, long playid, long dvrtype,
                                                 long link_type, long server_type)
{
    result_to_center_router_v1_task *request_task = new result_to_center_router_v1_task(ondb_sn,dev_handle,srcno,ids, chanid, stream_type, 
        transmit_ch, server_type, "", ip, playid, dvrtype, link_type, -1, -1);
    request_task->request_event();
}

int router_task_request_mgr::regist(const std::string ids, const std::string local_ip, unsigned short local_port,
                                    const std::string server_ip, unsigned short server_port)
{
    regist_task *request_task = new regist_task(ids, local_ip, local_port, server_ip, server_port);
    request_task->request_event();

    return 0;
}

void router_task_request_mgr::device_logout_pro(const std::string& ids)
{
    dev_logout_pro_task* ptr_task = new dev_logout_pro_task(ids);
    if (NULL != ptr_task)
    {
        ptr_task->request_event();
    }
}

//透传指令处理
void  router_task_request_mgr::transparent_cmd_pro(const std::string& ids, const std::string& cmd)
{
    do 
    {
        std::string ids_(ids);
        //解析xml
        xtXml xml;
        xml.LoadXMLStr(cmd.c_str());
        xtXmlNodePtr gwmsg = xml.getRoot();
        if (gwmsg.IsNull())
        {
            break;
        }

        //命令ID唯一
        long cmdid = ::str_to_num<long>(gwmsg.GetAttribute("id"));
        switch (cmdid)
        {

            //通知交换以SIP模式进行点播，要求接收/发送ip/port
        case GW_JOIN_SIP_PLAY_CMD:
            {
                sip_play_msg msg;
                msg.sender_ids     = ids_;
                msg.cmdid          = cmdid;
                msg.type           = ::str_to_num<int>(gwmsg.GetAttribute("type"));
                msg.sessionid       = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                msg.recvids         = gwmsg.GetAttribute("recvids"); 
                msg.recvmdiaChannel  = ::str_to_num<int>(gwmsg.GetAttribute("recvmdiaChannel")); 
                msg.sendids         = gwmsg.GetAttribute("sendids");  
                msg.sendmediaChannel = ::str_to_num<int>(gwmsg.GetAttribute("sendmediaChannel"));
                msg.streamtype     = ::str_to_num<int>(gwmsg.GetAttribute("streamtype"));
                msg.guid = gwmsg.GetAttribute("guid");
                switch(msg.type)
                {
                case MSG_SIP_2_SIP_OPEN_CALL:
                case MSG_SIP_2_SIP_PLAY:
                    {
                        msg.transmitchannel = ::str_to_num<int>(gwmsg.GetAttribute("transmitchannel"));
                        break;
                    }
                default:
                    {
                        msg.transmitchannel = -1;

                    }
                }
                gw_join_sip_play_pro_task* task = new gw_join_sip_play_pro_task(msg);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

            //收到对应会话的SDP
        case GW_JOIN_SIP_SDP_CMD:
            {
                sip_sdp_msg msg;
                msg.sender_ids = ids_;
                msg.cmdid = cmdid;
                msg.sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));

                xtXmlNodePtr psdp = xml.getNode("sdp");
                if (psdp.IsNull())
                {
                    break;
                }

                msg.sdp = Base64::decode64(psdp.GetValue());

                if (msg.sdp.empty())
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"transparent_cmd_pro |sdp empty! msg.sessionid[%d]",msg.sessionid);
                    break;
                }
                msg.sdp_len = msg.sdp.length();
                gw_join_sip_sdp_pro_task * task = new gw_join_sip_sdp_pro_task(msg);
                if (NULL != task)
                {
                    task->request_event();
                }

                break;
            }

            //告知交换关闭接收媒体流
        case GW_JOIN_SIP_CLOSE_RECV_CMD:
            {
                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                gw_join_sip_close_recv_task* task = new gw_join_sip_close_recv_task(sessionid,ids_);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

        case GW_JOIN_SIP_CLOSE_SEND_CMD:
            {
                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                gw_join_sip_close_send_task* task = new gw_join_sip_close_send_task(sessionid,ids_);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

            //告知交换推送媒体流
        case GW_JOIN_SIP_ADD_SEND_CMD:
            {
                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                std::string guid = gwmsg.GetAttribute("guid");
                gw_join_sip_add_send_task* task = new gw_join_sip_add_send_task(sessionid,ids_, guid);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

        case GW_JOIN_SIP_CLEAR_SESSION_CMD:
            {
                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                gw_join_sip_clear_session_pro_task *task = new gw_join_sip_clear_session_pro_task(sessionid,ids_);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

        case GW_JOIN_SIP_CLEAR_TRANS_CH:
            {

                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                gw_join_sip_clear_trans_ch_task* task = new gw_join_sip_clear_trans_ch_task(sessionid,ids_);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }
        default:
            {
                break;
            }
        }

    } while (0);
    //     gw_join_sip_parse_join_gateway_xml_cmd_task *task = new gw_join_sip_parse_join_gateway_xml_cmd_task(ids,cmd);
    //     if (task)
    //     {
    //         task->request_event();
    //     }
}

void router_task_request_mgr::set_log(void (*fn)(const char *))
{
    gs_pn_log_entry = fn;
}

void router_task_request_mgr::log(const char *fmt, ...)
{
    char log_buf[ROUTER_TASK_LOG_BUF_LEN]={0};
    va_list args;
    va_start(args, fmt);
    vsprintf(log_buf, fmt, args);
    va_end(args);
    log_buf[ROUTER_TASK_LOG_BUF_LEN - 1] = 0;
    DEBUG_LOG(NULL,ll_info,"%s",log_buf);
}

void router_task_request_mgr::gw_join_sip_respond_fail_to_gateway(const long sessionid,const std::string& ids,const std::string &guid,int errocode,const std::string& errorreasion)
{
    /*
    <!—当向交换要sdp时，交换处理异常回复-- >
    <gwmsg id=”920008-命令ID唯一” 
    sessionid=”会话ID”
    errorcode=”-2”
    errorreasion=”创建通道失败”> 
    </gwmsg>
    */
    xtXml xml;
    xml.LoadXMLStr("<gwmsg></gwmsg>");
    xtXmlNodePtr gwmsg = xml.getRoot();
    if (gwmsg.IsNull())
    {
        return;
    }
    gwmsg.SetAttribute("id",GW_JOIN_SIP_OPERATE_FAIL); 
    gwmsg.SetAttribute("sessionid",sessionid);
    gwmsg.SetAttribute("guid",guid);
    gwmsg.SetAttribute("errorcode",errocode);
    gwmsg.SetAttribute("errorreasion",errorreasion);

    std::string send_msg = xml.GetXMLStrEx();
    int msg_size = send_msg.length(); 

    pri_jk_engine::_()->send_transparent_cmd_to_center(ids, "", send_msg);
    DEBUG_LOG(DBLOGINFO,ll_error,"ERR sessionid[%d] %s",sessionid,errorreasion.c_str());

}
void router_task_request_mgr::gw_join_sip_respond_success_to_gateway(const long sessionid,const std::string& ids,const std::string &guid)
{
    /*
    <!—根据会话ID，交换通道OK反馈-- >
    <gwmsg id=”920009-命令ID唯一” 
    sessionid=”会话ID”> 
    </gwmsg>
    */
    xtXml xml;
    xml.LoadXMLStr("<gwmsg></gwmsg>");
    xtXmlNodePtr gwmsg = xml.getRoot();
    if (gwmsg.IsNull())
    {
        return;
    }
    gwmsg.SetAttribute("id",GW_JOIN_SIP_OPERATE_SUCCESS); 
    gwmsg.SetAttribute("sessionid",sessionid);
    gwmsg.SetAttribute("guid",guid);
    std::string send_msg = xml.GetXMLStrEx();
    int msg_size = send_msg.length();
    pri_jk_engine::_()->send_transparent_cmd_to_center(ids, "", send_msg);

    DEBUG_LOG(DBLOGINFO,ll_info,"respond to gateway transch ok! sessionid[%d]",sessionid);
}
void router_task_request_mgr::gw_join_sip_respond_trans_sdp_to_gateway(const long sessionid,const std::string& gateway_ids,const std::string &guid,const std::string& sdp)
{
    //透传sdp
    xtXml xml;
    xml.LoadXMLStr("<gwmsg></gwmsg>");
    xtXmlNodePtr gwmsg = xml.getRoot();
    if (gwmsg.IsNull())
    {
        return;
    }

    gwmsg.SetAttribute("id",GW_JOIN_SIP_ROUTER_SDP_CMD);

    gwmsg.SetAttribute("sessionid",sessionid);
    gwmsg.SetAttribute("guid",guid);

    xtXmlNodePtr psdp = gwmsg.NewChild("sdp");

    std::string str_ret = Base64::encode64(sdp).c_str();
    psdp.SetValue(str_ret.c_str());

    std::string send_msg = xml.GetXMLStrEx();
    int msg_size = send_msg.length();

    pri_jk_engine::_()->send_transparent_cmd_to_center(gateway_ids, "", send_msg);

    DEBUG_LOG(DBLOGINFO,ll_info,"sessionid[%d] respond -join gateway sdp,sdp_size[%d] sdp[%s]",sessionid,msg_size,Base64::decode64(str_ret).c_str());
}

uint32_t start_play_router_task::run()
{
#ifdef _USE_EXE_V1_
    int result = XTEngine::_()->start_play_v1(ondb_sn_,ids_, chanid_, strmtype_, db_url_, db_chanid_, db_type_,
        chanid2_, link_type_,login_name_,login_password_,login_port_);

    router_task_request_mgr::log("start play:ids(%s),chanid(%d),stream_type(%d),ip(%s),port(%d),result(%d)", 
        ids_.c_str(), chanid_, strmtype_, db_url_.c_str(), db_chanid_, result);

    if (result < 0)
    {
        router_task_request_mgr::error_to_center_v1(ondb_sn_,ids_, chanid_, strmtype_, OP_PLAY, ERR_PLAY_FAIL);
    }
#else
    int result = XTEngine::instance()->start_play(ondb_sn_,ids_, chanid_, strmtype_, localip_,db_url_, db_chanid_, db_type_,
        chanid2_, link_type_,login_name_,login_password_,login_port_);

    router_task_request_mgr::log("start play:ids(%s),chanid(%d),stream_type(%d),ip(%s),port(%d),result(%d)", 
        ids_.c_str(), chanid_, strmtype_, db_url_.c_str(), db_chanid_, result);

    if (result < 0)
    {
        router_task_request_mgr::reponse_play_fail_to_center(ondb_sn_,ids_, chanid_, strmtype_);
    }
#endif // end #ifdef _USE_EXE_V1_

    delete this;
    return 0;
}

uint32_t update_ids_router_task::run()
{
    int result = XTEngine::_()->update_ids(ids_, chanid_, strmtype_, new_ids_, new_chanid_);

    router_task_request_mgr::log("update ids:ids(%s),chanid(%d),stream_type(%d),new_ids(%s),new_chanid(%d),result(%d)", 
        ids_.c_str(), chanid_, strmtype_, new_ids_.c_str(), new_chanid_, result);

    delete this;
    return 0;
}

uint32_t stop_play_router_task::run()
{
    int result = XTEngine::_()->stop_play(ids_, chanid_, stream_type_);

    router_task_request_mgr::log("stop play:ids(%s),chanid(%d),stream_type(%d),result(%d)",
        ids_.c_str(), chanid_, stream_type_, result);

#ifdef _USE_EXE_V1_
    //停止点播向中心反馈点播失败 代替停点反馈
    router_task_request_mgr::error_to_center_v1(ondb_sn_,ids_, chanid_, stream_type_, OP_PLAY, SUCCESS_STOP_OK);
#else
    router_task_request_mgr::reponse_stop_reuslt_to_center(ondb_sn_,ids_, chanid_, stream_type_);
#endif // end #ifdef _USE_EXE_V1_

    delete this;
    return 0;
}

uint32_t stop_play_router_task2::run()
{
    int result = XTEngine::_()->stop_play_ids(ids_);

    router_task_request_mgr::log("stop play:ids(%s),result(%d)", ids_.c_str(), result);

    delete this;
    return 0;
}

uint32_t stop_play_router_task3::run()
{
    int result = XTEngine::_()->stop_play_trans_chid(chanid_);

    router_task_request_mgr::log("stop play:chanid(%d),result(%d)", chanid_, result);

    delete this;
    return 0;
}

uint32_t stop_play_router_by_strmid_task::run()
{
    //没有数先停点此路流
    int iRet = XTEngine::_()->stop_play_stramid(strmid_);
    DEBUG_LOG(DBLOGINFO,ll_info,"断线监测 停点uiStramId[%d]iRet[%d]",strmid_,iRet);

    delete this;
    return 0;
}
uint32_t stop_all_router_task::run()
{
    int result = XTEngine::_()->stop_allplay();

    router_task_request_mgr::log("stop all:result(%d)", result);

    delete this;
    return 0;
}

uint32_t create_transmit_src_task::run()
{
    long oper_code = -1;
    int srcno = -1;
    long chanid = -1;
    do 
    {
        if (device_.dev_handle < 0)
        {
            oper_code = -1;
            break;
        }

        //获取sdp
        char sdp[MAX_SDP_LEN]={0};
        long sdp_len = MAX_SDP_LEN;
        long data_type = -1;
        long get_sdp_ret = media_device::_()->get_sdp_by_handle(device_.dev_handle, sdp, sdp_len,data_type);
        if (get_sdp_ret < 0 || sdp_len < 1)
        {
            //等待sdp超时返回
            if (count_++ >= 300)
            {
                oper_code = -2;
                break;
            }
            //重新获取sdp
            get_sdp_ret = media_device::_()->get_sdp_by_handle(device_.dev_handle, sdp, sdp_len,data_type);
            if (get_sdp_ret < 0 || sdp_len <=0)
            {
                //失败重做
                return 10;
            }
        }

        //获取SDP解决媒体信息
        if (XTEngine::_()->cfg().snd_std_rtp)
        {
            //点播纯音频
            if (2 == device_.dev_strmtype)
            {
                std::string tmp;
                XTEngine::_()->del_m_of_sdp(sdp,sdp_len,"video",tmp);
                if (!tmp.empty())
                {
                    ::memset(sdp,0,sdp_len);
                    ::strncpy(sdp,tmp.c_str(),tmp.length());
                }
            }
            device_.tracknum = XTEngine::_()->parse_tracks_ex(sdp,sdp_len,device_.track_infos);
            if (device_.tracknum < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "response_center_result_task::run | parse_tracks_ex fail! |dev_ids[%s]dev_chid[%d]dev_strmtype[%d] dev_handle[%d] sdp[%s] sdp_len[%d]",
                    device_.dev_ids.c_str(),device_.dev_chanid,device_.dev_strmtype,device_.dev_handle,device_.key,device_.key_len);
                device_.tracknum = 0;
            }
        }

        //创建转发源
        int create_ret = - 1;
        int copy_send_num = XTEngine::_()->copy_send_num();
        if (copy_send_num > 1)
        {
            for (int nC = 0;nC < copy_send_num;++nC)
            {
                create_ret = XTEngine::_()->create_src_v1(device_, srcno);
                if (create_ret < 0)
                {
                    continue;
                }
            }
        }
        else
        {
            create_ret = XTEngine::_()->create_src_v1(device_, srcno);
            if (create_ret < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "response_center_result_task::run()| create_src_v1 创建转发源失败! ondb_sn_[%s] ids[%s] dev_chid[%d] dev_strmtype[%d]",
                    ondb_sn_.c_str(),device_.dev_ids.c_str(),device_.dev_chanid,device_.dev_strmtype);
                oper_code = -8;
                break;
            }
        }

        int updtae_ret = XTEngine::_()->save_sdp_to_srv(srcno,sdp,sdp_len,data_type);
        if (updtae_ret < 0 )
        {
            oper_code = -6;
            DEBUG_LOG(DBLOGINFO,ll_error,
                "response_center_result_task::run()| update_sdp 保存SDP到发送端失败! ondb_sn_[%s] ids[%s] update_sdp fail ondb_sn_[%s] ids[%s] dev_chid[%d]dev_strmtype[%d] sdp_len[%d]!",
                ondb_sn_.c_str(),device_.dev_ids.c_str(),device_.dev_chanid,device_.dev_strmtype,sdp_len);
            break;
        }

        int get_man_ch_ret = XTEngine::_()->get_main_chanid(srcno, chanid);
        if (get_man_ch_ret < 0)
        {
            oper_code = -7;
            break;
        }
        //成功完成所有操作
        oper_code = 1;
    } while (0);

    if (0 < oper_code)
    {
        //向中心反馈成功
        router_task_request_mgr::response_to_center_v1(ondb_sn_,device_.dev_handle, srcno, device_.dev_ids, device_.dev_chanid,
            device_.dev_strmtype, chanid, device_.db_url, device_.db_chanid, device_.db_type, device_.link_type, 9);
    }
    else
    {
        //查询是否SIP设备发过来的流已点播
        int ret = 0;
        if (!media_device::_()->is_md_handle(device_.dev_handle))
        {
            ret = XTEngine::_()->stop_capture(device_);
            if (ret < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,"create_transmit_src_task | stop_capture fail! ret[%d]",ret);
            }
        }
        ret = XTEngine::_()->destroy_src(srcno);
        if (ret < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"create_transmit_src_task | destroy_src fail! ret[%d]",ret);
        }
        //向中心反馈失败
        //router_task_request_mgr::error_to_center_v1(ondb_sn_,device_.dev_ids, srcno, device_.dev_strmtype, OP_PLAY, ERR_PLAY_FAIL);
    }
    delete this;
    return 0;
}

uint32_t result_to_center_router_v1_task::run()
{
    std::ostringstream os;
    os << stream_type_;
    other_info_ = os.str();

    //未链接中心不反馈
    if (CXTRouter::_()->use_jk())
    {
        if (-1 == transmit_ch_)
        {
            pri_jk_engine::_()->result_to_center(ids_, chanid_, -1, -1, other_info_,ondb_sn_);
            if (OP_STOP == ctl_id_)
            {
                //未获取到sdp不能进行转发操作清理转发资源
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "result_to_center_router_v1_task::run() 向中心反馈停止点播成功 执行完成 | ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d]!",
                    ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_);
            }
            else
            {
                if (ERR_PLAY_FAIL != err_id_)
                {
                    //未获取到sdp不能进行转发操作清理转发资源
                    DEBUG_LOG(DBLOGINFO,ll_error,
                        "result_to_center_router_v1_task::run() 向中心反馈点播失败 执行完成 | ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d] srcno[%d] transmit_ch[%d]!",
                        ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_,srcno_,transmit_ch_);
                }
                else
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,
                        "result_to_center_router_v1_task::run() 向中心反馈点播失败 执行完成 | ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d] srcno[%d] transmit_ch[%d]",
                        ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_,srcno_,transmit_ch_);
                }
            }
        }
        else
        {
            pri_jk_engine::_()->result_to_center(ids_, chanid_, transmit_ch_, result_type_, other_info_,ondb_sn_);
            DEBUG_LOG(DBLOGINFO,ll_error,
                "result_to_center_router_v1_task::run() 向中心反馈点播成功 执行完成 | ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d] srcno[%d] transmit_ch[%d] OnDb start play success!",
                ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_,srcno_,transmit_ch_);
        }
    }
    delete this;
    return 0;
}

uint32_t result_to_center_router_task::run()
{
    std::ostringstream os;
    os << stream_type_;
    other_info_ = os.str();
    long oper_code = -1;
    do 
    {
        if (-1 == server_id_)
        {
            break;
        }

        char sdp[MAX_SDP_LEN]={0};
        long sdp_len = MAX_SDP_LEN;
        long data_type = -1;
        long get_ret = media_device::_()->get_sdp_by_handle(dev_handle_,sdp,sdp_len,data_type);
        if (get_ret < 0 || sdp_len < 1)
        {
            //等待sdp返回
            if (count_++ >= 300)
            {
                WRITE_LOG(DBLOGINFO,ll_info,
                    "result_to_center_router_task::run|向中心反馈点播结果 获取sdp失败重试| count_[%d] ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d] srcno[%d] transmit_ch[%d]",
                    count_,ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_,srcno_,server_id_);
                if (DEV_HC == dvrtype_)
                {
                    sdp_len = sizeof(g_sdp_hc);
                    ::memcpy(sdp,g_sdp_hc,sdp_len);
                }
                else
                {
                    break;
                }
            }
            else
            {
                return 10;
            }
        }

        long dev_streamtype = XTEngine::_()->get_dev_stremtype_by_srcno(srcno_);
        if (2 == dev_streamtype)
        {
            std::string src_sdp;
            XTEngine::_()->del_m_of_sdp(sdp,sdp_len,"video",src_sdp);
            if (!src_sdp.empty())
            {
                ::memset(sdp,0,sdp_len);
                ::strncpy(sdp,src_sdp.c_str(),src_sdp.length());
                sdp_len = src_sdp.length();
            }
            else
            {
                DEBUG_LOG(DBLOGINFO,ll_info,
                    "result_to_center_router_task::run()| ondb_sn_[%s] ids[%s] sdp_len[%d] no std sdp no del_m_of_sdp trans pri stream!",ondb_sn_.c_str(),ids_.c_str(),sdp_len);
            }

        }
        int save_sdp_ret = XTEngine::_()->save_sdp_to_srv(srcno_,sdp,sdp_len,data_type);
        if (save_sdp_ret < 0 )
        {
            oper_code = -2;
            DEBUG_LOG(DBLOGINFO,ll_info,"result_to_center_router_task::run()| ondb_sn_[%s] ids[%s] update_sdp fail sdp_len[%d]!",ondb_sn_.c_str(),ids_.c_str(),sdp_len);
            break;
        }
        oper_code = 1;
    } while (0);

    if(oper_code < 0)
    {
        if (-1 != server_id_)
        {
            if (OP_PLAY == ctl_id_)
            {
                //接收和转发源创建成功但sdp获取失败！
                router_task_request_mgr::stop_play(ondb_sn_,ids_, chanid_,stream_type_);
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "result_to_center_router_task::run() Get dvice sdp fail, router stop and respones reuslt to center! | ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d] srcno[%d] transmit_ch[%d]!",
                    ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_,srcno_,server_id_);
            }
        }
        else
        {
            pri_jk_engine::_()->result_to_center(ids_, chanid_, -1, -1, other_info_,ondb_sn_);
            if (OP_STOP == ctl_id_)
            {
                //执行停止时的失败反馈
                DEBUG_LOG(DBLOGINFO,ll_error,"result_to_center_router_task::run() |Response stop success result to center,execute finish!| ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d]!",
                    ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_);
            }
            else if (OP_PLAY == ctl_id_)
            {
                //执行点播时的失败反馈 接收或转发源没有创建成功
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "result_to_center_router_task::run() Play fail! stop and response center stop result!| ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d] srcno[%d] transmit_ch[%d]!",
                    ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_,srcno_,server_id_);
            }
        }
    }
    else
    {
        //点播成功
        pri_jk_engine::_()->result_to_center(ids_, chanid_, server_id_, result_type_, other_info_,ondb_sn_);
        DEBUG_LOG(DBLOGINFO,ll_info,
            "result_to_center_router_task::run()| Play success! Responce success reuslt to center,execute finish!| ondb_sn_[%s] ids[%s] dev_ch[%d] dev_streamtype[%d] srcno[%d] transmit_ch[%d] OnDb start play success!",
            ondb_sn_.c_str(),ids_.c_str(),chanid_,stream_type_,srcno_,server_id_);
    }

    delete this;
    return 0;
}

uint32_t regist_task::run()
{
    std::string ids = pri_jk_engine::_()->get_local_ids();

    int ret = media_server::regist(ids, local_ip, local_port, server_ip, server_port);

    router_task_request_mgr::log("regist_task:ids(%s) lip(%s) lport(%d) sip(%s) sport(%d) ret(%d)", 
        ids.c_str(), local_ip.c_str(), local_port, server_ip.c_str(), server_port, ret);

    delete this;
    return 0;
}

//JK设备下线相关处理
uint32_t dev_logout_pro_task::run()
{
    //停点相关转发
    DEBUG_LOG(DBLOGINFO,ll_info,"dev_logout_pro_task stop play ids_[%s]",ids_.c_str());
    int ret = XTEngine::_()->pro_dev_logout(ids_);

    session_inf_t session;
    if ( 0 <= gw_join_sip_session_mgr::_()->get_session_by_ids(ids_,session))
    {
        if ( MSG_OPEN_RECV == session.type)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,"dev_logout_pro_task pro GW created recv info ids_[%s]",ids_.c_str());
            XTEngine::_()->rtp_close_recv(session.dev_handle);
        }
    }

    //清理网关对接时的相关管理
    DEBUG_LOG(DBLOGINFO,ll_info,"dev_logout_pro_task clear GW session ids_[%s]",ids_.c_str());
    gw_join_sip_session_mgr::_()->del_session_by_ids(ids_);
    delete this;
    return 0;
}

//sip对接
/////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t gw_join_sip_parse_join_gateway_xml_cmd_task::run()
{
    do 
    {
        //解析xml
        xtXml xml;
        xml.LoadXMLStr(cmd_.c_str());
        xtXmlNodePtr gwmsg = xml.getRoot();
        if (gwmsg.IsNull())
        {
            break;
        }

        //命令ID唯一
        long cmdid = ::str_to_num<long>(gwmsg.GetAttribute("id"));
        switch (cmdid)
        {

            //通知交换以SIP模式进行点播，要求接收/发送ip/port
        case GW_JOIN_SIP_PLAY_CMD:
            {
                sip_play_msg msg;
                msg.sender_ids     = ids_;
                msg.cmdid          = cmdid;
                msg.type           = ::str_to_num<int>(gwmsg.GetAttribute("type"));
                msg.sessionid       = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                msg.recvids         = gwmsg.GetAttribute("recvids"); 
                msg.recvmdiaChannel  = ::str_to_num<int>(gwmsg.GetAttribute("recvmdiaChannel")); 
                msg.sendids         = gwmsg.GetAttribute("sendids");  
                msg.sendmediaChannel = ::str_to_num<int>(gwmsg.GetAttribute("sendmediaChannel"));
                msg.streamtype     = ::str_to_num<int>(gwmsg.GetAttribute("streamtype"));
                msg.guid = gwmsg.GetAttribute("guid");
                switch(msg.type)
                {
                case MSG_SIP_2_SIP_OPEN_CALL:
                case MSG_SIP_2_SIP_PLAY:
                    {
                        msg.transmitchannel = ::str_to_num<int>(gwmsg.GetAttribute("transmitchannel"));
                        break;
                    }
                default:
                    {
                        msg.transmitchannel = -1;

                    }
                }
                gw_join_sip_play_pro_task* task = new gw_join_sip_play_pro_task(msg);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

            //收到对应会话的SDP
        case GW_JOIN_SIP_SDP_CMD:
            {
                sip_sdp_msg msg;
                msg.sender_ids = ids_;
                msg.cmdid = cmdid;
                msg.sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));

                xtXmlNodePtr psdp = xml.getNode("sdp");
                if (psdp.IsNull())
                {
                    break;
                }

                msg.sdp = Base64::decode64(psdp.GetValue());

                if (msg.sdp.empty())
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"transparent_cmd_pro |sdp empty! msg.sessionid[%d]",msg.sessionid);
                    break;
                }
                msg.sdp_len = msg.sdp.length();
                gw_join_sip_sdp_pro_task * task = new gw_join_sip_sdp_pro_task(msg);
                if (NULL != task)
                {
                    task->request_event();
                }

                break;
            }

            //告知交换关闭接收媒体流
        case GW_JOIN_SIP_CLOSE_RECV_CMD:
            {
                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                gw_join_sip_close_recv_task* task = new gw_join_sip_close_recv_task(sessionid,ids_);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

        case GW_JOIN_SIP_CLOSE_SEND_CMD:
            {
                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                gw_join_sip_close_send_task* task = new gw_join_sip_close_send_task(sessionid,ids_);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

            //告知交换推送媒体流
        case GW_JOIN_SIP_ADD_SEND_CMD:
            {
                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                std::string guid = gwmsg.GetAttribute("guid");
                gw_join_sip_add_send_task* task = new gw_join_sip_add_send_task(sessionid,ids_, guid);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

        case GW_JOIN_SIP_CLEAR_SESSION_CMD:
            {
                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                gw_join_sip_clear_session_pro_task *task = new gw_join_sip_clear_session_pro_task(sessionid,ids_);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }

        case GW_JOIN_SIP_CLEAR_TRANS_CH:
            {

                long sessionid = ::str_to_num<long>(gwmsg.GetAttribute("sessionid"));
                gw_join_sip_clear_trans_ch_task* task = new gw_join_sip_clear_trans_ch_task(sessionid,ids_);
                if (NULL != task)
                {
                    task->request_event();
                }
                break;
            }
        default:
            {
                break;
            }
        }

    } while (0);

    delete this;
    return 0;
}

uint32_t gw_join_sip_play_pro_task::run()
{
    std::string error_reasion;
    DEBUG_LOG(DBLOGINFO,ll_info,
        "gw_join_sip_play_pro_task::run() | sessionid[%d] cmid[%d] guid[%s] type[%d] recvids[%s] sendids[%s]",
        msg_.sessionid,msg_.cmdid,msg_.guid.c_str(),msg_.type,msg_.recvids.c_str(),msg_.sendids.c_str());
    long ret_code = 0;
    bool demux_r = CXTRouter::_()->get_demux_mode_r();
    switch(msg_.type)
    {
    case MSG_OPEN_RECV://0-接收端口(公司系统-外部)
        {
            if (msg_.recvids.empty())
            {
                DEBUG_LOG(DBLOGINFO,ll_info,
                    "gw_join_sip_play_pro_task::run() |ERR MSG_OPEN_RECV recv_ids is empty! sessionid[%d]",msg_.sessionid);
                ret_code = -1;
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_OPEN_RECV recv_ids is empty!");
                break;
            }
            session_inf_t session;
            session.sessionid = msg_.sessionid;
            session.recv_ids = msg_.recvids;
            session.stremtype = msg_.streamtype;
            session.type = msg_.type;
            session.recv_chid = msg_.recvmdiaChannel;
            long ret_code = gw_join_sip_session_mgr::_()->save_session(session);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run() |sessionid[%d] ret_code[%d]|save_session fail!",msg_.sessionid,ret_code);
                error_reasion.assign("gw_join_sip_play_pro_task::run() MSG_OPEN_RECV save_session fail!");
                ret_code = -2;
                break;
            }

            dev_handle_t dev_handle = -1;
            ret_code = XTEngine::instance()->sip_create_r(msg_.recvids,msg_.recvmdiaChannel,msg_.streamtype,demux_r,sdp_,dev_handle);

            DEBUG_LOG(DBLOGINFO,ll_info,
                "gw_join_sip_play_pro_task::run |sip_create_r sessionid[%d] sip_create_r cmdid[%d] recvids[%s] ch[%d] streamtype[%d] dev_handle[%d] sip_create_r ret[%d]",
                msg_.sessionid, msg_.cmdid, msg_.recvids.c_str(), msg_.recvmdiaChannel, msg_.streamtype,dev_handle,ret_code);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run() |sessionid[%d] ret_code[%d] |sip_create_r fail!",msg_.sessionid,ret_code);
                error_reasion.assign("gw_join_sip_play_pro_task::run() MSG_OPEN_RECV sip_create_r fail!");
                ret_code = -1;
                break;
            }

            session.dev_handle = dev_handle;
            gw_join_sip_session_mgr::_()->update_session(session);
            break;
        }
    case MSG_OPEN_SEND://1-发送端口(外部系统-公司系统)
        {
            if (msg_.sendids.empty())
            {
                DEBUG_LOG(DBLOGINFO,ll_info,
                    "gw_join_sip_play_pro_task::run() | ERR MSG_OPEN_SEND sendids is empty! sessionid[%d]",msg_.sessionid);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_OPEN_SEND sendids is empty");
                ret_code = -1;
                break;
            }
            session_inf_t session;
            session.sessionid = msg_.sessionid;
            session.send_ids = msg_.sendids;
            session.stremtype = msg_.streamtype;
            session.type = msg_.type;
            session.send_chid = msg_.sendmediaChannel;
            long ret_code = gw_join_sip_session_mgr::_()->save_session(session);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run() |sessionid[%d] ret_code[%d] |2.save_session fail!",msg_.sessionid,ret_code);
                ret_code = -2;
                error_reasion.assign("gw_join_sip_play_pro_task::run() MSG_OPEN_SEND save_session fail!");
                break;
            }

            dev_handle_t dev_handle = -1;
            int srcno = -1;
            ret_code = XTEngine::instance()->gw_join_sip_create_s( msg_.sendids,msg_.sendmediaChannel,msg_.streamtype,sdp_,srcno,dev_handle);

            DEBUG_LOG(DBLOGINFO,ll_info,
                "gw_join_sip_play_pro_task::run |gw_join_sip_create_s sessionid[%d] cmdid[%d] sendids[%s] ch[%d] streamtype[%d]执行结果[%d]",
                msg_.sessionid,msg_.cmdid,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype,ret_code);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run |sessionid[%d] ret_code[%d] |gw_join_sip_create_s fail!",msg_.sessionid,ret_code);
                error_reasion.assign("gw_join_sip_play_pro_task::run() MSG_OPEN_SEND gw_join_sip_create_s fail!");
                ret_code = -3;
                break;
            }

            session.srcno = srcno;
            gw_join_sip_session_mgr::_()->update_session(session);
            break;
        }

    case MSG_OPEN_CALL://2-发送/接收端口(呼叫-双向)
        {
            if (msg_.recvids.empty() || msg_.sendids.empty())
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run() |ERR MSG_OPEN_CALL ids is empty sessionid[%d]",msg_.sessionid);
                error_reasion.assign("gw_join_sip_play_pro_task::run() MSG_OPEN_CALL ids is empty");
                ret_code = -11;
                break;
            }

            session_inf_t session;
            session.recv_ids = msg_.recvids;
            session.recv_chid = msg_.recvmdiaChannel;
            session.send_ids = msg_.sendids;
            session.send_chid = msg_.sendmediaChannel;
            session.sessionid = msg_.sessionid;
            session.stremtype = msg_.streamtype;
            session.type = MSG_OPEN_CALL;
            long ret_code = gw_join_sip_session_mgr::_()->save_session(session);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run() |sessionid[%d] ret_code[%d] |MSG_OPEN_CALL save_session fail!",msg_.sessionid,ret_code);
                error_reasion.assign("gw_join_sip_play_pro_task::run() MSG_OPEN_CALL save_session fail");
                ret_code = -4;
                break;
            }

            ret_code = XTEngine::_()->gw_join_sip_create_rs(msg_.recvids,msg_.recvmdiaChannel,msg_.sendids,msg_.sendmediaChannel,msg_.streamtype,sdp_,demux_r);

            DEBUG_LOG(DBLOGINFO,ll_info,
                "gw_join_sip_play_pro_task::run |gw_join_sip_create_rs sessionid[%d] cmid[%d] gw_join_sip_create_rs recvids[%s] recvch[%d] sendids[%s] sendch[%d] streamtype[%d]执行结果[%d]",
                msg_.sessionid,msg_.cmdid,msg_.recvids.c_str(),msg_.recvmdiaChannel,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype,ret_code);

            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run |sessionid[%d] ret_code[%d] |gw_join_sip_create_rs fail!",msg_.sessionid,ret_code);
                error_reasion.assign("gw_join_sip_play_pro_task::run() MSG_OPEN_CALL gw_join_sip_create_rs fail");
                ret_code = -5;
                break;
            }
            break;
        }
    case MSG_SIP_2_SIP_PLAY://sip-sip 通知交换建立发送 此时中心告知转发通道 抛出sdp
        {
            if (msg_.sendids.empty())
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run() |ERR MSG_SIP_2_SIP_PLAY sendids is empty sessionid[%d]",msg_.sessionid);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_PLAY sendids is empty");
                ret_code = -13;
                break;
            }
            //保存会话
            session_inf_t session;
            session.recv_ids = msg_.recvids;
            session.recv_chid = msg_.recvmdiaChannel;
            session.send_ids = msg_.sendids;
            session.send_chid = msg_.sendmediaChannel;
            session.sessionid = msg_.sessionid;
            session.stremtype = msg_.streamtype;
            session.type = MSG_SIP_2_SIP_PLAY;
            ret_code = gw_join_sip_session_mgr::_()->save_session(session);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run() |ret_code[%d] sessionid[%d] | MSG_SIP_2_SIP_PLAY save_session fail!",ret_code,session.sessionid);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_PLAY save_session fail");
                ret_code = -6;
                break;
            }
            dev_handle_t link_handle = -1;
            long stremid = -1;
            device_link_t dev_link_inf;
            ret_code = media_device::_()->find_link(msg_.sendids.c_str(),msg_.recvmdiaChannel,msg_.streamtype,dev_link_inf);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_play_pro_task::run | find_link fail! sessionid[%d] cmid[%d] recvids[%s] recvch[%d] sendids[%s] sendch[%d] streamtype[%d] ret_code[%d]",
                    msg_.sessionid,msg_.cmdid,msg_.recvids.c_str(),msg_.recvmdiaChannel,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype,ret_code);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_PLAY find_link  fail recv not created!");
                ret_code = -7;
                break;
            }
            else
            {
                link_handle = dev_link_inf.link;
                stremid = dev_link_inf.strmid;
            }
            DEBUG_LOG(DBLOGINFO,ll_info,
                "gw_join_sip_play_pro_task::run |gw_join_sip_start_capture sessionid[%d] cmid[%d] recvids[%s] recvch[%d] sendids[%s] sendch[%d] streamtype[%d] handle[%d] stremid[%d]",
                msg_.sessionid,msg_.cmdid,msg_.recvids.c_str(),msg_.recvmdiaChannel,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype,link_handle,stremid);

            //创建转发
            int srcno = XTEngine::_()->gw_join_sip_create_transmit_machine_made(msg_.sendids,msg_.sendmediaChannel,msg_.streamtype,msg_.transmitchannel,link_handle,stremid);
            DEBUG_LOG(DBLOGINFO,ll_info,
                "gw_join_sip_play_pro_task::run |gw_join_sip_create_transmit_machine_made sessionid[%d] srcno[%d] sendids[%s] sendch[%d] streamtype[%d]",
                msg_.sessionid,srcno,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype);
            if (srcno < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run |gw_join_sip_create_transmit_machine_made fail! sessionid[%d] sendids[%s] sendch[%d] streamtype[%d]",
                    msg_.sessionid,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_PLAY gw_join_sip_create_transmit_machine_made fail");
                ret_code = -8;
                break;
            }

            //生成交换发送sdp
            src_info dev_src;
            ret_code = XTEngine::_()->get_src_no(srcno,dev_src);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run |get_src_no fail! sessionid[%d] ret_code[%d] sendids[%s] sendch[%d] streamtype[%d]",
                    msg_.sessionid,ret_code,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_PLAY get_src_no fail src not created!");
                ret_code = -30;
                break;
            }

            ret_code = XTEngine::_()->sip_create_sdp_s(srcno,dev_src.device.dev_strmtype,dev_src.device.key,dev_src.device.key_len,sdp_);
            DEBUG_LOG(DBLOGINFO,ll_info,
                "gw_join_sip_play_pro_task::run |gw_join_sip_create_transmit_machine_made sessionid[%d] ret_code[%d] sendids[%s] sendch[%d] streamtype[%d]",
                msg_.sessionid,ret_code,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run |sip_create_sdp_s  fail! sessionid[%d] ret_code[%d] sendids[%s] sendch[%d] streamtype[%d]",
                    msg_.sessionid,ret_code,msg_.sendids.c_str(),msg_.sendmediaChannel,msg_.streamtype);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_PLAY sip_create_sdp_s fail");
                ret_code = -9;
                break;
            }

            session.dev_handle  = link_handle;
            session.srcno = srcno;
            ret_code = gw_join_sip_session_mgr::_()->update_session(session);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run |ret_code[%d] sessionid[%d] |MSG_SIP_2_SIP_PLAYupdate_session fail!",ret_code,msg_.sessionid);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_PLAY update_session fail");
                ret_code = -10;
                break;
            }
            break;
        }

    case MSG_SIP_2_SIP_OPEN_CALL://sip-sip，通知交换建立发送/接收
        {
            dev_handle_t recv_dev_handle = -1;
            int srcno = -1;
            if (msg_.recvids.empty() || msg_.sendids.empty())
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run | ERR MSG_SIP_2_SIP_OPEN_CALL ids is empty sessionid[%d]",msg_.sessionid);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_OPEN_CALL ids is empty  fail");
                ret_code = -11;
                break;
            }
            session_inf_t session;
            session.recv_ids = msg_.recvids;
            session.recv_chid = msg_.recvmdiaChannel;
            session.send_ids = msg_.sendids;
            session.send_chid = msg_.sendmediaChannel;
            session.sessionid = msg_.sessionid;
            session.stremtype = msg_.streamtype;
            session.transmitchannel = msg_.transmitchannel;
            session.type = MSG_SIP_2_SIP_OPEN_CALL;
            ret_code = gw_join_sip_session_mgr::_()->save_session(session);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run |ret_code[%d] sessionid[%d]|gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_OPEN_CALL save_session  fail!",ret_code,msg_.sessionid);
                error_reasion.assign("");
                ret_code = -12;
                break;
            }

            ret_code = XTEngine::_()->sip_2_sip_create_free_transmit_channel(
                msg_.recvids,msg_.sendids,msg_.transmitchannel,msg_.recvmdiaChannel,msg_.streamtype,demux_r,sdp_,srcno,recv_dev_handle);
            DEBUG_LOG(DBLOGINFO,ll_info,
                "gw_join_sip_play_pro_task::run |sip_2_sip_create_free_transmit_channel sessionid[%d] srcno[%d] dev_handle[%d] ret_code[%d]",
                msg_.sessionid,srcno,recv_dev_handle,ret_code);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run | sessionid[%d] gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_OPEN_CALL sip_2_sip_create_free_transmit_channel  fail",msg_.sessionid);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_OPEN_CALL sip_2_sip_create_free_transmit_channel  fail");
                ret_code = -13;
                break;
            }

            session.dev_handle = recv_dev_handle;
            session.srcno = srcno;
            ret_code = gw_join_sip_session_mgr::_()->update_session(session);
            if (ret_code < 0)
            {
                DEBUG_LOG(DBLOGINFO,ll_error,
                    "gw_join_sip_play_pro_task::run |ret_code[%d] sessionid[%d] ERR MSG_SIP_2_SIP_OPEN_CALL update_session  fail!",ret_code,msg_.sessionid);
                error_reasion.assign("gw_join_sip_play_pro_task::run() ERR MSG_SIP_2_SIP_OPEN_CALL update_session  fail!");
                ret_code = -14;
                break;
            }
            break;
        }
    default://未知类型
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_play_pro_task::run |sip play type na! sessionid[%d]",msg_.sessionid);
            error_reasion.assign("gw_join_sip_play_pro_task::run() sip play type na!");
            ret_code = -15;
            break;
        }
    }

    if (sdp_.empty())
    {
        error_reasion.assign("gw_join_sip_play_pro_task::run |the respond sdp is empty!");
        ret_code = -16;
    }
    if (ret_code < 0)
    {
        DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_play_pro_task::run | fail! type[%d] ret_code[%d] sessionid[%d]",msg_.type,ret_code,msg_.sessionid);
        router_task_request_mgr::gw_join_sip_respond_fail_to_gateway(msg_.sessionid,msg_.sender_ids,msg_.guid,0,error_reasion);
    }
    else
    {
        //回传网关交换生成的sdp
        router_task_request_mgr::gw_join_sip_respond_trans_sdp_to_gateway(msg_.sessionid,msg_.sender_ids,msg_.guid,sdp_);
    }

    delete this;
    return 0;
}

uint32_t gw_join_sip_sdp_pro_task::run()
{	
    int oper_code = 0;
    do 
    {
        long ret = 0;
        session_inf_t session;
        ret = gw_join_sip_session_mgr::_()->get_session(msg_.sessionid,session);
        if (ret < 0)
        {
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_sdp_pro_task::run |ret[%d] sessionid[%d] |get seesion fail!",ret,msg_.sessionid);
            oper_code = -1;
            break;
        }

        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_sdp_pro_task::run |cmid[%d] sessionid[%d] sdp_len[%d] session.type[%d] recv_ids[%s] recv_chid[%d] stremtype[%d]",
            msg_.cmdid,msg_.sessionid,msg_.sdp_len,session.type,session.recv_ids.c_str(),session.recv_chid,session.stremtype);
        switch(session.type)
        {
        case MSG_OPEN_RECV:
            {
                ret = XTEngine::_()->gw_join_sip_save_sdp_to_access(session.recv_ids,session.recv_chid,session.stremtype,msg_.sdp.c_str(),msg_.sdp_len);

                DEBUG_LOG(DBLOGINFO,ll_info,
                    "gw_join_sip_sdp_pro_task::run |sessionid[%d] |open recv finish - recved the sdp of gateway,sdp:\n%s save_sdp_to_access ret[%d]",
                    msg_.sessionid,msg_.sdp.c_str(),ret);
                if (ret < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,
                        "gw_join_sip_sdp_pro_task::run |ret[%d] sessionid[%d]|gw_join_sip_save_sdp_to_access fail!",ret,session.sessionid);
                    oper_code = -2;
                }
                break;
            }

        case MSG_OPEN_SEND:
        case MSG_SIP_2_SIP_PLAY:
        case MSG_SIP_2_SIP_OPEN_CALL:
            {

                ret = gw_join_sip_session_mgr::_()->save_sdp_to_session(msg_.sessionid,msg_.sdp.c_str(),msg_.sdp_len);
                DEBUG_LOG(DBLOGINFO,ll_info,
                    "gw_join_sip_sdp_pro_task::run |sessionid[%d] sessiontype[%d] |open recv finish - recved the sdp of gateway,sdp:\n%s save_sdp_to_session ret[%d]",msg_.sessionid,session.type,msg_.sdp.c_str(),ret);
                if (ret < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,
                        "gw_join_sip_sdp_pro_task::run |ret[%d] sessionid[%d] |save_sdp_to_session fail!",ret,msg_.sessionid);
                    oper_code = -3;
                }
                break;
            }

        case MSG_OPEN_CALL:
            {
                ret = XTEngine::_()->gw_join_sip_save_sdp_rs(msg_.sessionid,msg_.sdp.c_str(),msg_.sdp_len);
                DEBUG_LOG(DBLOGINFO,ll_info,
                    "gw_join_sip_sdp_pro_task::run |sessionid[%d] call open recv and send finish - recved the sdp of gateway,sdp:\n%s gw_join_sip_save_sdp_rs ret[%d]",msg_.sessionid,msg_.sdp.c_str(),ret);
                if (ret < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_sdp_pro_task::run |sessionid[%d] ret[%d] |gw_join_sip_save_sdp_rs fail!",msg_.sessionid,ret);
                    oper_code = -4;
                }
                break;
            }
        default:
            {
                DEBUG_LOG(DBLOGINFO,ll_error, "gw_join_sip_sdp_pro_task::run |sessinid[%d] sip_sdp_pro::run type na!",msg_.sessionid);
                oper_code = -6;
                break;
            }
        }

        if (oper_code < 0)
        {
            break;
        }
        oper_code=1;
    } while (false);

    if (oper_code < 0)
    {
        router_task_request_mgr::gw_join_sip_respond_fail_to_gateway(msg_.sessionid,msg_.sender_ids,"0",PRO_SDP_FAIL,"sip_sdp_pro_task::run() fail!");
    }

    delete this;
    return 0;
}

uint32_t gw_join_sip_close_recv_task::run()
{
    long ret_code = 0;
    do 
    {  
        long ret = XTEngine::_()->gw_join_sip_close_r(sessionid_);
        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_close_recv_task::run | sessionid[%d] close_recv | gw_join_sip_close_r ret[%d]",sessionid_,ret);

        if (ret < 0)
        {
            ret_code = -1;
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_close_recv_task::run |sessionid[%d] ret[%d] |gw_join_sip_close_r fail",sessionid_,ret);
            break;
        }
        ret_code =1;
    }while(0);
    if (ret_code < 0)
    {
        router_task_request_mgr::gw_join_sip_respond_fail_to_gateway(sessionid_,sendids_,"0",0,"sip_close_recv_task::run()fail!");
    }

    delete this;
    return 0;
};


uint32_t gw_join_sip_add_send_task::run()
{
    long ret_code = 0;
    do 
    {
        long ret = XTEngine::_()->gw_join_sip_add_send(sessionid_);
        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_add_send_task::run | sessionid[%d] Add Send gw_join_sip_add_send ret[%d]",sessionid_,ret);
        if (ret < 0)
        {
            ret_code = -1;
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_add_send_task::run |sessionid[%d] ret[%d] |gw_join_sip_add_send fail!",sessionid_,ret);
            break;
        }
        ret_code =1;
    } while (0);

    if (ret_code < 0)
    {
        router_task_request_mgr::gw_join_sip_respond_fail_to_gateway(sessionid_,sendids_,guid_,ADD_SEND_STREAM_FAIL,"sip_add_send_task::run()fail!");
    }
    else
    {
        router_task_request_mgr::gw_join_sip_respond_success_to_gateway(sessionid_,sendids_,guid_);
    }

    delete this;
    return 0;
}

uint32_t gw_join_sip_close_send_task::run()
{
    int ret_code = 0;
    do 
    {
        long ret = XTEngine::_()->gw_join_sip_close_s(sessionid_);
        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_close_send_task::run |sessionid[%d] Close Send gw_join_sip_close_s ret[%d]",sessionid_,ret);

        if (ret < 0)
        {
            ret_code = -1;
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_close_send_task::run |sessionid[%d] ret[%d] |gw_join_sip_close_s fail!",sessionid_,ret);
            break;
        } 
        ret_code =1;
    } while (0);
    if (ret_code < 0)
    {
        router_task_request_mgr::gw_join_sip_respond_fail_to_gateway(sessionid_,sendids_,"0",0,"sip_close_send_task::run()fail!");
    }

    delete this;
    return 0;
}

uint32_t gw_join_sip_clear_trans_ch_task::run()
{
    int ret_code = 0;
    do 
    {
        long ret = XTEngine::_()->gw_join_sip_clear_trans_ch(sessionid_);
        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_clear_trans_ch_task::run |sessionid[%d] Clear Trans CH gw_join_sip_clear_trans_ch ret[%d]",sessionid_,ret);
        if (ret < 0)
        {
            ret_code = -1;
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_clear_trans_ch_task::run |sessionid[%d] ret[%d] |gw_join_sip_clear_trans_ch fail!",sessionid_,ret);
            break;
        }
    } while (0);
    if (ret_code < 0)
    {
        router_task_request_mgr::gw_join_sip_respond_fail_to_gateway(sessionid_,sendids_,"0",0,"gw_join_sip_clear_trans_ch::run() fail!");
    }
    delete this;
    return 0;
}

uint32_t gw_join_sip_clear_session_pro_task::run()
{
    int ret_code = 0;
    do 
    {
        long ret = gw_join_sip_session_mgr::_()->clear_session(sessionid_);
        DEBUG_LOG(DBLOGINFO,ll_info,"gw_join_sip_clear_session_pro_task::run |sessionid_[%d] Clear SIP Session clear_session ret[%d]",sessionid_,ret);
        if (ret < 0)
        {
            ret_code = -1;
            DEBUG_LOG(DBLOGINFO,ll_error,"gw_join_sip_clear_session_pro_task::run |sessionid[%d] ret[%d] |sip_clear_sesssion fail!",sessionid_,ret);
            break;
        }

        ret_code =1;
    } while (0);

    if (ret_code < 0)
    {
        router_task_request_mgr::gw_join_sip_respond_fail_to_gateway(sessionid_,sendids_,"0",0,"sip_clear_session_pro_task::run() fail!");
    }
    delete this;
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
