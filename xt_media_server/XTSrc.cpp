#include "XTSrc.h"
#include "XTChan.h"
#include "XTRtp.h"
#include "share_type_def.h"
#include <algorithm>
#include <vector>

extern MS_CFG ms_cfg;
extern void MEDIA_SVR_PRINT(const xt_log_level ll,const char* format,...);
multi_code_mgr multi_code_mgr::self_;
XTSrc XTSrc::self_;

int XTSrc::get_trackid(const std::string& tsr)
{
    if (tsr.empty())
    {
        return -1;
    }

    std::string::size_type pos = tsr.find_last_not_of("0123456789");
    if (std::string::npos == pos)
    {
        return -1;
    }

    std::string trackid = tsr.substr(pos + 1);
    return atoi(trackid.c_str());
}

int XTSrc::create_src_defult(int* srcno,char sdp[],int* sdp_len,const long chanid,const std::string& local_bind_ip)
{
    int ret_code = -1;
    do 
    {
        int tracknum = 2;
        int trk_ids[2] = {1,2};
        char *tracknames[2]={"video","audio"};
        int _srcno = -1;
        ret_code = create_src(tracknum,trk_ids,tracknames,_srcno,chanid);
        if (ret_code < 0)
        {
            ret_code = -1;
            break;
        }

        xt_track_t trackinfs[2]={0};
        for (int t=0; t < tracknum; ++t)
        {
            xt_track_t std_track;
            get_track(_srcno,trk_ids[t],std_track);
            trackinfs[t].trackid = std_track.trackid;
            trackinfs[t].chanid = std_track.chanid;
            trackinfs[t].tracktype = std_track.tracktype;
            ::strncpy(trackinfs[t].trackname,std_track.trackname,MAX_TRACKNAME_LEN);
        }

        //生成sdp
        int _sdp_len = -1;
        char _sdp[2048] = {0};
        ret_code = create_sdp(trackinfs,tracknum,_sdp,_sdp_len,local_bind_ip);
        if (ret_code < 0)
        {
            break;
        }
        ::strncpy(sdp,_sdp,_sdp_len);
        ::memmove(sdp_len,&_sdp_len,sizeof(int));
        ::memmove(srcno,&_srcno,sizeof(int));

        ret_code = 0;
    } while (0);
    return ret_code;
}
int XTSrc::create_src_sdp(int* srcno,xt_track_t* trackinfos,int* tracknum,const char* sdp,const long sdp_len,const long chanid)
{
    int ret_code = -1;
    do 
    {
        write_lock_t lock(m_mutex);
        xt_src *src = new xt_src;
        if (!src)
        {
            ret_code = -1;
            break;
        }

        //创建私有流的转发源
        xt_track_t track;
        track.trackid = PRI_TRACK_ID;
        ::memset(track.trackname,0,MAX_TRACKNAME_LEN);
        track.tracktype = -1;
        if (chanid >= 0)
        {
            track.chanid = chanid;
            XTChan::instance()->active_chan(track.chanid, true);
        }
        else
        {
            unsigned long chanid = 0;
            ret_code = XTChan::instance()->get_freechan(chanid);
            if (ret_code < 0)
            {
                ret_code = -2;
                break;
            }
            track.chanid = chanid;
            XTChan::instance()->active_chan(track.chanid, true);
        }

        int _srcno = track.chanid;
        if (ret_code<0 || _srcno<0)
        {
            if (src)
            {
                delete src;
                src = NULL;
            }
            ret_code = -3;
            break;
        }
        m_srcs[_srcno] = src;
        src->srcno = _srcno;
        src->tracks.push_back(track);
        ::memmove(srcno,&_srcno,sizeof(int));

        //创建标准流的转发源
        if (ms_cfg.snd_std_rtp)
        {
            xt_track_t arr_track[MAX_TRACK]={0};
            int _track_name = pare_tracks(sdp,sdp_len,arr_track);

            for (int index=0; index < _track_name; ++index)
            {
                unsigned long chanid = 0;
                int ret = XTChan::instance()->get_freestdchan(chanid);
                if (ret < 0)
                {
                    continue;
                }

                trackinfos[index].chanid = chanid;
                trackinfos[index].trackid = arr_track[index].trackid;
                trackinfos[index].tracktype = arr_track[index].tracktype;
                ::strncpy(trackinfos[index].trackname,arr_track[index].trackname,MAX_TRACKNAME_LEN);
                src->tracks.push_back(trackinfos[index]);
                XTChan::instance()->active_chan(trackinfos[index].chanid, true);
                ++trackinfos;
            }
            ::memmove(tracknum,&_track_name,sizeof(int));
        }
        ret_code = 0;
    } while (0);
    return ret_code;
}

int XTSrc::create_src(int tracknum, int *trackids, char *tracknames[], int &srcno, long chanid)
{
    int ret = 0;
    do 
    {
        write_lock_t lock(m_mutex);

        //私有流
        xt_track_t track;
        track.trackid = PRI_TRACK_ID;
        ::memset(track.trackname,0,MAX_TRACKNAME_LEN);
        track.tracktype = -1;
        if (chanid >= 0)
        {//上层指定转发服务通道
            if (XTChan::_()->is_chan_used(chanid))
            {
                ret = -1;
                break;
            }
            track.chanid = chanid;
            ret = XTChan::_()->active_chan(track.chanid, true);
            if (ret < 0)
            {
                break;
            }
        }
        else
        {//自动分配转发服务通道
            unsigned long tmp_ch = 0;
            ret = XTChan::_()->get_freechan(tmp_ch);
            if (ret <0)
            {
                break;
            }
            else
            {
                track.chanid = tmp_ch;
                XTChan::_()->active_chan(track.chanid, true);
            }
        }
        xt_src *src = new xt_src;
        if ( NULL == src)
        {
            ret = -2;
            break;
        }
        srcno = track.chanid;
        m_srcs[srcno] = src;
        src->srcno = srcno;
        src->tracks.push_back(track);

        //标准流
        if (ms_cfg.snd_std_rtp && trackids)
        {
            for (int t =  0;t < tracknum;++t)
            {
                unsigned long chanid = 0;
                ret = XTChan::_()->get_freestdchan(chanid);
                if (ret < 0)
                {
                    MEDIA_SVR_PRINT(level_info, "create_src:get_freestdchan fail: ret[%d]", ret);
                    break;
                }
                xt_track_t track;
                track.trackid = trackids[t];
                track.chanid = chanid;
                ::memset(track.trackname,0,MAX_TRACKNAME_LEN);

                if (tracknames)
                {
                    ::strcpy(track.trackname, tracknames[t]);
                    if (0 == ::strcmp(track.trackname,"video"))
                    {
                        track.tracktype = 0;
                    }
                    if (0 == ::strcmp(track.trackname,"audio"))
                    {
                        track.tracktype = 1;
                    }
                }
                src->tracks.push_back(track);
                ret = XTChan::_()->active_chan(track.chanid, true);
                if (ret < 0) break;
            }
        }
    } while (0);
    return ret;
}

int XTSrc::destroy_src(int srcno)
{
    write_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return 0;
    }
    xt_src *src = it->second;
    if (src)
    {
        std::list<xt_track_t>::iterator itr = src->tracks.begin();
        for (;itr != src->tracks.end();++itr)
        {
            xt_track_t &track = *itr;
            XTRtp::instance()->del_send_chan(track.chanid);
            XTChan::instance()->active_chan(track.chanid, false);

            MEDIA_SVR_PRINT(level_info, "destroy_src: srcno[%d] chan[%d]", srcno, track.chanid);
        }

        if (NULL != src)
        {
            delete src;
            src = NULL;
        }
    }
    m_srcs.erase(srcno);
    return 0;
}

int XTSrc::clear_send(int srcno)
{
    write_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return 0;
    }

    xt_src *src = it->second;
    if (src)
    {
        std::list<xt_track_t>::iterator itr = src->tracks.begin();
        for (;itr != src->tracks.end();++itr)
        {
            xt_track_t &track = *itr;
            XTRtp::instance()->del_send_chan(track.chanid);

            MEDIA_SVR_PRINT(level_info, "clear_send: chan[%d]", track.chanid);
        }
    }

    return 0;
}

int XTSrc::clear_allsend()
{
    write_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator itr = m_srcs.begin();
    for(;itr!= m_srcs.end();++itr)
    {
        xt_src *src = itr->second;
        if (src)
        {
            std::list<xt_track_t>::iterator itr = src->tracks.begin();
            for (;itr != src->tracks.end();++itr)
            {
                xt_track_t &track = *itr;
                XTRtp::instance()->del_send_chan(track.chanid);

                MEDIA_SVR_PRINT(level_info, "clear_send: chan[%d]", track.chanid);
            }
        }
    }

    return 0;
}

int XTSrc::get_tracknum(int srcno)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return -1;
    }

    xt_src *src = it->second;
    if (src)
    {
        return src->tracks.size()-1;
    }

    return 0;
}

int XTSrc::get_trackids(int srcno, int *ids)
{
    read_lock_t lock(m_mutex);
    int *tmp = ids;
    std::map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return -1;
    }

    xt_src *src = it->second;
    if (!src)
    {
        return -1;
    }

    std::list<xt_track_t>::iterator itr = src->tracks.begin();
    for (;itr != src->tracks.end();++itr)
    {
        if (itr->trackid < 0)
        {
            continue;
        }

        *tmp = itr->trackid;
        tmp += 1;
    }

    return 0;
}

int XTSrc::get_track(int srcno, int trackid, xt_track_t &track)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return -1;
    }
    if (!it->second)
    {
        return -2;
    }

    std::list<xt_track_t>::iterator itr = it->second->tracks.begin();
    for (;itr != it->second->tracks.end();++itr)
    {
        //当trackid为-1时，表示获取主trackid
        if ((-1 == trackid) || (itr->trackid == trackid))
        {
            ::strncpy(track.trackname,itr->trackname,MAX_TRACKNAME_LEN);
            track.trackid = itr->trackid;
            track.tracktype = itr->tracktype;
            track.frametype = itr->frametype;
            track.chanid = itr->chanid;
            return 0;
        }
    }
    return -3;
}

int XTSrc::get_main_track(int srcno, xt_track_t &track)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return -1;
    }

    xt_src *src = it->second;
    if (!src)
    {
        return -1;
    }

    std::list<xt_track_t>::iterator itr = src->tracks.begin();
    for (;itr != src->tracks.end();++itr)
    {
        if (itr->trackid < 0)
        {
            ::strncpy(track.trackname,itr->trackname,MAX_TRACKNAME_LEN);
            track.trackid = itr->trackid;
            track.tracktype = itr->tracktype;
            track.frametype = itr->frametype;
            track.chanid = itr->chanid;
            return 0;
        }
    }
    return -1;
}

int XTSrc::get_tracks(int srcno, list<xt_track_t> &tracks)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return -1;
    }
    xt_src *src = it->second;
    if (!src)
    {
        return -1;
    }
    std::list<xt_track_t>::iterator itr = src->tracks.begin();
    for (;itr != src->tracks.end();++itr)
    {
        tracks.push_back(*itr);
    }
    return 0;
}

int XTSrc::set_std_track_frametype(const int srcno,const char* sdp,const long sdp_len)
{
    int ret_code = -1;
    do 
    {
        xt_sdp::parse_buffer_t pb(sdp, sdp_len);
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            ret_code = -2;
            break;
        }

        read_lock_t lock(m_mutex);
        std::map<int, xt_src*>::iterator itsrc = m_srcs.find(srcno);
        if (m_srcs.end() == itsrc) return -3;
        xt_src *ptr_src = itsrc->second;
        if (!ptr_src)
        {
            ret_code = -3;
            break;
        }
        for(xt_sdp::sdp_session_t::medium_container_t::iterator itrm = xsdp.media_.begin();xsdp.media_.end() != itrm;++itrm)
        {
            for (std::list<xt_track_t>::iterator itr_track = ptr_src->tracks.begin();itr_track != ptr_src->tracks.end();++itr_track)
            {
                //not std track
                if (0 > itr_track->trackid) continue;

                std::map<std::string, std::list<std::string> >::iterator itr_rtpmap_snd_lst = itrm->attribute_helper_.attributes_.find("rtpmap");

                if (itrm->attribute_helper_.attributes_.end() == itr_rtpmap_snd_lst) continue;
                if (itr_rtpmap_snd_lst->second.empty()) continue;

                //parse first rtpmap
                std::string rtpmap_v = itr_rtpmap_snd_lst->second.front();
                unsigned rtpmap_payload_format = 0;
                char codec_name[128] = {0};
                unsigned rtp_timestamp_frequency = 0;
                unsigned num_channels = 0;

                if (4 != ::sscanf(rtpmap_v.c_str(), "%u %[^/]/%u/%u", &rtpmap_payload_format, codec_name, &rtp_timestamp_frequency, &num_channels)
                    && 3 != ::sscanf(rtpmap_v.c_str(), "%u %[^/]/%u", &rtpmap_payload_format, codec_name, &rtp_timestamp_frequency)
                    && 2 != ::sscanf(rtpmap_v.c_str(), "%u %s", &rtpmap_payload_format, codec_name)) break;

                //parse trackid
                std::map<std::string, std::list<std::string> >::iterator itr_trk = itrm->attribute_helper_.attributes_.find("control");
                if (itr_trk == itrm->attribute_helper_.attributes_.end()) continue;
                std::string trk_v = itr_trk->second.front();
                int trk_id = get_trackid(trk_v);

                //video
                if (0 == itrm->name_.compare("video"))
                {
                    if ( (trk_id!=-1 && trk_id==itr_track->trackid) || 0 == std::strcmp(itr_track->trackname,"video") )
                    {
                        if (0 == std::strcmp(codec_name,"H264"))
                        {
                            itr_track->frametype = OV_H264;
                        } 
                        else if (0 == std::strcmp(codec_name,"H265"))
                        {
                            itr_track->frametype = OV_H265;
                        }
                    }
                }


                //audio
                if (0 == itrm->name_.compare("audio"))
                {
                    if ( (trk_id!=-1 && trk_id==itr_track->trackid) ||  0 == std::strcmp(itr_track->trackname,"audio") )
                    {
                        if (0 == std::strcmp(codec_name,"MPEG4-GENERIC"))
                        {
                            itr_track->frametype = OV_AAC;
                        }
                        else if (0 == std::strcmp(codec_name,"PCMU") || 0 == std::strcmp(codec_name,"PCMA"))
                        {
                            itr_track->frametype = OV_G711;
                        }
                        else
                        {
                            itr_track->frametype = OV_AUDIO;
                        }
                    }
                }
            }//end for (std::list<xt_track_t>::iterator itr_track = ptr_src->tracks.begin();itr_track != ptr_src->tracks.end();++itr_track)
        } // end for(xt_sdp::sdp_session_t::medium_container_t::iterator itrm = xsdp.media_.begin();xsdp.media_.end() != itrm;++itrm)
        ret_code = 0;
    } while (0);
    return ret_code;
}

int XTSrc::add_sink(int srcno, xt_sink_t &sink)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return -1;
    }

    xt_src *src = it->second;
    if (!src)
    {
        return -1;
    }

    src->sinks.push_back(sink);

    return 0;
}

int XTSrc::del_sink(int srcno, xt_sink_t &sink)
{
    read_lock_t lock(m_mutex);
    std:: map<int, xt_src*>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return -1;
    }

    xt_src *src = it->second;
    if (!src)
    {
        return -1;
    }

    std::list<xt_sink_t>::iterator itr = src->sinks.begin();
    for (;itr != src->sinks.end();++itr)
    {
        xt_sink_t &s = *itr;
        if (s.session == sink.session &&
            s.chanid == sink.chanid &&
            s.addr == sink.addr &&
            s.port == sink.port &&
            s.demux == sink.demux &&
            s.demuxid == sink.demuxid)
        {
            src->sinks.erase(itr);
            break;
        }
    }

	int sink_num =0;
	itr = src->sinks.begin();
	for (;itr != src->sinks.end();++itr)
	{
		xt_sink_t &s = *itr;
		if (s.session == sink.session &&
			s.chanid == sink.chanid &&
			s.addr == sink.addr &&
			s.port == sink.port &&
			s.demux == sink.demux &&
			s.demuxid == sink.demuxid)
		{
			sink_num++;
		}
	}
    return sink_num;
}

int XTSrc::del_sink(void *session)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator itr1 = m_srcs.begin();
    for (;itr1 != m_srcs.end();++itr1)
    {
        xt_src *src = itr1->second;
        if (!src)
        {
            continue;
        }

        std::list<xt_sink_t>::iterator itr2 = src->sinks.begin();
        for (;itr2 != src->sinks.end();)
        {
            xt_sink_t &s = *itr2;
            if (s.session == session)
            {
                itr2 = src->sinks.erase(itr2);
            }
            else
            {
                ++itr2;
            }
        }
    }

    return 0;
}

int XTSrc::get_srcno_by_sink(int &srcno, const xt_sink_t &sink)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();++itr)
    {
        xt_src *src = itr->second;
        if (!src)
        {
            continue;
        }

        std::list<xt_sink_t>::iterator itr = src->sinks.begin();
        for (;itr != src->sinks.end();++itr)
        {
            xt_sink_t &s = *itr;
            if (s.srcno_prime == sink.srcno_prime &&
                s.session == sink.session &&
                s.addr == sink.addr &&
                s.port == sink.port &&
                s.demux == sink.demux &&
                s.demuxid == sink.demuxid)
            {
                srcno = src->srcno;
                return 0;
            }
        }
    }

    return -1;
}

int XTSrc::find_src(void *session, std::vector<int> &srcs)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();++itr)
    {
        xt_src *src = itr->second;
        if (!src)
        {
            continue;
        }

        srcs.push_back(src->srcno);
    }

    return 0;
}


int XTSrc::find_src(unsigned long chanid)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();++itr)
    {
        xt_src *src = itr->second;
        if (!src)
        {
            continue;
        }

        std::list<xt_track_t> &tracks = src->tracks;
        std::list<xt_track_t>::iterator i = tracks.begin();
        for (;i!=tracks.end();++i)
        {
            xt_track_t &t = *i;
            if (t.chanid == chanid)
            {
                return src->srcno;
            }
        }
    }

    return -1;
}

int XTSrc::get_chanid(unsigned long& out_chanid,const int srcno,const char* trackname)
{
    read_lock_t lock(m_mutex);
    std::map<int, xt_src*>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();++itr)
    {
        xt_src *src = itr->second;
        if (!src)
        {
            continue;
        }

        std::list<xt_track_t> &tracks = src->tracks;
        std::list<xt_track_t>::iterator i = tracks.begin();
        for (;i!=tracks.end();++i)
        {
            xt_track_t &t = *i;
            if (0 == strcmp(t.trackname,trackname))
            {
                out_chanid = t.chanid;
                return src->srcno;
            }
        }
    }

    return -1;
}

int XTSrc::pare_tracks(const char* sdp,const int sdp_len,xt_track_t* track_info)
{
    if (track_info == NULL)
    {
        return -1;
    }
    int track_num = 0;
    xt_sdp::parse_buffer_t pb(sdp, sdp_len);
    xt_sdp::sdp_session_t xsdp;
    try
    {
        xsdp.parse(pb);
    }
    catch(...)
    {
        return -2;
    }

    std::map<std::string, std::list<std::string> >::iterator itr_ctrl;
    bool is_exists_ctrl = false;
    xt_sdp::sdp_session_t::medium_container_t::iterator itr = xsdp.media_.begin();
    for(track_num=0; xsdp.media_.end() != itr && track_num < MAX_TRACK;++track_num,++itr)
    {
        if (itr->attribute_helper_.exists("control"))
        {
            itr_ctrl = itr->attribute_helper_.attributes_.find("control");
            if (itr->attribute_helper_.attributes_.end() != itr_ctrl)
            {
                if (!itr_ctrl->second.empty())
                {
                    std::string ctrl_value;
                    int trackid=-1;
                    ctrl_value = itr_ctrl->second.front();
                    if (1 == ::sscanf(ctrl_value.c_str(), "track%d",&trackid) 
                        || 1 == ::sscanf(ctrl_value.c_str(), "trackID=%d",&trackid))
                    {
                        track_info->trackid = trackid;
                        is_exists_ctrl = true;
                    }
                }
            }
        }
        if ( 0 == itr->name_.compare("video"))
        {
            if (!is_exists_ctrl)
            {
                track_info->trackid = 1;
            }
            track_info->tracktype = 0; 
            ::strncpy(track_info->trackname,"video",MAX_TRACKNAME_LEN);
        }
        else if ( 0 == itr->name_.compare("audio"))
        {
            if (!is_exists_ctrl)
            {
                track_info->trackid = 2;
            }

            track_info->tracktype = 1;
            ::strncpy(track_info->trackname,"audio",MAX_TRACKNAME_LEN);

            continue;
        }
        else
        {
            track_info->trackid = -1;
            track_info->tracktype = -1;
            ::memcpy(track_info->trackname,itr->name_.c_str(),MAX_TRACKNAME_LEN);
        }

        ++track_info;
    }

    return track_num;
}
int XTSrc::create_sdp(xt_track_t trackinfs[],const int tracknum,char sdp[],int& sdp_len,const std::string& local_bind_ip)
{
    int ret_code = -1;
    do 
    {
        char _sdp[] = "v=0\no=- 1430622498429749 1 IN IP4 0.0.0.0\ns=RTP/RTCP stream from IPNC\nt=0 0\na=tool:XTMeadiaServer v2015.08.05\na=type:broadcast\na=control:*\n";
        int len = ::strlen(_sdp);
        xt_sdp::parse_buffer_t pb(_sdp,len); 
        xt_sdp::sdp_session_t xsdp;
        try
        {
            xsdp.parse(pb);
        }
        catch(...)
        {
            ret_code = -3;
            break;
        }
        std::string control_vaule;
        xsdp.origin().address_ = local_bind_ip;
        xsdp.media_.clear();
        for (int t = 0; t < tracknum; ++t)
        {
            xt_sdp::sdp_session_t::medium_t m;
            if (0 == ::strcmp(trackinfs[t].trackname, "video"))
            {
                m.name_ = "video";
                xt_sdp::sdp_session_t::codec_t  h264("H264", 96, 90000); 
                m.codecs_.push_back(h264);

                xt_sdp::sdp_session_t::codec_t  h265("H265", 97, 90000);
                m.codecs_.push_back(h265);
                control_vaule.assign("track1");
            }

            if (0 == ::strcmp(trackinfs[t].trackname, "audio"))
            {
                m.name_ = "audio";
                control_vaule.assign("track2");

                //pcmu 0
                xt_sdp::sdp_session_t::codec_t g711_pcmu_8k_1("PCMU",0,8000);
                m.codecs_.push_back(g711_pcmu_8k_1);
                xt_sdp::sdp_session_t::codec_t g711_pcmu_8k_2("PCMU",0,8000,"","2");
                xt_sdp::sdp_session_t::codec_t g711_pcmu_16k_1("PCMU",0,16000);
                m.codecs_.push_back(g711_pcmu_16k_1);
                xt_sdp::sdp_session_t::codec_t g711_pcmu_16k_2("PCMU",0,16000,"","2");
                m.codecs_.push_back(g711_pcmu_16k_2);
                m.codecs_.push_back(g711_pcmu_8k_2);
                xt_sdp::sdp_session_t::codec_t g711_pcmu_32k_1("PCMU",0,32000);
                m.codecs_.push_back(g711_pcmu_32k_1);
                xt_sdp::sdp_session_t::codec_t g711_pcmu_32k_2("PCMU",0,32000,"","2");
                m.codecs_.push_back(g711_pcmu_32k_2);
                xt_sdp::sdp_session_t::codec_t g711_pcmu_44_1k_1("PCMU",0,44100);
                m.codecs_.push_back(g711_pcmu_44_1k_1);
                xt_sdp::sdp_session_t::codec_t g711_pcmu_44_1k_2("PCMU",0,44100,"","2");
                m.codecs_.push_back(g711_pcmu_44_1k_2);

                //puma 规定是8
                xt_sdp::sdp_session_t::codec_t g711_pcma_8k_1("PCMA",8,8000);
                m.codecs_.push_back(g711_pcma_8k_1);
                xt_sdp::sdp_session_t::codec_t g711_pcma_8k_2("PCMA",8,8000,"","2");
                m.codecs_.push_back(g711_pcma_8k_2);
                xt_sdp::sdp_session_t::codec_t g711_pcma_16k_1("PCMA",8,16000);
                m.codecs_.push_back(g711_pcma_16k_1);
                xt_sdp::sdp_session_t::codec_t g711_pcma_16k_2("PCMA",8,16000,"","2");
                m.codecs_.push_back(g711_pcma_16k_2);
                xt_sdp::sdp_session_t::codec_t g711_pcma_32k_1("PCMA",8,32000);
                m.codecs_.push_back(g711_pcma_32k_1);
                xt_sdp::sdp_session_t::codec_t g711_pcma_32k_2("PCMA",8,32000,"","2");
                m.codecs_.push_back(g711_pcma_32k_2);
                xt_sdp::sdp_session_t::codec_t g711_pcma_44_1k_1("PCMA",8,44100);
                m.codecs_.push_back(g711_pcma_44_1k_1);
                xt_sdp::sdp_session_t::codec_t g711_pcma_44_1k_2("PCMA",8,44100,"","2");
                m.codecs_.push_back(g711_pcma_44_1k_2);

                //aac_lc rfc 3016
                //a=fmtp:100 profile-level-id=24;object=23;bitrate=64000
                xt_sdp::sdp_session_t::codec_t aac_mpega_1("MP4A-LATM",100,90000);
                m.codecs_.push_back(aac_mpega_1); 
                xt_sdp::sdp_session_t::codec_t aac_mpega_2("MP4A-LATM",100,90000,"","2");
                m.codecs_.push_back(aac_mpega_2);

                //aac 96以上  rfc1890
                xt_sdp::sdp_session_t::codec_t  aac_8k_1("MPEG4-GENERIC", 98, 8000);
                m.codecs_.push_back(aac_8k_1);
                xt_sdp::sdp_session_t::codec_t  aac_8k_2("MPEG4-GENERIC", 98, 8000,"","2"); 
                m.codecs_.push_back(aac_8k_2);
                xt_sdp::sdp_session_t::codec_t  aac_16k_1("MPEG4-GENERIC", 98, 16000); 
                m.codecs_.push_back(aac_16k_1);
                xt_sdp::sdp_session_t::codec_t  aac_16k_2("MPEG4-GENERIC", 98, 16000,"","2"); 
                m.codecs_.push_back(aac_16k_2); 
            }
            xt_sdp::sdp_session_t::connection_t cvalue(xt_sdp::ipv4,local_bind_ip);
            m.connections_.push_back(cvalue);
            Rtp_Handle rtp_trans_src;
            XTRtp::instance()->get_rtp(trackinfs[t].chanid,rtp_trans_src); 
            m.protocol_ = "RTP/AVP";
            m.port_ = rtp_trans_src.port;
            if (rtp_trans_src.multiplex)
            {
                if (!xsdp.attribute_helper_.exists("rtpport-mux"))
                {
                    xsdp.add_attribute("rtpport-mux");
                }
                std::ostringstream demuxid;
                demuxid<<rtp_trans_src.multid;
                m.add_attribute("muxid",demuxid.str());
            }
            m.add_attribute("control",control_vaule.c_str());
            m.add_attribute("sendonly");
            xsdp.media_.push_back(m);
        }
        try
        {
            std::ostringstream oss;
            xsdp.encode(oss);
            sdp_len = oss.str().length();
            ::strncpy(sdp,oss.str().c_str(),sdp_len);
        }
        catch(...)
        {
            ret_code = -4;
            break;
        }
        ret_code = 0;
    } while (0);

    return ret_code;
} 
