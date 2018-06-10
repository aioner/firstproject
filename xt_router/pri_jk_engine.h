#ifndef PRI_JK_ENGINE_H__
#define PRI_JK_ENGINE_H__

#include "rpc/rpc_message_queue.h"
#include "JkMainClientRpcClient.h"

#include <boost/noncopyable.hpp>
#include <boost/atomic.hpp>

class pri_jk_engine: boost::noncopyable
{
private:
    pri_jk_engine(void);
    static pri_jk_engine self;
    static void jk_thread_func(void);

public:
    static pri_jk_engine* _(){return &self;}

    bool init();
    int uninit();

    bool result_to_center(const std::string& ids, long chanid, long centerid, long res1, const std::string& res2,const std::string& exinfo);
    bool send_transparent_cmd_to_center(const std::string& ids, const std::string& ip, const std::string& cmds);
    bool sync_get_logininfo_from_center(const std::string&ids, std::string&name, std::string&pwd, long& port, std::string& res1, long&sub_type);

    void set_gateway(const std::string&ids, const std::string& ip);
    void set_center_ids(const std::string&ids);
    void set_local_ids(const std::string&ids);

    const std::string& get_gateway_ids() const { return m_sIDS_Gateway; }
    const std::string& get_gateway_ip() const { return m_sIPS_Gateway; }
    const std::string& get_center_ids() const { return m_sIDS_Center; }
    const std::string& get_local_ids() const { return m_sIDS_Local; }

private:
    // 初始化
    bool jk_init();
    bool jk_uninit();
    // 消息队列服务
    rpc::message_queue::service_t m_service;
    // 消息通道
    rpc::message_queue::client_channel_t m_channel;
    // jk消息队列
    boost::thread m_jk_thread;
    // jk对象
    JkMainClientRpcStub jk_rpc_stub_;
    boost::atomic_bool m_run;
    boost::atomic_bool m_init;
    std::string m_sIDS_Gateway;     // 同步网关
    std::string m_sIPS_Gateway;
    std::string m_sIDS_Center;      //中心
    std::string m_sIDS_Local;       //交换网关
    volatile bool m_channel_bind_ok;
    volatile bool m_channel_connect_ok;
};
#endif //#ifndef PRI_JK_ENGINE_H__