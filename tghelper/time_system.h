///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����time_system.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��25��
// ������������ʱ�����
//
// 1�����нӿ����Ϊ�̰߳�ȫ
// 2������boost���е�mutex����ʵ���߳�ͬ��
// 3��֧��windows/linux��ƽ̨
//
// �޶���־
// [2012-03-25]		���������汾
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

//ϵͳ��ʱ������10ms
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
            //������̳��޶��ù���
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

    //����ʱ������������runonce��ʱ�¼���
    //����һ��raw_timer��Դ��
    //����¼����ڲ������߳�������
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
        //������������ն�ʱ�¼�������������ش˷���
        //bFastRelease��ʾ�Ƿ�Ϊϵͳ�˳�ʱ������Դ���մ���
        virtual void onDispatchEvent(time_event_id tid, bool bFastRelease) {	}

    public:
        virtual bool start(uint32_t tm_period = SYSTEM_CORE_TIME_PERIOD);
        virtual void stop();

        //��ʱ���û�ͨ���˺������롢�������޸Ĵ�����ʱʱ��
        //1��newEventΪ����ֵ��֪ͨ�û��ö�ʱ���¼����´���
        //2��delayMS=0����ʾ����ָ����ʱ��
        //3��tidΪ�û��趨��tidֵ�����û���֤��Ψһ�ԣ�������ƺʹ�ͳ�Ķ�ʱ����̫һ��
        bool make_time_event(time_event_id tid, uint32_t delayMS, bool &newEvent);


        inline bool runable() { return !m_bQuit; }
    private:
        bool m_bQuit;

        //�����¼�
        boost::mutex m_schedule_mutex;
        boost::condition_variable m_schedule_event;

        boost::thread * m_scheduler;
    protected:
        boost::mutex m_time_events_mutex;
        std::map<time_event_id, uint32_t> m_time_events;
    };


}

#endif

