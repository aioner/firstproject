#ifndef _RTP_SINK_H_INCLUDED
#define _RTP_SINK_H_INCLUDED

#include "novtable.h"

#include<stdint.h>
#include <boost/shared_ptr.hpp>
#include "xt_media_client_types.h"
#include "rtp_unpack.h"

namespace xt_media_client
{
    //struct rtcp_recv_report_t
    //{
    //    uint32_t    ssrc;           /* 报告生成方SSRC */
    //    uint32_t    fractionLost;   /* The fraction of RTP data packets from source specified by
    //                                   SSRC that were lost since previous SR/RR packet was sent. */
    //    uint32_t    cumulativeLost; /* Total number of RTP data packets from source specified by
    //                                   SSRC that have been lost since the beginning of reception. */
    //    uint32_t    sequenceNumber; /* Sequence number that was received from the source specified
    //                                   by SSRC. */
    //    uint32_t    jitter;         /* Estimate of the statistical variance of the RTP data packet inter
    //                                       arrival time. */
    //    uint32_t    lSR;            /* The middle 32 bits of the NTP timestamp received. */
    //    uint32_t    dlSR;           /* Delay since the last SR. */
    //};

    struct rtp_prof_info_t
    {
        uint64_t rtp_total_bytes;
    };

    class MEDIA_CLIENT_NO_VTABLE rtcp_report_callback_t
    {
    public:
    	virtual void on_rtcp_receive(uint32_t ssrc, const xt_media_client_rtcp_sr_t *sr) = 0;
    	virtual void on_rtcp_send(uint32_t ssrc, const xt_media_client_rtcp_rr_t *rr) = 0;
    protected:
    	virtual ~rtcp_report_callback_t() {}
    };

    class MEDIA_CLIENT_NO_VTABLE frame_data_dump_callback_t
    {
    public:
        virtual void on_frame_dump(void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc) = 0;
    protected:
        virtual ~frame_data_dump_callback_t() {}
    };

    struct multicast_param_t
    {
        const char *ip;
        uint16_t port;
        uint8_t ttl;
    };

    class MEDIA_CLIENT_NO_VTABLE rtp_sink_t
    {
    public:
        enum open_rtp_mode
        {
            raw_rtp_output,
            frame_output,
            unknown_output
        };

        virtual bool open_rtp(const char *ip, uint16_t rtp_port, uint16_t rtcp_port, open_rtp_mode open_mode, bool demux, uint32_t& demuxid, const multicast_param_t *multicast = NULL, rv_context demux_handler = NULL) = 0;
        virtual void active_rtp(frame_data_dump_callback_t *cb) = 0;
        virtual void close_rtp() = 0;
        virtual bool add_remote_address(const char *ip, uint16_t rtp_port, uint16_t rtcp_port, bool demux, uint32_t demuxid) = 0;
        //virtual bool get_rtcp_rr(rtcp_recv_report_t *rr) const = 0;
        virtual bool get_rtp_prof_info(rtp_prof_info_t *rpi) const = 0;
        virtual uint32_t get_ssrc() const = 0;
		virtual long rtcp_send_fir() = 0;
		virtual void add_unpacker(int payload, const rtp_unpack_ptr &unpacker) = 0;
		virtual bool is_demux(){return 0;}
		virtual msink_handle* get_handle(){return 0;}

		void set_info(const xt_sink_info_t &info){m_info = info;}
		void get_info(xt_sink_info_t &info){info = m_info;}

        virtual void register_rtcp_callback(rtcp_report_callback_t *cb) = 0;
    protected:
        virtual ~rtp_sink_t() {}

	public:
		xt_sink_info_t m_info;
    };

    typedef boost::shared_ptr<rtp_sink_t> rtp_sink_ptr;
}

#endif //_RTP_SINK_H_INCLUDED
