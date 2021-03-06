///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：bc_mp.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：媒体数据处理单元 -- 广播型内置msink_rv_rtp
///////////////////////////////////////////////////////////////////////////////////////////
#include "bc_mp.h"
#include "mssrc_rtp.h"
#include "mssrc_frame.h"
#include "msink_rtp.h"
#include "msink_rv_rtp.h"
#include "msink_memory.h"
#include "mp_caster.h"
#include <../rv_adapter/rv_api.h>
#include "mp_caster.h"
#include "share_type_def.h"
#include "mp_28181_ps.h"

#ifdef _ANDROID
#include "xt_config_cxx.h"
#endif

long nRTP_MAX_SIZE = MP_PSEUDO_RTP_MAX_SIZE;

#define NXT_SYSTEM	175			//外系统对接类型
#define XT_VGA		172			//XTVGA数据类型

//数据块头
#define CHUNK_HEADER_FOURCC 'HUHC'

#define LEN_XTHEAD		24			//私有头长度

#define H264HEADERSIZE          2            /* H264 header size              */
#define RTP_HEVC_HEADERS_SIZE 3	 /* H265 header size              */
#define RTP_SEND_BUF_SIZE 4*1024

//流的模式:0为标准RTP模式，1为PS流模式
#define MP_STD_MODE     0
#define MP_PS_MODE      1

extern void writeLog(int nLevel, const char* szLogName, const char* szLogFmt, ...);

#ifndef _OS_WINDOWS
#include <sys/time.h>
inline unsigned long  GetTickCount()
{
    struct timeval tv;
    gettimeofday(&tv,0);
    return tv.tv_sec*1000+tv.tv_usec/1000;
}
#endif

int g_task_size = 1024;
int g_ssrc_num = 1024;
int g_sink_num = 1024;
namespace xt_mp_caster
{
    raddr_cb bc_mp::m_raddr_cb = NULL;
    ///////////////////////////////////////////////////////////////////////////////////////////
    // bc_mp
    bc_mp::bc_mp() :
        m_active(false),
        m_last_pesudo_ts(0),
        m_last_pesudo_rtp_ts(0),
        m_last_frame_in_rtp_sn(0)
#ifdef _USE_RTP_SEND_CONTROLLER
        ,m_bitrate_controller()
        ,m_network_changed_callback(NULL)
        ,m_network_changed_callback_ctx(NULL)
#endif
    {
        set_object_id((EMP_OBJECT_ID)(EMP_OBJ_MP + BROADCAST_MP));
        m_bReady = false;
    }
    bc_mp::~bc_mp()
    {
        close();
    }

    void bc_mp::recycle_alloc_event()
    {
        m_active = false;
        m_bReady = false;
        m_last_pesudo_ts = 0;
    }

    void bc_mp::recycle_release_event()
    {
        close();
    }


    bool bc_mp::open(bc_mp_descriptor * descriptor, mssrc_h hmssrc, msink_h hmsink, uint32_t *multID)
    {
#ifdef _ANDROID
        nRTP_MAX_SIZE = xt_config::router_module::get<int>("config.caster_cfg.MaxSize", MP_PSEUDO_RTP_MAX_SIZE);
#else
        nRTP_MAX_SIZE = config::_()->MaxSize(MP_PSEUDO_RTP_MAX_SIZE);
#endif
        nRTP_MAX_SIZE = nRTP_MAX_SIZE>256 ? nRTP_MAX_SIZE:1400;

#ifdef _USE_RTP_SEND_CONTROLLER
#ifdef _ANDROID
        uint32_t start_bitrate = xt_config::router_module::get<uint32_t>("config.caster_cfg.bitrate_controller.start_bitrate", 8 * 1024 * 1024 * 8);
        __android_log_print(ANDROID_LOG_ERROR, "xt_config::get", "config.caster_cfg.bitrate_controller.start_bitrate(%u)", start_bitrate);

        uint32_t min_bitrate = xt_config::router_module::get<uint32_t>("config.caster_cfg.bitrate_controller.min_bitrate", 8 * 1024 * 8);
        __android_log_print(ANDROID_LOG_ERROR, "xt_config::get", "config.caster_cfg.bitrate_controller.min_bitrate(%u)", min_bitrate);

        uint32_t max_bitrate = xt_config::router_module::get<uint32_t>("config.caster_cfg.bitrate_controller.max_bitrate", 8 * 1024 * 1024 * 8);
        __android_log_print(ANDROID_LOG_ERROR, "xt_config::get", "config.caster_cfg.bitrate_controller.max_bitrate(%u)", max_bitrate);

        uint32_t max_rtt_thr = xt_config::router_module::get<uint32_t>("config.caster_cfg.bitrate_controller.rtt_thr", 1500);
        __android_log_print(ANDROID_LOG_ERROR, "xt_config::get", "config.caster_cfg.bitrate_controller.rtt_thr(%u)", max_rtt_thr);

        uint32_t max_rtcp_priod_thr = xt_config::router_module::get<uint32_t>("config.caster_cfg.bitrate_controller.rtcp_expires", 5500);
        __android_log_print(ANDROID_LOG_ERROR, "xt_config::get", "config.caster_cfg.bitrate_controller.rtcp_expires(%u)", max_rtcp_priod_thr);
#else
        uint32_t start_bitrate = 4 * 8 * 1024 * 1024;
        uint32_t min_bitrate = config::_()->min_bitrate(1024 * 1024);
        uint32_t max_bitrate = config::_()->max_bitrate(4 * 8 * 1024 * 1024);
        uint32_t max_rtt_thr = config::_()->max_rtt_thr(2500);
        uint32_t max_rtcp_priod_thr = config::_()->max_rtcp_priod_thr(5);
#endif

        m_bitrate_controller.reset(new bitrate_controller_t(start_bitrate, min_bitrate, max_bitrate, max_rtt_thr, max_rtcp_priod_thr));
        m_bitrate_controller->add_bitrate_observer(this);
#endif

        boost::unique_lock<boost::shared_mutex> lock(m_resource_locker);
        bool bRet = false;
        do
        {
            if (!descriptor) break;
            if (m_bReady) break;
            if (META_MSSRC == descriptor->ssrc_type) break;

            m_last_pesudo_ts = 0;
            m_last_pesudo_rtp_ts = 0;

            //内置msink_rv_rtp
            bool msink_build_status = false;
            msink_rv_rtp *sink =
                static_cast<msink_rv_rtp *>(caster::self()->m_objpools.forceAllocObject(
                (EMP_OBJECT_ID)(EMP_OBJ_SINK + RV_RTP_MSINK)));
            if (sink)
            {
                //sink->open存在打开错误的风险，由于地址设置错误引发
                if (sink->open(descriptor->local_address,
                    (MP_TRUE == descriptor->manual_rtcp),
                    (MP_TRUE == descriptor->active_now),
                    descriptor->msink_multicast_rtp_ttl,
                    descriptor->msink_multicast_rtcp_ttl,
                    descriptor->multiplex,
                    multID
#ifdef _USE_RTP_SEND_CONTROLLER
                    ,(NULL == m_bitrate_controller.get()) ? NULL : m_bitrate_controller->create_rtcp_observer()
#endif
                    ))
                {
                    sink->m_mp = this;
                    msink_build_status = true;
                    add_msink(sink);
                    if (hmsink) hmsink->hmsink = sink;
                }
                else
                {
                    sink->release();
                    break;
                }
            }

            //内置mssrc
            bool mssrc_build_status = false;
            switch(descriptor->ssrc_type)
            {
            case FRAME_MSSRC:
                {
                    mssrc_frame *ssrc =
                        static_cast<mssrc_frame *>(caster::self()->m_objpools.forceAllocObject(
                        (EMP_OBJECT_ID)(EMP_OBJ_SSRC + FRAME_MSSRC)));
                    if (ssrc)
                    {
                        mssrc_build_status = true;
                        ssrc->open(MP_TRUE == descriptor->active_now);
                        add_mssrc(ssrc);
                        if (hmssrc) hmssrc->hmssrc = ssrc;
                    }
                }
                break;
            case RTP_MSSRC:
                {
                    mssrc_rtp *ssrc =
                        static_cast<mssrc_rtp *>(caster::self()->m_objpools.forceAllocObject(
                        (EMP_OBJECT_ID)(EMP_OBJ_SSRC + RTP_MSSRC)));
                    if (ssrc)
                    {
                        mssrc_build_status = true;
                        ssrc->open(MP_TRUE == descriptor->active_now);
                        add_mssrc(ssrc);
                        if (hmssrc) hmssrc->hmssrc = ssrc;
                    }
                }
                break;
            }
            m_bReady = true;
            m_active = (MP_TRUE == descriptor->active_now);
            bRet = true;
        } while (false);

        return bRet;
    }

    //更新丢包重传开关 add by songlei 20150708
    void bc_mp::update_resend_flag(const int resend_flag)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_resource_locker);

        std::list<msink *>::iterator itr = m_msinks.begin();
        for (;m_msinks.end() != itr; itr++)
        {
            switch((*itr)->get_type())
            {
            case MEMORY_MSINK:
                break;
            case RV_RTP_MSINK:
                {
                    msink_rv_rtp * sink_rv_rtp = static_cast<msink_rv_rtp *>(*itr);
                    if (sink_rv_rtp)
                    {
                        sink_rv_rtp->update_resend_flag(resend_flag);
                    }
                    break;
                }

            default:
                break;
            }
        }
    }
    void bc_mp::close()
    {
        boost::unique_lock<boost::shared_mutex> lock(m_resource_locker);
        if (!m_bReady) return;
        m_bReady = false;
        m_active = false;
        m_last_pesudo_ts = 0;
        m_last_pesudo_rtp_ts = 0;

        //释放全部的mssrc和msink
        clear_mssrc();
        clear_msink();
    }
    bool bc_mp::active(bool bActive)
    {
        boost::unique_lock<boost::shared_mutex> lock(m_resource_locker);
        if (!m_bReady) return false;
        m_active = bActive;
        //遍历mssrc和msink
        std::list<mssrc *>::iterator it1;
        for (it1 = m_mssrcs.begin(); it1 != m_mssrcs.end(); it1++)
        {
            switch((*it1)->get_type())
            {
            case FRAME_MSSRC:
                static_cast<mssrc_frame *>(*it1)->active(bActive);
                break;
            case RTP_MSSRC:
                static_cast<mssrc_rtp *>(*it1)->active(bActive);
                break;
            }
        }
        std::list<msink *>::iterator it2;
        for (it2 = m_msinks.begin(); it2 != m_msinks.end(); it2++)
        {
            switch((*it2)->get_type())
            {
            case MEMORY_MSINK:
                static_cast<msink_memory *>(*it2)->active(bActive);
                break;
            case RV_RTP_MSINK:
                static_cast<msink_rv_rtp *>(*it2)->active(bActive);
                break;
            }
        }

        return true;
    }

    //资源管理 -- 排他锁由bc_mp的管理类调用
    bool bc_mp::add_rtp_sink(rtp_sink_descriptor* descriptor, msink_h hsink)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) return false;

            msink_rtp *sink =
                static_cast<msink_rtp *>(caster::self()->m_objpools.forceAllocObject(
                (EMP_OBJECT_ID)(EMP_OBJ_SINK + RTP_MSINK)));
            if (!sink) break;
            if (!sink->open(&static_cast<msink_rv_rtp *>(m_msinks.front())->m_hrv,
                descriptor
#ifdef _USE_RTP_SEND_CONTROLLER
                ,m_bitrate_controller.get()
#endif
                ))
            {
                sink->release();
                break;
            }
            add_msink(sink);
            static_cast<msink_rv_rtp *>(m_msinks.front())->inc_viewer();
            bRet = true;
            hsink->hmsink = sink;
        } while (false);

        return bRet;
    }

	uint16_t bc_mp::get_sink_sn()
	{
		return m_last_frame_in_rtp_sn;
	}

    bool bc_mp::del_msink(msink * hmsink)
    {
        bool bRet = mp::del_msink(hmsink);
        if (bRet)
        {
            static_cast<msink_rv_rtp *>(m_msinks.front())->dec_viewer();
        }
        return bRet;
    }
    //数据引擎实现
    bool bc_mp::do_mssrc_task(uint32_t &delayMS)
    {
        bool bRet = false;
        //注意加共享锁
        //任务描述:
        //1、根据m_max_bandwidth设定判断是否时间就绪，mrtp
        //	 0 == m_max_bandwidth表示不做时间准入的带宽整形处理
        //2、提取m_mssrcs.front()节点中的rtp数据，投送至m_msinks.front()
        //3、投送成功返回true，要求引擎引发下级do_msink_task()流水任务
        //   投送失败则返回false，引擎将不做任何处理，除非delayMS > 0
        //4、delayMS > 0表示要求引擎申请一个延时任务
        boost::shared_lock<boost::shared_mutex> lock(m_resource_locker);

        if (!m_mssrc_task_mutex.timed_lock(
            boost::get_system_time() +
            boost::posix_time::millisec(MP_MSSRC_TASK_LOCK_TM)))
        {
            return bRet;
        }

        do
        {
            if (!m_bReady) break;
            if (!m_active) break;
            delayMS = 0;
            uint32_t canSendSize = 0xFFFFFFFF;

            //数据搬移任务
            switch(m_mssrcs.front()->get_type())
            {
            case FRAME_MSSRC:
                {
                    //帧数据输出，带宽整形控制对成帧数据进行
                    delayMS = do_mssrc_task_frame(
                        static_cast<mssrc_frame *>(m_mssrcs.front()),
                        static_cast<msink_rv_rtp *>(m_msinks.front()),
                        canSendSize);
                }
                break;
            case RTP_MSSRC:
                {
                    //RTP数据输出，带宽整形控制对RTP包数据进行
                    delayMS = do_mssrc_task_rtp(
                        static_cast<mssrc_rtp *>(m_mssrcs.front()),
                        static_cast<msink_rv_rtp *>(m_msinks.front()),
                        canSendSize);
                }
                break;
            }

            bRet = true;
        } while (false);

        m_mssrc_task_mutex.unlock();

        return bRet;
    }
	//从mrtp队列中以帧为单位抛出数据
    uint32_t bc_mp::do_mssrc_task_frame(
        mssrc_frame *hmssrc, msink_rv_rtp * hmsink,
        uint32_t canSendSize)
    {
        return hmssrc->pump_frames_out(hmsink, NULL,canSendSize);;
    }
	//从rtp队列中以包为单位抛出数据
    uint32_t bc_mp::do_mssrc_task_rtp(
        mssrc_rtp *hmssrc, msink_rv_rtp * hmsink,
        uint32_t canSendSize)
    {
        return hmssrc->pump_rtps_out(hmsink, canSendSize);;
    }

    bool bc_mp::do_msink_task()
    {
        bool bRet = false;
        //注意加共享锁
        //任务描述:
        //1、驱动m_msinks.front()发送清空内部的缓冲区
        boost::shared_lock<boost::shared_mutex> lock(m_resource_locker);
        if (!m_msink_task_mutex.timed_lock(
            boost::get_system_time() +
            boost::posix_time::millisec(MP_MSINK_TASK_LOCK_TM)))
        {
            return bRet;
        }
        do
        {
            if (!m_bReady) break;
            if (!m_active) break;
            bRet = m_msinks.front()->do_task();
        } while(false);
        m_msink_task_mutex.unlock();
        return bRet;
    }

    //媒体数据推入函数组
	bool bc_mp::pump_rtp_in(MP_IN mssrc *hmssrc,MP_IN void *rtp)
	{
		bool bRet = false;
		do 
		{
			if (!m_bReady) break;
			if (!m_active) break;
			//无人关注，不推送数据
			if (0 == static_cast<msink_rv_rtp *>(m_msinks.front())->get_viewer()) 
			{
				break;
			}
			rtp_block *block = static_cast<rtp_block *>(rtp);
			static_cast<msink_rv_rtp *>(m_msinks.front())->rv_write_rtp(block);
			bRet = true;
		} while (0);
		return bRet;
	}
    //1、根据目前待广播数量决定是否进行分发。
    //拷贝复制模式，函数执行分包动作
    bool bc_mp::pump_frame_in(
        MP_IN mssrc * hmssrc,			//目标mssrc句柄
        MP_IN uint8_t *frame,			//数据帧缓冲区
        MP_IN uint32_t framesize,		//数据帧长度
        MP_IN bool	   frameTS_opt,		//数据帧时间戳外部设置有效性，false为内部计算时间戳(mp_caster_config.h)
        MP_IN uint32_t frameTS,			//数据帧时间戳外部设定
        MP_IN uint8_t  framePayload,	//数据帧伪装媒体类型 0xFF表示用缺省配置(mp_caster_config.h)
        MP_IN XTFrameInfo &info,
        MP_IN uint8_t priority,          //发送数据优先级
        MP_IN bool is_std_data,
        MP_IN bool use_ssrc,
        MP_IN uint32_t ssrc)
    {
        boost::shared_lock<boost::shared_mutex> lock(m_resource_locker);
        boost::mutex::scoped_lock frame_in_lock(m_pump_in_task_mutex);

        if (is_std_data)
        {
            if (info.datatype==NXT_SYSTEM)
            {
                framePayload = 96;
                info.datatype = XT_VGA;
            }
            else if(framesize > LEN_XTHEAD)//多rtp包帧数据
            {
                SHeader head;
                int len = sizeof(SHeader);
                ::memcpy(&head, frame, len);
                if (info.frametype==OV_AAC)
                {
                    return pump_frame_in_aac(hmssrc, frame+LEN_XTHEAD, framesize-LEN_XTHEAD, head.uTimeStamp, framePayload,priority,use_ssrc,ssrc);
                }
                else if (OV_H265 == info.frametype)
                {	
                    return pump_frame_in_hevc(hmssrc,frame+LEN_XTHEAD, framesize-LEN_XTHEAD, head.uTimeStamp, framePayload,priority,use_ssrc,ssrc);
                }
                else if (OV_H264==info.frametype||is_h264(frame, framesize))
                {
                      return pump_frame_in_s(hmssrc, frame+LEN_XTHEAD, framesize-LEN_XTHEAD, head.uTimeStamp, framePayload,priority, info.streamtype,use_ssrc,ssrc);
                }
                else if (head.uFourCC==CHUNK_HEADER_FOURCC)	
                {
                    frame += LEN_XTHEAD;
                    framesize -= LEN_XTHEAD;

                    frameTS_opt = true;
                    frameTS = head.uTimeStamp;
                }
            }
        }
        else 
        {
            if (info.frametype==OV_AAC)
                info.frametype = OV_AUDIO;
        }
		//单rtp包帧数据
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!m_active) break;
#if (MP_CASTER_PARAM_CHECK)
            if (!hmssrc) break;
            if (!frame) break;
            if (!framesize) break;
            if (FRAME_MSSRC != hmssrc->get_type()) break;
#endif
            //无人关注，不推送数据
            if (0 == static_cast<msink_rv_rtp *>(m_msinks.front())->get_viewer()) 
			{
				break;
			}

            if (framesize > 1024*1024)
            {
                break;
            }

            //缓冲处理_SL2014-9-13
            //////////////////////////////////////////////////////////////////////////////
            if (caster::self()->m_engine->task_size() > g_task_size*1024)
            {
                break;
            }

            if (static_cast<mssrc_frame *>(hmssrc)->m_fifo.size() > g_ssrc_num ||
                static_cast<msink_rv_rtp *>(m_msinks.front())->m_fifo.size() > g_sink_num*1024)
            {
                break;
            }

            //分包处理
            rtp_mblock * mrtp = static_cast<rtp_mblock *>(m_mrtp_pool.alloc_item(false));
            if (!mrtp)
            {
                //尝试分配方式，引起m_mrtp_pool动态扩展
                mrtp = static_cast<rtp_mblock *>(
                    tghelper::recycle_pool_build_item<rtp_mblock>(&m_mrtp_pool, false));

            }

            mrtp->assign();
            uint32_t pack_nums = m_rtp_pool.calc_slice_nums(
                framesize,
                nRTP_MAX_SIZE, //MP_PSEUDO_RTP_MAX_SIZE,
                MP_PSEUDO_RTP_HEAD_SIZE);
            bRet = m_rtp_pool.try_alloc_any_items(mrtp, pack_nums, false);
            if (!bRet)
            {
                mrtp->release();
                break;
            }

            mrtp->m_bFrameInfo = true;
            ::memcpy(&mrtp->m_infoFrame, &info, sizeof(XTFrameInfo));
            bRet = mrtp->write(frame, framesize, nRTP_MAX_SIZE/*MP_PSEUDO_RTP_MAX_SIZE*/, MP_PSEUDO_RTP_HEAD_SIZE);
            if (!bRet)
            {
                mrtp->release();
                break;
            }

            //伪造rtp包头信息
            rv_rtp_param rtp_param;
            ::construct_rv_rtp_param(&rtp_param);
            
            rtp_param.sByte = MP_PSEUDO_RTP_HEAD_SIZE;
            //提供外部sequenceNumber计数
            rtp_param.sequenceNumber = m_last_frame_in_rtp_sn;
            m_last_frame_in_rtp_sn += pack_nums;

            rtp_param.payload = (0xFF == framePayload) ? MP_PSEUDO_PAYLOAD_TYPE : framePayload;
            if (frameTS_opt)
                rtp_param.timestamp = frameTS;
            else
            {
                //合成内部时间戳方式
                if (0 == m_last_pesudo_ts)
                {
                    rtp_param.timestamp = 0;
                    m_last_pesudo_ts = GetTickCount();
                    m_last_pesudo_rtp_ts = 0;
                }
                else
                {
                    uint32_t nowTS = GetTickCount();
                    if (0 == (nowTS - m_last_pesudo_ts))
                        m_last_pesudo_rtp_ts += 1;
                    else
                        m_last_pesudo_rtp_ts += (nowTS - m_last_pesudo_ts) * MP_PSEUDO_TS_CLOCK;
                    rtp_param.timestamp = m_last_pesudo_rtp_ts;
                    m_last_pesudo_ts = nowTS;
                }
            }

			mrtp->set_rtp_param(&rtp_param);
            mrtp->m_priority = priority;
            mrtp->m_use_ssrc = use_ssrc;
            mrtp->m_ssrc = ssrc;
#ifdef USE_POST_TASK
			bRet = static_cast<mssrc_frame *>(hmssrc)->pump_frame_in(mrtp);
#else
			bRet = static_cast<mssrc_frame *>(hmssrc)->pump_frames_out(static_cast<msink_rv_rtp *>(m_msinks.front()),mrtp,0xFFFFFFFF);
#endif

            //mrtp->release();
        } while (false);
        return bRet;
    }

    bool bc_mp::is_h264(uint8_t *frame, uint32_t framesize)
    {
        SHeader head;
        int len = sizeof(SHeader);

        if (!frame || framesize<len)
        {
            return false;
        }

        ::memcpy(&head, frame, len);

        if ( head.uFourCC==CHUNK_HEADER_FOURCC && (head.uMediaType==0||head.uMediaType==65536) )
        {
            return true;
        }

        return false;
    }

	mp_bool bc_mp::pump_frame_in_aac(
		MP_IN mssrc *hmssrc,			//目标mssrc句柄
		MP_IN uint8_t *frame,			//帧数据(去除私有头)
		MP_IN uint32_t framesize,		//数据帧长度
		MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
		MP_IN uint8_t  framePayload,    // 96
        MP_IN uint8_t priority,
        MP_IN bool use_ssrc,
        MP_IN uint32_t ssrc)
	{
		bool bRet = true;

        if (!frame)
        {
            return false;
        }

        if (!m_bReady || !m_active)
        {
            return false;
        }

        //无人关注，不推送数据
        if (0 == static_cast<msink_rv_rtp *>(m_msinks.front())->get_viewer())
        {
            return false;
        }			

        //分包处理
        rtp_mblock *mrtp = static_cast<rtp_mblock *>(m_mrtp_pool.alloc_item(false));
        if (!mrtp)
        {
            //尝试分配方式，引起m_mrtp_pool动态扩展
            mrtp = static_cast<rtp_mblock *>(
                tghelper::recycle_pool_build_item<rtp_mblock>(&m_mrtp_pool, false));

        }	

        mrtp->assign();
        mrtp->m_bFrameInfo = false;

        // 拆包	
        bool ret = frame_unpack_aac(mrtp, frame, framesize, nRTP_MAX_SIZE);
        if (!ret)
        {
            mrtp->release();
            return false;
        }


        int pack_nums = mrtp->get_container().size();

        //伪造rtp包头信息
        rv_rtp_param rtp_param;
        ::construct_rv_rtp_param(&rtp_param);
        
        rtp_param.sByte = MP_PSEUDO_RTP_HEAD_SIZE;

        //提供外部sequenceNumber计数
        rtp_param.sequenceNumber = m_last_frame_in_rtp_sn;
        m_last_frame_in_rtp_sn += pack_nums;
        rtp_param.payload = framePayload;

        //合成内部时间戳方式
        rtp_param.timestamp = frameTS;

		mrtp->set_rtp_param(&rtp_param);
        mrtp->m_priority = priority;
        mrtp->m_use_ssrc = use_ssrc;
        mrtp->m_ssrc = ssrc;
		bRet = static_cast<mssrc_frame *>(hmssrc)->pump_frame_in(mrtp);

        mrtp->release();

        return bRet;
    }

    mp_bool bc_mp::frame_unpack_aac(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu)
    {
        if (!mrtp || !frame)
        {
            return false;
        }

        if (framesize < 5)
        {
            return false;
        }

        uint32_t maxPayload = mtu - MP_PSEUDO_RTP_HEAD_SIZE;
        uint32_t len = framesize;
        uint8_t *data = frame;
        uint8_t off = 0;
        const uint32_t AU_HEADER_SIZE = 4;
        uint8_t packet[4096];
        packet[0] = 0x00;
        packet[1] = 0x10;
        packet[2] = (framesize & 0x1fe0) >> 5;
        packet[3] = (framesize & 0x1f) << 3;

        while (len > maxPayload) 
        {
            rtp_block *rtp = m_rtp_pool.force_alloc_any();
            if (!rtp)
            {
                return false;
            }

            rtp->assign();
            rtp->m_bFrameInfo = false;

            memcpy(packet + AU_HEADER_SIZE, data + off, maxPayload);

            rtp->write(packet, framesize+AU_HEADER_SIZE, MP_PSEUDO_RTP_HEAD_SIZE);

            mrtp->push_byte_block(rtp);
            rtp->release();

            len -= maxPayload;
            off += maxPayload;
        }
        if (len <= maxPayload)
        {
            rtp_block *rtp = m_rtp_pool.force_alloc_any();
            if (!rtp)
            {
                return false;
            }

            rtp->assign();
            rtp->m_bFrameInfo = false;

            memcpy(packet + AU_HEADER_SIZE, data + off, len);

            rtp->write(packet, framesize+AU_HEADER_SIZE, MP_PSEUDO_RTP_HEAD_SIZE);

            mrtp->push_byte_block(rtp);
            rtp->release();
        }

        return true;
    }

	mp_bool bc_mp::pump_frame_in_s(
		MP_IN mssrc *hmssrc,			//目标mssrc句柄
		MP_IN uint8_t *frame,			//帧数据(去除私有头)
		MP_IN uint32_t framesize,		//数据帧长度
		MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
		MP_IN uint8_t  framePayload,    // 96
        MP_IN uint8_t priority,
        MP_IN uint8_t stream_mode,
        MP_IN bool use_ssrc,
        MP_IN uint32_t ssrc)	
	{
		bool bRet = true;

        if (!frame)
        {
            return false;
        }

        if (!m_bReady || !m_active)
        {
            return false;
        }

        //无人关注，不推送数据
        if (0 == static_cast<msink_rv_rtp *>(m_msinks.front())->get_viewer())
        {
            return false;
        }

        //分包处理
        rtp_mblock * mrtp = static_cast<rtp_mblock *>(m_mrtp_pool.alloc_item(false));
        if (!mrtp)
        {
            //尝试分配方式，引起m_mrtp_pool动态扩展
            mrtp = static_cast<rtp_mblock *>(
                tghelper::recycle_pool_build_item<rtp_mblock>(&m_mrtp_pool, false));

        }

        mrtp->assign();
        mrtp->m_bFrameInfo = false;

        bool ret = false;
        if(stream_mode == MP_STD_MODE)
        {
            // 拆包
            ret = frame_unpack(mrtp, frame, framesize, nRTP_MAX_SIZE,frameTS,framePayload);

        }
        else if(stream_mode == MP_PS_MODE)
        {
            // 拆包
            ret = frame_unpack_ps(mrtp, frame, framesize, nRTP_MAX_SIZE, frameTS);
        }

        if (!ret)
        {
            mrtp->release();
            return false;
        }


        int pack_nums = mrtp->get_container().size();

        //伪造rtp包头信息
        rv_rtp_param rtp_param;

        ::construct_rv_rtp_param(&rtp_param);

        rtp_param.sByte = MP_PSEUDO_RTP_HEAD_SIZE;

        //提供外部sequenceNumber计数
        rtp_param.sequenceNumber = m_last_frame_in_rtp_sn;
        m_last_frame_in_rtp_sn += pack_nums;
        rtp_param.payload = framePayload;

        //合成内部时间戳方式
        rtp_param.timestamp = frameTS;

		mrtp->set_rtp_param(&rtp_param);
        mrtp->m_priority = priority;
        mrtp->m_use_ssrc = use_ssrc;
        mrtp->m_ssrc = ssrc;
#ifdef USE_POST_TASK
		//此处对帧进行排序以便发出时有序，多帧可能多线程压入
		bRet = static_cast<mssrc_frame *>(hmssrc)->pump_frame_in(mrtp);
#else
		//直接发出数据试试，不使用线程调度arm下CPU不够用情况
		bRet = static_cast<mssrc_frame *>(hmssrc)->pump_frames_out(static_cast<msink_rv_rtp *>(m_msinks.front()),mrtp,0xFFFFFFFF);
#endif
        //mrtp->release();

        return bRet;
    }

    mp_bool bc_mp::pes_unpack_rtp(rtp_mblock *mrtp, uint8_t *pes_slice, uint32_t slice_size, uint32_t mtu, uint32_t frameTS)
    {
        if (!mrtp || !pes_slice)
        {
            return false;
        }

        if (slice_size == 0)
        {
            return false;
        }

        int nSize = 0;
        int offset = 0;
        uint8_t *buffer = pes_slice;
        int maxPayload = mtu - RTP_HDR_LEN;

        while(slice_size > 0)
        {
            nSize = ((slice_size>maxPayload)?maxPayload:slice_size);

            rtp_block *rtp = m_rtp_pool.force_alloc_any();
            if (!rtp)
            {
                return false;
            }

            rtp->assign();
            rtp->m_bFrameInfo = false;

            rtp->write(buffer+offset, nSize, MP_PSEUDO_RTP_HEAD_SIZE);
            mrtp->push_byte_block(rtp);
            rtp->release();

            offset +=nSize;
            slice_size -= nSize;

        }
        return true;
 
    }

    mp_bool bc_mp::pump_frame_h264_to_ps(		
        MP_IN mssrc *hmssrc,			//目标mssrc句柄
        MP_IN uint8_t *frame,			//帧数据(去除私有头)
        MP_IN uint32_t framesize,		//数据帧长度
        MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
        MP_IN uint8_t  framePayload,    // 96
        MP_IN uint8_t priority,
        MP_IN uint8_t stream_mode,
        MP_IN bool use_ssrc,
        MP_IN uint32_t ssrc)	
    {
        bool bRet = true;

        if (!frame)
        {
            return false;
        }

        if (!m_bReady || !m_active)
        {
            return false;
        }

        //无人关注，不推送数据
        if (0 == static_cast<msink_rv_rtp *>(m_msinks.front())->get_viewer())
        {
            return false;
        }

        //分包处理
        rtp_mblock * mrtp = static_cast<rtp_mblock *>(m_mrtp_pool.alloc_item(false));
        if (!mrtp)
        {
            //尝试分配方式，引起m_mrtp_pool动态扩展
            mrtp = static_cast<rtp_mblock *>(
                tghelper::recycle_pool_build_item<rtp_mblock>(&m_mrtp_pool, false));

        }

        mrtp->assign();
        mrtp->m_bFrameInfo = false;


        static uint32_t maxPayload;
        static int  nSizePos=0;

        static char temp[65536];
        static char Pos = 0;

        uint8_t *first_frame_char = frame;

        //去掉001和0001两种情况
        while (('\0' == *first_frame_char) && (framesize > 0))
        {
            first_frame_char++;
        }

        if (('\1' == *first_frame_char) && (framesize > 0))
        {
            first_frame_char++;
        }

        uint32_t nal = *first_frame_char;
        uint8_t naltype = nal & 0x1f;

        if(is_NALU_SPS(naltype) || is_NALU_PPS(naltype) || is_NALU_SEI(naltype))
        {
            printf("naltpye : [%d]\n",naltype);
            memcpy(temp+PS_I_ALL_LEN+Pos,frame,framesize);
            Pos += framesize;
            return true;
        }
        else if(is_NALU_IDR(naltype))
        {

            // 1 package for ps header   
            gb28181_make_ps_header(temp + nSizePos, (unsigned long long)frameTS);  
            nSizePos += PS_HDR_LEN;

            // 如果是I帧的话，则添加系统头  
            gb28181_make_sys_header(temp + nSizePos);  
            nSizePos += SYS_HDR_LEN;

            // psm头 (也是map)  
            gb28181_make_psm_header(temp + nSizePos);  
            nSizePos += PSM_HDR_LEN;

        }
		else
		{
            // 1 package for ps header   
            gb28181_make_ps_header(temp + nSizePos, (unsigned long long)frameTS);  
            nSizePos += PS_HDR_LEN;			
			
		}

		maxPayload = PS_PES_PAYLOAD_SIZE - nSizePos - PES_HDR_LEN - Pos;
		int nSize = 0;
		uint32_t offset = 0;
		uint32_t start = 1;

		while(framesize > 0)
		{
			if(start = 0)
			{
				nSizePos = 0;
				Pos = 0;
				maxPayload = PS_PES_PAYLOAD_SIZE - PES_HDR_LEN;
			}
			nSize = ((framesize>maxPayload)?maxPayload:framesize);
			
			// 添加pes头  
			gb28181_make_pes_header(temp+nSizePos, 0xE0, nSize, 
									(unsigned long long)frameTS, (unsigned long long)frameTS);
			nSizePos += PES_HDR_LEN;

			memcpy(temp+nSizePos+Pos,frame+offset,nSize);
			pes_unpack_rtp(mrtp, (uint8_t *)temp, nSize+nSizePos+Pos, nRTP_MAX_SIZE, frameTS);

			offset += nSize;
			framesize -= nSize;
			start = 0;

		}

		nSizePos = 0;
		Pos = 0;		


        int pack_nums = mrtp->get_container().size();
        printf("get rtp pack num :[%d]\n",pack_nums);

        //伪造rtp包头信息
        rv_rtp_param rtp_param;
        ::construct_rv_rtp_param(&rtp_param);
        rtp_param.sByte = MP_PSEUDO_RTP_HEAD_SIZE;

        //提供外部sequenceNumber计数
        rtp_param.sequenceNumber = m_last_frame_in_rtp_sn;
        m_last_frame_in_rtp_sn += pack_nums;
        rtp_param.payload = framePayload;

        //合成内部时间戳方式
        rtp_param.timestamp = frameTS;

        mrtp->set_rtp_param(&rtp_param);
        mrtp->m_priority = priority;
        mrtp->m_use_ssrc = use_ssrc;
        mrtp->m_ssrc = ssrc;
        bRet = static_cast<mssrc_frame *>(hmssrc)->pump_frame_in(mrtp);

        mrtp->release();

        return bRet;
    }

	mp_bool bc_mp::pump_frame_in_hevc(
		MP_IN mssrc *hmssrc,			//目标mssrc句柄
		MP_IN uint8_t *frame,			//帧数据(去除私有头)
		MP_IN uint32_t framesize,		//数据帧长度
		MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
		MP_IN uint8_t  framePayload,    // 96
        MP_IN uint8_t priority,
        MP_IN bool use_ssrc,
        MP_IN uint32_t ssrc)	
	{
		mp_bool ret_code = MP_TRUE;
		rtp_mblock *mrtp = NULL;
		do 
		{
			//前置条件
			if (NULL == frame || !m_bReady || !m_active)
			{
				ret_code = MP_FALSE;
				break;
			}

            //无人关注 不推送数据
            if (0 == static_cast<msink_rv_rtp *>(m_msinks.front())->get_viewer())
            {
                ret_code = MP_FALSE;
                break;
            }

            //分包处理
            mrtp = static_cast<rtp_mblock *>(m_mrtp_pool.alloc_item(false));
            if (NULL == mrtp)
            {
                //尝试分配方式，引起m_mrtp_pool动态扩展
                mrtp = static_cast<rtp_mblock *>(
                    tghelper::recycle_pool_build_item<rtp_mblock>(&m_mrtp_pool, false));
            }

            mrtp->assign();
            mrtp->m_bFrameInfo = false;

            //拆包
            ret_code = frame_unpack_hevc(mrtp,frame,framesize,nRTP_MAX_SIZE);
            if (MP_FALSE == ret_code)
            { 
                break;
            }

            int pack_nums = mrtp->get_container().size();

            //伪造rtp包头信息
            rv_rtp_param rtp_param;
            ::construct_rv_rtp_param(&rtp_param);

            rtp_param.sByte = MP_PSEUDO_RTP_HEAD_SIZE;

            //提供外部sequenceNumber计数
            rtp_param.sequenceNumber = m_last_frame_in_rtp_sn;
            m_last_frame_in_rtp_sn += pack_nums;
            rtp_param.payload = framePayload;

            //合成内部时间戳方式
            rtp_param.timestamp = frameTS;

			mrtp->set_rtp_param(&rtp_param);
            mrtp->m_priority  = priority; 
            mrtp->m_use_ssrc = use_ssrc;
            mrtp->m_ssrc = ssrc;
			if (static_cast<mssrc_frame *>(hmssrc)->pump_frame_in(mrtp))
			{
				//ret_code = MP_FALSE;
				//20150519 modefy 
				ret_code = MP_TRUE;
				break;
			}			

        } while (false);

        if (NULL != mrtp)
        {
            mrtp->release();
        }
        return ret_code;
    }
	 
	mp_bool bc_mp::frame_unpack(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu,MP_IN uint32_t frameTS,			//时间戳(私有头获得)			  
		MP_IN uint8_t  framePayload)
    {
        if (!mrtp || !frame)
        {
            return false;
        }

        if (framesize < 5)
        {
            return false;
        }

        uint32_t maxPayload = mtu - MP_PSEUDO_RTP_HEAD_SIZE;

		//去掉001和0001两种情况
		while (('\0' == *frame) && (framesize > 0))
		{
			frame++;
			framesize--;
		}

		if (('\1' == *frame) && (framesize > 0))
		{
			frame++;
			framesize--;
		}

        uint32_t naltype = *frame;
        if (framesize <= maxPayload)
        {
            rtp_block *rtp = m_rtp_pool.force_alloc_any();
            if (!rtp)
            {
                return false;
            }

            rtp->assign();
            rtp->m_bFrameInfo = false;
            rtp->write(frame, framesize, MP_PSEUDO_RTP_HEAD_SIZE);
            mrtp->push_byte_block(rtp);
            //rtp->release();
        }
        else
        {
            naltype = *frame;
            frame++;
            framesize--;

            maxPayload = mtu - MP_PSEUDO_RTP_HEAD_SIZE - H264HEADERSIZE;

            uint32_t start = 1;          /* Start value used for FU header     */
            uint32_t end   = 0;          /* End value used for FU header       */
            uint32_t first = 0;
            uint32_t rtpDataStart = 0;
            while (end == 0)
            {
                uint32_t rtpPayload = maxPayload < framesize ? maxPayload : framesize;
                if (framesize == rtpPayload)
                    end = 1;

                uint8_t fu[H264HEADERSIZE];
                /* FU indicator                                                   */
                fu[0] = (naltype & 0x60) | 28;

                /* FU header                                                      */
                fu[1] = (start << 7) | (end << 6) | (naltype & 0x1f);

                uint8_t data[4*1024];
                ::memcpy(data, fu, H264HEADERSIZE);
                ::memcpy(data+H264HEADERSIZE, frame+first, rtpPayload);

                rtp_block *rtp = m_rtp_pool.force_alloc_any();
                if (rtp)
                {
                    rtp->assign();
                    rtp->m_bFrameInfo = false;
                    rtp->write(data, rtpPayload+H264HEADERSIZE, MP_PSEUDO_RTP_HEAD_SIZE);

                    mrtp->push_byte_block(rtp);
                    //rtp->release();
                }

                framesize -= rtpPayload;
                first   += rtpPayload;
                start    = 0;
            }
        }

        return true;
    }

    mp_bool bc_mp::frame_unpack_ps(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu, uint32_t frameTS)
    {
        if (!mrtp || !frame)
        {
            return false;
        }

        if (framesize < 5)
        {
            return false;
        }

        static uint32_t maxPayload;
        static int  nSizePos=0;

        static char temp[4096];
        static char Pos = 0;

        uint8_t *first_frame_char = frame;

        //去掉001和0001两种情况
        while (('\0' == *temp) && (framesize > 0))
        {
            first_frame_char++;
        }

        if (('\1' == *temp) && (framesize > 0))
        {
            first_frame_char++;
        }

        uint32_t nal = *first_frame_char;
        uint8_t naltype = nal & 0x1f;

        if(is_NALU_SPS(naltype) || is_NALU_PPS(naltype) || is_NALU_SEI(naltype))
        {
            memcpy(temp+PS_I_ALL_LEN+Pos,frame,framesize);
            Pos += framesize;
            return true;
        }
        else if(is_NALU_IDR(naltype))
        {

            // 1 package for ps header   
            gb28181_make_ps_header(temp + nSizePos, (unsigned long long)frameTS);  
            nSizePos += PS_HDR_LEN;

            // 如果是I帧的话，则添加系统头  
            gb28181_make_sys_header(temp + nSizePos);  
            nSizePos += SYS_HDR_LEN;

            // psm头 (也是map)  
            gb28181_make_psm_header(temp + nSizePos);  
            nSizePos += PSM_HDR_LEN;

            maxPayload = mtu - RTP_HDR_LEN - nSizePos - PES_HDR_LEN - Pos;

        }

        if (framesize <= maxPayload)
        {
            //每次帧的长度不要超过short类型，过了就得分片进循环行发送  
           // nSize = (nFrameLen > PS_PES_PAYLOAD_SIZE) ? PS_PES_PAYLOAD_SIZE : nFrameLen;  
            // 添加pes头  
            //(pPacker->s64CurPts / 100), (pPacker->s64CurPts/300)
            gb28181_make_pes_header(temp+nSizePos, 0xE0, framesize+Pos, (unsigned long long)frameTS, (unsigned long long)frameTS);
            nSizePos += PES_HDR_LEN;

            memcpy(temp+nSizePos+Pos, frame, framesize);

            rtp_block *rtp = m_rtp_pool.force_alloc_any();
            if (!rtp)
            {
                return false;
            }

            rtp->assign();
            rtp->m_bFrameInfo = false;

            rtp->write(temp, framesize+Pos+nSizePos, MP_PSEUDO_RTP_HEAD_SIZE);
            mrtp->push_byte_block(rtp);
            rtp->release();
        }
        else
        {
            uint32_t start = 1;          /* Start value used for FU header     */
            uint32_t end   = 0;          /* End value used for FU header       */
            uint32_t offset = 0;

            while (end == 0)
            {
                if(!start)
                {
                    nSizePos = 0;
                    Pos = 0;
                    maxPayload = mtu - MP_PSEUDO_RTP_HEAD_SIZE - PES_HDR_LEN;
                }

                uint32_t rtpPayload = maxPayload < framesize ? maxPayload : framesize;
                if (framesize == rtpPayload)
                    end = 1;

                gb28181_make_pes_header(temp + nSizePos, 0xE0, rtpPayload, (unsigned long long)frameTS, (unsigned long long)frameTS);
                nSizePos += PES_HDR_LEN;
                ::memcpy(temp + nSizePos + Pos, frame+offset, rtpPayload);

                rtp_block *rtp = m_rtp_pool.force_alloc_any();
                if (rtp)
                {
                    rtp->assign();
                    rtp->m_bFrameInfo = false;
                    rtp->write(temp, rtpPayload + nSizePos + Pos, MP_PSEUDO_RTP_HEAD_SIZE);
                    mrtp->push_byte_block(rtp);
                    rtp->release();
                }

                framesize -= rtpPayload;
                offset   += rtpPayload;
                start    = 0;
            }
        }

        return true;
    }

    //20150313 songlei
    mp_bool bc_mp::frame_unpack_hevc(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu)
    {
        mp_bool ret_code = MP_TRUE;

        //去掉001和0001两种情况
        while (('\0' == *frame) && (framesize > 0))
        {
            frame++;
            framesize--;
        }

        if (('\1' == *frame) && (framesize > 0))
        {
            frame++;
            framesize--;
        }

        int max_payload_size = mtu - MP_PSEUDO_RTP_HEAD_SIZE;
        const uint8_t *buf = frame;
        int len = framesize;
        int rtp_payload_size = max_payload_size - RTP_HEVC_HEADERS_SIZE;

        int nal_type  = (buf[0] >> 1) & 0x3F;

        //send it as one single NAL unit 
        if (len <= max_payload_size)
        {
            // use the original NAL unit buffer and transmit it as RTP payload
            rtp_block *rtp = m_rtp_pool.force_alloc_any();

            if ( NULL != rtp)
            {
                rtp->assign();
                rtp->m_bFrameInfo = false;

                rtp->write(buf, len, MP_PSEUDO_RTP_HEAD_SIZE);
                mrtp->push_byte_block(rtp);
                rtp->release();
            }
            else
            {
                ret_code = MP_FALSE;
            }

        }
        else 
        {
            /*create the HEVC payload header and transmit the buffer as fragmentation units (FU)	 
            0                   1
            0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |F|   Type    |  LayerId  | TID |
            +-------------+-----------------+
            F       = 0
            Type    = 49 (fragmentation unit (FU))
            LayerId = 0
            TID     = 1*/
            uint8_t fu[RTP_HEVC_HEADERS_SIZE];
            fu[0] = 49 << 1;
            fu[1] = 1;

            /*create the FU header
            0 1 2 3 4 5 6 7
            +-+-+-+-+-+-+-+-+
            |S|E|  FuType   |
            +---------------+
            S       = variable
            E       = variable
            FuType  = NAL unit type*/
            fu[2]  = nal_type;

            //set the S bit: mark as start fragment
            fu[2] |= 1 << 7;

            // pass the original NAL header
            buf += 2;
            len -= 2;
            uint8_t send_data[RTP_SEND_BUF_SIZE];
            while (len > rtp_payload_size) 
            {
                // complete and send current RTP packet  
                ::memset(send_data,0,RTP_SEND_BUF_SIZE);
                ::memcpy(send_data, fu, RTP_HEVC_HEADERS_SIZE);
                ::memcpy(send_data+RTP_HEVC_HEADERS_SIZE, buf, rtp_payload_size);

                rtp_block *rtp = m_rtp_pool.force_alloc_any();
                if ( NULL != rtp)
                {
                    rtp->assign();
                    rtp->m_bFrameInfo = false;
                    rtp->write(send_data,max_payload_size, MP_PSEUDO_RTP_HEAD_SIZE);
                    mrtp->push_byte_block(rtp);
                    rtp->release();
                }
                else
                {
                    ret_code = MP_FALSE;
                }

                buf += rtp_payload_size;
                len -= rtp_payload_size;

                // reset the S bit 
                fu[2] &= ~(1 << 7);
            }

            // set the E bit: mark as last fragment
            fu[2] |= 1 << 6;

            // complete and send last RTP packet
            ::memset(send_data,0,RTP_SEND_BUF_SIZE);
            ::memcpy(send_data, fu, RTP_HEVC_HEADERS_SIZE);
            ::memcpy(send_data+RTP_HEVC_HEADERS_SIZE, buf, len);

            rtp_block *rtp = m_rtp_pool.force_alloc_any();
            if (NULL != rtp)
            {
                rtp->assign();
                rtp->m_bFrameInfo = false;
                rtp->write(send_data, len+RTP_HEVC_HEADERS_SIZE, MP_PSEUDO_RTP_HEAD_SIZE);
                mrtp->push_byte_block(rtp);
                rtp->release();
            }
            else
            {
                ret_code = MP_FALSE;
            }
        }	

        return ret_code;
    }

    void bc_mp::set_raddr_cb(raddr_cb cb)
    {
        m_raddr_cb = cb;
    }

	void bc_mp::set_file_path(const char *file)
	{
		boost::unique_lock<boost::shared_mutex> lock(m_resource_locker);

		std::list<msink *>::iterator itr = m_msinks.begin();
		for (;m_msinks.end() != itr; itr++)
		{
			switch((*itr)->get_type())
			{
			case MEMORY_MSINK:
				break;
			case RV_RTP_MSINK:
				{
					msink_rv_rtp * sink_rv_rtp = static_cast<msink_rv_rtp *>(*itr);
					if (sink_rv_rtp)
					{
						sink_rv_rtp->set_file_path(file);
					}
					break;
				}

			default:
				break;
			}
		}
	}

#ifdef _USE_RTP_SEND_CONTROLLER
    void bc_mp::register_network_changed_callback(mp_network_changed_callback_t cb, void *ctx)
    {
        m_network_changed_callback = cb;
        m_network_changed_callback_ctx = ctx;
    }

    void bc_mp::on_network_change(uint32_t bitrate, uint8_t fraction_lost, uint32_t rtt)
    {
        if (NULL != m_network_changed_callback)
        {
            m_network_changed_callback(m_network_changed_callback_ctx, this, bitrate, fraction_lost, rtt);
        }
    }
#endif
}
