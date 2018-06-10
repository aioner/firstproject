///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����caster_engine.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��23��
// �������������̵߳�������
// 1������boost::threadpool��Ϊ�ڲ���������
// 2������2����ˮ������ģʽ, mssrc_task����mssrc���ݴ���
//	  ������mp��ת����ϵ�������ݷַ����̣���Ͷ��msink_task
// 3��msink_task����msink�����ݴ���
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef CASTER_ENGINE_
#define CASTER_ENGINE_

#include "mp_caster_config.h"
#include <boost/threadpool.hpp>
#include <tghelper/time_system.h>
#include "mp.h"

namespace xt_mp_caster
{	

    //��������
    typedef enum ECASTER_TASK_
    {
        ECASTER_TASK_MSSRC,				//��׼MSSRC����������û�����Ͷ��
        ECASTER_TASK_MSINK,				//��׼MSINK������MSSRC���������Ͷ��
    } ECASTER_TASK;
    class caster_engine //: public tghelper::share_once_timer
    {
        //����mp_caster������ͷ�
        friend class caster;
    protected:
        // threadnums	����������߳�����
        // core_period	��ʱ��ʱ������С��ʱ��϶ 0 - ��ʾ����tghelperĬ������
        caster_engine(uint32_t threadnums, uint32_t core_period = 0);
        virtual ~caster_engine();

        virtual void onDispatchEvent(tghelper::time_event_id tid, bool bFastRelease);


    public:
        virtual bool post_task(mp *hmp, ECASTER_TASK task);
        virtual bool post_delay_mssrc_task(mp *hmp, uint32_t delayMS);
        virtual void stop();

        int task_size(){return m_tp.pending();}

    public:
        //�����
        boost::threadpool::pool m_tp;
    };
}

#endif
