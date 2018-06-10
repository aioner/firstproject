//
//create by songlei 20160316
//
#include "dps_ch_mgr.h"
#include "share_type_def.h"
#include "sdp.h"
#include "parse_buffer.h"
#include "dps_cfg_mgr.h"
#include "dps_data_send_mgr.h"
#include "dps_stream_monitor.h"

#ifdef _WIN32
#ifdef _DEBUG
# pragma comment(lib,"xt_sdp_d.lib")
#pragma message("Auto Link xt_sdp_d.lib")
#else
# pragma comment(lib,"xt_sdp.lib")
#pragma message("Auto Link xt_sdp.lib")
#endif // end #ifdef _DEBUG
#endif // end #ifdef _WIN32

//class src_info_t
////////////////////////////////////////////////////////////////////////////////////

void src_info_t::assign() 
{
    ++usr_;
}

void src_info_t::realse()
{
    if (0 == --usr_)
    {
        if (-1 < srcno_)
        {
            dps_data_send_mgr::_()->destroy_src(srcno_);
            delete this;
        }
    }
}

void src_info_t::set_sdp(const char* sdp,const long sdp_len,const long data_type)
{
    do 
    {
        data_type_ = data_type;
        sdp_len_ = sdp_len;

        if (NULL == sdp || sdp_len < 1)
        {
            track_num_ = -1;
            break;
        }
        std::memcpy(sdp_,sdp,sdp_len_);
        
        write_lock_t lock(mutex_track_infos_);
        track_infos_.clear();
        xt_sdp::parse_buffer_t pb(sdp_, sdp_len_);
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            track_num_ = -1;
            break;
        }

        std::map<std::string, std::list<std::string> >::iterator itr_ctrl;
        int ctrl_id=-1;
        bool is_exists_ctrl = false;
        track_ctx_t track_info;
        xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
        for(; xsdp.media_.end() != itr;++itr)
        {
            if (itr->attribute_helper_.exists("control"))
            {
                itr_ctrl = itr->attribute_helper_.attributes_.find("control");
                if (itr->attribute_helper_.attributes_.end() != itr_ctrl)
                {
                    if (!itr_ctrl->second.empty())
                    {
                        int trackid = find_trackid(itr_ctrl->second.front());
                        if (-1 != trackid)
                        {
                            track_info.track_id = trackid;
                            is_exists_ctrl = true;
                        }
                    }
                }
            }
            if ( 0 == itr->name_.compare("video"))
            {
                if (!is_exists_ctrl)
                {
                    track_info.track_id = 1;
                }
                track_info.track_type = MEDIA_TYPE_VIDEO; 
                std::strncpy(track_info.track_name,"video",DPS_MAX_TRACK_NAME_LEN);
            }
            else if ( 0 == itr->name_.compare("audio"))
            {
                if (!is_exists_ctrl)
                {
                    track_info.track_id = 2;
                }

                track_info.track_type = MEDIA_TYPE_AUDIO;
                std::strncpy(track_info.track_name,"audio",DPS_MAX_TRACK_NAME_LEN);
            }
            else
            {
                track_info.track_id = -1;
                track_info.track_type = MEDIA_TYPE_NA;
                std::strncpy(track_info.track_name,itr->name_.c_str(),DPS_MAX_TRACK_NAME_LEN);
            }
            track_infos_.push_back(track_info);
        }
        track_num_ = track_infos_.size();
    } while (0);
}
////////////////////////////////////////////////////////////////////////////////////

//class dps_trans_src_mgr
////////////////////////////////////////////////////////////////////////////////////
dps_trans_src_mgr dps_trans_src_mgr::my_;

t_src_info_handle_t dps_trans_src_mgr::malloc(const src_info_t& src)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_trans_src_handles_);
    t_src_info_handle_t ts_handle = T_SRC_INFO_HANDLE_NA;
    t_src_info_handle_container_t_itr_t itr_hsrc = trans_src_handles_.begin();

	int existFlag = 0;
    for(;trans_src_handles_.end() != itr_hsrc;++itr_hsrc)
    {
		existFlag = 0;
        ts_handle = *itr_hsrc;
        if (src.srcno_ == ts_handle->srcno_)
        {
			existFlag = 1;
            break;
        }
    }

    if (existFlag == 0/*T_SRC_INFO_HANDLE_NA == ts_handle*/)
    {
        ts_handle = new src_info_t(src);
        if (T_SRC_INFO_HANDLE_NA != ts_handle)
        {
            trans_src_handles_.push_back(ts_handle);
        }
    }

    if (T_SRC_INFO_HANDLE_NA != ts_handle) ts_handle->assign();

    return ts_handle;
}
void dps_trans_src_mgr::free(const t_src_info_handle_t ts_handle)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_trans_src_handles_);
    t_src_info_handle_container_t_itr_t itr_hsrc = trans_src_handles_.begin();
    while(trans_src_handles_.end() != itr_hsrc)
    {
        if (ts_handle == *itr_hsrc)
        {
            ts_handle->realse();
            itr_hsrc = trans_src_handles_.erase(itr_hsrc);
        }
        else
        {
            ++itr_hsrc;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////

//class dps_dev_stream_t
////////////////////////////////////////////////////////////////////////////////////

bool dps_dev_stream_t::src_is_create()
{
    read_lock_t lock(mutex_trans_src_handle_);
    bool ret_code = false;
    do
    {
        if (T_SRC_INFO_HANDLE_NA == trans_src_handle_)
        {
            ret_code = false;
            break;
        }
        ret_code = true;
    } while (0);
    return ret_code;
}
void dps_dev_stream_t::set_dev_handle(const dev_handle_t& hanle)
{
    dev_handle_ = hanle;
    recv_data_refurbish();
    dps_break_monitor_mgr::_()->post(this);
}

void dps_dev_stream_t::send_media_data(unsigned char* data,const long len, const long frame_type,const long data_type,const long time_stamp,const unsigned long ssrc)
{
    write_lock_t lock(mutex_trans_src_handle_);
    if (NULL != send_data_cb_)
    {
        media_type_t media_type = parse_media_type(frame_type);
        if ( MEDIA_TYPE_NA != media_type)
        {
            if (T_SRC_INFO_HANDLE_NA != trans_src_handle_)
            {
                send_data_cb_(trans_src_handle_->srcno_,trans_src_handle_->parse_track_id(media_type),data,len,frame_type,data_type,time_stamp);
            }
            else
            {
                // write fail log ......
            }
        }
        else
        {
            // write fail log ......
        }
    }

}

media_type_t dps_dev_stream_t::parse_media_type(const unsigned int frame_type)
{
    media_type_t media_type = MEDIA_TYPE_NA;
    switch (frame_type)
    {
    case OV_VIDEO_I:
    case OV_VIDEO_P:
    case OV_VIDEO_B:
    case OV_H264:
    case OV_H264_I:
    case OV_H264_P:
    case OV_H264_B:
    case OV_H264_SEI:
    case OV_H264_SPS:
    case OV_H264_PPS:
    case OV_H265:
        {
            media_type = MEDIA_TYPE_VIDEO;
            break;
        }
    case OV_AUDIO:
    case OV_G711:
    case OV_AAC:
        {
            media_type = MEDIA_TYPE_AUDIO;
            break;
        }
    default:
        break;
    }
    return media_type;
}

int dps_dev_stream_t::add_src_info(const src_info_t& src_info)
{
    int ret_code = -1;
    do 
    {
        read_lock_t lock(mutex_trans_src_handle_);
        trans_src_handle_ = dps_trans_src_mgr::_()->malloc(src_info);
        if (T_SRC_INFO_HANDLE_NA == trans_src_handle_)
        {
            ret_code = -1;
            break;
        }
    } while (0);
    return ret_code;
}

void dps_dev_stream_t::clear_src_info()
{
    set_send_data_cb(NULL);
    read_lock_t lock(mutex_trans_src_handle_);
    if (trans_src_handle_ != T_SRC_INFO_HANDLE_NA)
    {
        dps_trans_src_mgr::_()->free(trans_src_handle_);
    }
}

t_src_info_handle_t& dps_dev_stream_t::get_src()
{
    read_lock_t lock(mutex_trans_src_handle_);
    return trans_src_handle_;
}

std::string dps_dev_stream_t::get_create_time() const
{
    std::string strtime = to_iso_extended_string(create_time_);
    std::string::size_type uiPos = strtime.find("T");
    if (uiPos != std::string::npos)
    {
        strtime.replace(uiPos,1,std::string(" "));
    }
    uiPos = strtime.find(".");
    if (uiPos != std::string::npos)
    {
        strtime.replace(uiPos,1,std::string(":"));
    }
    return strtime;
}

void dps_dev_stream_t::stop_capture()
{
    int ret = dps_device_access_mgr::_()->stop_capture(dev_handle_);
    set_state(STATE_CLOSE);
}
////////////////////////////////////////////////////////////////////////////////////


//class dps_ch_mgr
////////////////////////////////////////////////////////////////////////////////////
dps_ch_mgr dps_ch_mgr::my_;

int dps_ch_mgr::init()
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_dps_ch_infos_);
    dps_cfg_mgr::dps_dev_list_container_t dev_lst;
    dps_cfg_mgr::_()->loading_dev_list(dev_lst);

    dps_dev_s_handle_t s_handle = DPS_DEV_S_HANDLE_NA;
    dps_ch_info_t ch_info;
    for(dps_cfg_mgr::dps_dev_list_container_itr_t itr = dev_lst.begin(); dev_lst.end() != itr;++itr)
    {
        s_handle = static_cast<dps_dev_s_handle_t>(new dps_dev_stream_t());
        if (DPS_DEV_S_HANDLE_NA == s_handle) continue;

        ch_info.ch = itr->transmit_ch;
        ch_info.s_handle = s_handle;

        s_handle->set_device(*itr);
        dps_ch_infos_.push_back(ch_info);
    }
    return dps_ch_infos_.empty() ? -1 : 0;
}
void dps_ch_mgr::uninit()
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_dps_ch_infos_);
    for (dps_ch_infos_container_itr_t itr = dps_ch_infos_.begin(); dps_ch_infos_.end() != itr; ++itr)
    {
        if (DPS_DEV_S_HANDLE_NA != itr->s_handle)
        {
            delete itr->s_handle;
            itr->s_handle = DPS_DEV_S_HANDLE_NA;
        }
    }
    dps_ch_infos_.clear();
}
void dps_ch_mgr::get_s_handle_by_ch(const dps_ch_t ch,dps_dev_s_handle_container_t& s_handles)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_dps_ch_infos_);
    s_handles.clear();
    for (dps_ch_infos_container_itr_t itr = dps_ch_infos_.begin(); dps_ch_infos_.end() != itr; ++itr)
    {
        if (ch == itr->ch)
        {
            s_handles.push_back(itr->s_handle);
        }
    }
}

srcno_t dps_ch_mgr::get_srcno_by_ch_and_streamtype(const dps_ch_t ch,const long stream_type)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_dps_ch_infos_);
    for (dps_ch_infos_container_itr_t itr = dps_ch_infos_.begin(); dps_ch_infos_.end() != itr; ++itr)
    {
        dps_dev_s_handle_t s_handle = itr->s_handle;
        if (stream_type == s_handle->get_device().stream_type &&ch == s_handle->get_device().transmit_ch)
        {
            return s_handle->get_src()->srcno_;
        }
    }
    return -1;
}

void dps_ch_mgr::get_all_s_handle(dps_dev_s_handle_container_t& s_handles)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_dps_ch_infos_);
    s_handles.clear();
    for (dps_ch_infos_container_itr_t itr = dps_ch_infos_.begin(); dps_ch_infos_.end() != itr; ++itr)
    {
            s_handles.push_back(itr->s_handle);
    }
}

void dps_ch_mgr::play_all_ch(play_cb_t cb)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_dps_ch_infos_);
    for (dps_ch_infos_container_itr_t itr = dps_ch_infos_.begin(); dps_ch_infos_.end() != itr; ++itr)
    {
        if (NULL != cb)
        {
            cb(itr->s_handle);
        }
    }
}

void dps_ch_mgr::stop_all_ch(stop_cb_t cb)
{
    boost::lock_guard<boost::detail::spinlock> lock(mutex_dps_ch_infos_);
    for (dps_ch_infos_container_itr_t itr = dps_ch_infos_.begin(); dps_ch_infos_.end() != itr; ++itr)
    {
        if (NULL != cb)
        {
            if (itr->s_handle->is_open())
            {
                cb(itr->s_handle);
            }
        }
    }
}

void dps_ch_mgr::stop_ch(const dps_ch_t ch,stop_cb_t cb)
{
    dps_dev_s_handle_container_t s_hadles;
    get_s_handle_by_ch(ch,s_hadles);
    dps_dev_s_handle_container_itr_t itr = s_hadles.begin();
    for(; s_hadles.end() != itr; ++itr)
    {
        cb(*itr);
    }

}
////////////////////////////////////////////////////////////////////////////////////
