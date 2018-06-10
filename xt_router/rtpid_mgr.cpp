#include "rtpid_mgr.h"

rtp_id_mr rtp_id_mr::self_;


rtp_id_mr::rtp_id_mr():
rtp_id_(RTP_ID_VALUE_NA)
{

}
rtp_id_mr::~rtp_id_mr()
{
    rtp_id_lst_.clear();
}

rtp_id_t rtp_id_mr::create_rtpid(const rtp_id_attr_t& rtp_id_attr)
{
    boost::unique_lock<boost::shared_mutex>(rtp_id_mgr_mutex);
    rtp_id_t ret_rtp_id = 0;
    bool is_find = false;
    rtp_id_container_handle_t itr = rtp_id_lst_.begin();
    for(; rtp_id_lst_.end() != itr; ++itr)
    {
        if (!itr->active)
        {
            is_find = true;
           itr->active = true;
           itr->srcno = rtp_id_attr.srcno;
           itr->sdp = rtp_id_attr.sdp;
           itr->dev_handle = rtp_id_attr.dev_handle;
           itr->direction = rtp_id_attr.direction;

		   itr->m_name = rtp_id_attr.m_name;

           ret_rtp_id = itr->rtp_id;
           break;
        }
    }
    if (!is_find)
    {
        rtp_id_attr_t a;
        a.rtp_id = ++rtp_id_;
        a.active = true;
        a.direction = rtp_id_attr.direction;
        a.sdp = rtp_id_attr.sdp;
        a.dev_handle = rtp_id_attr.dev_handle;
        a.srcno = rtp_id_attr.srcno;
		a.m_name = rtp_id_attr.m_name;
        rtp_id_lst_.push_back(a);

        ret_rtp_id = a.rtp_id;
    }

    return ret_rtp_id; 
}
void rtp_id_mr::free_rtpid(rtp_id_t id)
{
    boost::unique_lock<boost::shared_mutex>(rtp_id_mgr_mutex);
    rtp_id_container_handle_t itr = rtp_id_lst_.begin();
    for(; rtp_id_lst_.end() != itr; ++itr)
    {
        if (itr->rtp_id == id)
        {
            itr->active = false;
        }
    }
}

long rtp_id_mr::get_rtpid_attr(const rtp_id_t rtp_id,rtp_id_attr_t& rtp_id_attr)
{
    long ret_code=0;
    do 
    {
        boost::unique_lock<boost::shared_mutex>(rtp_id_mgr_mutex);
        bool is_find = false;
        rtp_id_container_handle_t itr = rtp_id_lst_.begin();
        for(; rtp_id_lst_.end() != itr; ++itr)
        {
            if (itr->rtp_id == rtp_id)
            {
                rtp_id_attr = *itr;
                is_find = true;
				break;
            }
        }

        if (!is_find)
        {
            ret_code = -1;
            break;
        }
        ret_code = 1;
    } while (0);
    return ret_code;
}

long rtp_id_mr::update_sdp_by_rtpid(const rtp_id_t rtp_id,const std::string& sdp)
{
    long ret_code=0;
    do 
    {
        boost::unique_lock<boost::shared_mutex>(rtp_id_mgr_mutex);
        bool is_find = false;
        rtp_id_container_handle_t itr = rtp_id_lst_.begin();
        for(; rtp_id_lst_.end() != itr; ++itr)
        {
            if (itr->rtp_id == rtp_id && itr->active)
            {
				itr->sdp = sdp;
                is_find = true;
				break;
            }
        }

        if (!is_find)
        {
            ret_code = -1;
            break;
        }
        ret_code = 1;
    } while (0);
    return ret_code;
}

void rtp_id_mr::add_rtp_dst_to_rtpid(const rtp_id_t rtpid,const rtp_dst_info_t& dst)
{
    boost::unique_lock<boost::shared_mutex>(rtp_id_mgr_mutex);
    rtp_id_container_handle_t itr = rtp_id_lst_.begin();
    for(; rtp_id_lst_.end() != itr; ++itr)
    {
        if (itr->rtp_id == rtpid && itr->active)
        {
            itr->send_dsts.push_back(dst);
        }
    }
}

void rtp_id_mr::del_rtp_dst_to_rtpid(const rtp_id_t rtpid,const rtp_dst_info_t& dst)
{
	boost::unique_lock<boost::shared_mutex>(rtp_id_mgr_mutex);
	rtp_id_container_handle_t itr = rtp_id_lst_.begin();
	for(; rtp_id_lst_.end() != itr; ++itr)
	{
		if (itr->rtp_id == rtpid && itr->active)
		{
			itr->send_dsts.clear();
		}
	}
}

void rtp_id_mr::set_strmid(const dev_handle_t &dev_handle,long strmid)
{
	boost::unique_lock<boost::shared_mutex>(rtp_id_mgr_mutex);
	rtp_id_container_handle_t itr = rtp_id_lst_.begin();
	for(; rtp_id_lst_.end() != itr; ++itr)
	{
		if (itr->dev_handle == dev_handle)
		{
			itr->strmid = strmid;
		}
	}
}

long rtp_id_mr::get_strmid(const dev_handle_t &dev_handle)
{
	boost::unique_lock<boost::shared_mutex>(rtp_id_mgr_mutex);
	rtp_id_container_handle_t itr = rtp_id_lst_.begin();
	for(; rtp_id_lst_.end() != itr; ++itr)
	{
		if (itr->dev_handle == dev_handle)
		{
			return itr->strmid;
		}
	}

	return -1;
}
