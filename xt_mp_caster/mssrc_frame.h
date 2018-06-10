///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：mssrc_frame.h
// 创 建 者：汤戈
// 创建时间：2012年03月26日
// 内容描述：媒体数据源 -- 帧类型
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
    具备一个fifo的实体类
    1、open/close成对调用确保实体有效，其中基类m_bReady变量标识open/close调用状态
    2、active/recycle_alloc_event/recycle_release_event引发m_active的状态变化
    3、pump_frame_in/pump_frame_out是数据的写入和读取接口，并受控于m_active状态
    */
    class mssrc_frame : public mssrc
    {
    public:
        mssrc_frame();
        virtual ~mssrc_frame();

    public:
        bool open(bool bActive);						//创建
        void close();									//关闭
        bool active(bool bActive);						//激活
        inline bool isAcitve() { return m_active; }

        bool pump_frame_in(rtp_mblock *mrtp);			//分包数据帧写入
        rtp_mblock *pump_frame_out();					//分包数据帧读取
        //配额提取方式RTP数据包，返回实际提取数量
		uint32_t pump_frames_out(msink_rv_rtp * hmsink,rtp_mblock *mrtp, uint32_t canSendSize);

    protected:
        //对象池的分配和去配过程仅对mssrc的fifo写入读出的m_active准入方式进行控制
        virtual void recycle_alloc_event();				
        virtual void recycle_release_event();

    public:
        tghelper::recycle_queue m_fifo;
        bool m_active;
    };
}

#endif

