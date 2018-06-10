///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����mssrc_frame.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��26��
// ����������ý������Դ -- ֡����
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSSRC_FRAME_ENTITY_
#define MSSRC_FRAME_ENTITY_

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
    class mssrc_frame : public mssrc
    {
    public:
        mssrc_frame();
        virtual ~mssrc_frame();

    public:
        bool open(bool bActive);						//����
        void close();									//�ر�
        bool active(bool bActive);						//����
        inline bool isAcitve() { return m_active; }

        bool pump_frame_in(rtp_mblock *mrtp);			//�ְ�����֡д��
        rtp_mblock *pump_frame_out();					//�ְ�����֡��ȡ
        //�����ȡ��ʽRTP���ݰ�������ʵ����ȡ����
		uint32_t pump_frames_out(msink_rv_rtp * hmsink,rtp_mblock *mrtp, uint32_t canSendSize);

    protected:
        //����صķ����ȥ����̽���mssrc��fifoд�������m_active׼�뷽ʽ���п���
        virtual void recycle_alloc_event();				
        virtual void recycle_release_event();

    public:
        tghelper::recycle_queue m_fifo;
        bool m_active;
    };
}

#endif

