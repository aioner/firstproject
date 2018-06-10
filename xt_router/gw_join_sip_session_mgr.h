#ifndef _GW_JOIN_SIP_SESSION_MGR_H__
#define _GW_JOIN_SIP_SESSION_MGR_H__
#include <boost/noncopyable.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include "common_type.h"

typedef struct _struct_session_info_type_
{
    long sessionid;   // 会话ID
    std::string recv_ids;//接收时的源ids
    std::string send_ids;
    int       type;
    long       stremtype;
    long       recv_chid;
    long       send_chid;//发送时的源通道
    std::string sdp;//远端sdp
    long sdp_len;
    int transmitchannel;    //转发服务通道
    long dev_handle;//关联设备句柄
    int srcno;//转发源

    _struct_session_info_type_():sessionid(-1),recv_ids(""),send_ids(""),type(-1),
       stremtype(-1),recv_chid(-1),send_chid(-1),sdp(),sdp_len(0),transmitchannel(-1),dev_handle(-1),srcno(-1)
    {}
    typedef std::list<rtp_dst_info_t> rtp_dst_container_t;
    typedef rtp_dst_container_t::iterator rtp_dst_container_handle_t;
    rtp_dst_container_t send_dsts; //推流目的信息

    typedef _struct_session_info_type_ my_type;
    my_type &operator=(const my_type& other)
    {
        this->sessionid = other.sessionid;
        this->recv_ids = other.recv_ids;
        this->send_ids = other.send_ids;
        this->type = other.type;
        this->stremtype = other.stremtype;
        this->recv_chid = other.recv_chid;
        this->send_chid = other.send_chid;
        this->sdp = other.sdp;
        this->sdp_len = other.sdp_len;
        this->transmitchannel = other.transmitchannel;
        this->dev_handle = other.dev_handle;
        this->srcno = other.srcno;
        this->send_dsts = other.send_dsts;
        return *this;
    }
}session_inf_t,*ptr_session_inf_t;

class gw_join_sip_session_mgr : boost::noncopyable
{
public:
    static gw_join_sip_session_mgr* instance(){return &self_;}
    static gw_join_sip_session_mgr* _(){return &self_;}
protected:
    gw_join_sip_session_mgr();
    ~gw_join_sip_session_mgr();
    static gw_join_sip_session_mgr self_;
public:
    long save_session(const session_inf_t& inf);
    void del_session(const long sessionid);
    void del_session_by_ids(const std::string& ids);
    long get_session_by_ids(const std::string&ids, session_inf_t& outinf);
    void clear_session_all();
    long get_session(const long sessionid,session_inf_t& outinf);
    long get_session(const std::string& recvids,const std::string& sendids,session_inf_t& outinf);
    long clear_session(const long sessionid);
    long save_sdp_to_session(const long sessionid,const char *sdp, const long len_sdp);
    long save_send_dst(const long sessionid,const rtp_dst_info_t& dst);
    long update_session(const session_inf_t& session);
    void get_session_all(std::map<long,session_inf_t>& session);
private:
    //sip会话管理
    ///////////////////////////////////////////////////////////////////////////
    boost::recursive_mutex         mutex_sessionid;    //gw_jion_sip_session_mgr资源锁
    std::map<long,session_inf_t>       gw_jion_sip_session_mgr;  //会话管理table session_id to session_inf_to
    ///////////////////////////////////////////////////////////////////////////
};
#endif //#ifndef _GW_JOIN_SIP_SESSION_MGR_H__