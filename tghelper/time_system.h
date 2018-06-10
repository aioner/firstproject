///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：time_system.h
// 创 建 者：汤戈
// 创建时间：2012年03月25日
// 内容描述：定时器组件
//
// 1、所有接口类均为线程安全
// 2、采用boost库中的mutex对象实现线程同步
// 3、支持windows/linux跨平台
//
// 修订日志
// [2012-03-25]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef TIME_SYSTEM_
#define TIME_SYSTEM_

#include<stdint.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>


#ifdef _WIN32
#include <iostream>
#include <windows.h>
#include <Mmsystem.h>
#define INVALID_TIMER_ID		0
#else //linux
#include <time.h>
#include <signal.h>
#define INVALID_TIMER_ID		timer_t( -1 )
#endif

//系统定时器精度10ms
#define SYSTEM_CORE_TIME_PERIOD		10
#define SYSTEM_CORE_TIMER_RES		10


namespace tghelper
{
    namespace inner
    {
        class raw_timer : private boost::noncopyable
        {
        public:
            raw_timer();
            virtual ~raw_timer()
            {	stop();	}
        public:
            virtual bool start(uint32_t tm_period = SYSTEM_CORE_TIME_PERIOD);
            virtual void stop();
            bool started();
#ifdef _WIN32
            inline uint32_t id() { return m_id; }
#else
            inline timer_t id() { return m_id; }
#endif
        protected:
            //由子类继承修订该功能
            virtual void onTimeEvent() {	}

        protected:
            uint32_t m_tm_period;

#ifdef _WIN32
        private:
            static  void CALLBACK timeout(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
           //static void CALLBACK timeout(unsigned int id, unsigned int, unsigned long context, unsigned long, unsigned long);

        private:
            unsigned int m_id;
#else
        private:
            static void timeout( sigval_t t );

        private:
            timer_t m_id;
            bool m_started;

#endif
        };
    }

    //共享定时器，产生各种runonce定时事件，
    //共享一个raw_timer资源，
    //输出事件在内部独立线程中运行
    typedef void * time_event_id;
    class share_once_timer : public inner::raw_timer
    {
    public:
        share_once_timer();
        virtual ~share_once_timer();

    private:
        virtual void onTimeEvent();
        void dispatchfunc();

    protected:
        //由子类决定最终定时事件派送情况，重载此方法
        //bFastRelease表示是否为系统退出时快速资源回收处理
        virtual void onDispatchEvent(time_event_id tid, bool bFastRelease) {	}

    public:
        virtual bool start(uint32_t tm_period = SYSTEM_CORE_TIME_PERIOD);
        virtual void stop();

        //定时器用户通过此函数申请、撤销和修改触发定时时长
        //1、newEvent为返回值，通知用户该定时器事件是新创建
        //2、delayMS=0，表示撤销指定定时器
        //3、tid为用户设定的tid值，由用户保证其唯一性，这种设计和传统的定时器不太一样
        bool make_time_event(time_event_id tid, uint32_t delayMS, bool &newEvent);


        inline bool runable() { return !m_bQuit; }
    private:
        bool m_bQuit;

        //调度事件
        boost::mutex m_schedule_mutex;
        boost::condition_variable m_schedule_event;

        boost::thread * m_scheduler;
    protected:
        boost::mutex m_time_events_mutex;
        std::map<time_event_id, uint32_t> m_time_events;
    };


}

#endif

