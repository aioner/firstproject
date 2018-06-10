///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����msink_rv_rtp.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��26��
// ����������ý�������� -- rv adapter���� RTP��
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSINK_RV_RTP_ENTITY_
#define MSINK_RV_RTP_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "xt_mp_caster_def.h"
#include <../rv_adapter/rv_def.h>
#include <boost/thread/mutex.hpp>
#include "ReSendMan.h"
#include "rtp_packet_block.h"
#ifdef _USE_RTP_TRAFFIC_SHAPING
#include "traffic_shaping.h"
#endif  //_USE_RTP_TRAFFIC_SHAPING

#ifdef _USE_RTP_SEND_CONTROLLER
#include "send_side_controller.h"
#endif

#define MAX_RTP_PACK	10240

typedef void (*pSink_RtcpCB)(uint32_t ssrc, rv_rtcp_info &send, rv_rtcp_info &recieve,
							 uint8_t *ip,uint16_t port, rv_bool multiplex,uint32_t multid);
typedef rv_bool (*pRtcpAppMsgCB)(
                                 uint32_t	   serverid,
                                 uint8_t        subtype,
                                 uint32_t       ssrc,
                                 uint8_t*       name,
                                 uint8_t*       userData,
                                 uint32_t       userDataLen);
typedef rv_bool (*pRtcpRawDataCB)(
                                  mp_handle sink,
                                  uint8_t *buffer,
                                  uint32_t buffLen,
                                  rv_net_address *remoteAddress,
                                  rv_bool *pbDiscardBuffer);

struct PktInfo
{
    unsigned long count;
    double interval;
    unsigned long tm;
};

namespace xt_mp_caster
{
    /*
    �߱�һ��fifo��ʵ����
    1��open/close�ɶԵ���ȷ��ʵ����Ч�����л���m_bReady������ʶopen/close����״̬
    2��active/recycle_alloc_event/recycle_release_event����m_active��״̬�仯
    3��pump_frame_in/pump_frame_out�����ݵ�д��Ͷ�ȡ�ӿڣ����ܿ���m_active״̬

    */
    class msink_rv_rtp : public msink
#ifdef _USE_RTP_TRAFFIC_SHAPING
    , public flow_out_callback_t
#endif	//_USE_RTP_TRAFFIC_SHAPING
    {
        friend class bc_mp;
    public:
        msink_rv_rtp();
        virtual ~msink_rv_rtp();

    public:
        bool open(rv_net_address local_address,
            bool manual_rtcp,
            bool bActive,
            uint8_t multicast_rtp_ttl,
            uint8_t multicast_rtcp_ttl,
            mp_bool multiplex,
            uint32_t *multID = NULL
#ifdef _USE_RTP_SEND_CONTROLLER
            ,rtcp_observer_t *rtcp_observer = NULL
#endif
            );
        void close();
        bool active(bool bActive);
        inline bool isAcitve() { return m_active; }

        bool pump_rtp_in(rtp_block *rtp);				//rtp���ݰ�д��
		void pump_rtp_out();							//rtp���ݰ��Ƴ���rv_adapter
        bool pump_mrtp_in(rtp_mblock *mrtp);			//mrtp����γ�rtp���ݰ�д��
		void pump_mrtp_out(rtp_mblock *mrtp);           //mrtp����γ�rtp���ݰ��Ƴ���rv_adapter  
		uint16_t m_last_frame_in_rtp_sn;//ʹ���ⲿ����
        virtual bool do_task()
        {
            pump_rtp_out();
            return true;
        }

        inline void inc_viewer()
        {
            boost::mutex::scoped_lock lock(m_viewer_mutex);
            m_viewer_nums++;
        }

        inline void dec_viewer()
        {
            boost::mutex::scoped_lock lock(m_viewer_mutex);
            if (m_viewer_nums > 0)	m_viewer_nums--;
        }

        inline uint32_t get_viewer() 
        {
            return m_viewer_nums; 
        }

        bool read_rtcp_sr(rtcp_send_report *sr);
        bool read_rtcp_rr(rtcp_receive_report *rr);

        //���¶����ش����� add by songlei 20150708
        inline void update_resend_flag(const int resend_flag)
        {
            m_nReSend = resend_flag;
        }

    public:
        virtual void recycle_alloc_event();
        virtual void recycle_release_event();

        static void OnRtpReceiveEvent_AdapterFunc(
            RV_IN rv_handler  hrv,
            RV_IN rv_context context);
        static void RtpDemuxEventHandler(
            RV_IN void*   hDemux,
            RV_IN void*  context);
        static void RV_CALLCONV OnRtcpSendHandler_AdapterFunc(
            RV_IN rv_handler hrv,
            RV_IN rv_context context,
            RV_IN uint32_t ssrc,
            RV_IN uint8_t *rtcpPack,
            RV_IN uint32_t size);
        static void RV_CALLCONV OnRtcpReceiveHandler_AdapterFunc(
            RV_IN rv_handler hrv,
            RV_IN rv_context context,
            RV_IN uint32_t ssrc,
            RV_IN uint8_t *rtcpPack,
            RV_IN uint32_t size,
			RV_IN uint8_t *ip,
			RV_IN uint16_t port,
			RV_IN rv_bool multiplex,
			RV_IN uint32_t multid);

        static rv_bool RV_CALLCONV OnRtcpAppEventHandler_CB(
            RV_IN  rv_handler	 hrv,
            RV_IN  rv_context	 context,
            RV_IN  uint8_t        subtype,
            RV_IN  uint32_t       ssrc,
            RV_IN  uint8_t*       name,
            RV_IN  uint8_t*       userData,
            RV_IN  uint32_t       userDataLen); /* userData in bites!, not in 4 octet words */

        static rv_bool RV_CALLCONV OnRtcpRawBufferReceived_CB(
            RV_IN  rv_handler	 hrv,
            RV_IN  rv_context	 context,
            RV_IN uint8_t *buffer,
            RV_IN uint32_t buffLen,
            RV_OUT rv_net_address *remoteAddress,
            RV_OUT rv_bool *pbDiscardBuffer);

        //��������������rv_adapter
        void write_to_rv_adapter(rtp_block *rtp, bool bReSend = false);

    public:
        tghelper::recycle_queue m_fifo;
        rv_net_address m_local_address;
        rv_handler_s m_hrv;
        mp_bool m_multiplex;
        uint32_t m_multiplexID;	// ����ID
        bool m_active;
        boost::mutex m_viewer_mutex;
        uint32_t m_viewer_nums;
        tghelper::recycle_queue m_sr_fifo;
        tghelper::recycle_queue m_rr_fifo;

        // ��������
        //TrafficShaping m_trafficShaping;

        // �����ط�
        ReSendMan	m_manReSend;

		//�Ƿ����ش�
        int m_nReSend;

		//�Ƿ��������ȼ�
		int m_open_pri;

        mp *m_mp;

		std::string m_file_path;

    public:
        // ����RTCP����ص�
        pSink_RtcpCB m_pSink_RtcpCB;

        // ͨ����
        long m_nSID;

        // ��������Ϣ
        PktInfo m_iPkt;

        // Rtcp�ص�
        void setSink_RtcpCB(pSink_RtcpCB func){m_pSink_RtcpCB = func;}

        // ����APP Message
        pRtcpAppMsgCB m_pAppMsgCB;
        void setSink_AppMsgCB(pRtcpAppMsgCB func){m_pAppMsgCB = func;}

        pRtcpRawDataCB m_pRtcpRawCB;
        void rtcp_set_rawcb(pRtcpRawDataCB cb){m_pRtcpRawCB = cb;}

		void set_file_path(const char *file);

		void write_file(rtp_block *rtp);

        int m_scheck_sum;

    public:
		void internel_write_to_rv_adapter(rtp_block *rtp);
		inner::rtp_pool m_rtp_pool; //�ڴ�أ������ɿ������Ʋ�����rtp_block
		void rv_write_rtp(rtp_block *rtp);
#ifdef _USE_RTP_TRAFFIC_SHAPING
        void on_flow_out(void *flow);

        std::auto_ptr<traffic_shaping_wrapper_t> m_traffic_shaping;
#endif

#ifdef _USE_RTP_SEND_CONTROLLER
        std::auto_ptr<rtcp_observer_t> m_rtcp_observer;
#endif
    };
}

#endif
