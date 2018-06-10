///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����mssrc_rv_rtp.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��04��05��
// ����������ý������Դ -- rv_rtp������
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSSRC_RV_RTP_ENTITY_
#define MSSRC_RV_RTP_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "xt_mp_caster_def.h"
#include <../rv_adapter/rv_def.h>

namespace xt_mp_caster
{
    /*
    �߱�һ��fifo��ʵ���࣬�ڲ�����rv_adpater�����ݽ��ջ���
    1��open/close�ɶԵ���ȷ��ʵ����Ч�����л���m_bReady������ʶopen/close����״̬
    2��active/recycle_alloc_event/recycle_release_event����m_active��״̬�仯
    3��pump_frame_in/pump_frame_out�����ݵ�д��Ͷ�ȡ�ӿڣ����ܿ���m_active״̬
    4�����಻���������ⲿ���������͹���
    */
    class mssrc_rv_rtp : public mssrc
    {
    public:
        mssrc_rv_rtp();
        virtual ~mssrc_rv_rtp();

    public:
        bool open(mp* hmp,
            inner::rtp_pool *rtp_pool,
            rv_net_address local_address,
            bool manual_rtcp,
            bool bActive,
            bool bMulticastRTP,
            rv_net_address multicast_rtp_address,
            bool bMulticastRTCP,
            rv_net_address multicast_rtcp_address);		//����
        void close();											//�ر�
        bool active(bool bActive);								//����
        inline bool isAcitve() { return m_active; }

    protected:
        bool pump_rtp_in(rtp_block *rtp);			//RTP���ݰ�д�� -- ���ṩ�ڲ�����д��

    public:
        rtp_block *pump_rtp_out();					//RTP���ݰ���ȡ
        //�����ȡ��ʽRTP���ݰ�������ʵ����ȡ����
        uint32_t pump_rtps_out(std::list<rtp_block *> &resultBlocks, uint32_t canSendSize);

        bool read_rtcp_sr(rtcp_send_report *sr);
        bool read_rtcp_rr(rtcp_receive_report *rr);

    protected:
        //����صķ����ȥ�����
        //1����mssrc��fifoд�������m_active׼�뷽ʽ���п���
        //2��ȥ��������Զ��ر�rv_adapter��session��Դ
        virtual void recycle_alloc_event();
        virtual void recycle_release_event();

        static void OnRtpReceiveEvent_AdapterFunc(
            RV_IN rv_handler  hrv,
            RV_IN rv_context context);
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
            RV_IN uint32_t size);

    private:
        tghelper::recycle_queue m_fifo;
        rv_net_address m_local_address;
        rv_handler_s m_hrv;
        bool m_active;
        tghelper::recycle_queue m_sr_fifo;
        tghelper::recycle_queue m_rr_fifo;
        mp* m_mp;
        inner::rtp_pool *m_rtp_pool;
    };
}

#endif
