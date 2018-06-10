/*@
*@FileNmae:  xmpp_client.cpp
*@Describe:  XMPP信令会话处理实现文件
*@Author:    songlei
*@Date:      2014-11-24
**/
#ifdef _USE_XMPP_FUNC_
#include "xmpp_client.h"
#include "media_server.h"
#include "XTRouter.h"
#include "xtXml.h"
#include "xmpp_task.h"
#include "XTRouterLog.h"

#define XMPP_OK 0          
#define DATA_NA -1         // 无效数据 
#define DECIMAL_SYSTEM 10   // 十进制
#define OPERATE_FAIL -1     // 操作失败
#define OPERATE_SUCCESS 0   // 操作成功
#define SDP_SIZE_MAX 2048   // sdp最大长度
#define MULTIPLEX_YES "yes" // 复用
#define MULTIPLEX_NO  "no"  // 不复用

using namespace xmpp_ret_code;

xmpp_client xmpp_client::obj_;
xmpp_client::xmpp_client():
is_xmpp_(false)
,m_run(true)
,m_pThread(NULL)
,is_stop_all_(false)
{
    //构造中不做任务操作
}

xmpp_client::~xmpp_client()
{
}

//回调
///////////////////////////////////////////////////////////////////////////////////
void xmpp_client:: XmppOutLogCB(void* pUser, const char* log)
{
    //printf("\nXmppGlooxApply log show:\n%s\n\n",log);
    WRITE_LOG(XmppGlooxApplyLog,ll_info,"%s",log);
}

void xmpp_client::XmppClientRcvPrensenceCB(void* pUser, const int OnlineStatus, const char* jid, const int pri, const char* status)
{
    xmpp_client* pclient = static_cast<xmpp_client*>(pUser);
    if (NULL == pclient)
    {
        return;
    }
}

void xmpp_client::XmppRcvMsgCB(void* pUser, const char* jid, const char* type, const char* MsgBody)
{
    xmpp_client* pclient = static_cast<xmpp_client*>(pUser);
    if (NULL == pclient)
    {
        return;
    }

}

//信令处理回调
void xmpp_client::XmppRcvIQCB(void* pUser, const char* jid, const char* type, const char* id, const char* NameSpace,const char* IQBody)
{
    xmpp_client* client = static_cast<xmpp_client*>(pUser);
    if (NULL == client)
    {
        return;
    }

    //上下线信息不处理
    if (client->compare_str(NameSpace,XMNLS_ONLINESTATUS))
    {
        return;
    }

    printf("\nRcvIQ:From[%s]type[%s] NameSpace[%s]:\n  Body[%s]\n",jid,type,NameSpace,IQBody);

    ret_code_type ret = PARSE_SIGNALLING_SUCCESS;

    do 
    {
        //点播
        if (client->compare_str(NameSpace,XMNLS_DB_PLAY))
        {
            ret = client->parse_play_iq(jid,type,id,NameSpace,IQBody);

            if (PARSE_SIGNALLING_SUCCESS != ret)
            {
                break;
            }	

        }
        //停点
        else if (client->compare_str(NameSpace,XMNLS_DB_PLAYSTOP))
        {
            ret = client->parse_stop_play_iq(jid,type,id,NameSpace,IQBody);

            if (PARSE_SIGNALLING_SUCCESS != ret)
            {
                break;
            }
        }
        //开启TCP反向注册
        else if(client->compare_str(NameSpace,XMNLS_DB_PLAYINFO))
        {
            ret = client->parse_play_inform_request_iq(jid,type,id,NameSpace,IQBody);
            if (PARSE_SIGNALLING_SUCCESS != ret)
            {
                break;
            }

        }
        //停止TCP反向注册
        else if (client->compare_str(NameSpace,XMNLS_DB_PLAYSTOPINFO))
        {
            ret = client->parse_stop_inform_request_iq(id,type,id,NameSpace,IQBody);
            if (PARSE_SIGNALLING_SUCCESS != ret)
            {
                break;
            }

        }
        else
        {
            ret = PARSE_SIGNALLING_SUCCESS;
            printf("IQ NA! NameSpace[%s]\n",NameSpace);

        }


    } while (false);

    if (PARSE_SIGNALLING_SUCCESS != ret)
    {
        printf("err parse signalling fail [%d]\n",ret);
    }
}

/**
* 函数功能：中心断线回调抛出
* @param 
* @return
**/
void xmpp_client::XmppConnStreamErrorCB(void* pUser)
{
    printf("XmppConnStreamErrorCB");

    xmpp_client* client = static_cast<xmpp_client*>(pUser);
    if (NULL == client)
    {
        printf("XmppConnStreamErrorCB NULL");
        return;
    }

    client->is_xmpp_ = false;
    client->is_stop_all_ = true;
}

/**
* 函数功能：网卡禁用回调抛出
* @param 
* @return
**/
void xmpp_client::XmppConnIoErrorCB(void* pUser)
{
    printf("XmppConnIoErrorCB");

    xmpp_client* client = static_cast<xmpp_client*>(pUser);
    if (NULL == client)
    {
        printf("XmppConnIoErrorCB NULL");
        return;
    }

    client->is_xmpp_ = false;
    client->is_stop_all_ = true;
}
///////////////////////////////////////////////////////////////////////////////////

// 连接中心线程
void xmpp_client::ccs_link_fun()
{
    while (m_run)
    {
        if (!is_xmpp_)
        {	
            if (is_stop_all_)
            {
                xmpp_stop_all_task* ptr_task = new xmpp_stop_all_task();
                if (NULL != ptr_task)
                {
                    ptr_task->process_event();
                }
            }

            if (login(log_name_.c_str(),center_host_name_.c_str(),
                log_password_.c_str(),log_port_,log_res_id_.c_str()))
            {
                is_xmpp_ = true; 
                printf("css login success\n");
            }
            else
            {
                printf("css login fail\n");
            }
        }

        boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
    }
}

//业务接口
//////////////////////////////////////////////////////////////////////////////
//点播
ret_code_type xmpp_client::parse_play_iq(const char* jid, const char* type, const char* id,const char* name_space,const char* iq_body)
{
    ret_code_type ret_code = PARSE_SIGNALLING_SUCCESS;

    do 
    {
        xtXml xml_tmp;
        xml_tmp.LoadXMLStr(iq_body);

        xtXmlNodePtr playbody = xml_tmp.getRoot();
        if (playbody.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        xtXmlNodePtr play = xml_tmp.getNode(playbody,"play");
        if (play.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        //只处理点播请求
        if (!compare_str(PLAY_REQUEST,play.GetAttribute("action")))
        {
            break;
        }

        xtXmlNodePtr transmitInfo = xml_tmp.getNode(playbody,"transmitInfo");
        if (transmitInfo.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        xtXmlNodePtr original = xml_tmp.getNode(playbody,"original");
        if (original.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        iq_type<play_requst_body> iq;
        iq.id_ = id;
        iq.to_ = local_jid_;
        iq.from_ = jid;
        iq.type_ = type;
        iq.query_.xmlns_ = name_space;
        iq.query_.playbody_.play_.action_ = play.GetAttribute("action");
        if (iq.query_.playbody_.play_.action_ != PLAY_REQUEST)
        {
            break;
        }
        iq.query_.playbody_.play_.sessionType_ = play.GetAttribute("sessionType");
        iq.query_.playbody_.play_.sid_ = play.GetAttribute("sid");

        iq.query_.playbody_.transmitInfo_.sessionType_ = transmitInfo.GetAttribute("sessionType");
        xtXmlNodePtr arg = transmitInfo.GetFirstChild(iq.query_.playbody_.transmitInfo_.sessionType_.c_str());
        if (!arg.IsNull())
        {
            iq.query_.playbody_.transmitInfo_.arg.token_ = arg.GetAttribute("token");
            iq.query_.playbody_.transmitInfo_.arg.loginName_ = arg.GetAttribute("loginName");
            iq.query_.playbody_.transmitInfo_.arg.loginPassword_ = arg.GetAttribute("loginPassword");
            iq.query_.playbody_.transmitInfo_.arg.loginport_ = ::str_to_num<long>(arg.GetAttribute("loginPort"));
            iq.query_.playbody_.transmitInfo_.arg.playChannel_ = ::str_to_num<long>(arg.GetAttribute("playChannel"));
            iq.query_.playbody_.transmitInfo_.arg.playip_ = arg.GetAttribute("playIp");
            iq.query_.playbody_.transmitInfo_.arg.playPort_ = ::str_to_num<long>(arg.GetAttribute("playPort"));
            iq.query_.playbody_.transmitInfo_.arg.playType_ = ::str_to_num<int>(arg.GetAttribute("playType"));
            iq.query_.playbody_.transmitInfo_.arg.transmitChannel_ = ::str_to_num<long>(arg.GetAttribute("transmitChannel"));

            std::string reuseFlag = arg.GetAttribute("reuseFlag_r");
            if (XMPP_OK == reuseFlag.compare("yes"))
            {
                iq.query_.playbody_.transmitInfo_.arg.reuseFlag_r_ = true;
            }
            else
            {
                iq.query_.playbody_.transmitInfo_.arg.reuseFlag_r_ = false;
            }

            reuseFlag = arg.GetAttribute("reuseFlag_s");
            if (XMPP_OK == reuseFlag.compare("yes"))
            {
                iq.query_.playbody_.transmitInfo_.arg.reuseFlag_s_ = true; 

            }
            else
            {
                iq.query_.playbody_.transmitInfo_.arg.reuseFlag_s_ = true;

            }
        }
        else
        {
            ret_code = XML_PLAY_ARG_ERR;
            break;
        }

        iq.query_.playbody_.original_.requestor_ = original.GetAttribute("requestor");
        iq.query_.playbody_.original_.responder_ = original.GetAttribute("responder");
        iq.query_.playbody_.original_.mediaSrcChannel_ = ::str_to_num<long>(original.GetAttribute("mediaSrcChannel"));
        iq.query_.playbody_.original_.streamType_ = ::str_to_num<long>(original.GetAttribute("streamType"));

        //投递到工作线程
        xmpp_play_task* play_task = new xmpp_play_task(iq);
        if (play_task)
        {
            play_task->process_event();
        }


    } while (false);

    return ret_code;
}

//停点
ret_code_type xmpp_client::parse_stop_play_iq(const char* jid, const char* type, const char* id,const char* name_space,const char* iq_body)
{
    ret_code_type ret_code = PARSE_SIGNALLING_SUCCESS;

    do 
    {
        xtXml xml_tmp;
        xml_tmp.LoadXMLStr(iq_body);

        xtXmlNodePtr playbody = xml_tmp.getRoot();
        if (playbody.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        xtXmlNodePtr play = xml_tmp.getNode(playbody,"play");
        if (play.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        //只处理停点请求
        if (!compare_str(PLAY_STOP_REQUEST,play.GetAttribute("action")))
        {
            break;
        }

        xtXmlNodePtr transmitInfo = xml_tmp.getNode(playbody,"transmitInfo");
        if (transmitInfo.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        iq_type<stop_requst_body> iq;
        iq.from_ = jid;
        iq.to_ = local_jid_;
        iq.type_ = type;
        iq.id_ = id;

        iq.query_.xmlns_  = name_space;
        iq.query_.playbody_.play_.action_ = play.GetAttribute("action");
        if (iq.query_.playbody_.play_.action_ != PLAY_STOP_REQUEST)
        {
            break;
        }
        iq.query_.playbody_.play_.sessionType_ = play.GetAttribute("sessionType");
        iq.query_.playbody_.play_.sid_ = play.GetAttribute("sid");

        iq.query_.playbody_.transmitInfo_.sessionType_ = transmitInfo.GetAttribute("sessionType");
        xtXmlNodePtr arg = transmitInfo.GetFirstChild(iq.query_.playbody_.transmitInfo_.sessionType_.c_str());
        if (!arg.IsNull())
        {
            iq.query_.playbody_.transmitInfo_.arg.transmitChannel_ = ::str_to_num<long>(arg.GetAttribute("transmitChannel"));
            iq.query_.playbody_.transmitInfo_.arg.token_ = arg.GetAttribute("token");
        }
        else
        {
            ret_code = XML_STOP_ARG_ERR;
            break;
        }

        //投递
        xmpp_stop_play_task* stop_play_task = new xmpp_stop_play_task(iq);
        if (stop_play_task)
        {
            stop_play_task->process_event();
        }

    } while (false);

    return ret_code;
}

//反向注册
ret_code_type xmpp_client::parse_play_inform_request_iq(const char* jid, const char* type, const char* id,const char* name_space,const char* iq_body)
{
    ret_code_type ret_code = PARSE_SIGNALLING_SUCCESS;
    do 
    {

        xtXml xml_tmp;
        xml_tmp.LoadXMLStr(iq_body);

        xtXmlNodePtr playbody = xml_tmp.getRoot();
        if (playbody.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        xtXmlNodePtr play = xml_tmp.getNode(playbody,"play");
        if (play.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        xtXmlNodePtr transmitInfo = xml_tmp.getNode(playbody,"transmitInfo");
        if (transmitInfo.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        iq_type<play_inform_request_body> iq;
        iq.id_ = id;
        iq.from_ = jid;
        iq.type_ = type;
        iq.to_ = local_jid_;
        iq.query_.xmlns_ = name_space;
        iq.query_.playbody_.play_.action_ = play.GetAttribute("action");

        if (iq.query_.playbody_.play_.action_ != PLAY_INFORM_REQUST)
        {
            break;
        }

        iq.query_.playbody_.play_.sessionType_ = play.GetAttribute("sessionType");
        iq.query_.playbody_.play_.sid_ = play.GetAttribute("sid");

        iq.query_.playbody_.transmitInfo_.sessionType_ = transmitInfo.GetAttribute("sessionType");
        xtXmlNodePtr arg = transmitInfo.GetFirstChild(iq.query_.playbody_.transmitInfo_.sessionType_.c_str());
        if (!arg.IsNull())
        {
            iq.query_.playbody_.transmitInfo_.arg.routerip_ = arg.GetAttribute("routerip");
            iq.query_.playbody_.transmitInfo_.arg.routerport_ = ::str_to_num<unsigned short>(arg.GetAttribute("routerport"));
        }
        else
        {
            ret_code = XML_PLAY_INFRM_ARG_ERR;
            break;
        }

        //投递任务
        xmpp_play_inform_request_task* play_inform_request_task = new xmpp_play_inform_request_task(iq);
        if (play_inform_request_task)
        {
            play_inform_request_task->process_event();
        }

    } while (false);

    return ret_code;

}

ret_code_type xmpp_client::parse_stop_inform_request_iq(const char* jid, const char* type, const char* id,const char* name_space,const char* iq_body)
{
    ret_code_type ret_code = PARSE_SIGNALLING_SUCCESS;
    do 
    {
        xtXml xml_tmp;
        xml_tmp.LoadXMLStr(iq_body);

        xtXmlNodePtr playbody = xml_tmp.getRoot();
        if (playbody.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        xtXmlNodePtr play = xml_tmp.getNode(playbody,"play");
        if (play.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        xtXmlNodePtr transmitInfo = xml_tmp.getNode(playbody,"transmitInfo");
        if (transmitInfo.IsNull())
        {
            ret_code = PARSE_PLAY_SIGNALLING_FAIL;
            break;
        }

        iq_type<stop_inform_request_body> iq;
        iq.id_    = id;
        iq.from_  = jid;
        iq.type_  = type;
        iq.to_    = local_jid_;
        iq.query_.xmlns_ = name_space;
        iq.query_.playbody_.play_.action_ = play.GetAttribute("action");
        if (iq.query_.playbody_.play_.action_ != PLAY_STOP_INFORM_REQUEST)
        {
            break;
        }
        iq.query_.playbody_.play_.sessionType_ = play.GetAttribute("sessionType");
        iq.query_.playbody_.play_.sid_ = play.GetAttribute("sid");

        iq.query_.playbody_.transmitInfo_.sessionType_ = transmitInfo.GetAttribute("sessionType");
        xtXmlNodePtr arg = transmitInfo.GetFirstChild(iq.query_.playbody_.transmitInfo_.sessionType_.c_str());
        if (!arg.IsNull())
        {
            iq.query_.playbody_.transmitInfo_.arg.routerip_ = arg.GetAttribute("routerip");
            iq.query_.playbody_.transmitInfo_.arg.routerport_ = ::str_to_num<unsigned short>(arg.GetAttribute("routerport"));
        }
        else
        {
            ret_code = XML_STOP_INFRM_ARG_ERR;
            break;
        }

        xmpp_play_stop_inform_request_task *play_stop_inform_request_task = new xmpp_play_stop_inform_request_task(iq);
        if (play_stop_inform_request_task)
        {
            play_stop_inform_request_task->process_event();
        }

    } while (false);
    return ret_code;
}

//交换回复服务调度中心点播结果
ret_code_type xmpp_client::play_result_to_ccs(const iq_type<play_requst_body>& iq,int ret_state)
{
    ret_code_type ret_code = SEND_IQ__SUCCESS;

    do 
    {
        XmppTag playbody("playbody",STR_NULL,STR_NULL);

        XmppTag play("play","action",PLAY_REPLAY);
        play.addAttribute("sessionType",iq.query_.playbody_.play_.sessionType_.c_str());
        play.addAttribute("sid",iq.query_.playbody_.play_.sid_.c_str());

        playbody.addChild(&play);

        std::string str_iqtype;
        if (XMPP_OK <= ret_state)
        {		
            XmppTag transmitInfo("transmitInfo","sessionType",iq.query_.playbody_.transmitInfo_.sessionType_.c_str());

            std::string sessionType = iq.query_.playbody_.transmitInfo_.sessionType_;

            //if (XMPP_OK == iq.query_.playbody_.play_.sessionType_.compare(RTSP_RTP))
            {
                //XmppTag rtsp_rtp(RTSP_RTP,"token",iq.query_.playbody_.transmitInfo_.arg.token_.c_str());
                XmppTag play_arg(sessionType.c_str(),"token",iq.query_.playbody_.transmitInfo_.arg.token_.c_str());
                play_arg.addAttribute("playIp",iq.query_.playbody_.transmitInfo_.arg.playip_.c_str());			
                play_arg.addAttribute("playPort",iq.query_.playbody_.transmitInfo_.arg.playPort_);		
                play_arg.addAttribute("transmitChannel",iq.query_.playbody_.transmitInfo_.arg.transmitChannel_);			
                play_arg.addAttribute("playType",iq.query_.playbody_.transmitInfo_.arg.playType_);

                transmitInfo.addChild(&play_arg);
            }

            playbody.addChild(&transmitInfo);	

            str_iqtype = TYPE_RESULT;
        }
        else
        {			
            XmppTag error("error",STR_NULL,STR_NULL);
            error.addCData("XTRouter Play Fail!");
            playbody.addChild(&error);
            str_iqtype = TYPE_ERROR;
        }

        std::string str_xml = playbody.GetInputXml();
        printf("play_play-to[%s]:%s\n",iq.from_.c_str(),str_xml.c_str());

        if (!XmppSendIQ(iq.from_.c_str(),str_iqtype.c_str(),iq.id_.c_str(),iq.query_.xmlns_.c_str(),&playbody))
        {
            ret_code = SEND_IQ_FAIL;
            break;
        }		

    } while (false);

    return ret_code;
}

//交换回复服务调度中心停止点播成功
xmpp_ret_code::ret_code_type xmpp_client::stop_play_result_to_ccs(const iq_type<stop_requst_body>& iq,int ret_state)
{
    ret_code_type ret_code = SEND_IQ__SUCCESS;

    do 
    {

        XmppTag playbody("playbody",STR_NULL,STR_NULL);

        XmppTag play("play","action",PLAY_STOP_REPLY);
        play.addAttribute("sessionType",iq.query_.playbody_.play_.sessionType_.c_str());
        play.addAttribute("sid",iq.query_.playbody_.play_.sid_.c_str());

        playbody.addChild(&play);

        std::string str_iqtype = TYPE_RESULT;
        //停点失败
        if (XMPP_OK > ret_state)
        {
            XmppTag error("error",STR_NULL,STR_NULL);
            error.addCData("XTRouter Stop Play Fail!");
            playbody.addChild(&error);

            str_iqtype = TYPE_ERROR;

        }	

        std::string str_xml = playbody.GetInputXml();
        printf("stop_play:%s\n",str_xml.c_str());
        if (!XmppSendIQ(iq.from_.c_str(),str_iqtype.c_str(),iq.id_.c_str(),iq.query_.xmlns_.c_str(),&playbody))
        {
            ret_code = SEND_IQ_FAIL;
            break;
        }

    } while (false);	

    return ret_code;
}

ret_code_type xmpp_client::play_inform_replay_to_ccs(const iq_type<play_inform_request_body>& iq,int ret_state)
{
    ret_code_type ret = SEND_IQ__SUCCESS;
    do 
    {
        XmppTag playbody("playbody",STR_NULL,STR_NULL);

        XmppTag play("play","action",PLAY_INFORM_REPLAY);
        play.addAttribute("sessionType",iq.query_.playbody_.play_.sessionType_.c_str());
        play.addAttribute("sid",iq.query_.playbody_.play_.sid_.c_str());

        playbody.addChild(&play);

        std::string str_iqtype = TYPE_RESULT;
        //注册失败
        if (XMPP_OK > ret_state)
        {
            XmppTag error("error",STR_NULL,STR_NULL);
            error.addCData("XTRouter TCP Regist Fail!");
            playbody.addChild(&error);
            str_iqtype = TYPE_ERROR;

        }
        else
        {
            XmppTag transmitInfo("transmitInfo","sessionType",iq.query_.playbody_.transmitInfo_.sessionType_.c_str());
            XmppTag arg(iq.query_.playbody_.transmitInfo_.sessionType_.c_str(),STR_NULL,STR_NULL);
            transmitInfo.addChild(&arg);
            playbody.addChild(&transmitInfo);
        }

        std::string str_xml = playbody.GetInputXml();
        printf("play_inform_replay_to_ccs:%s\n",str_xml.c_str());

        if (!XmppSendIQ(iq.from_.c_str(),str_iqtype.c_str(),iq.id_.c_str(),iq.query_.xmlns_.c_str(),&playbody))
        {
            ret = SEND_IQ_FAIL;
            break;
        }

    } while (false);
    return ret;
}

ret_code_type xmpp_client::stop_inform_replay_to_ccs(const iq_type<stop_inform_request_body>& iq,int ret_state)
{
    ret_code_type ret = SEND_IQ__SUCCESS;
    do 
    {
        XmppTag playbody("playbody",STR_NULL,STR_NULL);

        XmppTag play("play","action",PLAY_STOP_INFORM_REPLAY);
        play.addAttribute("sessionType",iq.query_.playbody_.play_.sessionType_.c_str());
        play.addAttribute("sid",iq.query_.playbody_.play_.sid_.c_str());

        playbody.addChild(&play);

        std::string str_iqtype = TYPE_RESULT;
        //注册失败
        if (XMPP_OK > ret_state)
        {
            XmppTag error("error",STR_NULL,STR_NULL);
            error.addCData("XTRouter TCP Regist Fail!");
            playbody.addChild(&error);
            str_iqtype = TYPE_ERROR;
        }
        else
        {
            XmppTag transmitInfo("transmitInfo","sessionType",iq.query_.playbody_.transmitInfo_.sessionType_.c_str());
            XmppTag arg(iq.query_.playbody_.transmitInfo_.sessionType_.c_str(),STR_NULL,STR_NULL);
            transmitInfo.addChild(&arg);
            playbody.addChild(&transmitInfo);
        }

        std::string str_xml = playbody.GetInputXml();
        printf("play_inform_replay_to_ccs:%s\n",str_xml.c_str());

        if (!XmppSendIQ(iq.from_.c_str(),str_iqtype.c_str(),iq.id_.c_str(),iq.query_.xmlns_.c_str(),&playbody))
        {
            ret = SEND_IQ_FAIL;
            break;
        }

    } while (false);
    return ret;
}

bool  xmpp_client::is_xmpp()
{
    return is_xmpp_;
}

void xmpp_client::init_cfg()
{
    router_config cfg = CXTRouter::Instance()->get_router_cfg();
    log_name_ = cfg.log_usre_name;     
    log_password_ = cfg.log_usre_password;
    log_res_id_ = cfg.log_usre_res_id;  
    center_host_name_ = cfg.log_center_name;
    log_port_ = cfg.log_center_port;  
    local_ip_ = cfg.local_ip;
    xtmsg_listenport = cfg.xtmsg_listenport;
    rtsp_listenport = cfg.rtsp_listenport;
    tcp_listenport = cfg.tcp_listenport;
    regist_bind_port_ = cfg.regist_bind;
    udp_listenport = cfg.udp_listenport;

    local_jid_ = log_name_ + "@" + center_host_name_ + "/" + log_res_id_;
}

ret_code_type xmpp_client::start_xmpp_client()
{
    ret_code_type ret_code = LONGIN_CENTER_SUCCESS;

    do 
    {
        if (is_xmpp_)
        {
            ret_code = LOGIN_EXIST;
            break;
        }

        //初始化xmpp库
        XmppInit();

        //读取配置
        init_cfg();

        //创建工作线程
        m_pThread = new boost::thread(boost::bind(&xmpp_client::ccs_link_fun,this));

    } while (false);

    return ret_code;
}

void xmpp_client::stop_xmpp_client()
{
    m_run = false;
    if (m_pThread)
    {
        m_pThread->join();
    }

    if (is_xmpp())
    {
        XmppClientExit();
        XmppUnInit();
    }	
}

bool xmpp_client::login(const char* name,const char* server,
                        const char* password,const int port,
                        const char* source)
{
    printf("login css:name[%s] server[%s] password[%s] port[%d] source[%s]....\n",name,server,password,port,source);
    bool ret = true;
    do 
    {
        ret = XmppClientLogin(name,server,password,port,source);
        if (ret)
        {
            SetPresenceCallback(XmppClientRcvPrensenceCB,this);

            SetMessageCallback(XmppRcvMsgCB,this);

            SetIQCallback(XmppRcvIQCB,this);

            SetLogCallback(XmppOutLogCB,this);

            SetErrCallback(XmppConnStreamErrorCB, this);

            SetIOErrCallback(XmppConnIoErrorCB, this);
        }
        else
        {
            break;
        }


    } while (false);

    return ret;
}


//功能接口
//////////////////////////////////////////////////////////////////////////////
parse_url_ret_code xmpp_client::parse_url(const std::string& url,std::string& ip,uint32_t& port,long& ch)
{
    parse_url_ret_code ret = RET_PARSE_SUCCESS;
    ip.clear();
    port = 0;
    ch = -1;
    do 
    {
        const char* str = "//";
        std::string::size_type pos_s = url.find(str);
        std::string::size_type pos_e = url.find(":",pos_s);
        if (std::string::npos == pos_s ||
            std::string::npos == pos_e)
        {
            ret = RET_PARSE_IP_FAIL;
            break;
        }

        std::string::size_type pos_temp = pos_s +2;

        ip = url.substr(pos_temp,pos_e - pos_temp);

        pos_s = pos_e;
        pos_e = url.find("/",pos_s);
        if (std::string::npos == pos_e)
        {
            ret = RET_PARSE_PORT_FAIL;
            break;
        }
        pos_temp = pos_s + 1;
        std::string strport = url.substr(pos_temp,pos_e-pos_temp);
        port = ::str_to_num<uint32_t>(strport.c_str());


        pos_s = pos_e;	
        pos_e = url.find("/",pos_s+1);
        if (std::string::npos != pos_e)
        {
            ret = RET_PASE_CH_FAIL;
            break;

        }
        pos_temp = pos_s + 1;

        std::string strch = url.substr(pos_temp,pos_e);

        ch = ::str_to_num<long>(strch.c_str());

    } while (false);

    return ret;
}

bool xmpp_client::compare_str(const char* str1,const char* str2)
{
    bool compare_ret = false;
    std::string str_l = str1;
    std::string str_r = str2;
    if (str_l == str_r)
    {
        compare_ret = true;
    }
    return compare_ret;
}

int xmpp_client::to_xtrouter_linktype(const std::string& session_type,bool reuse_flag/*=false*/)
{

    // 1 私有tcp会话以及私有udp流
    // 2 私有tcp会话以及私有udp流多播
    // 9 私有tcp会话以及标准rtp流
    // 10 私有XMPP会话以及标准rtp流
    // 11 私有XMPP会话以及私有rtp流
    // 12 私有XMPP会话以及私有rtp混合端口复用流

    //默认为RTP
    int link_type = 3;

    // 0 私有tcp会话以及私有tcp流
    if (XMPP_OK == session_type.compare(TCP_TCP))
    {
        link_type = LINK_TCP_TCP_PRI;
    }
    // 3 私有tcp会话以及私有rtp混合流
    else if (( XMPP_OK == session_type.compare(TCP_XTRTP) && !reuse_flag) 
        || XMPP_OK == session_type.compare(NAT_TCP_RTP))
    {
        link_type = LINK_TCP_RTP_PRI;
    }
    // 5 私有tcp会话以及私有rtp混合端口复用流
    else if (XMPP_OK == session_type.compare(TCP_XTRTP) && reuse_flag)
    {
        link_type = LINK_TCP_RTP_DEMUX_PRI;

    }
    // 6 标准rtsp会话以及标准rtp流
    else if (XMPP_OK == session_type.compare(RTSP_RTP) && !reuse_flag)
    {
        link_type = LINK_RTSP_RTP_STD;

    }
    // 7 标准rtsp会话以及复用rtp流
    else if (XMPP_OK == session_type.compare(RTSP_RTP) && reuse_flag)
    {
        link_type = LINK_RTSP_RTP_DEMUX;

    }
    // 8 标准rtsp会话以及私有rtp流
    else if (XMPP_OK == session_type.compare(RSTP_XTRTP))
    {
        link_type = LINK_RTSP_RTP_PRI;
    }
    // 13 私有udp会话以及私有rtp混合流
    else if (XMPP_OK == session_type.compare(NAT_UDP_RTP))
    {
        link_type = LINK_UDP_RTP_PRI;
    }

    return link_type;
}

void xmpp_client::get_local_net_info(std::string& ip,long& playport,const int xt_link_type)
{	
    switch (xt_link_type)
    {
        //RTSP会话端口
    case LINK_RTSP_RTP_STD:
    case LINK_RTSP_RTP_PRI: 
    case LINK_RTSP_RTP_DEMUX:
        {
            playport = rtsp_listenport;
            break;
        }
        //xtmsg_tcp 会话端口
    case LINK_TCP_UDP_MULTI:
    case LINK_TCP_RTP_PRI:
    case LINK_TCP_RTP_MULTI:
    case LINK_TCP_RTP_DEMUX_PRI:
    case LINK_TCP_RTP_STD:
        {
            playport = xtmsg_listenport;
            break;
        }
        //TCP 会话端口
    case LINK_TCP_TCP_PRI:
        {
            playport = tcp_listenport;
            break;
        }
        //Udp会话端口
    case LINK_UDP_RTP_PRI:
        {
            playport = udp_listenport;
            break;
        }
    default:
        {
            playport = -1;
            break;
        }
    }

    ip = local_ip_;

}
//////////////////////////////////////////////////////////////////////////////

//测试接口
void xmpp_client::test_play(std::string ip,int play_type,int play_ch,int strame,int trans_ch)
{

    XmppTag playbody("playbody","","");

    XmppTag play("play","action",PLAY_REQUEST);
    play.addAttribute("sessionType","RTSP_RTP");
    play.addAttribute("sid","XT_SID_VGA_123654");

    playbody.addChild(&play);

    XmppTag transmitInfo("transmitInfo","sessionType",NAT_TCP_RTP);
    XmppTag _NAT_TCP_RTP(NAT_TCP_RTP,"token",ip.c_str());
    _NAT_TCP_RTP.addAttribute("playType",play_type);
    _NAT_TCP_RTP.addAttribute("playIp",ip.c_str());	
    _NAT_TCP_RTP.addAttribute("playChannel",play_ch);
    _NAT_TCP_RTP.addAttribute("playPort","8000");
    _NAT_TCP_RTP.addAttribute("reuseFlag_s","false");
    _NAT_TCP_RTP.addAttribute("reuseFlag_r","false");
    _NAT_TCP_RTP.addAttribute("loginport",8000);
    _NAT_TCP_RTP.addAttribute("loginName","admin");
    _NAT_TCP_RTP.addAttribute("loginPassword","12345");
    _NAT_TCP_RTP.addAttribute("transmitChannel",trans_ch);			
    transmitInfo.addChild(&_NAT_TCP_RTP);
    playbody.addChild(&transmitInfo);

    XmppTag original("original","requestor",local_jid_.c_str());
    original.addAttribute("responder",local_jid_.c_str());
    original.addAttribute("mediaSrcChannel",0);
    original.addAttribute("streamType",strame);
    playbody.addChild(&original);

    if (XmppSendIQ(local_jid_.c_str(), "set", "id_xt-id123456", XMNLS_DB_PLAY, &playbody))
    {
        printf("send play test success!\n");
    }
    else
    {
        printf("send play test fail!\n");
    }

}

void xmpp_client::test_stop(std::string token,long transmitChannel)
{
    XmppTag playbody("playbody","","");

    XmppTag play("play","action",PLAY_STOP_REQUEST);
    play.addAttribute("sessionType","RTSP_RTP");
    play.addAttribute("sid","XT_SID_VGA_123654");

    playbody.addChild(&play);

    XmppTag transmitInfo("transmitInfo","sessionType",NAT_TCP_RTP);
    XmppTag _NAT_TCP_RTP(NAT_TCP_RTP,"token",token.c_str());
    _NAT_TCP_RTP.addAttribute("transmitChannel",transmitChannel);			
    transmitInfo.addChild(&_NAT_TCP_RTP);
    playbody.addChild(&transmitInfo);

    if (XmppSendIQ(local_jid_.c_str(), "set", "id_xt-id123456", XMNLS_DB_PLAYSTOP, &playbody))
    {
        printf("send stop test success!\n");
    }
    else
    {
        printf("send stop test fail!\n");
    }

}

void xmpp_client::test_play_inform_request()
{
    XmppTag playbody("playbody","","");

    XmppTag play("play","action",PLAY_INFORM_REQUST);
    play.addAttribute("sessionType","NAT_TCP_RTP");
    play.addAttribute("sid","XT_SID_VGA_123654");
    playbody.addChild(&play);

    XmppTag transmitInfo("transmitInfo","sessionType",NAT_TCP_RTP);
    XmppTag xmpp_arg(NAT_TCP_RTP,"routerip","172.16.5.231");
    xmpp_arg.addAttribute("routerport",20002);
    transmitInfo.addChild(&xmpp_arg);
    playbody.addChild(&transmitInfo);

    std::string strxml = playbody.GetInputXml();

    if (XmppSendIQ(local_jid_.c_str(), "set", "id_xt-id123456", XMNLS_DB_PLAYINFO, &playbody))
    {
        printf("send play_inform_request test success!\n");
    }
    else
    {
        printf("send play_inform_request test fail!\n");
    }
}

void xmpp_client::test_stop_inform_request()
{
    XmppTag playbody("playbody","","");

    XmppTag play("play","action",PLAY_STOP_INFORM_REQUEST);
    play.addAttribute("sessionType","NAT_TCP_RTP");
    play.addAttribute("sid","XT_SID_VGA_123654");
    playbody.addChild(&play);

    XmppTag transmitInfo("transmitInfo","sessionType",NAT_TCP_RTP);
    XmppTag xmpp_arg(NAT_TCP_RTP,"routerip","172.16.5.231");
    xmpp_arg.addAttribute("routerport",20002);
    transmitInfo.addChild(&xmpp_arg);
    playbody.addChild(&transmitInfo);

    std::string strxml = playbody.GetInputXml();

    if (XmppSendIQ(local_jid_.c_str(), "set", "id_xt-id123456", XMNLS_DB_PLAYSTOPINFO, &playbody))
    {
        printf("send stop_inform_request test success!\n");
    }
    else
    {
        printf("send stop_inform_request test fail!\n");
    }
}
#endif //#ifdef _USE_XMPP_FUNC_
