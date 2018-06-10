///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：time_system.h
// 创 建 者：汤戈
// 创建时间：2012年03月25日
// 内容描述：定时器组件
//
// 1、所有接口类均为线程安全
// 2、采用boost库中的mutex对象实现线程同步
// 3、支持windows/linux跨平台
// 4、raw_timer linux版本改用信号触发实现
//
// 修订日志
// [2012-03-25]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////
#include "time_system.h"
#include <string.h>
#include <vector>
namespace tghelper
{
    namespace inner
    {
        ///////////////////////////////////////////////////////////////////////////////////////////
        //raw_timer 原始定时器，与操作系统相关
#ifdef _WIN32
        //loading lib
#pragma comment(lib,"winmm.lib")

        raw_timer::raw_timer() : m_id(INVALID_TIMER_ID), m_tm_period(0)
        {

        }
        bool raw_timer::started() { return (INVALID_TIMER_ID != m_id); }
        bool raw_timer::start(uint32_t tm_period)
        {
            if (started()) return false;
            if (0 == tm_period) return false;
            if (INVALID_TIMER_ID != (m_id = (uint32_t)::timeSetEvent(
                tm_period,
                SYSTEM_CORE_TIMER_RES,
                raw_timer::timeout,
                (uint32_t)this,
                TIME_PERIODIC)))
            {
                m_tm_period = tm_period;
                return true;
            }
            else
                return false;
        }

        void raw_timer::stop()
        {
            if (!started()) return;
            ::timeKillEvent(m_id);
            m_id = INVALID_TIMER_ID;
            m_tm_period = 0;
        }

        void CALLBACK raw_timer::timeout(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
        {
            raw_timer * timer = (raw_timer *)dwUser;
            if (!timer) return;
            timer->onTimeEvent();

        }
#else
        raw_timer::raw_timer() :m_id(INVALID_TIMER_ID),m_started(false), m_tm_period(0)
        {
            struct sigevent sige;
            ::memset(&sige, 0, sizeof(sige));

            sige.sigev_notify          = SIGEV_THREAD;
            sige.sigev_notify_function = &timeout;
            sige.sigev_value.sival_ptr = this;

            timer_create(CLOCK_REALTIME, &sige, &m_id);
        }
        bool raw_timer::started() { return m_started; }
        bool raw_timer::start(uint32_t tm_period)
        {
            if (started()) return false;
            if (0 == tm_period) return false;

            struct itimerspec its;
            ::memset(&its, 0, sizeof(its));

            its.it_value.tv_sec  = ( tm_period / 1000 );
            its.it_value.tv_nsec = ( tm_period % 1000 ) * 1000 * 1000;
            its.it_interval      = its.it_value;

            m_started = ( 0 == timer_settime( m_id, 0, &its, 0 ) );
            if (m_started) m_tm_period = tm_period;
            return m_started;
        }

        void raw_timer::stop()
        {
            if (!started()) return ;

            struct itimerspec its;
            ::memset(&its, 0, sizeof(its));

            m_started = !( 0 == timer_settime( m_id, 0, &its, 0 ) );
            if (!m_started) m_tm_period = 0;
        }

        void raw_timer::timeout(sigval_t t)
        {
            raw_timer * timer = (raw_timer *)(t.sival_ptr);
            if (!timer) return;
            timer->onTimeEvent();
        }
#endif
    }

    ///////////////////////////////////////////////////////////////////////////////////////////
    //share_once_timer 共享定时器
    share_once_timer::share_once_timer() :
    m_scheduler(0), m_bQuit(true)
    {

    }

    share_once_timer::~share_once_timer()
    {
        stop();
    }

    void share_once_timer::onTimeEvent()
    {
        m_schedule_event.notify_one();
    }

    void share_once_timer::dispatchfunc()
    {
        while(!m_bQuit)
        {
            boost::unique_lock<boost::mutex> lock(m_schedule_mutex);
            while(started())
            {
                m_schedule_event.wait(lock);
                if (m_bQuit) return;
                //dispatch time event
                std::vector<time_event_id> timeResults;
                //修改定时计数
                m_time_events_mutex.lock();
                std::map<time_event_id, uint32_t>::iterator it = m_time_events.begin();
                while(it != m_time_events.end())
                {
                    uint32_t current_delayMS = (*it).second;
                    if (current_delayMS <= m_tm_period)
                    {
                        time_event_id tid = (*it).first;
                        //it = m_time_events.erase(it);
                        m_time_events.erase(it++);

                        timeResults.push_back(tid);
                    }
                    else
                    {
                        (*it).second = current_delayMS - m_tm_period;
                        it++;
                    }
                }
                m_time_events_mutex.unlock();
                //退出锁定空间处理回调事件
                if (!timeResults.empty())
                {
                    std::vector<time_event_id>::iterator i;
                    for (i = timeResults.begin(); i != timeResults.end(); i++)
                    {
                        onDispatchEvent(*i, false);
                    }
                    timeResults.clear();
                }
            }
        }
    }

    bool share_once_timer::start(uint32_t tm_period)
    {
        bool bRet = false;
        if (inner::raw_timer::start(tm_period))
        {
            m_bQuit = false;
            m_scheduler = new boost::thread(boost::bind(&share_once_timer::dispatchfunc, this));
            bRet = true;
        }
        return bRet;
    }

    void share_once_timer::stop()
    {
        if (m_bQuit) return;
        m_bQuit = true;
        if (m_scheduler)
        {
            m_schedule_event.notify_one();
            m_scheduler->join();
            delete m_scheduler;
            m_scheduler = 0;
        }

        inner::raw_timer::stop();

        //清空所有残存定时项
        //clear timeEvents
        std::map<time_event_id, uint32_t>::iterator it;
        for (it = m_time_events.begin(); it != m_time_events.end(); it++)
        {
            onDispatchEvent((*it).first, true);
        }
        m_time_events.clear();
    }

    bool share_once_timer::make_time_event(time_event_id tid, uint32_t delayMS, bool &newEvent)
    {
        bool bRet = false;
        newEvent = false;
        boost::mutex::scoped_lock lock(m_time_events_mutex);
        do
        {
            if (!runable()) break;

            //增加定时事件
            //1,如果无此定时事件，则增加定时项
            //2,如果存在定时事件，则视现有定时长度确定是否修改
            std::map<time_event_id, uint32_t>::iterator it = m_time_events.find(tid);
            if (it == m_time_events.end())
            {
                //撤销失败，已撤销
                if (0 == delayMS) break;

                //增加定时项
                newEvent = true;
                m_time_events[tid] = delayMS;
            }
            else
            {
                //撤销定时项
                if(0 == delayMS)
                {
                    m_time_events.erase(it);
                }
                //修改定时项
                else
                {
                    uint32_t current_delayMS = m_time_events[tid];
                    if (current_delayMS < delayMS)
                        m_time_events[tid] = delayMS;
                }
            }
            bRet = true;
        } while (false);
        return bRet;
    }
}

