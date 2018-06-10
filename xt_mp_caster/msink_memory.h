///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：msink_memory.h
// 创 建 者：汤戈
// 创建时间：2012年03月26日
// 内容描述：媒体数据宿 -- 内存接口型
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MSINK_MEMORY_ENTITY_
#define MSINK_MEMORY_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "xt_mp_caster_def.h"

namespace xt_mp_caster
{
    /*
    具备一个fifo和两类回调函数接口的实体类
    1、open/close成对调用确保实体有效，其中基类m_bReady变量标识open/close调用状态
    2、active/recycle_alloc_event/recycle_release_event引发m_active的状态变化
    3、pump_frame_in/pump_frame_out是数据的写入和读取接口，并受控于m_active状态
    4、数据一律采用rtp_block方式写入，回调输出包含rtp包头的数据信息
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

        bool pump_rtp_in(rtp_block *rtp);				//rtp数据包写入
        void pump_rtp_out(mp * hmp);					//rtp数据包推出至回调函数，直到数据为空

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