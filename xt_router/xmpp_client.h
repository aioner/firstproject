/*@
*@FileNmae:  xmpp_client.h
*@Describe:  XMPP信令会话处理头文件
*@Author:    songlei
*@Date:      2014-11-24
**/
#ifndef _XMPP_H__
#define _XMPP_H__
#ifdef _USE_XMPP_FUNC_
#include "XmppGlooxApply.h"
#ifdef _WIN32
#pragma comment(lib,"XmppGlooxApply.lib")
#else
#define __stdcall
#endif //#ifdef _WIN32

#include "xmpp_type_def.h"
#include <boost/thread/mutex.hpp>
#include <map>
#include <string>
#include <stdint.h>
#include <functional>
#include<boost/thread.hpp>
#include <boost/atomic/atomic.hpp>

namespace xmpp_ret_code
{
    enum ret_code_type
    {
        XML_STOP_INFRM_ARG_ERR = -11, //停止TCP反向注册参数错误
        XML_PLAY_INFRM_ARG_ERR = -10, //开始TCP反向注册参数错误
        XML_STOP_ARG_ERR = -9,       //停点参数错误
        XML_PLAY_ARG_ERR = -8,       //点播参数错误
        MEMORY_ALLOT_FAIL = -7,      //内存分配失败
        SAVE_IQ_MSG_FAIL = -6,       //保存IQ消息失败
        PLAY_FAIL = -5,             //点播失败
        LOGIN_EXIST = -4,           //登录已存在
        PARSE_PLAY_SIGNALLING_FAIL = -3,//解析点播信令失败
        SEND_IQ_FAIL = -2,          // 发送IQ消息失败
        LOGIN_CENTER_FAIL = -1,     // 登录中心失败
        LONGIN_CENTER_SUCCESS = 0,  // 登录中心成功
        SEND_IQ__SUCCESS = 1,
        PARSE_SIGNALLING_SUCCESS=2, //解析播信令成功
        PLAY_SUCCESS=3,//点播成功
        SAVE_IQ_MSG_SUCCESS = 4,//保存IQ消息成功
    };

    enum parse_url_ret_code
    {
        RET_PASE_CH_FAIL = -3,       // 解析ch失败
        RET_PARSE_PORT_FAIL = -2,    // 解析Port失败
        RET_PARSE_IP_FAIL = -1,     // 解析IP部分失败
        RET_PARSE_SUCCESS  = 0,
    };
}

#define  IP_STR_SIZE 32
typedef struct _struct_play_src_info
{
    long transmit_channel;//指定的转发服务通道
    long dev_type; //SRC类型
    long play_channel;//要点播源的通道

}PLAY_SRC,*PPLAY_SRC;
#define  PLAY_SRC_SIZE sizeof(PLAY_SRC);

class xmpp_client : private boost::noncopyable
{
    //单例相关
    /////////////////////////////////////////////////
public:
    static xmpp_client* instance(){return &obj_;}
private:
    static xmpp_client obj_;
    xmpp_client();
    ~xmpp_client();

    //回调函数
    /////////////////////////////////////////////////
public:
    static void __stdcall XmppConnIoErrorCB(void* pUser);
    static void __stdcall XmppOutLogCB(void* pUser, const char* log);
    static void __stdcall XmppClientRcvPrensenceCB(void* pUser, const int OnlineStatus, const char* jid, const int pri, const char* status);
    static void __stdcall XmppRcvMsgCB(void* pUser, const char* jid, const char* type, const char* MsgBody);
    static void __stdcall XmppRcvIQCB(void* pUser, const char* jid, const char* type, const char* id, const char* NameSpace,const char* IQBody);
    static void __stdcall XmppConnStreamErrorCB(void* pUser);
    /////////////////////////////////////////////////

protected:
    // 中心监测线程函数
    void ccs_link_fun();

public:
    xmpp_ret_code::ret_code_type play_result_to_ccs(const iq_type<play_requst_body>& iq,int ret_state);
    xmpp_ret_code::ret_code_type stop_play_result_to_ccs(const iq_type<stop_requst_body>& iq,int ret_state);
    xmpp_ret_code::ret_code_type play_inform_replay_to_ccs(const iq_type<play_inform_request_body>& iq,int ret_state);
    xmpp_ret_code::ret_code_type stop_inform_replay_to_ccs(const iq_type<stop_inform_request_body>& iq,int ret_state);

public:
    xmpp_ret_code::ret_code_type start_xmpp_client();

    bool login(const char* name,const char* server,const char* password,const int port,const char* source);

    void stop_xmpp_client();
    bool is_xmpp();	

    xmpp_ret_code::ret_code_type parse_play_iq(const char* jid, const char* type, const char* id,const char* name_space,const char* iq_body);

    xmpp_ret_code::ret_code_type parse_stop_play_iq(const char* jid, const char* type, const char* id,const char* name_space,const char* iq_body);

    xmpp_ret_code::ret_code_type parse_play_inform_request_iq(const char* jid, const char* type, const char* id,const char* name_space,const char* iq_body);

    xmpp_ret_code::ret_code_type parse_stop_inform_request_iq(const char* jid, const char* type, const char* id,const char* name_space,const char* iq_body);

    //功能接口
    ///////////////////////////////////////////////////////////	
    //解析url
    xmpp_ret_code::parse_url_ret_code parse_url(const std::string& url,std::string& ip,uint32_t& port,long& ch);

    //比较字符串
    bool compare_str(const char* str1,const char* str2);
    ///////////////////////////////////////////////////////////
public:
    int to_xtrouter_linktype(const std::string& session_type,bool reuse_flag=false);

    void get_local_net_info(std::string& ip,long& playport,const int xt_link_type);

    const std::string& get_local_jid()    const {return local_jid_;}
    const std::string& get_local_ip()     const {return local_ip_;}
    const uint32_t get_regist_bind_port() const{return regist_bind_port_;}
    const std::string get_ccs_name() const {return center_host_name_;}

public:
    //测试接口
    void test_play(std::string ip,int play_type,int play_ch,int strame,int trans_ch);
    void test_stop(std::string token,long transmitChannel);
    void test_play_inform_request();
    void test_stop_inform_request();
private:
    void init_cfg();
private:
    boost::atomic_bool is_xmpp_;
    boost::atomic_bool is_stop_all_;
    std::string log_name_;     //登录Openfire用户名
    std::string log_password_; //密码
    std::string log_res_id_;   //资源ID
    std::string center_host_name_;//Openfire主机名
    uint32_t    log_port_;    //登录Openfire端口
    std::string local_ip_;    //本地IP
    std::string local_jid_;   //交换自己的jid/ids
    uint32_t    regist_bind_port_;//本地TCP反向链接端口

    unsigned long xtmsg_listenport;			//xtmsg监听端口
    unsigned long rtsp_listenport;			//rtsp监听端口
    unsigned long tcp_listenport;			//tcp发送&监听端口
    unsigned long udp_listenport;         //udp

    boost::atomic_bool m_run;
    boost::thread* m_pThread;				//中心监测线程
};
#endif //#ifdef _USE_XMPP_FUNC_
#endif //_XMPP_H__

