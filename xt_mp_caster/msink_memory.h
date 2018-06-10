///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����msink_memory.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��26��
// ����������ý�������� -- �ڴ�ӿ���
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSINK_MEMORY_ENTITY_
#define MSINK_MEMORY_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "xt_mp_caster_def.h"

namespace xt_mp_caster
{
    /*
    �߱�һ��fifo������ص������ӿڵ�ʵ����
    1��open/close�ɶԵ���ȷ��ʵ����Ч�����л���m_bReady������ʶopen/close����״̬
    2��active/recycle_alloc_event/recycle_release_event����m_active��״̬�仯
    3��pump_frame_in/pump_frame_out�����ݵ�д��Ͷ�ȡ�ӿڣ����ܿ���m_active״̬
    4������һ�ɲ���rtp_block��ʽд�룬�ص��������rtp��ͷ��������Ϣ
    */
    class msink_memory : public msink
    {
    public:
        msink_memory();
        virtual ~msink_memory();

    public:
        bool open(memory_sink_descriptor *descriptor, bool bActive);
        void close();
        bool active(bool bActive);
        inline bool isAcitve() { return m_active; }

        bool pump_rtp_in(rtp_block *rtp);				//rtp���ݰ�д��
        void pump_rtp_out(mp * hmp);					//rtp���ݰ��Ƴ����ص�������ֱ������Ϊ��

    protected:
        virtual void recycle_alloc_event();	
        virtual void recycle_release_event();

    private:
        tghelper::recycle_queue m_fifo;
        memory_sink_descriptor m_descriptor;
        bool m_active;
    };
}

#endif