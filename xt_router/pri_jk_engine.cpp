#include "pri_jk_engine.h"
#include "router_task.h"
#include "Router_config.h"
#include "XTRouter.h"
#include "XTRouterLog.h"

pri_jk_engine pri_jk_engine::self;
pri_jk_engine::pri_jk_engine(void)
:m_init(false)
,m_run(true)
,m_channel(m_service)
,m_jk_thread(),
m_channel_bind_ok(false),
m_channel_connect_ok(false)
{}

int pri_jk_engine::uninit()
{
    m_run = false;
    m_jk_thread.join();
    m_init = false; 
    jk_uninit();
    return 0;
}

bool pri_jk_engine::init()
{
    std::cout<<"start jk to center..."<<std::endl;
    m_jk_thread = boost::thread(jk_thread_func);
    return true;
}

void pri_jk_engine::jk_thread_func(void)
{
    bool is_ip = true;
    while (_()->m_run)
    {
        if (!_()->m_init)
        {
            if (!_()->jk_init())
            {
                continue;
            }
            _()->m_init = true;
        }
        _()->m_service.run_one();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(THREAD_CHECK_FREQUENCY));
    }
}

bool pri_jk_engine::jk_init()
{
    DEBUG_LOG(DBLOGINFO,ll_info,"jk_init client_cha_name[%s] server_chan_name[%s]",JK_MSG_QUEUE_CLIENT,JK_MSG_QUEUE_SERVER);
    // 初始化数据传输通道
    if (!m_channel_bind_ok)
    {
        if ( m_channel.bind(JK_MSG_QUEUE_CLIENT, 16, MSG_QUEUE_SIZE))
        {
            m_channel_bind_ok = true;
        }
    }
    std::cout<<get_cur_time_microsec()<<" |m_channel.bind"<<std::endl;

    if (!m_channel_bind_ok)
    {
        return false;
    }

    if (!m_channel.connect(JK_MSG_QUEUE_SERVER))
    {
        return false;
    }
    std::cout<<get_cur_time_microsec()<<" |m_channel.connect"<<std::endl;

    jk_rpc_stub_.set_channel(&m_channel);

    std::cout<<get_cur_time_microsec()<<" |jk_rpc_stub_.set_channel(&m_channel)"<<std::endl;

    jk_rpc_stub_.connect_event(true);

    std::cout<<get_cur_time_microsec()<<" |jk_rpc_stub_.connect_event(true)"<<std::endl;

    m_channel_connect_ok = true;

    //更新交换本地IP绑定IP配置
    std::string ip = config::_()->get_local_ip_systemset("0.0.0.0");
    CXTRouter::Instance()->update_cfg_local_ip(ip);
    DEBUG_LOG(DBLOGINFO,ll_info,"upate xtrouter local build ip[%s] sucesse!",ip.c_str());

    return true;
}
bool pri_jk_engine::jk_uninit()
{
    jk_rpc_stub_.connect_event(false);
    return true;
}

//从jk获取用户信息超时时间
#define GET_LOGIN_INFO_TIMEOUT_MILLSEC  5000

#define DEFINE_RPC_METHOD_CLOSURE(method)   \
class method##_Closure : public rpc::closure_t  \
{   \
public: \
    method##_Closure(method##_RequestType *request, method##_ResponseType *response)    \
    :request_(request), \
    response_(response) \
{}  \
    void done() \
{   \
    std::cout << #method " done" << std::endl;  \
    delete request_;    \
    delete response_;   \
    delete this;    \
}   \
private:    \
    method##_RequestType *request_; \
    method##_ResponseType *response_;   \
};

DEFINE_RPC_METHOD_CLOSURE(SetVideoCenterPlayID)
DEFINE_RPC_METHOD_CLOSURE(SetVideoCenterPlayIDEx)
DEFINE_RPC_METHOD_CLOSURE(SendTransparentCommand)

bool pri_jk_engine::result_to_center(const std::string& ids, long chanid, long centerid, long res1, const std::string& res2,const std::string& exinfo)
{
    if (!m_channel_connect_ok)
    {
        return false;
    }
    DEBUG_LOG(DBLOGINFO,ll_info,"执行向中心反馈点播结果 |result_to_center |dev_ids[%s] dev_chanid[%d] dev_streamtype[%s] transmit_ch[%d] result_dev_type[%d] exinfo[%s]",
        ids.c_str(),chanid,res2.c_str(),centerid,res1,exinfo.c_str());

    SetVideoCenterPlayIDEx_RequestType *request = new SetVideoCenterPlayIDEx_RequestType;
    SetVideoCenterPlayIDEx_ResponseType *response = new SetVideoCenterPlayIDEx_ResponseType;

    request->set_ids(ids.c_str());
    request->set_dvr_chid(chanid);
    request->set_vcenter_chid(centerid);
    request->set_res1(res1);
    request->set_str_res1(res2.c_str());
    std::cout << "exinfo:"<< std::endl;
    request->set_sExInfo(exinfo.c_str());

    rpc::control_t control;
    jk_rpc_stub_.SetVideoCenterPlayIDEx(control, request, response, new SetVideoCenterPlayIDEx_Closure(request, response));
    return true;
}

bool pri_jk_engine::send_transparent_cmd_to_center(const std::string& ids, const std::string& ip, const std::string& cmds)
{
    if (!m_channel_connect_ok)
    {
        return false;
    }

    DEBUG_LOG(DBLOGINFO,ll_info,"send_transparent_cmd_to_center |ids[%s] ip[%s] cmds[%s]\n",ids.c_str(),ip.c_str(),cmds.c_str());

    SendTransparentCommand_RequestType *request = new SendTransparentCommand_RequestType;
    SendTransparentCommand_ResponseType *response = new SendTransparentCommand_ResponseType;

    request->set_ids(ids.c_str());
    request->set_ips(ip.c_str());
    request->set_cmds(cmds.c_str());

    rpc::control_t control;
    jk_rpc_stub_.SendTransparentCommand(control, request, response, new SendTransparentCommand_Closure(request, response));
    return true;
}

bool pri_jk_engine::sync_get_logininfo_from_center(const std::string&ids, std::string&name, std::string&pwd, long& port, std::string& res1, long&sub_type)
{
    if (!m_channel_connect_ok)
    {
        return false;
    }

    GetLoginInfo_RequestType request;
    GetLoginInfo_ResponseType response;

    request.set_ids(ids.c_str());

    rpc::control_t control;
    control.set_timeout(GET_LOGIN_INFO_TIMEOUT_MILLSEC);
    if (!jk_rpc_stub_.GetLoginInfo(control, &request, &response, NULL))
    {
        return false;
    }

    name = response.get_name();
    pwd = response.get_pwd();
    port = response.get_port();
    res1 = response.get_res1();
    sub_type = response.get_res2();

    return true;
}

void pri_jk_engine::set_gateway(const std::string&ids, const std::string& ip)
{
    m_sIDS_Gateway = ids;
    m_sIPS_Gateway = ip;
}

void pri_jk_engine::set_center_ids(const std::string&ids)
{
    m_sIDS_Center = ids;
}

void pri_jk_engine::set_local_ids(const std::string&ids)
{
    m_sIDS_Local = ids;
}
