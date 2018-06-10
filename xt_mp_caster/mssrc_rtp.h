///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����mssrc_rtp.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��26��
// ����������ý������Դ -- rtp������
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSSRC_RTP_ENTITY_
#define MSSRC_RTP_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "xt_mp_caster_def.h"
#include "msink_rv_rtp.h"
namespace xt_mp_caster
{
    /*
    �߱�һ��fifo��ʵ����
    1��open/close�ɶԵ���ȷ��ʵ����Ч�����л���m_bReady������ʶopen/close����״̬
    2��active/recycle_alloc_event/recycle_release_event����m_active��״̬�仯
    3��pump_frame_in/pump_frame_out�����ݵ�д��Ͷ�ȡ�ӿڣ����ܿ���m_active״̬
    */
    class mssrc_rtp : public mssrc
    {
    public:
        mssrc_rtp();
        virtual ~mssrc_rtp();

    public:
        bool open(bool bActive);								//����
        void close();											//�ر�
        bool active(bool bActive);								//����
        inline bool isAcitve() { return m_active; }

        bool pump_rtp_in(rtp_block *rtp);			//RTP���ݰ�д��
        rtp_block *pump_rtp_out();					//RTP���ݰ���ȡ
        //�����ȡ��ʽRTP���ݰ�������ʵ����ȡ����
        uint32_t pump_rtps_out(msink_rv_rtp * hmsink, uint32_t canSendSize);

    protected:
        //����صķ����ȥ����̽���mssrc��fifoд�������m_active׼�뷽ʽ���п���
        virtual void recycle_alloc_event();	
        virtual void recycle_release_event();

    private:
        tghelper::recycle_queue m_fifo;
        bool m_active;
    };
}

#endif