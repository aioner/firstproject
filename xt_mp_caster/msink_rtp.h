///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����msink_rtp.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��26��
// ����������ý�������� -- RTP������
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSINK_RTP_ENTITY_
#define MSINK_RTP_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "xt_mp_caster_def.h"

#ifdef _USE_RTP_SEND_CONTROLLER
#include "send_side_controller.h"
#endif

namespace xt_mp_caster
{
    /*
    ����Ϊһ���������rv_adapter��RTP/RTCPĿ���ַ���ýӿ���
    */
    class msink_rtp : public msink
    {
    public:
        msink_rtp();
        virtual ~msink_rtp();

    public:
        bool open(rv_handler hrv, rtp_sink_descriptor *descriptor
#ifdef _USE_RTP_SEND_CONTROLLER
                ,bitrate_controller_t *bitrate_controller = NULL
#endif
            );
        void close(rv_handler hrv);

    protected:
        virtual void recycle_alloc_event();	
        virtual void recycle_release_event();

    public:
        rtp_sink_descriptor m_descriptor;
        rv_handler m_hrv;

    private:
#ifdef _USE_RTP_SEND_CONTROLLER
        bitrate_controller_t *m_bitrate_controller;
#endif
    };
}

#endif