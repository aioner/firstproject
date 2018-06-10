#include "media_link_impl.h"
#include "rtp_sink_impl.h"
#include "rtp_unpack_impl.h"
#include "rtp_unpack_h264.h"
#include "rtp_unpack_aac.h"
#include "rtp_unpack_h265.h"
#include "sdp_parser.h"
#include "msec_timer_mgr.h"

#include <string.h>
#include <boost/make_shared.hpp>

#ifdef _ARM_CODER
#define MAX_RTP_DEMUX 8*1024
#else
#define MAX_RTP_DEMUX 100*1024
#endif

extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);

namespace xt_media_client
{
    struct rtp_demux
    {
        void *demux;
        rv_rtp rtpH;
        rv_rtp_param p;
        rv_net_address address;
        uint8_t buf[RTP_PACKAGE_MAX_SIZE];
        uint32_t len;
        uint32_t m_exHead[16];
    };
	
	    rtcp_report_callback_impl::rtcp_report_callback_impl()
        :cb_(NULL),
        ctx_(NULL)
    {}

    rtcp_report_callback_impl::~rtcp_report_callback_impl()
    {
        cb_ = NULL;
        ctx_ = NULL;
    }

    void rtcp_report_callback_impl::register_rtcp_callback(xt_media_client_rtcp_report_callback_t cb, void *ctx)
    {
        cb_ = cb;
        ctx_ = ctx;
    }

    void rtcp_report_callback_impl::on_rtcp_receive(uint32_t ssrc, const xt_media_client_rtcp_sr_t *sr)
    {
        if (NULL != cb_)
        {
            cb_(ctx_, ssrc, sr, NULL);
           //XT_LOG(info, "on_rtcp_receive ctx(%p),ssrc=%0x,mNTPtimestamp=%0x,lNTPtimestamp=%0x,timestamp=%0x,packets=%0x,octets=%0x",ctx_,ssrc,sr->mNTPtimestamp,sr->lNTPtimestamp,sr->timestamp,sr->packets,sr->octets);
        }
    }

    void rtcp_report_callback_impl::on_rtcp_send(uint32_t ssrc, const xt_media_client_rtcp_rr_t *rr)
    {
        if (NULL != cb_)
        {
            cb_(ctx_, ssrc, NULL, rr);
            //XT_LOG(info, "on_rtcp_send ctx(%p),rr(%p)",ctx_,rr);
        }
    }

	no_frame_arrived_callback_impl::no_frame_arrived_callback_impl()
		:cb_(NULL)
		,ctx_(NULL)
		,priod_(0)
		,ts_frame_arrived_(0)
	{}

	no_frame_arrived_callback_impl::~no_frame_arrived_callback_impl()
	{
        msec_timer_mgr_t::instance()->remove_timer(this);
		cb_ = NULL;
		ctx_ = NULL;
	}

	void no_frame_arrived_callback_impl::register_no_frame_arrived_callback(uint32_t priod, xt_media_client_no_frame_arrived_callback_t cb, void *ctx)
	{
		priod_ = priod;
		cb_ = cb;
		ctx_ = ctx;

		msec_timer_mgr_t::instance()->add_timer(this);
	}

	void no_frame_arrived_callback_impl::update_frame_arrived_ts()
	{
		if (0 != priod_)
		{
			ts_frame_arrived_ = msec_timer_t::get_tick_count();
		}
	}

	uint32_t no_frame_arrived_callback_impl::on_expires()
	{
		if (0 == ts_frame_arrived_)
		{
			//md_log(debug, "first frame not arrived.");
			return priod_;
		}

		uint32_t now = msec_timer_t::get_tick_count();
		if ((now > ts_frame_arrived_) && (now - ts_frame_arrived_ >= priod_))
		{
			//md_log(debug, "now=%u,ts_frame_arrived_=%u,priod_=%u,no_frame_arrived_cb_=%p", now, ts_frame_arrived_, priod_,cb_);
			if (NULL != cb_)
			{
				if (0 != cb_(ctx_))
				{
					return 0;
				}
			}

			return priod_;
		}
		else
		{
			return ts_frame_arrived_ + priod_ - now;
		}
	}

    std::map<media_link_impl_base*, media_link_impl_base*> links_;
    spinlock_t links_metex_;

    spinlock_t rtp_mutex_;
    rtp_demux *rtp_demuxs_ = 0;
    uint32_t front_rtp_demux_ = 0;
    uint32_t tail_rtp_demux_ = 0;

    bool  run_thread_demux_	= true;
    boost::thread *thread_demux_ = 0;

    extern void func_rtp_demux();

    media_link_impl_base::media_link_impl_base(ports_mgr_t *ports_mgr)
        :session_(),
        ports_mgr_(ports_mgr),
        sdp_(),
        rtp_sinks_(),
        sdp_mutex_(),
        rtp_sinks_mutex_()
    {
        spinlock_t::scoped_lock _lock(links_metex_);
        links_[this] = this;
    }

    media_link_impl_base::~media_link_impl_base()
    {
        spinlock_t::scoped_lock _lock(links_metex_);
        links_.erase(this);
    }

    void media_link_impl_base::init()
    {
        spinlock_t::scoped_lock _lock(rtp_mutex_);
        rtp_demuxs_ = new rtp_demux[MAX_RTP_DEMUX];
        front_rtp_demux_ = 0;
        tail_rtp_demux_ = 0;

        run_thread_demux_ = true;
        thread_demux_ = new boost::thread(func_rtp_demux);
    }
    void media_link_impl_base::uninit()
    {
        spinlock_t::scoped_lock _lock(rtp_mutex_);
        if (thread_demux_)
        {
            run_thread_demux_ = false;
            thread_demux_->join();
        }
        if (rtp_demuxs_)
        {
            delete[] rtp_demuxs_;
        }
    }

    const std::string &media_link_impl_base::get_sdp() const
    {
        spinlock_t::scoped_lock _lock(sdp_mutex_);
        return sdp_;
    }

    xt_media_client_status_t media_link_impl_base::set_sdp(const std::string& sdp)
    {
        spinlock_t::scoped_lock _lock(sdp_mutex_);
        xt_media_client_status_t ret_code = MEDIA_CLIENT_STATUS_OK;
        do 
        {
            if (!sdp_.empty()|| sdp_.length()>0)
            {
                ret_code = MEDIA_CLIENT_STATUS_SDP_EXIST;
                md_log(md_log_info, "media_link_impl_base::set_sdp, sdp is exist update sdp_!");
            }
            sdp_ = sdp; 
        } while (0);
        return ret_code;
    }

    xt_media_client_status_t media_link_impl_base::register_rtcp_callback(rtcp_report_callback_t * cb)
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        for (std::size_t index = 0; index < rtp_sinks_.size(); ++index)
        {
            rtp_sinks_[index]->register_rtcp_callback(cb);
        }
        return MEDIA_CLIENT_STATUS_OK;
    }

    void media_link_impl_base::get_port(std::vector<xt_sink_info_t> &infos)
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);

        std::vector<rtp_sink_ptr>::iterator itr = rtp_sinks_.begin();
        for (;itr != rtp_sinks_.end();++itr)
        {
            rtp_sink_ptr &sink = *itr;

            xt_sink_info_t info;
            sink->get_info(info);

            infos.push_back(info);
        }
    }

    xt_media_client_status_t media_link_impl_base::create_link(xt_media_client_link_mode mode, uint32_t media_type, bool demux, 
		const char *multicast_ip, uint16_t multicast_port,const char* localip)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_UNKNOWN;
        switch (mode)
        {
        case MEDIA_CLIENT_LKMODE_STD:
            stat = std_create_link(media_type, demux,localip);
            break;
        case MEDIA_CLIENT_LKMODE_STD_MULTI:
            stat = std_multi_create_link(media_type, demux);
            break;
        case MEDIA_CLIENT_LKMODE_PRIV:
            stat = priv_create_link(media_type, demux);
            break;
        case MEDIA_CLIENT_LKMODE_PRIV_FAST:
            stat = priv_create_link2(media_type, demux, multicast_ip, multicast_port);
            break;
        default:
            md_log(md_log_error, "bad link mode(%d)", mode);
            stat = MEDIA_CLIENT_STATUS_BAD_LINK_MODE;
            break;
        }

        return stat;
    }

    xt_media_client_status_t media_link_impl_base::create_link(xt_media_client_link_mode mode, int track_num, bool demux)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_UNKNOWN;
        switch (mode)
        {
        case MEDIA_CLIENT_LKMODE_PUSH:// push model
            stat = create_recv(track_num, demux);
            break;
        default:
            md_log(md_log_error, "bad link mode(%d)", mode);
            stat = MEDIA_CLIENT_STATUS_BAD_LINK_MODE;
            break;
        }

        return stat;
    }
    xt_media_client_status_t media_link_impl_base::create_link(xt_media_client_link_mode mode,int track_num, bool demux, bool multicast, const char* multicastip, int* multiports)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_UNKNOWN;
        switch (mode)
        {
        case MEDIA_CLIENT_LKMODE_PUSH:// push model
            stat = create_recv(track_num, demux,multicast,multicastip,multiports);
            break;
        default:
            md_log(md_log_error, "bad link mode(%d)", mode);
            stat = MEDIA_CLIENT_STATUS_BAD_LINK_MODE;
            break;
        }
        return stat;
    }

    xt_media_client_status_t media_link_impl_base::create_link(const char *multicast_ip, uint16_t multicast_port)
    {
        md_log(md_log_error, "create_link multicast(%s,%u)", multicast_ip, multicast_port);

        if (!session_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        std::string sdp;
        xt_media_client_status_t stat = session_->describe(sdp);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "session describe failed,stat(%d)", stat);
            return stat;
        }

        set_sdp(sdp);

        rtp_sink_ptr rtp_sink = new_rtp_sink(XT_AVMEDIA_TYPE_UNKNOWN, XT_AV_CODEC_ID_NONE);

        multicast_param_t multicast_param = { 0 };
        multicast_param.ip = multicast_ip;
        multicast_param.port = multicast_port;
        multicast_param.ttl = 127;
        uint32_t demuxid = 0;
        if (!rtp_sink->open_rtp("0.0.0.0", multicast_port, multicast_port + 1, rtp_sink_t::unknown_output, false, demuxid, &multicast_param))
        {
            md_log(md_log_error, "rtp_open failed.-ports(%u,%u),multicast(%s,%u)", multicast_port, multicast_port + 1, multicast_ip, multicast_port);
            return MEDIA_CLIENT_STATUS_BAD_ADDR;
        }

        add_rtp_sink(rtp_sink);

        return MEDIA_CLIENT_STATUS_OK;
    }

    void media_link_impl_base::close_link()
    {
        if (!rtp_sinks_empty() && session_)
        {
            session_->teardown();
        }

        close_rtp_sinks();

        sdp_.clear();
    }

    xt_media_client_status_t media_link_impl_base::std_create_link(uint32_t media_type, bool demux,const char* localip)
    {
        if (!session_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        std::string sdp;
        xt_media_client_status_t stat = session_->describe(sdp);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "session describe failed,stat(%d)", stat);
            return stat;
        }

        md_log(md_log_debug, "session describe ok");

        std::vector<xt_sdp_media_info_t> sdp_media_infos;
        stat = session_->parse_sdp(sdp, sdp_media_infos);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "session parse_sdp failed,stat(%d)", stat);
            return stat;
        }

        md_log(md_log_debug, "session parse_sdp ok");

        if (!filter_media_streams(media_type, sdp_media_infos))
        {
            md_log(md_log_error, "session filter_media_streams failed");
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        md_log(md_log_debug, "session filter_media_streams ok");

        if (!streams_setup(localip,sdp_media_infos, demux, false))
        {
            md_log(md_log_error, "session streams_setup failed");
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        md_log(md_log_debug, "session streams_setup ok");

        set_sdp(sdp);

        return MEDIA_CLIENT_STATUS_OK;
    }

    // push model
    //////////////////////////////////////////////////////////////////////////
    xt_media_client_status_t media_link_impl_base::create_recv(int track_num, bool demux)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;

        for (int index = 0; index < track_num; ++index)
        {
            rtp_sink_ptr rtp_sink = new_rtp_sink(XT_AVMEDIA_TYPE_UNKNOWN, XT_AV_CODEC_ID_NONE);
            if (!rtp_sink)
            {
                md_log(md_log_error, "new rtp sink failed,track_num(%d),demux(%d)", track_num, demux);
                stat = MEDIA_CLIENT_STATUS_OPEN_RTP_FAIL;
                break;
            }

            uint32_t demuxid = 0;
            bool rtp_open_success = false;
            uint16_t ports[2] = { 0 };

            scoped_ports_mgr_helper<ports_mgr_t> _ports_mgr_helper(ports_mgr_.get_ports_mgr());
            while (_ports_mgr_helper.get_ports(2, ports,demux,1))
            {
                rv_context demux_handler = (rv_context)&s_rtp_demux_handler;
                if (rtp_sink->open_rtp("0.0.0.0", ports[0], ports[1], rtp_sink_t::raw_rtp_output, demux, demuxid, NULL, demux_handler))
                {
                   md_log(md_log_info, "rtp_open success.port(%d,%d),demux(%d),demuxid(%d)", ports[0], ports[1], demux, demuxid);
                    rtp_open_success = true;

                    xt_sink_info_t info;
                    info.index = index;
                    info.port_rtp = ports[0];
                    info.port_rtcp = ports[1];
                    info.demux = demux;
                    info.demuxid = demuxid;
                    rtp_sink->set_info(info);
                    break;
                }
                else
                {
                   md_log(md_log_info, "rtp_open fail.port(%d,%d),demux(%d),demuxid(%d)", ports[0], ports[1], demux, demuxid);
                }
                //复用下只使用一对端口如果失败更换端口重试
                if (demux)
                {
                    if (_ports_mgr_helper.update_demux_port())
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }

                }
                _ports_mgr_helper.ports_failed(2,  ports);
            }

            if (!rtp_open_success)
            {
                md_log(md_log_error, "create_recv fail.port(%d,%d), demux(%d)",ports[0], ports[1], demux);
                stat = MEDIA_CLIENT_STATUS_OPEN_RTP_FAIL;
                break;
            }
            else
            {
                md_log(md_log_error, "create_recv success.port(%d,%d), demux(%d),demuxid(%d)",ports[0], ports[1], demux, demuxid);
            }
            ports_mgr_.ports_used(2, ports);

            add_rtp_sink(rtp_sink);
        }

        return stat;
    }

    xt_media_client_status_t media_link_impl_base::create_recv(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;

        for (int index = 0; index < track_num; ++index)
        {
            uint32_t demuxid = 0;
            rtp_sink_ptr rtp_sink = new_rtp_sink(XT_AVMEDIA_TYPE_UNKNOWN, XT_AV_CODEC_ID_NONE);
            if (!rtp_sink)
            {
                md_log(md_log_error, "new rtp sink failed,track_num(%d),demux(%d)", track_num, demux);
                stat = MEDIA_CLIENT_STATUS_OPEN_RTP_FAIL;
                break;
            }

            if (multicast)
            {
                multicast_param_t multicast_param;
                int index_prt = index*2;
                multicast_param.ip = multicastip;
                multicast_param.ttl = 128;
                multicast_param.port = multiports[index_prt];
                rv_context demux_handler = (rv_context)&s_rtp_demux_handler;

                //modified by lichao, 20151106 在非win32系统上，接收端将绑定组播地址
#ifdef _WIN32
                if (rtp_sink->open_rtp("0.0.0.0", multiports[index_prt], multiports[index_prt]+1, rtp_sink_t::raw_rtp_output, demux, demuxid, &multicast_param, demux_handler))
#else
                if (rtp_sink->open_rtp(multicastip, multiports[index_prt], multiports[index_prt]+1, rtp_sink_t::raw_rtp_output, demux, demuxid, &multicast_param, demux_handler))
#endif
                {
                   md_log(md_log_info, "rtp_open success.multicast port(%d,%d),demux(%d),demuxid(%d)", multiports[index_prt], multiports[index_prt]+1, demux, demuxid);
                    xt_sink_info_t info;
                    info.index = index;
                    info.port_rtp = multiports[index_prt];
                    info.port_rtcp = multiports[index_prt]+1;
                    info.demux = demux;
                    info.demuxid = demuxid;
                    rtp_sink->set_info(info);
                }
                else
                {
                    md_log(md_log_error, "rtp_open multicast fail.port(%d,%d),demux(%d),demuxid(%d)", multiports[index], multiports[index]+1, demux, demuxid);
                    stat = MEDIA_CLIENT_STATUS_OPEN_RTP_FAIL;
                    break;
                }
            }
            else
            {
                bool rtp_open_success = false;
                uint16_t ports[2] = { 0 };
                scoped_ports_mgr_helper<ports_mgr_t> _ports_mgr_helper(ports_mgr_.get_ports_mgr());
                while (_ports_mgr_helper.get_ports(2, ports,demux,1))
                {
                    rv_context demux_handler = (rv_context)&s_rtp_demux_handler;
                    if (rtp_sink->open_rtp("0.0.0.0", ports[0], ports[1], rtp_sink_t::raw_rtp_output, demux, demuxid, NULL, demux_handler))
                    {
                       md_log(md_log_info, "rtp_open success.port(%d,%d),demux(%d),demuxid(%d)", ports[0], ports[1], demux, demuxid);
                        rtp_open_success = true;

                        xt_sink_info_t info;
                        info.index = index;
                        info.port_rtp = ports[0];
                        info.port_rtcp = ports[1];
                        info.demux = demux;
                        info.demuxid = demuxid;
                        rtp_sink->set_info(info);
                        break;
                    }
                    else
                    {
                       md_log(md_log_info, "rtp_open fail.port(%d,%d),demux(%d),demuxid(%d)", ports[0], ports[1], demux, demuxid);
                    }
                    //复用下只使用一对端口如果失败更换端口重试
                    if (demux)
                    {
                        if (_ports_mgr_helper.update_demux_port())
                        {
                            continue;
                        }
                        else
                        {
                            break;
                        }

                    }
                    _ports_mgr_helper.ports_failed(2,  ports);


                }

                if (!rtp_open_success)
                {
                    md_log(md_log_error, "create_recv fail.port(%d,%d), demux(%d)",ports[0], ports[1], demux);
                    stat = MEDIA_CLIENT_STATUS_OPEN_RTP_FAIL;
                    break;
                }
                else
                {
                   md_log(md_log_info, "create_recv success.port(%d,%d), demux(%d),demuxid(%d)",ports[0], ports[1], demux, demuxid);
                }
                ports_mgr_.ports_used(2, ports);
            }

            add_rtp_sink(rtp_sink);
        }
        return stat;
    }

    xt_media_client_status_t media_link_impl_base::set_packer(const std::string& sdp)
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;

        std::vector<xt_sdp_media_info_t> sdp_media_infos;
        bool ret = sdp_parser_t::parse_sip("", sdp.c_str(), sdp.length(), sdp_media_infos);
        if (!ret)
        {
            md_log(md_log_error, "session parse_sdp failed,stat(%d)", stat);
            return MEDIA_CLIENT_STATUS_BADPARAM;
        }

       md_log(md_log_info, "media_link_impl_base::set_packer sdp_num[%d]", sdp_media_infos.size());

        for (std::size_t index = 0; index < sdp_media_infos.size(); ++index)
        {
            std::size_t sink_index = 0;
            rtp_unpack_ptr packer;
            xt_av_media_t media_type = sdp_media_infos[index].media_type;
            xt_av_codec_id_t cid = sdp_media_infos[index].cid;
            switch (cid)
            {
            case XT_AV_CODEC_ID_H264:
                {
                    packer.reset(new rtp_unpack_h264_impl);
                    sink_index = 0;
                    break;
                }

            case XT_AV_CODEC_ID_AAC:
                {
                    packer.reset(new rtp_unpack_aac_impl);
                    sink_index = 1;
                    break;
                }

            case XT_AV_CODEC_ID_H265:
                {
                    packer.reset(new rtp_unpack_h265_impl);
                    sink_index = 0;
                    break;
                }
            default:
                {
                    uint32_t buf_capacity = 0;
                    uint32_t buf_max_bound = 0;
                    uint32_t frame_type = 0;
                    if (XT_AVMEDIA_TYPE_AUDIO == media_type)
                    {
                        buf_capacity = RTP_AUDIO_FRAME_NORMAL_SIZE;
                        buf_max_bound = RTP_AUDIO_FRAME_MAX_SIZE;
                        frame_type = OV_AUDIO;
                        sink_index = 1;
                    }
                    else
                    {
                        buf_capacity = RTP_VIDEO_FRAME_NORMAL_SIZE;
                        buf_max_bound = RTP_VIDEO_FRAME_MAX_SIZE;
                        frame_type = OV_VIDEO_I;
                        sink_index = 0;
                    }
                    packer.reset(new rtp_unpack_priv_impl(buf_capacity, buf_max_bound, frame_type, frame_type, DEVICE_VGA_DATA_TYPE));
                }
                break;
            }

            //当存在两个sink时按顺序第一个sink为视频第二个为音频
            if (get_rtp_sink_size() < 2)
            {
                sink_index = index;
            }

            //业务层协商时此处保存远端信息
            if (NULL == session_)
            {
                add_remote_address(sink_index,sdp_media_infos[index].uri, sdp_media_infos[index].port_rtp,sdp_media_infos[index].port_rtcp,sdp_media_infos[index].demux,sdp_media_infos[index].demux_id);
            }
			//复用时根据payload区分视频音频流，否则根据datatype区分
            int payload = is_demux() ? sdp_media_infos[index].payload : -1;
            stat = set_packer(sink_index, payload, packer);
            if (stat != MEDIA_CLIENT_STATUS_OK)
            {
                break;
            }
        }

        return stat;
    }
    //////////////////////////////////////////////////////////////////////////

    bool media_link_impl_base::is_demux()
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        return ((rtp_sinks_.size() > 0) && rtp_sinks_[0]->is_demux());
    }

    xt_media_client_status_t media_link_impl_base::std_multi_create_link(uint32_t media_type, bool demux,const char *multicast_ip, uint16_t multicast_port)
    {
        if (!session_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        std::string sdp;
        xt_media_client_status_t stat = session_->describe(sdp);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "session describe failed,stat(%d)", stat);
            return stat;
        }

        std::vector<xt_sdp_media_info_t> sdp_media_infos;
        stat = session_->parse_sdp(sdp, sdp_media_infos);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "session parse_sdp failed,stat(%d)", stat);
            return stat;
        }

        if (!streams_multi_setup(sdp_media_infos, demux, false,multicast_ip,multicast_port))
        {
            md_log(md_log_error, "session streams_multi_setup failed");
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        set_sdp(sdp);

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_impl_base::priv_create_link(uint32_t media_type, bool demux)
    {
        if (!session_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        std::string sdp;
        xt_media_client_status_t stat = session_->describe(sdp);
        if (MEDIA_CLIENT_STATUS_OK != stat)
        {
            md_log(md_log_error, "session describe failed,stat(%d)", stat);
            return stat;
        }

        if (!stream_setup("0.0.0.0",XT_AVMEDIA_TYPE_DEMUX, XT_AV_CODEC_ID_NONE, demux, false,NULL,0,media_type))
        {
            md_log(md_log_error, "session stream_setup failed");
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        set_sdp(sdp);

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_impl_base::priv_create_link2(uint32_t media_type, bool demux, const char *multicast_ip, uint16_t multicast_port)
    {
        if (!stream_setup("0.0.0.0",XT_AVMEDIA_TYPE_DEMUX, XT_AV_CODEC_ID_NONE, demux, true, multicast_ip, multicast_port,media_type))
        {
            return MEDIA_CLIENT_STATUS_SETUP_FAIL;
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_impl_base::start_capture(frame_data_dump_callback_t *cb)
    {
        active_rtp_sinks(cb);
        if (session_)
        {
            return session_->play();
        }
        return MEDIA_CLIENT_STATUS_OK;
    }

    bool media_link_impl_base::session_setup(std::vector<xt_session_param_t>&params, bool with_describe /* = false */)
    {
        if (!session_)
        {
            return false;
        }

        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;
        if (with_describe)
        {
            std::string sdp;
            stat = session_->describe_and_setup(params, sdp);

            if (MEDIA_CLIENT_STATUS_OK == stat)
            {
                set_sdp(sdp);
            }
        }
        else
        {
            stat = session_->setup(params);
        }

        return (MEDIA_CLIENT_STATUS_OK == stat);
    }

    bool ipv4_to_uint32(const char *ip, uint32_t &u);

    bool media_link_impl_base::stream_setup(const char* localip,xt_av_media_t media_type, xt_av_codec_id_t cid, bool demux, bool with_describe, 
		const char *multicast_ip, uint16_t multicast_port,uint32_t code_type /*= 0*/)
    {
        rtp_sink_ptr rtp_sink = new_rtp_sink(media_type, cid);
        if (!rtp_sink)
        {
            md_log(md_log_error, "new rtp sink failed,media_type(%d),cid(%d)", media_type, cid);
            return false;
        }

        uint32_t demuxid = 0;
        uint16_t ports[2] = { 0 };
        uint8_t mode = 0;

        if (NULL != multicast_ip)
        {
            multicast_param_t multicast_param = { 0 };
            multicast_param.ip = multicast_ip;
            multicast_param.port = multicast_port;
            multicast_param.ttl = 127;

            ports[0] = multicast_port;
            ports[1] = multicast_port + 1;

            rv_context demux_handler = (rv_context)&s_rtp_demux_handler;
            if (!rtp_sink->open_rtp(localip, ports[0], ports[1], rtp_sink_t::unknown_output, demux, demuxid, &multicast_param, demux_handler))
            {
                md_log(md_log_error, "rtp_open failed.-ports(%u,%u),multicast(%s,%u)", ports[0], ports[1], multicast_ip, multicast_port);
                return false;
            }

            if (!ipv4_to_uint32(multicast_ip, demuxid))
            {
                md_log(md_log_error, "ipv4_to_uint32 failed.-multicast_ip(%s)", multicast_ip);
                return false;
            }

            mode = 2;
        }
        else
        {
            bool rtp_open_success = false;
            scoped_ports_mgr_helper<ports_mgr_t> _ports_mgr_helper(ports_mgr_.get_ports_mgr());

            while (_ports_mgr_helper.get_ports(2, ports, demux,1))
            {
                rv_context demux_handler = (rv_context)&s_rtp_demux_handler;
                if (rtp_sink->open_rtp(localip, ports[0], ports[1], rtp_sink_t::unknown_output, demux, demuxid, NULL, demux_handler))
                {
                   md_log(md_log_info, "rtp_open ok.-media_type(%d),port(%d,%d),demux(%d),demuxid(%d)", media_type, ports[0], ports[1], demux, demuxid);
                    rtp_open_success = true;

                    xt_sink_info_t info;
                    info.index = -1;
                    info.port_rtp = ports[0];
                    info.port_rtcp = ports[1];
                    info.demux = demux;
                    info.demuxid = demuxid;
                    rtp_sink->set_info(info);
                    break;
                }
                //复用下只使用一对端口如果失败更换端口重试
                if (demux)
                {
                    if (_ports_mgr_helper.update_demux_port())
                    {
                        continue;
                    }
                    else
                    {
                        break;
                    }

                }
                _ports_mgr_helper.ports_failed(2,  ports);
            }

            if (!rtp_open_success)
            {
                md_log(md_log_error, "rtp open failed");
                return false;
            }

            //added by lichao, 20150210 解决部分端口没有释放的问题
            ports_mgr_.ports_used(2, ports);
        }

        std::vector<xt_session_param_t> params(1);
        params[0].code = code_type;
        params[0].stream_type = media_type;
        params[0].client_ctx.ssrc = rtp_sink->get_ssrc();
        params[0].client_ctx.demux = demux;
        params[0].client_ctx.demuxid = demuxid;
        params[0].client_ctx.mode = mode;
        params[0].client_ctx.rtp_port = ports[0];
        params[0].client_ctx.rtcp_port = ports[1];

        if (!session_setup(params, with_describe))
        {
            return false;
        }

        xt_session_server_info_t server_info = { 0 };
        session_->get_server_info(server_info);
        if (!rtp_sink->add_remote_address(server_info.ip, params[0].server_ctx.rtp_port, params[0].server_ctx.rtcp_port, 0 != params[0].server_ctx.demux, params[0].server_ctx.demuxid))
        {
            return false;
        }

        add_rtp_sink(rtp_sink);

        return true;
    }

    bool media_link_impl_base::streams_setup(const char* localip,const std::vector<xt_sdp_media_info_t>& sdp_media_infos, 
		bool demux, bool with_describe /* = false */)
    {
        for (std::size_t index = 0; index < sdp_media_infos.size(); ++index)
        {
            if (!stream_setup(localip,sdp_media_infos[index].media_type, sdp_media_infos[index].cid, demux, with_describe))
            {
                return false;
            }
        }

        return true;
    }

    bool media_link_impl_base::streams_multi_setup(const std::vector<xt_sdp_media_info_t>& sdp_media_infos, bool demux, bool with_describe,const char *multicast_ip, uint16_t multicast_port)
    {
		for (std::size_t index = 0; index < sdp_media_infos.size(); ++index)
		{
			if (!stream_multi_setup(sdp_media_infos[index].media_type, sdp_media_infos[index].cid, demux, with_describe,multicast_ip,multicast_port))
			{
				return false;
			}
		}
        return true;
    }

    void media_link_impl_base::add_rtp_sink(const rtp_sink_ptr&rtp_sink)
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        rtp_sinks_.push_back(rtp_sink);
    }

    void media_link_impl_base::active_rtp_sinks(frame_data_dump_callback_t *cb)
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        for (std::size_t index = 0; index < rtp_sinks_.size(); ++index)
        {
            rtp_sinks_[index]->active_rtp(cb);
        }
    }

    std::size_t media_link_impl_base::get_rtp_sink_size()const
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        return rtp_sinks_.size();
    }

    xt_media_client_status_t media_link_impl_base::set_packer(std::size_t index, int payload, const rtp_unpack_ptr &packer)
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        if (index< rtp_sinks_.size())
        {
            rtp_sinks_[index]->add_unpacker(payload, packer);

            xt_sink_info_t sink;
            rtp_sinks_[index]->get_info(sink);
           md_log(md_log_info,"media_link_impl_base::set_packer: index[%d] port_rtp[%d] port_rtcp[%d] demux[%d] demuxid[%d]",
                sink.index,sink.port_rtp,sink.port_rtcp,sink.demux,sink.demuxid);
            return MEDIA_CLIENT_STATUS_OK;
        }

        return MEDIA_CLIENT_STATUS_BADPARAM;
    }

    bool media_link_impl_base::add_remote_address(std::size_t index,const char *ip, uint16_t rtp_port, uint16_t rtcp_port, bool demux, uint32_t demuxid)
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        if (index< rtp_sinks_.size())
        {
           md_log(md_log_info,"media_link_impl_base::add_remote_address: index[%d] ip[%s] port_rtp[%d] port_rtcp[%d] demux[%d] demuxid[%d]",
                index,ip,rtp_port,rtcp_port,demux,demuxid);

            return rtp_sinks_[index]->add_remote_address(ip,rtp_port,rtcp_port,demux,demuxid);
        }
        return false;
    }

    void media_link_impl_base::close_rtp_sinks()
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        for (std::size_t index = 0; index < rtp_sinks_.size(); ++index)
        {
            rtp_sinks_[index]->close_rtp();
        }
    }

    bool media_link_impl_base::rtp_sinks_empty() const
    {
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        return rtp_sinks_.empty();
    }

    rtp_sink_ptr media_link_impl_base::new_rtp_sink(xt_av_media_t media_type, xt_av_codec_id_t cid)
    {
        if (XT_AVMEDIA_TYPE_DEMUX==media_type || 
            media_type==XT_AVMEDIA_TYPE_UNKNOWN || 
            cid==XT_AV_CODEC_ID_NONE)
        {
            return boost::make_shared<rtp_sink_impl>(RTP_FRAME_NORMAL_SIZE, RTP_FRAME_MAX_SIZE);
        }

        rtp_unpack_ptr packer;

        switch (cid)
        {
        case XT_AV_CODEC_ID_H264:
            packer.reset(new rtp_unpack_h264_impl);
            break;
        case XT_AV_CODEC_ID_AAC:
            packer.reset(new rtp_unpack_aac_impl);
            break;
        case XT_AV_CODEC_ID_H265:
            packer.reset(new rtp_unpack_h265_impl);
            break;
        default:
            {
                uint32_t buf_capacity = 0;
                uint32_t buf_max_bound = 0;
                uint32_t frame_type = 0;
                if (XT_AVMEDIA_TYPE_AUDIO == media_type)
                {
                    buf_capacity = RTP_AUDIO_FRAME_NORMAL_SIZE;
                    buf_max_bound = RTP_AUDIO_FRAME_MAX_SIZE;
                    frame_type = OV_AUDIO;
                }
                else
                {
                    buf_capacity = RTP_VIDEO_FRAME_NORMAL_SIZE;
                    buf_max_bound = RTP_VIDEO_FRAME_MAX_SIZE;
                    frame_type = OV_VIDEO_I;
                }

                packer.reset(new rtp_unpack_priv_impl(buf_capacity, buf_max_bound, frame_type, frame_type, DEVICE_VGA_DATA_TYPE));
            }
            break;
        }

        return boost::make_shared<rtp_sink_impl>(packer);
    }

    //// rtcp-based feedback message add by songlei 20150605
    xt_media_client_status_t media_link_impl_base::rtcp_send_fir()
    {
        xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        long ret = 0;
        for (std::size_t index = 0; index < rtp_sinks_.size(); ++index)
        {
            ret = rtp_sinks_[index]->rtcp_send_fir();
            if (ret < 0)
            {
                stat = MEDIA_CLIENT_RTCP_SEND_FIR_FAIL;
            }
        } 
        return stat;
    }

    void media_link_impl_base::s_rtp_demux_handler(void *hdemux,void *ctx)
    {
        rtp_demux_handler(hdemux);
    }

    void func_rtp_demux()
    {
        while (run_thread_demux_)
        {
            rtp_demux rtp;
            bool f = false;
            do 
            {
                spinlock_t::scoped_lock _lock(rtp_mutex_);

                if (front_rtp_demux_ != tail_rtp_demux_)
                {
                    f = true;
                    ::memcpy(&rtp, &rtp_demuxs_[front_rtp_demux_], sizeof(rtp_demux));
                    if (front_rtp_demux_+1 >= MAX_RTP_DEMUX)
                    {
                        front_rtp_demux_ = 0;
                    }
                    else
                    {
                        front_rtp_demux_ += 1;
                    }
                }
            } while (false);

            if (!f)
            {
                boost::this_thread::sleep(boost::posix_time::millisec(1));
                continue;
            }

            do 
            {
                spinlock_t::scoped_lock _lock(xt_mp_sink_library::instance()->rtp_to_sink_map_mutex_);

                rtp_sink_impl *sink = xt_mp_sink_library::instance()->get_rtp_sink(rtp.rtpH);
                if (!sink)
                {
                    break;
                }

                int ret = rtp_sink_impl::pump_demux_rtp(sink, rtp.buf, rtp.len, rtp.p, rtp.address);
                if (ret == 0)
                {
                    break;
                }

                ::mp_pump_demux_rtp(sink->get_handle(), rtp.demux, rtp.buf, rtp.len, &rtp.rtpH, NULL, &rtp.p, &rtp.address);
            } while (false);
        }
    }

    void media_link_impl_base::rtp_demux_handler(void *hdemux)
    {
        uint8_t buf[RTP_PACKAGE_MAX_SIZE];
        uint32_t len = sizeof(buf);

        rv_rtp rtpH;
        rv_rtp_param p;
        rv_net_address address;

        bool ret = true;
        while (ret)
        {
            ret = ::mp_read_demux_rtp(hdemux, buf, len, &rtpH, NULL, &p, &address);

            if ((!ret) || ((uint32_t)p.len > len))
            {
                break;
            }

            spinlock_t::scoped_lock _lock(rtp_mutex_);
            uint32_t new_tail_rtp_demux = tail_rtp_demux_;

            if (new_tail_rtp_demux+1>=MAX_RTP_DEMUX)
            {
                new_tail_rtp_demux = 0;
            }
            else
            {
                new_tail_rtp_demux += 1;
            }

            if (new_tail_rtp_demux == front_rtp_demux_)
            {
                continue;
            }
            else
            {
                tail_rtp_demux_ = new_tail_rtp_demux;
            }

            rtp_demuxs_[tail_rtp_demux_].address = address;
            rtp_demuxs_[tail_rtp_demux_].demux = hdemux;
            rtp_demuxs_[tail_rtp_demux_].len =p.len;
            rtp_demuxs_[tail_rtp_demux_].p = p;
            rtp_demuxs_[tail_rtp_demux_].rtpH = rtpH;
            if (p.extensionBit==1)
            {
                ::memcpy(rtp_demuxs_[tail_rtp_demux_].m_exHead,p.extensionData,p.extensionLength*sizeof(uint32_t));                
                rtp_demuxs_[tail_rtp_demux_].p.extensionData = rtp_demuxs_[tail_rtp_demux_].m_exHead;
            }
            ::memcpy(&rtp_demuxs_[tail_rtp_demux_].buf, buf, len);
        }
    }

	bool media_link_impl_base::stream_multi_setup(xt_av_media_t media_type, xt_av_codec_id_t cid, bool demux, bool with_describe, const char *multicast_ip, uint16_t multicast_port)
	{
		rtp_sink_ptr rtp_sink = new_rtp_sink(media_type, cid);
		bool rtp_open_success = false;
		if (!rtp_sink)
		{
			md_log(md_log_error, "new rtp sink failed,media_type(%d),cid(%d)", media_type, cid);
			return false;
		}

		uint32_t demuxid = 0;
		uint16_t ports[2] = { 0 };
		uint8_t mode = 0;

		if (NULL != multicast_ip)
		{
			multicast_param_t multicast_param = { 0 };
			multicast_param.ip = multicast_ip;
			multicast_param.port = multicast_port;
			multicast_param.ttl = 127;

			ports[0] = multicast_port;
			ports[1] = multicast_port + 1;

			scoped_ports_mgr_helper<ports_mgr_t> _ports_mgr_helper(ports_mgr_.get_ports_mgr());
			while (_ports_mgr_helper.get_ports(2, ports,demux,1))
			{
				rv_context demux_handler = (rv_context)&s_rtp_demux_handler;
				if (rtp_sink->open_rtp("0.0.0.0", ports[0], ports[1], rtp_sink_t::unknown_output, demux, demuxid, &multicast_param, demux_handler))
				{
					md_log(md_log_info, "rtp_open ok.-media_type(%d),port(%d,%d),demux(%d),demuxid(%d)", media_type, ports[0], ports[1], demux, demuxid);
					rtp_open_success = true;

					xt_sink_info_t info;
					info.index = -1;
					info.port_rtp = ports[0];
					info.port_rtcp = ports[1];
					info.demux = demux;
					info.demuxid = demuxid;
					rtp_sink->set_info(info);
					break;
				}
				//复用下只使用一对端口如果失败更换端口重试
				if (demux)
				{
					if (_ports_mgr_helper.update_demux_port())
					{
						continue;
					}
					else
					{
						break;
					}

				}
				_ports_mgr_helper.ports_failed(2,  ports);
			}

			if (!rtp_open_success)
			{
				md_log(md_log_error, "rtp open failed");
				return false;
			}

			//added by lichao, 20150210 解决部分端口没有释放的问题
			ports_mgr_.ports_used(2, ports);
		}
	

		std::vector<xt_session_param_t> params(1);
		params[0].stream_type = media_type;
		params[0].client_ctx.ssrc = rtp_sink->get_ssrc();
		params[0].client_ctx.demux = demux;
		params[0].client_ctx.demuxid = demuxid;
		params[0].client_ctx.mode = mode;
		params[0].client_ctx.rtp_port = ports[0];
		params[0].client_ctx.rtcp_port = ports[1];
		params[0].is_unicast = 0;
		strncpy (params[0].destination,multicast_ip,MEDIA_CLIENT_IP_LEN);

		if (!session_setup(params, with_describe))
		{
			return false;
		}

		md_log(md_log_info,"media_link_impl_base::streams_setup: destination[%s],stream_type[%d]",
			params[0].destination,params[0].stream_type);

		xt_session_server_info_t server_info = { 0 };
		session_->get_server_info(server_info);
		if (!rtp_sink->add_remote_address(server_info.ip, params[0].server_ctx.rtp_port, params[0].server_ctx.rtcp_port, 0 != params[0].server_ctx.demux, params[0].server_ctx.demuxid))
		{
			return false;
		}

		add_rtp_sink(rtp_sink);
		return true;
	}

    media_link_impl::media_link_impl(ports_mgr_t *ports_mgr)
        :media_link_impl_base(ports_mgr),
        cb_(NULL),
        ctx_(NULL)
    {}

    xt_media_client_status_t media_link_impl::get_header(uint8_t *data, uint32_t *length)
    {
        if ((NULL == data) || (NULL == length))
        {
            return MEDIA_CLIENT_STATUS_BADPARAM;
        }

        const std::string& sdp = media_link_impl_base::get_sdp();
        (void)memcpy(data, sdp.c_str(), sdp.length());
        *length = sdp.length();       
        md_log(md_log_debug, "media_link_impl get_header sdp[%s],length[%d]",sdp.c_str(),sdp.length());

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_impl::play(xt_media_client_frame_callback_t cb, void *ctx)
    {
        {
            spinlock_t::scoped_lock _lock(cb_mtx_);
            cb_ = cb;
            ctx_ = ctx;
        }

        //added by lichao, 20151210 回调置空 即注销了外部数据回调
        if (NULL == cb)
        {
            return MEDIA_CLIENT_STATUS_OK;
        }

        return start_capture(this);
    }

    xt_media_client_status_t media_link_impl::close()
    {
        close_link();
        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_impl::pause()
    {
        if (!get_session())
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        return get_session()->pause();
    }

    xt_media_client_status_t media_link_impl::seek(double npt, float scale, uint32_t *seq, uint32_t *timestamp)
    {
        if (!get_session())
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        return get_session()->play(npt, scale, seq, timestamp);
    }

    xt_media_client_status_t media_link_impl::register_rtcp_callback(xt_media_client_rtcp_report_callback_t cb, void *ctx)
    {
        rtcp_report_callback_impl::register_rtcp_callback(cb, ctx);
        return media_link_impl_base::register_rtcp_callback(this);
    }

    xt_media_client_status_t media_link_impl::get_ports_info(std::vector<xt_sink_info_t>& ports_info)
    {
        media_link_impl_base::get_port(ports_info);
        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_impl::rtcp_send_fir()
    {
        return media_link_impl_base::rtcp_send_fir();
    }

    xt_media_client_status_t media_link_impl::set_sdp(const std::string& sdp)
    {
        return media_link_impl_base::set_sdp(sdp);
    }

    media_link_impl2::media_link_impl2(ports_mgr_t *ports_mgr)
        :media_link_impl(ports_mgr),
        prev_rtp_total_bytes_(0),
        last_rtp_total_bytes_(0)
        //last_rtcp_rr_()
    {
        //(void)memset(&last_rtcp_rr_, 0, sizeof(rtcp_recv_report_t));
    }

    media_link_impl2::~media_link_impl2()
    {
        thread_timer::close();
    }

    xt_media_client_status_t media_link_impl2::query_prof_info(xt_rtp_prof_info_t *prof)
    {
        if (NULL == prof)
        {
            return MEDIA_CLIENT_STATUS_BADPARAM;
        }

        //{
        //    spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        //    while (rtp_sinks_[0]->get_rtcp_rr(&last_rtcp_rr_));
        //}

        //prof->recv_packet_cumulative_lost = last_rtcp_rr_.cumulativeLost;
        //prof->recv_packet_fraction_lost = (last_rtcp_rr_.fractionLost >> 8);
        //prof->recv_jitter = last_rtcp_rr_.jitter / 90;

        if (last_rtp_total_bytes_ > prev_rtp_total_bytes_)
        {
            prof->recv_Bps = static_cast<uint32_t>((last_rtp_total_bytes_ - prev_rtp_total_bytes_) * 1000.0 / RTP_BPS_CALA_PRIOD);
        }
        else
        {
            prof->recv_Bps = 0;
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_impl2::play(xt_media_client_frame_callback_t cb, void *ctx)
    {
        xt_media_client_status_t stat = media_link_impl::play(cb, ctx);
        if (MEDIA_CLIENT_STATUS_OK == stat)
        {
            thread_timer::set_interval(RTP_BPS_CALA_PRIOD);
        }

        return stat;
    }

    void media_link_impl2::on_timer()
    {
        rtp_prof_info_t rpi = { 0 };
        spinlock_t::scoped_lock _lock(rtp_sinks_mutex_);
        if (!rtp_sinks_.empty() && rtp_sinks_[0]->get_rtp_prof_info(&rpi))
        {
            prev_rtp_total_bytes_ = last_rtp_total_bytes_;
            last_rtp_total_bytes_ = rpi.rtp_total_bytes;
        }
    }

    media_link_ref_impl_t::media_link_ref_impl_t(ports_mgr_t *ports_mgr)
        :media_link_impl_base(ports_mgr),
        obs_(),
        mutex_()
    {}

	void media_link_impl::on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc)
	{
		no_frame_arrived_callback_impl::update_frame_arrived_ts();

		if (cb_)
		{
			cb_(ctx_, this, data, length, frame_type, data_type, timestamp, ssrc);
		}
	}
	//使用引用media_link，create link时工厂中对象为该类实例
	void media_link_ref_t::on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc)
	{
		no_frame_arrived_callback_impl::update_frame_arrived_ts();
		if (cb_)
		{
			if ((2 != stream_type_) || ((OV_AUDIO == frame_type) || (HC_AUDIO == frame_type) || (OV_AAC == frame_type)))
			{
				//XT_LOG(debug, "on_frame_dump_std ctx_(%p) data(%p) length(%u),frame_type(%u) timestamp(%u) ssrc(%u)", ctx_, data, length, frame_type, timestamp, ssrc);
				//printf("media_link_ref_t ssrc:%d\n",ssrc);
				cb_(ctx_, this, data, length, frame_type, data_type, timestamp, ssrc);
			}
#ifdef _USE_VEDIO_DEBUG
			if ((OV_AUDIO == frame_type) || (HC_AUDIO == frame_type) || (OV_AAC == frame_type))
			{}
			else
			{
				if (NULL != debug_video_file_.get())
				{
					debug_video_file_->write((const char *)data + 24, length - 24);
				}
			}
#endif
		}
	}

    void media_link_ref_impl_t::on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc)
    {
        scoped_lock _lock(mutex_);
        for (frame_cb_container_t::const_iterator it = obs_.begin(); obs_.end() != it; ++it)
        {
            if (*it)
            {
                (*it)->on_frame_dump(data, length, frame_type, data_type, timestamp, ssrc);
   //             XT_LOG(info, "on_frame_dump,data(%p),length(%u),frame_type(%u), data_type(%u),timestamp(%u),ssrc(%u)",data, length, frame_type, data_type, timestamp, ssrc);
            }
        }
    }

    void media_link_ref_impl_t::register_frame_dump_observer(frame_data_dump_callback_t *cb)
    {
        scoped_lock _lock(mutex_);
        obs_.push_back(cb);
    }

    void media_link_ref_impl_t::unregister_frame_dump_observer(frame_data_dump_callback_t *cb)
    {
        scoped_lock _lock(mutex_);
        std::vector<frame_data_dump_callback_t *>::iterator it = std::find(obs_.begin(), obs_.end(), cb);
        if (obs_.end() != it)
        {
            obs_.erase(it);
        }
    }

    bool media_link_ref_impl_t::empty() const
    {
        scoped_lock _lock(mutex_);
        return obs_.empty();
    }

    xt_media_client_status_t media_link_ref_impl_t::get_header(uint8_t *data, uint32_t *length)
    {
        if ((NULL == data) || (NULL == length))
        {
            return MEDIA_CLIENT_STATUS_BADPARAM;
        }

        const std::string& sdp = media_link_impl_base::get_sdp();

        (void)memcpy(data, sdp.c_str(), sdp.length());
        *length = sdp.length();

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_ref_impl_t::play()
    {
        std::size_t n = 0;
        {
            scoped_lock _lock(mutex_);
            n = obs_.size();
        }

        if (1 == n)
        {
            return start_capture(this);
        }

        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_ref_impl_t::close()
    {
        close_link();
        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_ref_impl_t::pause()
    {
        if (!get_session())
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        return get_session()->pause();
    }

    xt_media_client_status_t media_link_ref_impl_t::seek(double npt, float scale, uint32_t *seq, uint32_t *timestamp)
    {
        if (!get_session())
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        return get_session()->play(npt, scale, seq, timestamp);
    }

    media_link_ref_t::media_link_ref_t()
        :impl_(NULL),
        stream_type_(0),
        cb_(NULL),
        ctx_(NULL)
    {}

    void media_link_ref_t::init(media_link_ref_impl_t *impl, uint32_t stream_type)
    {
        impl_ = impl;
        stream_type_ = stream_type;
    }

    media_link_ref_t::~media_link_ref_t()
    {
        spinlock_t::scoped_lock _lock(cb_mtx_);
        cb_ = NULL;
        ctx_ = NULL;
    }

    xt_media_client_status_t media_link_ref_t::get_header(uint8_t *data, uint32_t *length)
    {
        if (NULL == impl_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        xt_media_client_status_t stat = impl_->get_header(data, length);
        if ((MEDIA_CLIENT_STATUS_OK == stat) && (2 == stream_type_))
        {
            int len = *length;
            if (!sdp_parser_t::remove_on_pure_audio(reinterpret_cast<char *>(data), len))
            {
                return MEDIA_CLIENT_STATUS_BAD_SDP_CTX;
            }

            *length = len;
        }
        return stat;
    }

    xt_media_client_status_t media_link_ref_t::play(xt_media_client_frame_callback_t cb, void *ctx)
    {
        if (NULL == impl_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        {
            spinlock_t::scoped_lock _lock(cb_mtx_);
            cb_ = cb;
            ctx_ = ctx;
        }

        //added by lichao, 20151210 回调置空 即注销了外部数据回调
        if (NULL == cb)
        {
            return MEDIA_CLIENT_STATUS_OK;
        }

        if (NULL != impl_)
        {
            impl_->register_frame_dump_observer(this);
        }

        return impl_->play();
    }

    xt_media_client_status_t media_link_ref_t::close()
    {
        if (NULL == impl_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }

        impl_->unregister_frame_dump_observer(this);
        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_ref_t::pause()
    {
        if (NULL == impl_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }
        return impl_->pause();
    }

    xt_media_client_status_t media_link_ref_t::seek(double npt, float scale, uint32_t *seq, uint32_t *timestamp)
    {
        if (NULL == impl_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }
        return impl_->seek(npt, scale, seq, timestamp);
    }

    xt_media_client_status_t media_link_ref_t::register_rtcp_callback(xt_media_client_rtcp_report_callback_t cb, void *ctx)
    {
        rtcp_report_callback_impl::register_rtcp_callback(cb, ctx);
        return impl_->register_rtcp_callback(this);
    }

    xt_media_client_status_t media_link_ref_t::get_ports_info(std::vector<xt_sink_info_t>& ports_info)
    {
        if (NULL == impl_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }
        impl_->get_port(ports_info);
        return MEDIA_CLIENT_STATUS_OK;
    }

    xt_media_client_status_t media_link_ref_t::rtcp_send_fir()
    {
        if (NULL == impl_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }
        return impl_->rtcp_send_fir();
    }

    xt_media_client_status_t media_link_ref_t::set_sdp(const std::string& sdp)
    {
        if (NULL == impl_)
        {
            return MEDIA_CLIENT_STATUS_NULLPTR;
        }
        return impl_->set_sdp(sdp);
    }

	xt_media_client_status_t media_link_impl::register_no_frame_arrived_callback(uint32_t priod, xt_media_client_no_frame_arrived_callback_t cb, void *ctx)
	{
		no_frame_arrived_callback_impl::register_no_frame_arrived_callback(priod, cb, ctx);
		return MEDIA_CLIENT_STATUS_OK;
	}

	xt_media_client_status_t media_link_ref_t::register_no_frame_arrived_callback(uint32_t priod, xt_media_client_no_frame_arrived_callback_t cb, void *ctx)
	{
		no_frame_arrived_callback_impl::register_no_frame_arrived_callback(priod, cb, ctx);
		return MEDIA_CLIENT_STATUS_OK;
	}
}
