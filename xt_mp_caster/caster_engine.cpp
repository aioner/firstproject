///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：caster_engine.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：多线程调度引擎
///////////////////////////////////////////////////////////////////////////////////////////

#include "caster_engine.h"
#include "mp.h"

namespace xt_mp_caster
{
    //内部回调
    namespace inner
    {
        void mssrc_task_func(caster_engine *engine, mp *hmp)
        {
            if (hmp && engine /*&& engine->runable()*/)
            {
                uint32_t delay = 0;
                if (hmp->do_mssrc_task(delay))
                {
                    //engine->post_task(hmp, ECASTER_TASK_MSINK);//该任务负责将msink的数据写入到rv库
                }

                if (0 < delay)
                    engine->post_delay_mssrc_task(hmp, delay);
                hmp->release();
            }
        }

        void msink_task_func(caster_engine *engine, mp *hmp)
        {
            if (hmp && engine /*&& engine->runable()*/)
            {
                hmp->do_msink_task();
                hmp->release();
            }
        }
    }

    bool caster_engine::post_task(mp *hmp, ECASTER_TASK task)
    {
        bool bRet = false;
        do 
        {
            //if (!runable()) break;
            if (!hmp) break;
            hmp->assign();
            switch (task)
            {
            case ECASTER_TASK_MSSRC:
                m_tp.schedule(boost::bind(inner::mssrc_task_func, this, hmp));
                break;
            case ECASTER_TASK_MSINK:
                m_tp.schedule(boost::bind(inner::msink_task_func, this, hmp));
                break;
            }
            bRet = true;
        } while (false);
        return bRet;
    }

    bool caster_engine::post_delay_mssrc_task(mp *hmp, uint32_t delayMS)
    {
        //增加,撤销定时事件
        //1,如果无此定时事件，则增加定时项
        //2,如果存在定时事件，则视现有定时长度确定是否修改
        //if (!runable()) return false;
        bool bRet = false;
        bool newEvent = false;
        hmp->assign();
        do 
        {
            //if (!make_time_event(hmp, delayMS, newEvent)) break;
            bRet = true;
        } while (false);
        //if (!newEvent) hmp->release();//外面已经release
        return bRet;
    }

    void caster_engine::stop()
    {
        //tghelper::share_once_timer::stop();
        m_tp.wait();
    }

    void caster_engine::onDispatchEvent(tghelper::time_event_id tid, bool bFastRelease) 
    {
        mp *hmp = (mp *)tid;
        if (!bFastRelease) post_task(hmp, ECASTER_TASK_MSSRC);
        hmp->release();
    }

    caster_engine::caster_engine(uint32_t threadnums, uint32_t core_period) : 
    m_tp(threadnums)
    {
		printf("\ncaster_engine threadnums=%d\n",threadnums);
        /*if (0 == core_period) start();
        else start(core_period);*/
    }
    caster_engine::~caster_engine()
    {
        //stop();
    }
}
