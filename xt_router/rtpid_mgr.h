#ifndef _RTP_MGR_H__
#define _RTP_MGR_H__
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "common_type.h"
typedef struct __strcut_rtp_id_attr_type_
{
    rtp_id_t rtp_id;
    bool active;
    sdp_direction_t direction;
    std::string m_name;//video audio
    std::string sdp;
    int srcno;
    dev_handle_t dev_handle;
	long strmid;

    typedef __strcut_rtp_id_attr_type_ my_t;
    typedef std::list<rtp_dst_info_t> rtp_dst_container_t;
    typedef rtp_dst_container_t::iterator rtp_dst_container_handle_t;
    rtp_dst_container_t send_dsts;

    __strcut_rtp_id_attr_type_():
    rtp_id(0),active(false),direction(dir_na),m_name(),sdp(),srcno(-1),dev_handle(-1),strmid(-1){};

    my_t& operator=(const my_t& rf)
    {
        this->rtp_id = rf.rtp_id;
        this->active = rf.active;
        this->direction = rf.direction;
        this->sdp = rf.sdp;
        this->srcno = rf.srcno;
        this->dev_handle = rf.dev_handle;
		this->strmid = rf.strmid;
        this->m_name = rf.m_name;
        return *this;
    }

}rtp_id_attr_t,*ptr_rtp_id_attr_t;

class rtp_id_mr : boost::noncopyable
{
public:
    static rtp_id_mr* instance(){return &self_;}
    static rtp_id_mr* _(){return &self_;}
protected:
    rtp_id_mr();
    ~rtp_id_mr();
    static rtp_id_mr self_;

public:
    rtp_id_t create_rtpid(const rtp_id_attr_t& rtp_id_attr);
     void free_rtpid(rtp_id_t id);//rtpid释放后可复用，不用重新生成
     long get_rtpid_attr(const rtp_id_t rtp_id,rtp_id_attr_t& rtp_id_attr);
     long update_sdp_by_rtpid(const rtp_id_t rtp_id,const std::string& sdp);
     void add_rtp_dst_to_rtpid(const rtp_id_t rtpid,const rtp_dst_info_t& dst);
	 void del_rtp_dst_to_rtpid(const rtp_id_t rtpid,const rtp_dst_info_t& dst);
	 void set_strmid(const dev_handle_t &dev_handle,long strmid);
	 long get_strmid(const dev_handle_t &dev_handle);
public:
    typedef std::vector<rtp_id_attr_t> rtp_id_container_t;
    typedef std::vector<rtp_id_attr_t>::iterator rtp_id_container_handle_t;
private:
    boost::shared_mutex rtp_id_mgr_mutex;
    rtp_id_container_t rtp_id_lst_;//该结构用于保存所有sdp，并能根据sdp的rtpid查询
    rtp_id_t rtp_id_;
};


#endif //#ifndef _RTP_MGR_H__
