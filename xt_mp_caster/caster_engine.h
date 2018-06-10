///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：caster_engine.h
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：多线程调度引擎
// 1、采用boost::threadpool作为内部调度引擎
// 2、构造2级流水线任务模式, mssrc_task激励mssrc数据处理，
//	  并根据mp的转发关系产生数据分发过程，并投递msink_task
// 3、msink_task激励msink的数据处理
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef CASTER_ENGINE_
#define CASTER_ENGINE_

#include "mp_caster_config.h"
#include <boost/threadpool.hpp>
#include <tghelper/time_system.h>
#include "mp.h"

namespace xt_mp_caster
{	

    //任务类型
    typedef enum ECASTER_TASK_
    {
        ECASTER_TASK_MSSRC,				//标准MSSRC任务，由外界用户发起投递
        ECASTER_TASK_MSINK,				//标准MSINK任务，由MSSRC任务结束是投递
    } ECASTER_TASK;
    class caster_engine //: public tghelper::share_once_timer
    {
        //仅由mp_caster构造和释放
        friend class caster;
    protected:
        // threadnums	任务池驱动线程数量
        // core_period	延时定时器的最小延时间隙 0 - 表示采用tghelper默认设置
        caster_engine(uint32_t threadnums, uint32_t core_period = 0);
        virtual ~caster_engine();

        virtual void onDispatchEvent(tghelper::time_event_id tid, bool bFastRelease);


    public:
        virtual bool post_task(mp *hmp, ECASTER_TASK task);
        virtual bool post_delay_mssrc_task(mp *hmp, uint32_t delayMS);
        virtual void stop();

        int task_size(){return m_tp.pending();}

    public:
        //任务池
        boost::threadpool::pool m_tp;
    };
}

#endif
