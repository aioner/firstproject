///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mp.h
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：媒体数据处理单元
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MP_ENTITY_
#define MP_ENTITY_

#include "mp_caster_config.h"
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <tghelper/recycle_pool.h>
#include <tghelper/recycle_pools.h>
#include <tghelper/byte_pool.h>
#include "xt_mp_caster_def.h"

namespace xt_mp_caster
{
    typedef enum EMP_OBJECT_ID_
    {
        EMP_OBJ_NONE = 0,

        EMP_OBJ_MP = 1000,
        EMP_OBJ_MP_END = 1999,

        EMP_OBJ_SSRC = 2000,
        EMP_OBJ_SSRC_END = 2999,

        EMP_OBJ_SINK = 3000,
        EMP_OBJ_SINK_END = 3999,

        EMP_OBJ_RTCP_SR = 5000,
        EMP_OBJ_RTCP_RR = 5001,
    } EMP_OBJECT_ID;

    class mp_object : public tghelper::recycle_pool_item
    {
    protected:
        mp_object() : m_obj_id(EMP_OBJ_NONE), m_manager(0), m_bReady(false)
        {		}
        virtual ~mp_object()
        {		}

        virtual void set_object_id(EMP_OBJECT_ID obj_id)
        { if (EMP_OBJ_NONE != obj_id) m_obj_id = obj_id; }


    public:
        inline EMP_OBJECT_ID get_object_id()
        { return m_obj_id;	}

        static bool validate(EMP_OBJECT_ID object_id)
        {
            bool bRet = false;
            if (((EMP_OBJ_MP < object_id) && (object_id < EMP_OBJ_MP_END)) ||
                ((EMP_OBJ_SSRC < object_id) && (object_id < EMP_OBJ_SSRC_END)) ||
                ((EMP_OBJ_SINK < object_id) && (object_id < EMP_OBJ_SINK_END)) ||
                (EMP_OBJ_RTCP_SR == object_id) || (EMP_OBJ_RTCP_RR == object_id))
                bRet = true;
            return bRet;
        }

        inline bool isMP()
        {
            return ((EMP_OBJ_MP < m_obj_id) && (m_obj_id < EMP_OBJ_MP_END));
        }

        inline bool isMSSRC()
        {
            return ((EMP_OBJ_SSRC < m_obj_id) && (m_obj_id < EMP_OBJ_SSRC_END));
        }

        inline bool isMSINK()
        {
            return ((EMP_OBJ_SINK < m_obj_id) && (m_obj_id < EMP_OBJ_SINK_END));
        }
    protected:
        EMP_OBJECT_ID m_obj_id;
        mp_object * m_manager;
        bool m_bReady;
    };

    class mssrc : public mp_object
    {
    protected:
        mssrc()
        { mp_object::set_object_id(EMP_OBJ_SSRC);	}
        virtual ~mssrc()
        {		}

        virtual void set_object_id(EMP_OBJECT_ID obj_id)
        {
            if ((EMP_OBJ_SSRC < obj_id) && (obj_id < EMP_OBJ_SSRC_END))
            { mp_object::set_object_id(obj_id);	}
        }
    public:
        inline mssrc_type get_type()
        { return (mssrc_type)(get_object_id() - EMP_OBJ_SSRC); }
    };

    class msink : public mp_object
    {
    protected:
        msink()
        { mp_object::set_object_id(EMP_OBJ_SINK);	}
        virtual ~msink()
        {		}
        virtual void set_object_id(EMP_OBJECT_ID obj_id)
        {
            if ((EMP_OBJ_SINK < obj_id) && (obj_id < EMP_OBJ_SINK_END))
            { mp_object::set_object_id(obj_id);	}
        }
    public:
        inline msink_type get_type()
        { return (msink_type)(get_object_id() - EMP_OBJ_SINK); }

        //子类如果带任务行为，可重载该函数，方便挂入调度引擎
        virtual bool do_task() { return false; }
    };

    class forward_table
    {
    public:
        forward_table()
        {

        }
        ~forward_table()
        {

        }

    public:
        std::map<uint32_t, mssrc *> m_mssrcs;
        std::map<msink *, mssrc *> m_forward_table;
    };

    //rtp包头描述数据类型
    //	外部采用rv_adapter的rv_rtp_unpack()函数从rtp package中提取m_rtp_param
    //	外部调用rv_adapter的write_rtp()函数时，会利用m_rtp_param发送数据
    //rtp宏数据块，用于描述帧型数据块的共性rtp包头信息（payload, ts, mark等等）
    class rtp_mblock : public tghelper::byte_macro_block
    {
    protected:
        virtual void recycle_alloc_event()
        {
            m_bFrameInfo = false;
            memset(&m_infoFrame, 0, sizeof(XTFrameInfo));
            memset(&m_rtp_param, 0, sizeof(rv_rtp_param));
        }

    public:
        void set_rtp_param(rv_rtp_param *rtp_param)
        { memcpy(&m_rtp_param, rtp_param, sizeof(rv_rtp_param)); }
    public:
        rv_rtp_param m_rtp_param;

        bool m_use_ssrc;
        uint32_t m_ssrc;

		bool m_bFrameInfo;
		XTFrameInfo m_infoFrame;
		uint8_t m_priority;
	};
	//rtp数据块，
	class rtp_block : public tghelper::byte_block
	{
	public:
		rtp_block(const uint32_t block_size) : tghelper::byte_block(block_size), m_bind_block(0), m_resend(false)
		{		}
	protected:
		virtual void recycle_alloc_event()
		{
			m_bFrameInfo = false;
			memset(&m_infoFrame, 0, sizeof(XTFrameInfo));
			memset(&m_rtp_param, 0, sizeof(rv_rtp_param));
            m_priority = 0;
            m_resend = false;
            m_use_ssrc = false;
        }
        virtual void recycle_release_event()
        {
            if (m_bind_block)
            {
                m_bind_block->release();
                m_bind_block = 0;
            }
        }

    public:
        void set_rtp_param(rv_rtp_param *rtp_param)
        { memcpy(&m_rtp_param, rtp_param, sizeof(rv_rtp_param)); }

		void ser_rtp_prority(uint8_t prority)
		{
			m_priority = prority;
		}
        void set_bind_block(byte_block *bind_block)
        {
            if (m_bind_block)
            {
                m_bind_block->release();
            }
            m_bind_block = bind_block;
            if (m_bind_block)
            {
                m_bind_block->assign();
            }
        }
        inline byte_block *get_bind_block() { return m_bind_block; }
    public:
        rv_rtp_param m_rtp_param;

        bool m_use_ssrc;

        uint32_t m_ssrc;

        bool m_bFrameInfo;
        XTFrameInfo m_infoFrame;

		uint8_t m_priority;

		bool m_resend;

        uint32_t m_exHead[16];
    private:
        byte_block * m_bind_block;		//外挂用户block
    };

    namespace inner
    {
        typedef tghelper::any_byte_pool<
            rtp_block,
            MP_BLOCK_POOL_SIZE,
            MP_BLOCK_POOL_EXPAND,
            MP_BLOCK_POOL_DEFAULT> rtp_pool;
    }


    class mp : public mp_object
    {
        friend class caster;
        //阻止用户直接分配基类
    protected:
        mp() :
             m_rtp_pool(),
                 m_mrtp_pool()
             { mp_object::set_object_id(EMP_OBJ_MP);	}
             virtual ~mp()
             {		}
             virtual void set_object_id(EMP_OBJECT_ID obj_id)
             {
                 if ((EMP_OBJ_MP < obj_id) && (obj_id < EMP_OBJ_MP_END))
                 { mp_object::set_object_id(obj_id);	}
             }
             //原型设计备注函数，不产生实际作用，仅为说明mp内部资源锁控方式
    private:
        inline bool dummy_inner_work()
        {
            boost::shared_lock<boost::shared_mutex> lock(m_resource_locker);
            return false;
        }

        inline bool dummy_outer_work()
        {
            boost::unique_lock<boost::shared_mutex> lock(m_resource_locker);
            return false;
        }
    public:
        inline mp_type get_type()
        { return (mp_type)(get_object_id() - EMP_OBJ_MP); }

        virtual bool active(bool bActive) { return false; }

        //更新丢包重传开关 add by songlei 20150708
        virtual void update_resend_flag(const int resend_flag){};

        //执行mssrc任务动作
        //1，bwShaper = false任务由外部用户通过pump释放
        //2, bwShaper = true 任务由定时器释放
        virtual bool do_mssrc_task(uint32_t &delayMS){ return dummy_inner_work(); }
        virtual bool do_msink_task(){ return dummy_inner_work(); }
		virtual bool pump_rtp_in(MP_IN mssrc *hmssrc,MP_IN void *rtp){return dummy_inner_work();};
        //拷贝复制模式，函数执行分包动作
        virtual bool pump_frame_in(
            MP_IN mssrc * hmssrc,			//目标mssrc句柄
            MP_IN uint8_t *frame,			//数据帧缓冲区
            MP_IN uint32_t framesize,		//数据帧长度
            MP_IN bool	   frameTS_opt,		//数据帧时间戳外部设置有效性，false为内部计算时间戳(mp_caster_config.h)
            MP_IN uint32_t frameTS,			//数据帧时间戳 == 0表示内部计算时间戳
            MP_IN uint8_t  framePayload,	//数据帧伪装媒体类型 0xFF表示用缺省配置(mp_caster_config.h)
            MP_IN XTFrameInfo &info,
            MP_IN uint8_t priority,
            MP_IN bool is_std_data,
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc)
        { return dummy_inner_work(); }

#if 0
        //内部资源操作函数有子类具体实现，不同的子类会有不同的应用规则
    protected:
        virtual bool add_mssrc(mssrc * hmssrc) 	{ return dummy_outer_work(); }
        virtual bool del_mssrc(mssrc * hmssrc)	{ return dummy_outer_work(); }
        virtual void clear_mssrc() 	{ return dummy_outer_work(); }

        virtual bool add_msink(msink * hmsink) 	{ return dummy_outer_work(); }
        virtual bool del_msink(msink * hmsink)	{ return dummy_outer_work(); }
        virtual void clear_msink() 	{ return dummy_outer_work(); }
#endif
        //下列资源函数内部不资源锁，锁控交由子类实现
    protected:
        inline void share_lock()   { m_resource_locker.lock_shared(); }
        inline void share_unlock() { m_resource_locker.unlock_shared(); }
        inline void unique_lock()   { m_resource_locker.lock(); }
        inline void unique_unlock() { m_resource_locker.unlock(); }

        virtual bool add_mssrc(mssrc * hmssrc);
        virtual bool del_mssrc(mssrc * hmssrc);
        virtual void clear_mssrc();

        virtual bool add_msink(msink * hmsink);
        virtual bool del_msink(msink * hmsink);
        virtual void clear_msink();


    protected:
        boost::shared_mutex m_resource_locker;		//内部调度用共享锁保护m_mssrcs和m_msinks，外部用排他锁修改
        inner::rtp_pool m_rtp_pool;				//内存池，管理由拷贝复制产生的rtp_block
        tghelper::recycle_pool m_mrtp_pool;			//内存池，管理由帧分包，拷贝复制产生的mrtp_block

        //流水线串行运行锁，仅对需要进行串行化的MP中使用
        boost::mutex m_pump_in_task_mutex;
        boost::timed_mutex m_mssrc_task_mutex;
        boost::timed_mutex m_msink_task_mutex;

        forward_table m_relation;
        std::list<mssrc *> m_mssrcs;
        std::list<msink *> m_msinks;
    };


    class mp_pools
    {
    public:
        mp_pools()
        {
            buildObjectPools();
        }

        ~mp_pools()
        {

        }

        mp_object *forceAllocObject(EMP_OBJECT_ID object_id);
        void buildObjectPools();

    private:
        tghelper::recycle_pools m_objpools;
    };

    class rtcp_sr : public mp_object
    {
    public:
        rtcp_sr()
        { mp_object::set_object_id(EMP_OBJ_RTCP_SR);	}
        virtual ~rtcp_sr()
        {		}

    public:
        void write_srinfo(uint32_t ssrc, const rv_rtcp_srinfo &srinfo);
        void read_srinfo(uint32_t &ssrc, rv_rtcp_srinfo &srinfo);
        void read_rtcp_send_report(rtcp_send_report &sr);
        void read_rtcp_send_report(rtcp_send_report *sr);
    protected:
        virtual void recycle_alloc_event();

    private:
        rv_rtcp_srinfo m_srinfo;
        uint32_t	m_ssrc;
    };

    class rtcp_rr : public mp_object
    {
    public:
        rtcp_rr()
        { mp_object::set_object_id(EMP_OBJ_RTCP_RR);	}
        virtual ~rtcp_rr()
        {		}

    public:
        void write_rrinfo(uint32_t ssrc, const rv_rtcp_rrinfo &rrinfo);
        void read_rrinfo(uint32_t &ssrc, rv_rtcp_rrinfo &rrinfo);
        void read_rtcp_receive_report(rtcp_receive_report &rr);
        void read_rtcp_receive_report(rtcp_receive_report *rr);
    protected:
        virtual void recycle_alloc_event();

    private:
        rv_rtcp_rrinfo m_rrinfo;
        uint32_t	m_ssrc;
    };


}

#endif
