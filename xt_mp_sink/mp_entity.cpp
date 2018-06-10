//#include "stdafx.h"
#include "sink_singleton.h"
#include "mp_entity.h"
#include <strstream>
#include "XTDemuxMan.h"
#include <stdarg.h>
#include <sys/types.h>
#include "sink_config.h"
#include "xt_av_check.h"

#define MP_PSEUDO_TS_CLOCK		90
#define MAX_DROP	1024
#define tp_pool_max_num 10*1024
#define usrcount_max_num 1024
#define LAST_FRAME_SN_INIT (-1)
int g_log = 0; 
#define MAX_SN 65535	//SN最大值
int WAIT_RESEND = 10;	//重发等待
int LEN_RTP_CACHE = 1024;		//缓冲队列长度
int MAX_RESEND = 10;			//一次最大重传请求数
int VGA_ORDER = 1;				//开启VGA编码器排序(支持丢包重传)

namespace xt_mp_sink
{

#ifdef use_recycle_entity_pool_func__
    //entity管理
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    boost::recursive_mutex mp_entity::entity_pool_mutex_;
    std::vector<mp_entity*> mp_entity::recycle_entity_pool_;
    char mp_entity::m_license[128];
	boost::atomic<int> mp_entity::m_nReSendCfg;
	bool mp_entity::m_bSetReSend = false;
    void mp_entity::init_recycle_entity_pool()
    {
        boost::unique_lock<boost::recursive_mutex> lock_entity_pool(entity_pool_mutex_);
        recycle_entity_pool_.clear();

#ifdef _WIN32
        strcpy(m_license, "D:/NetMCSet/license");
#else
        strcpy(m_license, "/etc/xtconfig/d/netmcset/license");
#endif//#ifdef _WIN32
    }
    mp_entity* mp_entity::malloc_entity()
    {
        mp_entity* entity_ptr=NULL;
        do 
        {
            boost::unique_lock<boost::recursive_mutex> lock_entity_pool(entity_pool_mutex_);
            if (recycle_entity_pool_.empty())
            {
                entity_ptr = new mp_entity;
                recycle_entity_pool_.push_back(entity_ptr);
                break;
            }
            else if (xt_av_check::inst()->check_sn(m_license)<0 &&
                    recycle_entity_pool_.size() >= 8)
            {
                return NULL;
            }
            else
            {
                bool is_exist = false;
                std::vector<mp_entity*>::iterator itr = recycle_entity_pool_.begin();
                for(; recycle_entity_pool_.end() != itr; ++itr)
                {
                    if ((*itr)->m_state == mp_entity::MP_IDLE_STATE)
                    {
                        is_exist = true;
                        entity_ptr = *itr;
                        break;
                    }
                }
                if (!is_exist)
                {
                    entity_ptr = new xt_mp_sink::mp_entity;
                    recycle_entity_pool_.push_back(entity_ptr);
                    break;
                }
            }
        } while (0);

        return entity_ptr;
    }
    bool mp_entity::check_entity(const mp_entity* ptr)
    {
        boost::unique_lock<boost::recursive_mutex> lock_entity_pool(entity_pool_mutex_);
        std::vector<mp_entity*>::iterator itr = recycle_entity_pool_.begin();
        for(; recycle_entity_pool_.end() != itr; ++itr)
        {
            if (ptr == *itr)
            {
                return true;
            }
        }
        return false;
    }

    void mp_entity::free_entity_all()
    {
        boost::unique_lock<boost::recursive_mutex> lock_entity_pool(entity_pool_mutex_);
        std::vector<mp_entity*>::iterator itr = recycle_entity_pool_.begin();
        while(recycle_entity_pool_.end() != itr)
        {
            utility::destruct_ptr(*itr);
            itr++;
        }
        recycle_entity_pool_.clear();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif //#ifdef use_recycle_entity_pool_func__

    boost::recursive_mutex mp_entity::m_mEntity;
    boost::recursive_mutex mp_entity::m_mEntityClose;
    std::set<mp_handle> mp_entity::m_setSinkHandleMrg;

    uint32_t bitfieldSet(
        uint32_t    value,
        uint32_t    bitfield,
        int32_t         nStartBit,
        int32_t         nBits)
    {
        uint32_t mask = (1 << nBits) - 1;

        return (value & ~(mask << nStartBit)) +
            ((bitfield & mask) << nStartBit);
    }
	//非复用rtp流接口
    void mp_entity::OnRtpReceiveEvent( rv_handler hrv,rv_context context )
    {

        mp_entity *pThis = static_cast<mp_entity*>(context);
        if(NULL == pThis)
        {
            DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::OnRtpReceiveEvent invalid!");
            return;
        }

        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::OnRtpReceiveEvent | recv data of the rv libs ptr_this[%p]",pThis);

        //一级流水
        if (pThis->m_isDirectOutput && VGA_ORDER>0)
        {
			//数据处理过程中，不会重复投递
			if (pThis->m_pump_flags)
			{
				pThis->m_pump_flags = false;
				pThis->assign();
				if (!sink_inst::sink_singleton()->tp_inst()->schedule( boost::bind(&mp_entity::pump_rtp_in_sj, pThis,hrv)))
				{
					pThis->release();
					DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::7| ptr_this[%p] sink_singleton()->tp_inst()->schedule fail!",pThis);
				}
			}
        }
        else
        {
            pThis->pump_rtp_in(hrv);
        }
    }

    void  mp_entity::OnRtcpSendEvent( rv_handler hrv,rv_context context, 
        uint32_t ssrc,uint8_t *rtcpPack, 
        uint32_t size )
    {
        return;//edit bu zhouzx 2015/11/26
        boost::recursive_mutex::scoped_lock lock2(m_mEntityClose);

        mp_entity *pThis = static_cast<mp_entity*>(context);
        if(pThis == null) return;

        if (!xt_mp_sink::mp_entity::is_valid(pThis))
        {
            DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::OnRtcpSendEvent entity valid [%p]",pThis);
            return;
        }

        pThis->pump_rtcp_in(hrv,pThis->m_ssrc);
    }

    void mp_entity::OnRtcpReceiveEvent( rv_handler hrv,rv_context context, 
        uint32_t ssrc,uint8_t *rtcpPack, uint32_t size ,
		uint8_t *ip,uint16_t port,rv_bool multiplex,uint32_t multid)
    {
        boost::recursive_mutex::scoped_lock lock2(m_mEntityClose);

        mp_entity *pThis = static_cast<mp_entity*>(context);
        if(pThis == null) return;

        if (!xt_mp_sink::mp_entity::is_valid(pThis))
        {
            return;
        }

        //update send addr
        rv_net_address addr;
        _construct_rv_address((int8_t*)ip, port, &addr);

        pThis->update_srcaddr_rtcp(addr);
    }

    rv_bool mp_entity::OnRtcpAppEventHandler_CB(
        RV_IN  rv_handler	 hrv,
        RV_IN  rv_context	 context,
        RV_IN  uint8_t        subtype,
        RV_IN  uint32_t       ssrc,
        RV_IN  uint8_t*       name,
        RV_IN  uint8_t*       userData,
        RV_IN  uint32_t       userDataLen)
    {
        boost::recursive_mutex::scoped_lock lock2(m_mEntityClose);

        mp_entity *pThis = static_cast<mp_entity*>(context);
        if(pThis == null) return true;

        if (!xt_mp_sink::mp_entity::is_valid(pThis))
        {
            DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::OnRtcpAppEventHandler_CB entity valid [%p]",pThis);
            return true;
        }

        switch (subtype)
        {
        case 9://XTSR
            {
                if (userData && userDataLen>0 && userDataLen<128)
                {
                    pThis->m_bManualSR_cfg = true;

                    int len = 0;
                    uint8_t data[128];
                    ::memcpy(data, userData, userDataLen);

                    ::memcpy(&pThis->m_xtsr.nDeviceType, data+len, 4); len+=4;
                    ::memcpy(&pThis->m_xtsr.nLinkRet, data+len, 4); len+=4;
                    ::memcpy(&pThis->m_xtsr.nFrameType,data+len,  4); len+=4;
                    ::memcpy(&pThis->m_xtsr.sr.nSSRC, data+len, 4); len+=4;
                    ::memcpy(&pThis->m_xtsr.sr.mNTPtimestamp, data+len, 4); len+=4;
                    ::memcpy(&pThis->m_xtsr.sr.lNTPtimestamp, data+len, 4); len+=4;
                    ::memcpy(&pThis->m_xtsr.sr.nRtpTimestamp, data+len, 4); len+=4;
                    ::memcpy(&pThis->m_xtsr.sr.nPackets, data+len, 4); len+=4;
                    ::memcpy(&pThis->m_xtsr.sr.nOctets, data+len, 4); len+=4;
					//结构体是按4字节对齐的
					//::memcpy(&(pThis->m_xtsr), data,sizeof XTSR);

                    DEBUG_LOG("xtsr_sink",LL_INFO,"TS:[%d] - [%d] - [%d]",pThis->m_xtsr.sr.mNTPtimestamp, pThis->m_xtsr.sr.lNTPtimestamp, pThis->m_xtsr.sr.nRtpTimestamp);

                    if (pThis->m_funcSR)
                    {
                        pThis->m_funcSR(pThis->m_xtsr.nDeviceType, pThis->m_xtsr.nLinkRet, pThis->m_xtsr.nFrameType, &pThis->m_xtsr.sr, pThis->m_puserdata);
                    }
                }
                break;
            }
        default:
            {
                break;
            }
        }

        return true;
    }

    void mp_entity::_construct_rv_address( int8_t * ip,uint16_t port,rv_net_address * rv_address )
    {
        rv_net_ipv4 address;
        construct_rv_net_ipv4(&address);
        rv_inet_pton4(&address, (char *)ip);
        address.port = port;
        convert_ipv4_to_rvnet(rv_address, &address);
    }

    mp_entity::mp_entity(uint32_t source_fifo_size ,
        uint32_t sink_fifo_size ,
        uint32_t sndout_fifo_size ,
        uint32_t snd_out )
        : m_sink(sink_fifo_size,false)
        , m_source()
        , m_bActive(false)
        , m_rcv_rtcp_fifo(2,true)
        , m_snd_rtcp_fifo(2,true)
        , m_snd_out_fifo(0,false)
        , m_rtp_fifo(0,false)
        , m_state(MP_IDLE_STATE)
        , m_pRcvFrameCB(null)
        , m_pRptEventCB(null)
        , m_pContext(null)
        , m_rtp_ssrc(0)
        , m_rtcp_ssrc(0)
        , m_isDirectOutput(false)
        , m_bManualRtcp(false)
        , m_debug_fifo(0,false)
        , m_lastFrameMarkerSN(-1)
        , m_ssrc(0)
        , m_bManualSR_cfg(false)
        , m_funcSR(NULL)
        , m_puserdata(NULL)
        , m_tmSrcAddr(0)
        , m_nFrame(0)
        , m_tForceLevel(2000)
        , m_tForceRR(1)
        , m_fir_seq(0)
        , m_ref(0)
    { 
        ::memset(&m_handle, 0, sizeof(m_handle));
        //::memset(&m_rcvSeg, 0, sizeof(m_rcvSeg));
        ::memset(&m_srcaddr, 0, sizeof(m_srcaddr));

		m_block_index = 0;
		wait_frames =0;
		m_pump_flags = true;
		m_block_pool = new rtp_block[LEN_RTP_CACHE];//2M
    }

    mp_entity::~mp_entity( void )
    {
		if (m_block_pool)
		{
			delete []m_block_pool;
			m_block_pool = NULL;
		}
    }
	int mp_entity::mp_set_resend(int resend,int wait_resend, int max_resend, int vga_order)
	{
		if (wait_resend < 5 || max_resend < 1)
		{
			return -1;
		}

		m_bSetReSend = true;
		m_nReSendCfg = resend;
		WAIT_RESEND = wait_resend;
		MAX_RESEND = max_resend;
		VGA_ORDER = vga_order;

		return 0;
	}
    uint32_t mp_entity::mp_open(xt_mp_descriptor* mp_des,p_msink_handle handle, bool multiplex, uint32_t *multid, bool bOpend)
    {
        DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open|ptr_entity[%p] port[%d] start....",this,mp_des->local_address.port);
        int num = 1;
        int sub = 1024;

        g_log = sink_config::inst()->log_level(-1);
        m_nLostCfg = sink_config::inst()->lost(-1);
        m_tmSrcAddrInterval = sink_config::inst()->tm_readd_rmt(1000); 
        m_jarlessPackets = sink_config::inst()->jarless_packets(4);	
        m_syncPackets = sink_config::inst()->sync_packets(300);	
        m_waitFrames = sink_config::inst()->wait_frames(15);
        m_rcheck_sum_cfg = sink_config::inst()->check_sum(0);
        LEN_RTP_CACHE = sink_config::inst()->pkt_cache(1024);

		//如果这4个参数通过接口动态修改，将不在从配置文件中获取
		if (!m_bSetReSend)
		{
			m_nReSendCfg = sink_config::inst()->resend(1);
			WAIT_RESEND = sink_config::inst()->wait_resend(10);
			MAX_RESEND = sink_config::inst()->max_resend(10);
			VGA_ORDER = sink_config::inst()->vga_order(0);
		}

        //重排参数
        /////////////////////////////////////////////////////////////////////////////////////////////////////////
        m_jarlessPackets     = m_jarlessPackets < 0  ? 0  : ( m_jarlessPackets > 4   ? 4   : m_jarlessPackets );
        m_syncPackets        = m_syncPackets    < 0  ? 0  : ( m_syncPackets    > 600 ? 600 : m_syncPackets );
        m_waitFrames         = m_waitFrames     < 15 ? 15 : ( m_waitFrames     > 250  ? 250  : m_waitFrames );
        m_jPackets           = 0;
        m_nIncoming          = 0;
        /////////////////////////////////////////////////////////////////////////////////////////////////////////

        LEN_RTP_CACHE = LEN_RTP_CACHE>256? LEN_RTP_CACHE:256;
        m_port = mp_des->local_address.port;  //////// \\\\\\\\ 打日志用
        
        //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        m_bActive = false;
        m_lastFrameMarkerSN = -1;
        m_pRcvFrameCB = mp_des->onReceiveDataEvent;
        m_pRcvRtpCB = mp_des->onReceiveRtpEvent;
        m_pContext = mp_des->context;
        m_mode = mp_des->mode;
        m_bManualRtcp = mp_des->manual_rtcp == 1 ? true : false;
        m_isDirectOutput = mp_des->is_direct_output == 1 ? true : false;

        rv_session_descriptor des;
        construct_rv_session_descriptor(&des);
        _construct_rv_address(mp_des->local_address.ip_address,mp_des->local_address.port,&des.local_address);
        des.context = this;
        des.user_context = this;
        des.manual_rtcp = mp_des->manual_rtcp;
        des.onRtpRcvEvent = mp_entity::OnRtpReceiveEvent;
        des.onRtcpSndSREvent = mp_entity::OnRtcpSendEvent;
        des.onRtcpRcvSRRREvent = mp_entity::OnRtcpReceiveEvent;
        des.multicast_rtp_address_opt = mp_des->rtp_multi_cast_opt;
        _construct_rv_address(mp_des->rtp_multi_cast_address.ip_address,mp_des->rtp_multi_cast_address.port,&des.multicast_rtp_address);
        des.multicast_rtp_ttl = mp_des->rtp_multi_cast_ttl;

        des.multicast_rtcp_address_opt = mp_des->rtcp_multi_cast_opt;
        _construct_rv_address(mp_des->rtcp_multi_cast_address.ip_address, mp_des->rtcp_multi_cast_address.port,&des.multicast_rtcp_address);
        des.multicast_rtcp_ttl = mp_des->rtcp_multi_cast_ttl;
        uint32_t ret = 0;
        this->assign();
        do
        {
            m_bMultiplex = false;
            if (multiplex)
            {
                m_bMultiplex = true;

                //初始化复用
                XTDemuxMan::instance()->init(num,sub);
                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open open_session 1!");
                if (!bOpend)
                {
                    ::memset(&m_handle, 0, sizeof(m_handle));
                    bool ret = XTDemuxMan::instance()->open_session(&des, &m_multID, &m_handle,m_port);
                    if (!ret)
                    {
                        DEBUG_LOG(SINK_CALL,LL_ERROE,"mp_open open_session fail 1!");
                        break;
                    }
                    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open open_session success 1 m_handle.hrtp[%p]",m_handle.hrtp);
                }
                if (multid)
                {
                    *multid = m_multID;
                }
                set_manual_rtcp(&m_handle, true);
            }
            else
            {
                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open open_session 2!");
                if (!bOpend)
                {
                    ::memset(&m_handle, 0, sizeof(m_handle));
                    if(!open_session(&des,&m_handle))
                    {
                        DEBUG_LOG(SINK_CALL,LL_ERROE,"mp_open open_session fail 2!");
                        break;
                    }
                }
                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open open_session success 2 m_handle.hrtp[%p]",m_handle.hrtp);
            }

            m_state = MP_OPEN_STATE;
            if (!bOpend)
            {
                //::memcpy(&m_des, mp_des, sizeof(xt_mp_descriptor));
                m_rtp_ssrc = get_rtp_ssrc(&m_handle);
                m_rtcp_ssrc = get_rtcp_ssrc(&m_handle);
                set_rtp_receive_buffer_size(&m_handle,MP_RECEIVE_BUFFER_SIZE);
                set_rtp_transmit_buffer_size(&m_handle,MP_SEND_BUFFER_SIZE);
                RtcpSetAppEventHandler(&m_handle, mp_entity::OnRtcpAppEventHandler_CB);
                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open RtcpSetAppEventHandler m_handle.hrtp[%p]",m_handle.hrtp);
            }
            handle->rtp_ssrc = m_rtcp_ssrc>>16;
            handle->rtcp_ssrc = m_rtcp_ssrc;
            ret = 1;
        } while (false);

        sink_inst::sink_singleton()->add_ent(this);
        DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open sink_inst::sink_singleton()->add_ent(this) m_handle.hrtp[%p]",m_handle.hrtp);

        ClearReSend();
        DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open ClearReSend m_handle.hrtp[%p]",m_handle.hrtp);
        this->release();

        DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open|ptr_entity[%p] port[%d] end!",this,mp_des->local_address.port);

		return ret;
    }

    void mp_entity::mp_setdemux_handler(rv_context func)
    {
        setdemux_handler(func);
    }

    uint32_t mp_entity::mp_active(uint32_t bActive)
    {
        if(m_state == MP_IDLE_STATE) return -1;
        if(m_bActive)
        {
            if(bActive)
                return 0;

            m_bActive = false;
            m_source.reset();
        }
        else
        {
            if(!bActive) return 0;
            m_bActive = true;
        }
        return 0;
    }

    void mp_entity::ClearReSend()
    {
        boost::recursive_mutex::scoped_lock lock(m_mLost);

        m_lastSn = 0;
        m_bInitLast = false;
        m_listAck.clear();
    }

    uint32_t mp_entity::mp_close()
    {
        DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close| ptr_entity[%p] start...",this);

        /*
        此处暂时不锁，此处理加锁后会导致rv_adapter的在sip推流状态时，close工作线程
        与mp_entity::pump_rtp_in_sj死锁。工作线程中会调用RvRtpSeliSelectUntil(1)
        -》DispatchMessage(&msg);-》OnRtpReceiveEvent-》mp_entity::pump_rtp_in_sj
        */
        uint32_t ret = -1;
        do
        {
            if(m_state != MP_OPEN_STATE) break;

            DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close | sink_inst::sink_singleton()->del_ent[%p] start", this);
            sink_inst::sink_singleton()->del_ent(this);
            DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close | sink_inst::sink_singleton()->del_ent[%p] end !", this);

            while(this->use_count() > 1) boost::this_thread::yield();
            DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close | ptr_this[%p]  wait m_ref end start close...",this);

            if (m_bMultiplex)
            {
                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close| Multiplex mp_clear_rtcp_remote_address m_handle.hrtp[%p]",m_handle.hrtp);
                mp_clear_rtcp_remote_address();

                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close| Multiplex mp_clear_rtp_remote_address m_handle.hrtp[%p]",m_handle.hrtp);
                mp_clear_rtp_remote_address();

                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close| Multiplex close_session m_handle.hrtp[%p]",m_handle.hrtp);
                XTDemuxMan::instance()->close_session(&m_handle);
                m_handle.hrtp = NULL;
                m_handle.hrtcp = NULL;
            }
            else
            {
                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close| mp_clear_rtcp_remote_address m_handle.hrtp[%p]",m_handle.hrtp);
                long ret_code = mp_clear_rtcp_remote_address();
                if (ret_code < 0 )
                {
                }

                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close| mp_clear_rtp_remote_address m_handle.hrtp[%p]",m_handle.hrtp);
                ret_code = mp_clear_rtp_remote_address();
                if (ret_code < 0 )
                {
                }

                DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close| _close_session m_handle.hrtp[%p]",m_handle.hrtp);
                if (!_close_session())
                {
                    DEBUG_LOG(SINK_CALL,LL_ERROE,"mp_entity::mp_close| m_handle.hrtp[%p] _close_session fail!",m_handle.hrtp);
                }
            }
            ret = 0;
            m_state = MP_IDLE_STATE;
        } while (0);
        DEBUG_LOG(SINK_CALL,LL_INFO,"mp_entity::mp_close| ptr_entity[%p] ret[%d] mp_close end!",this,ret);
        return ret;
    }

    // 计算丢失包
    void mp_entity::CalLost(std::vector<uint16_t> &listLost, uint16_t sn1, uint16_t sn2)
    {
        if (sn1>sn2 && (sn1-sn2)<MAX_DROP)
        {
            DEBUG_LOG("sink_calcerr",LL_INFO,"mp_entity::CalLost | ptr_this[%p] sn1[%d] - sn2[%d]",this,sn1, sn2);
        }
        else if (sn1<sn2 && (sn2-sn1)>MAX_DROP)
        {
            DEBUG_LOG("sink_calcerr",LL_INFO,"mp_entity::CalLost | ptr_this[%p] sn1[%d] - sn2[%d]",this,sn1, sn2);
        }
        else if (sn1 == sn2)//sn1 last_sn,sn2 curr_sn
        {
        }
        else if (sn1 < sn2)
        {
            for (uint16_t nS = sn1+1;nS < sn2;++nS)
            {
                RcvSeg seg;
                seg.sn = nS;
                seg.time = 0;
                listLost.push_back(nS);
                if (listLost.size() > MAX_DROP)
                {
                    break;
                }

                if (nS == MAX_SN)
                {
                    break;
                }
            }
        }
        else if (sn1 > sn2)
        {
            for (uint16_t nS = sn1+1;sn1!=MAX_SN && nS<=MAX_SN;++nS)
            {
                RcvSeg seg;
                seg.sn = nS;
                seg.time = 0;
                listLost.push_back(nS);
                if (listLost.size() > MAX_DROP)
                {
                    break;
                }

                if (nS == MAX_SN)
                {
                    break;
                }
            }

            for (uint16_t nS = 0;nS < sn2;++nS)
            {
                RcvSeg seg;
                seg.sn = nS;
                seg.time = 0;
                listLost.push_back(nS);
                if (listLost.size() > MAX_DROP)
                {
                    break;
                }

                if (nS == MAX_SN)
                {
                    break;
                }
            }
        }
    }

    // SN比较
    bool mp_entity::IsSmaller(uint16_t sn1, uint16_t sn2)
    {
        if (sn1<sn2 && (sn2-sn1)<MAX_SN/2)
        {
            return true;
        }
        else if (sn1>sn2 && (sn1-sn2)>MAX_SN/2)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    // 计算差值
    uint16_t mp_entity::CalDif(uint16_t sn1, uint16_t sn2)
    {
        if (sn1<sn2 && (sn2-sn1)<MAX_SN/2)
        {
            return sn2-sn1;
        }
        else if (sn1>sn2 && (sn1-sn2)>MAX_SN/2)
        {
            return sn1-sn2;
        }
        else if(sn1>sn2)
        {
            return sn1-sn2;
        }
        else if (sn1<=sn2)
        {
            return sn2-sn1;
        }

        return 0;
    }

    bool mp_entity::IsReSend(rv_rtp_param &param)
    {
        if (param.extensionBit && param.extensionLength>=1 && param.extensionData)
        {
            if (*param.extensionData == 0xCCBBAA00)
            {
                return true;
            }
        }

        return false;
    }

    unsigned long  crctable[256] =
    {
        0X00000000L,  0X04c11db7L,  0X09823b6eL,  0X0d4326d9L,  
        0X130476dcL,  0X17c56b6bL,  0X1a864db2L,  0X1e475005L,  
        0X2608edb8L,  0X22c9f00fL,  0X2f8ad6d6L,  0X2b4bcb61L,  
        0X350c9b64L,  0X31cd86d3L,  0X3c8ea00aL,  0X384fbdbdL,  
        0X4c11db70L,  0X48d0c6c7L,  0X4593e01eL,  0X4152fda9L,  
        0X5f15adacL,  0X5bd4b01bL,  0X569796c2L,  0X52568b75L,
        0X6a1936c8L,  0X6ed82b7fL,  0X639b0da6L,  0X675a1011L,  
        0X791d4014L,  0X7ddc5da3L,  0X709f7b7aL,  0X745e66cdL,  
        0X9823b6e0L,  0X9ce2ab57L,  0X91a18d8eL,  0X95609039L,  
        0X8b27c03cL,  0X8fe6dd8bL,  0X82a5fb52L,  0X8664e6e5L,  
        0Xbe2b5b58L,  0Xbaea46efL,  0Xb7a96036L,  0Xb3687d81L,  
        0Xad2f2d84L,  0Xa9ee3033L,  0Xa4ad16eaL,  0Xa06c0b5dL,
        0Xd4326d90L,  0Xd0f37027L,  0Xddb056feL,  0Xd9714b49L,  
        0Xc7361b4cL,  0Xc3f706fbL,  0Xceb42022L,  0Xca753d95L,  
        0Xf23a8028L,  0Xf6fb9d9fL,  0Xfbb8bb46L,  0Xff79a6f1L,  
        0Xe13ef6f4L,  0Xe5ffeb43L,  0Xe8bccd9aL,  0Xec7dd02dL,  
        0X34867077L,  0X30476dc0L,  0X3d044b19L,  0X39c556aeL,  
        0X278206abL,  0X23431b1cL,  0X2e003dc5L,  0X2ac12072L,  
        0X128e9dcfL,  0X164f8078L,  0X1b0ca6a1L,  0X1fcdbb16L,  
        0X018aeb13L,  0X054bf6a4L,  0X0808d07dL,  0X0cc9cdcaL,  
        0X7897ab07L,  0X7c56b6b0L,  0X71159069L,  0X75d48ddeL,  
        0X6b93dddbL,  0X6f52c06cL,  0X6211e6b5L,  0X66d0fb02L,  
        0X5e9f46bfL,  0X5a5e5b08L,  0X571d7dd1L,  0X53dc6066L,
        0X4d9b3063L,  0X495a2dd4L,  0X44190b0dL,  0X40d816baL,  
        0Xaca5c697L,  0Xa864db20L,  0Xa527fdf9L,  0Xa1e6e04eL,  
        0Xbfa1b04bL,  0Xbb60adfcL,  0Xb6238b25L,  0Xb2e29692L,  
        0X8aad2b2fL,  0X8e6c3698L,  0X832f1041L,  0X87ee0df6L,  
        0X99a95df3L,  0X9d684044L,  0X902b669dL,  0X94ea7b2aL,  
        0Xe0b41de7L,  0Xe4750050L,  0Xe9362689L,  0Xedf73b3eL,
        0Xf3b06b3bL,  0Xf771768cL,  0Xfa325055L,  0Xfef34de2L,  
        0Xc6bcf05fL,  0Xc27dede8L,  0Xcf3ecb31L,  0Xcbffd686L,  
        0Xd5b88683L,  0Xd1799b34L,  0Xdc3abdedL,  0Xd8fba05aL,  
        0X690ce0eeL,  0X6dcdfd59L,  0X608edb80L,  0X644fc637L,  
        0X7a089632L,  0X7ec98b85L,  0X738aad5cL,  0X774bb0ebL,  
        0X4f040d56L,  0X4bc510e1L,  0X46863638L,  0X42472b8fL,  
        0X5c007b8aL,  0X58c1663dL,  0X558240e4L,  0X51435d53L,  
        0X251d3b9eL,  0X21dc2629L,  0X2c9f00f0L,  0X285e1d47L,  
        0X36194d42L,  0X32d850f5L,  0X3f9b762cL,  0X3b5a6b9bL,  
        0X0315d626L,  0X07d4cb91L,  0X0a97ed48L,  0X0e56f0ffL,  
        0X1011a0faL,  0X14d0bd4dL,  0X19939b94L,  0X1d528623L,
        0Xf12f560eL,  0Xf5ee4bb9L,  0Xf8ad6d60L,  0Xfc6c70d7L,  
        0Xe22b20d2L,  0Xe6ea3d65L,  0Xeba91bbcL,  0Xef68060bL,  
        0Xd727bbb6L,  0Xd3e6a601L,  0Xdea580d8L,  0Xda649d6fL,  
        0Xc423cd6aL,  0Xc0e2d0ddL,  0Xcda1f604L,  0Xc960ebb3L,  
        0Xbd3e8d7eL,  0Xb9ff90c9L,  0Xb4bcb610L,  0Xb07daba7L,  
        0Xae3afba2L,  0Xaafbe615L,  0Xa7b8c0ccL,  0Xa379dd7bL,
        0X9b3660c6L,  0X9ff77d71L,  0X92b45ba8L,  0X9675461fL,  
        0X8832161aL,  0X8cf30badL,  0X81b02d74L,  0X857130c3L,  
        0X5d8a9099L,  0X594b8d2eL,  0X5408abf7L,  0X50c9b640L,  
        0X4e8ee645L,  0X4a4ffbf2L,  0X470cdd2bL,  0X43cdc09cL,  
        0X7b827d21L,  0X7f436096L,  0X7200464fL,  0X76c15bf8L,  
        0X68860bfdL,  0X6c47164aL,  0X61043093L,  0X65c52d24L,  
        0X119b4be9L,  0X155a565eL,  0X18197087L,  0X1cd86d30L,  
        0X029f3d35L,  0X065e2082L,  0X0b1d065bL,  0X0fdc1becL,  
        0X3793a651L,  0X3352bbe6L,  0X3e119d3fL,  0X3ad08088L,  
        0X2497d08dL,  0X2056cd3aL,  0X2d15ebe3L,  0X29d4f654L,  
        0Xc5a92679L,  0Xc1683bceL,  0Xcc2b1d17L,  0Xc8ea00a0L,
        0Xd6ad50a5L,  0Xd26c4d12L,  0Xdf2f6bcbL,  0Xdbee767cL,
        0Xe3a1cbc1L,  0Xe760d676L,  0Xea23f0afL,  0Xeee2ed18L,
        0Xf0a5bd1dL,  0Xf464a0aaL,  0Xf9278673L,  0Xfde69bc4L,
        0X89b8fd09L,  0X8d79e0beL,  0X803ac667L,  0X84fbdbd0L,
        0X9abc8bd5L,  0X9e7d9662L,  0X933eb0bbL,  0X97ffad0cL,
        0Xafb010b1L,  0Xab710d06L,  0Xa6322bdfL,  0Xa2f33668L,
        0Xbcb4666dL,  0Xb8757bdaL,  0Xb5365d03L,  0Xb1f740b4L
    };
    unsigned long CRC_32( unsigned char * aData, unsigned long aSize )
    {
        unsigned long crc32 = 0;
        unsigned long tabitem;
        while(aSize--)
        {
            tabitem = ( crc32 >> 24 ) ^ *(aData++);
            crc32   = ( crc32 << 8 )  ^ crctable[tabitem];
        }

        return crc32;
    }

	//私有流
    void mp_entity::pump_rtp_in(rv_handler hrv)
    {
        //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::pump_rtp_in | ptr_this[%p] recv data start...",this);
        rv_rtp_param param;
        ::construct_rv_rtp_param(&param);
        this->assign();
        bool post_task = true;
        do 
        {
            //////////////////////////////////////////////////////////////////////////
            if (m_rtp_fifo.size() > LEN_RTP_CACHE)
            {
                m_rtp_fifo.pop(true);
            }
            if (m_snd_out_fifo.size() > LEN_RTP_CACHE)
            {
                m_snd_out_fifo.pop(true);
            }
            if (m_sink.size() > LEN_RTP_CACHE)
            {
                m_sink.pop(true);
            }
            //////////////////////////////////////////////////////////////////////////*/

            static uint32_t tickCount = GetTickCount();
            rtp_packet_block* block = alloc_rtp_block();
            if (block == NULL)
            {
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in | ptr_this[%p] get memory fail!",this);
                this->release();
                return;
            }

            block->assign();
            block->m_bFrame = false;
            rv_net_address addr;
            ::memset(&param, 0, sizeof(rv_rtp_param));
            rv_bool read_result = read_rtp(hrv,block->get_raw(),block->size(),&param,&addr);
            do 
            {
                if (RV_ADAPTER_TRUE != read_result)
                {
                    break;
                }
                if (this->use_count() > usrcount_max_num)
                {
                    std::size_t _tp_nums = sink_inst::sink_singleton()->tp_inst()->pending();
                    std::size_t _post_tp_nums = sink_inst::sink_singleton()->post_tp_inst()->pending();
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in | ptr_this[%p] _tp_nums[%d] _post_tp_nums[%d] use_count fail!",this,_tp_nums,_post_tp_nums);
                    read_result = RV_ADAPTER_FALSE;
                    post_task = false;
                    break;
                }
                //读取后进行判定防止RV库不停的抛数据 modify by 20160303 songlei
                if(m_state != MP_OPEN_STATE || !m_bActive)
                {
                    DEBUG_LOG(SINK_ERROE,LL_NORMAL_INFO,"mp_entity::pump_rtp_in ptr_this[%p] not active!",this);
                    read_result = RV_ADAPTER_FALSE;
                    post_task = false;
                    break;
                }
                //模拟丢包
                if (m_nLostCfg>0)
                {
                    static int nN = 0;
                    if ((nN*m_nLostCfg)%100==0)
                    {
                        nN += 1;
                        read_result = RV_ADAPTER_FALSE;
                        break;
                    }
                    nN += 1;
                }
            } while (0);

            if(read_result == RV_ADAPTER_TRUE)
            {
                // 值校验
                if (m_rcheck_sum_cfg > 0)
                {
                    unsigned char *crc = block->get_raw()+ param.len-4;

                    unsigned long sum1 = 0;
                    ::memcpy(&sum1, crc, 4);

                    unsigned long sum2 = CRC_32(block->get_raw()+param.sByte, param.len-param.sByte-4);
                    if (sum1 != sum2)
                    {
                        block->release();
                        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in | ptr_this[%p] checksum fail!",this);
                        continue;
                    }
                    param.len -= 4;
                }

                boost::unique_lock<boost::recursive_mutex> lock(m_member_variable_mutex);
                m_ssrc = param.sSrc;
                if (!m_bInitLast)
                {
                    m_bInitLast = true;
                    m_lastSn = param.sequenceNumber;
                }

                if (!param.extensionBit)
                {
                    param.extensionLength = 0;
                    param.extensionData = NULL;
                }

                //////////////////////////////////////////////////////////////////////////
                if (param.extensionBit && param.extensionData)
                { 
                    uint32_t info[3];

                    if (IsReSend(param))
                    {
                        if (param.extensionLength >= 4)
                        {
                            block->m_bFrame = true;
                            info[0] = param.extensionData[1];
                            info[1] = param.extensionData[2];
                            info[2] = param.extensionData[3];
                        }
                    }
                    else
                    {
                        if (param.extensionLength >= 3)
                        {
                            block->m_bFrame = true;
                            info[0] = param.extensionData[0];
                            info[1] = param.extensionData[1];
                            info[2] = param.extensionData[2];
                        }
                    }

                    if (block->m_bFrame)
                    {
                        block->m_frame[0] = ::ntohl(info[0]);
                        block->m_frame[1] = ::ntohl(info[1]);
                        block->m_frame[2] = ::ntohl(info[2]);
                    }
                }
                //////////////////////////////////////////////////////////////////////////*/

                // RESEND
                if (IsReSend(param))
                {
                    if (m_nReSendCfg <= 0)
                    {
                        block->release();
                        DEBUG_LOG(SINK_ERROE,LL_ERROE,"drop1");
                        continue;
                    }
                }

                if (m_nReSendCfg>0)
                {
                    boost::recursive_mutex::scoped_lock lock(m_mLost);
                    if (!IsReSend(param))
                    {
                        if (CalDif(m_lastSn, param.sequenceNumber)>=MAX_DROP)
                        {
                            m_lastSn = param.sequenceNumber;
                        }
                        //CalLost(m_listAck, m_lastSn, param.sequenceNumber);
                        if (m_listAck.size() > MAX_DROP)
                        {
                            DEBUG_LOG("sink_calcerr",LL_ERROE,"mp_entity::pump_rtp_in| ptr_this[%p] SN[%d] - [%d]",this,m_lastSn, param.sequenceNumber);
                        }
                        while (m_listAck.size() > MAX_DROP)
                        {
                            m_listAck.erase(m_listAck.begin());
                        }
                        if (IsSmaller(m_lastSn, param.sequenceNumber))
                        {
                            m_lastSn = param.sequenceNumber;
                        }
                    }

                    // 更新已请求数据
                    bool bReSend= false;
                    std::vector<RcvSeg>::iterator it = m_listAck.begin();
                    for (;it != m_listAck.end();)
                    {
                        RcvSeg &segA = *it;

                        if (segA.sn == param.sequenceNumber)
                        {
                            it = m_listAck.erase(it);
                            bReSend = true;
                            continue;
                        }

                        if (IsSmaller(segA.sn, m_lastFrameMarkerSN))
                        {
                            it = m_listAck.erase(it);
                            continue;
                        }

                        if (segA.time == 0)
                        {
                            segA.time = WAIT_RESEND-5;
                        }
                        else
                        {
                            segA.time += 1;
                        }
                        ++it;
                    }

                    if (IsReSend(param) && !bReSend)
                    {
                        if (param.sequenceNumber-m_lastSn>1000)
                        {
                            DEBUG_LOG("sink_calcerr",LL_ERROE,"mp_entity::pump_rtp_in| ptr_this[%p] SN[%d] - [%d]",this,m_lastSn, param.sequenceNumber);
                        }

                        block->release();
                        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in| ptr_this[%p] drop2",this);
                        continue;
                    }
                }

                if (m_nReSendCfg > 0)
                {
                    boost::recursive_mutex::scoped_lock lock(m_mLost);
                    // 请求数据
                    std::vector<RcvSeg>::iterator it = m_listAck.begin();
                    char list_sn_buffer[1024];
                    memset(list_sn_buffer,0x0,1024);
                    uint32_t snsize =0;
                    for (int nR = 0;nR<MAX_RESEND && it!=m_listAck.end();++it,++nR)
                    {
                        RcvSeg &segA = *it;

                        if (segA.time >= WAIT_RESEND)
                        {
                            segA.time = 1;

                            // 屏蔽rtcp信道不发送数据bug(临时方案)
                            //////////////////////////////////////////////////////////////////////////
                            if (GetTickCount()-m_tmSrcAddr > m_tmSrcAddrInterval)
                            {
                                m_tmSrcAddr = GetTickCount();
                                add_srcaddr();
                            }
                            //////////////////////////////////////////////////////////////////////////

                            if (m_nReSendCfg == 1)
                            {
                                RtcpAppMessage msg;
                                uint8_t name[4] = {'N','A','C','K'};
                                msg.subtype = 1;
                                ::memcpy(msg.name, name, sizeof(name));

                                uint32_t sn = segA.sn;
                                msg.userData = (uint8_t*)&sn;
                                msg.userDataLength = sizeof(uint32_t);
                                RtcpSendApps(&m_handle, &msg, 1, false);
                            }
                            else
                            {
                                uint32_t sn = segA.sn;
                                ::memcpy(list_sn_buffer+snsize,(uint8_t*)&sn,sizeof(sn));
                                snsize+=sizeof(sn);
                            }                        
                        }
                    }
                    //不能发送空的重传请求
                    if( snsize > 0 &&list_sn_buffer)
                    {
                        RtcpAppMessage msg;
                        uint8_t name[4] = {'N','A','C','K'};
                        msg.subtype = 3;
                        ::memcpy(msg.name, name, sizeof(name));

                        boost::recursive_mutex::scoped_lock lock2(xt_mp_sink::mp_entity::m_mEntityClose);
                        msg.userData = (uint8_t*)&list_sn_buffer;
                        msg.userDataLength = snsize;

                        if (xt_mp_sink::mp_entity::is_valid(this))
                        {
                            RtcpSendApps(&m_handle, &msg, 1, false);
                        }
                    }
                }

                uint32_t newTick = (GetTickCount()-tickCount)*MP_PSEUDO_TS_CLOCK;
                if (!IsReSend(param))
                {
                    manual_send_rtcp_rr(&m_handle,param.sSrc,newTick,param.timestamp,param.sequenceNumber);

                    unsigned long time = GetTickCount();
                    if (time-m_tForceRR > m_tForceLevel)
                    {
                        m_tForceRR = time;
                        force_send_rtcp_rr(&m_handle, false, NULL, 0);
                    }
                }
                block->set_size(param.len);
                block->set_marker(param.marker);
                block->set_payload_type(param.payload);
                block->set_sbyte(param.sByte);
                block->set_sn(param.sequenceNumber);
                block->set_ssrc(param.sSrc);
                block->set_ts(param.timestamp);
                block->set_params(param.len-param.sByte,param.sByte);
                block->set_head_param(param);
                bool ret = m_rtp_fifo.push(block);
                if (!ret)
                {
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in | ptr_this[%p] m_rtp_fifo.push fail!",this);
                }
                block->release();
                DEBUG_LOG("sink_rtpin",LL_NORMAL_INFO, "mp_entity::pump_rtp_in| ptr_this[%p] 读取数据 - sn:%d ts:%d  pt:%d ds:%d fifozize:%d", this,param.sequenceNumber,param.timestamp, param.payload, param.len,m_rtp_fifo.size());
            }
            else
            {
                block->release();
                break;
            }
        } while (true);

        if (post_task)
        {
            this->assign();
            if (!sink_inst::sink_singleton()->tp_inst()->schedule(boost::bind(&mp_entity::mp_task_data_switcher,this)))
            {
                this->release();
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in| ptr_this[%p] sink_singleton()->tp_inst()->schedule fail!",this);
            }
        }
        this->release();
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::pump_rtp_in | ptr_this[%p] recv data end!",this);
    }

    void mp_entity::pump_rtp_in(RV_IN void *buf,
        RV_IN uint32_t buf_len,
        RV_IN rv_rtp_param *p,
        RV_IN rv_net_address *address)
    {
        //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
            if (!buf || !p || !address)
            {
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in1 | ptr_this[%p] buf p address is empty pro fail!",this);
                return;
            }

            DEBUG_LOG("sink_demux_rtpin",LL_NORMAL_INFO,"mp_open pump_rtp_in1 | ptr_this[%p] sn[%d] len[%d] sbyte[%d]",this,p->sequenceNumber, p->len, p->sByte);
            this->assign();
            int nRet = -1;
            rv_rtp_param param;
            ::memcpy(&param, p, sizeof(rv_rtp_param));
            bool post_task = true;
            do 
            {
                if(m_state != MP_OPEN_STATE || !m_bActive)
                {
                    DEBUG_LOG("sink_rtpin",LL_ERROE,"mp_entity::pump_rtp_in1 | ptr_this[%p] 读取数据 - 非活动状态!",this);
                    post_task = false;
                    break;
                }
                if (this->use_count() > usrcount_max_num)
                {
                    std::size_t _tp_nums = sink_inst::sink_singleton()->tp_inst()->pending();
                    std::size_t _post_tp_nums = sink_inst::sink_singleton()->post_tp_inst()->pending();
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in11 | ptr_this[%p] _tp_nums[%d] _post_tp_nums[%d] use_count fail!",this,_tp_nums,_post_tp_nums);
                    post_task = false;
                    break;
                }
                //模拟丢包
                if (m_nLostCfg>0)
                {
                    static int nN = 0;
                    if ((nN*m_nLostCfg)%100==0)
                    {
                        nN += 1;
                        post_task = false;
                        break;
                    }
                    nN += 1;
                }

                if (m_rtp_fifo.size() > LEN_RTP_CACHE)
                {
                    m_rtp_fifo.pop(true);
                }

                if (m_snd_out_fifo.size() > LEN_RTP_CACHE)
                {
                    m_snd_out_fifo.pop(true);
                }

                if (m_sink.m_queue.size() > LEN_RTP_CACHE)
                {
                    m_sink.pop(true);
                }

                static uint32_t tickCount = GetTickCount();
                rtp_packet_block* block = alloc_rtp_block();
                if (!block || block->size()<param.len)
                {
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in1 | ptr_this[%p] get memory fail!",this);
                    post_task = false;
                    break;
                }

                block->assign();
                block->m_bFrame = false;
                ::memcpy(block->get_raw(), buf, param.len);

                boost::unique_lock<boost::recursive_mutex> lock(m_member_variable_mutex);
                m_ssrc = param.sSrc;
                if (!m_bInitLast)
                {
                    m_bInitLast = true;
                    m_lastSn = param.sequenceNumber;
                }

                if (!param.extensionBit)
                {
                    param.extensionLength = 0;
                    param.extensionData = NULL;
                }

                //////////////////////////////////////////////////////////////////////////
                if (param.extensionBit && param.extensionData)
                { 
                    uint32_t info[3];

                    if (IsReSend(param))
                    {
                        if (param.extensionLength >= 4)
                        {
                            block->m_bFrame = true;
                            info[0] = param.extensionData[1];
                            info[1] = param.extensionData[2];
                            info[2] = param.extensionData[3];
                        }
                    }
                    else
                    {
                        if (param.extensionLength >= 3)
                        {
                            block->m_bFrame = true;
                            info[0] = param.extensionData[0];
                            info[1] = param.extensionData[1];
                            info[2] = param.extensionData[2];
                        }
                    }

                    if (block->m_bFrame)
                    {
                        block->m_frame[0] = ::ntohl(info[0]);
                        block->m_frame[1] = ::ntohl(info[1]);
                        block->m_frame[2] = ::ntohl(info[2]);
                    }
                }
                //////////////////////////////////////////////////////////////////////////*/

                // RESEND
                if (IsReSend(param))
                {
                    if (m_nReSendCfg <= 0)
                    {
                        block->release();
                        continue;
                    }
                }

                if (m_nReSendCfg>0)
                {
                    boost::recursive_mutex::scoped_lock lock(m_mLost);
                    if (!IsReSend(param))
                    {
                        if (CalDif(m_lastSn, param.sequenceNumber)>=MAX_DROP)
                        {
                            m_lastSn = param.sequenceNumber;
                        }
                        //CalLost(m_listAck, m_lastSn, param.sequenceNumber);
                        if (m_listAck.size() > MAX_DROP)
                        {
                            DEBUG_LOG("sink_calcerr",LL_ERROE,"mp_open pump_rtp_in1 | ptr_this[%p] SN[%d] - [%d]",this,m_lastSn, param.sequenceNumber);
                        }

                        while (m_listAck.size() > MAX_DROP)
                        {
                            m_listAck.erase(m_listAck.begin());
                        }

                        if (IsSmaller(m_lastSn, param.sequenceNumber))
                        {
                            m_lastSn = param.sequenceNumber;
                        }
                    }

                    // 更新已请求数据
                    bool bReSend= false;
                    std::vector<RcvSeg>::iterator it = m_listAck.begin();
                    for (;it != m_listAck.end();)
                    {
                        RcvSeg &segA = *it;

                        if (segA.sn == param.sequenceNumber)
                        {
                            it = m_listAck.erase(it);
                            bReSend = true;
                            continue;
                        }

                        if (IsSmaller(segA.sn, m_lastFrameMarkerSN))
                        {
                            it = m_listAck.erase(it);
                            continue;
                        }

                        if (segA.time == 0)
                        {
                            segA.time = WAIT_RESEND-5;
                        }
                        else
                        {
                            segA.time += 1;
                        }

                        ++it;
                    }

                    if (IsReSend(param) && !bReSend)
                    {
                        if (param.sequenceNumber-m_lastSn>1000)
                        {
                            DEBUG_LOG("sink_calcerr",LL_ERROE,"mp_open pump_rtp_in1 | ptr_this[%p] SN[%d] - [%d]",this,m_lastSn, param.sequenceNumber);
                        }

                        block->release();
                        continue;
                    }
                }

                
                if (m_nReSendCfg > 0)
                {
                    boost::recursive_mutex::scoped_lock lock(m_mLost);
                    // 请求数据
                    std::vector<RcvSeg>::iterator it = m_listAck.begin();
                    char list_sn_buffer[1024];
                    memset(list_sn_buffer,0x0,1024);
                    uint32_t snsize =0;
                    for (int nR = 0;nR<MAX_RESEND && it!=m_listAck.end();++it,++nR)
                    {
                        RcvSeg &segA = *it;

                        if (segA.time >= WAIT_RESEND)
                        {
                            segA.time = 1;

                            // 屏蔽rtcp信道不发送数据bug(临时方案)
                            //////////////////////////////////////////////////////////////////////////
                            if (GetTickCount()-m_tmSrcAddr > m_tmSrcAddrInterval)
                            {
                                m_tmSrcAddr = GetTickCount();
                                add_srcaddr();
                            }
                            //////////////////////////////////////////////////////////////////////////

                            if (m_nReSendCfg == 1)
                            {
                                RtcpAppMessage msg;
                                uint8_t name[4] = {'N','A','C','K'};
                                msg.subtype = 1;
                                ::memcpy(msg.name, name, sizeof(name));

                                uint32_t sn = segA.sn;
                                msg.userData = (uint8_t*)&sn;
                                msg.userDataLength = sizeof(uint32_t);
                                RtcpSendApps(&m_handle, &msg, 1, false);
                            }
                            else
                            {
                                uint32_t sn = segA.sn;
                                ::memcpy(list_sn_buffer+snsize,(uint8_t*)&sn,sizeof(sn));
                                snsize+=sizeof(sn);
                            }                        
                        }
                    }
                    //不能发送空的重传请求
                    if( snsize > 0 &&list_sn_buffer)
                    {
                        RtcpAppMessage msg;
                        uint8_t name[4] = {'N','A','C','K'};
                        msg.subtype = 3;
                        ::memcpy(msg.name, name, sizeof(name));

                        boost::recursive_mutex::scoped_lock lock2(xt_mp_sink::mp_entity::m_mEntityClose);
                        msg.userData = (uint8_t*)&list_sn_buffer;
                        msg.userDataLength = snsize;

                        if (xt_mp_sink::mp_entity::is_valid(this))
                        {
                            RtcpSendApps(&m_handle, &msg, 1, false);
                        }
                    }
                }

                uint32_t newTick = (GetTickCount()-tickCount)*MP_PSEUDO_TS_CLOCK;
                if (!IsReSend(param))
                {
                    manual_send_rtcp_rr(&m_handle,param.sSrc,newTick,param.timestamp,param.sequenceNumber);

                    unsigned long time = GetTickCount();
                    if (time-m_tForceRR > m_tForceLevel)
                    {
                        m_tForceRR = time;
                        force_send_rtcp_rr(&m_handle, false, NULL, 0);
                    }
                }

                block->set_size(param.len);
                block->set_marker(param.marker);
                block->set_payload_type(param.payload);
                block->set_sbyte(param.sByte);
                block->set_sn(param.sequenceNumber);
                block->set_ssrc(param.sSrc);
                block->set_ts(param.timestamp);
                block->set_params(param.len-param.sByte,param.sByte);
                block->set_head_param(param);
                m_rtp_fifo.push(block);

                block->release();

                DEBUG_LOG("sink_rtpin",LL_NORMAL_INFO,
                    "mp_open pump_rtp_in1 | ptr_this[%p] 读取数据 - sn:%d ts:%d  pt:%d ds:%d fifozize:%d",this, param.sequenceNumber,param.timestamp, param.payload, param.len,m_rtp_fifo.size());
            } while (0);

            if (post_task)
            {
                this->assign();
                if(!sink_inst::sink_singleton()->tp_inst()->schedule(boost::bind(&mp_entity::mp_task_data_switcher,this)))
                {
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in 2| ptr_this[%p] sink_singleton()->tp_inst()->schedule fail!",this);
                    this->release();
                }
            }
            this->release();
    }

    void mp_entity::pump_rtcp_in( rv_handler hrv,uint32_t ssrc )
    {
        //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        this->assign();
        do
        {
            if(m_state != MP_OPEN_STATE || !m_bActive)
            {
                DEBUG_LOG("pump_rtcp_in",LL_ERROE,"pump_rtcp_in | m_state=%d m_bActive=%d\n",m_state,m_bActive);
                break;
            }

            rv_rtcp_info info;
            if(!get_rtcp_sourceinfo(hrv, ssrc, &info))
            {
                DEBUG_LOG("pump_rtcp_in",LL_ERROE,"pump_rtcp_in |get_rtcp_sourceinfo error !!!!!\n");
                break;
            }

            if(info.sr.valid)
            {
                rtcp_send_report snd_rtcp;
                snd_rtcp.ssrc = ssrc;
                snd_rtcp.lNTPtimestamp = info.sr.lNTPtimestamp;
                snd_rtcp.mNTPtimestamp = info.sr.mNTPtimestamp;
                snd_rtcp.octets = info.sr.octets;
                snd_rtcp.packets = info.sr.packets;
                snd_rtcp.timestamp = info.sr.timestamp;
                m_snd_rtcp_fifo.try_push(snd_rtcp);
            }

            if(info.rrTo.valid)
            {
                rtcp_receive_report rcv_rtcp;
                rcv_rtcp.cumulativeLost = info.rrTo.cumulativeLost;
                rcv_rtcp.dlSR = info.rrTo.dlSR;
                rcv_rtcp.fractionLost = info.rrTo.fractionLost;
                rcv_rtcp.jitter = info.rrTo.jitter;
                rcv_rtcp.lSR = info.rrTo.lSR;
                rcv_rtcp.sequenceNumber = info.rrTo.sequenceNumber;
                rcv_rtcp.ssrc = ssrc;
                m_rcv_rtcp_fifo.try_push(rcv_rtcp);
            }
        } while (0);
        this->release();
    }

    uint32_t mp_entity::_transfer_video_byteblock()
    {
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::_transfer_video_byteblock| ptr_this[%p] start...",this);
        uint32_t ret = 0;
        do
        {
            boost::unique_lock<boost::recursive_mutex> lock(m_member_variable_mutex);
            rtp_packet_block * rtp = static_cast<rtp_packet_block *>(m_rtp_fifo.pop());
            if(rtp == null)
            {
                rtp_macro_block * block = alloc_macro_block();
                block->assign();
                pump_param p;
                p.mode = m_mode;
                p.queue = &m_snd_out_fifo;
                p.mblock = block;
                if(m_source.dataRecombine(rtp,m_lastFrameMarkerSN,p))
                {
                    m_sink.push(block);
                    ret++;
                }
                block->release();
                break;
            }
            DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::_transfer_video_byteblock| ptr_this[%p] runing....",this);

            rtp_macro_block * block = alloc_macro_block();
            block->assign();
            pump_param p;
            p.mode = m_mode;
            p.queue = &m_snd_out_fifo;
            p.mblock = block;
            if(m_source.dataRecombine(rtp,m_lastFrameMarkerSN,p))
            {
                m_sink.push(block);
                ret++;
            }
            block->release();
        } while (true);
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::_transfer_video_byteblock| ptr_this[%p] end!",this);
        return ret;
    }

    void mp_entity::_post_caster_task()
    {
        //三级流水
        switch (m_mode)
        {
        case MP_BOTH_MSINK:
        case MP_MEMORY_MSINK:
            {
                this->assign();
                if (!sink_inst::sink_singleton()->post_tp_inst()->schedule(boost::bind(&mp_entity::caster_data_out,this)))
                {
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::4| ptr_this[%p] sink_singleton()->tp_inst()->schedule fail!",this);
                    this->release();
                }
            }
            break;
        case MP_RV_RTP_MSINK:
            {
                this->assign();
                if (!sink_inst::sink_singleton()->tp_inst()->schedule(boost::bind(&mp_entity::send_data_out,this)))
                {
                    this->release();
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::3| ptr_this[%p] sink_singleton()->tp_inst()->schedule fail!",this);
                }
            }
            break;
        default:
            break;
        }
    }

    void mp_entity::mp_task_data_switcher()
    {
        //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::mp_task_data_switcher | ptr_this[%p] start..",this);
        do
        {
            if(m_state != MP_OPEN_STATE || !m_bActive)
            {
                DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::mp_task_data_switcher not active!ptr_this[%p]",this);
                break;
            }

            if(m_isDirectOutput)
            {
                if(m_pRcvRtpCB != null)
                {
                    DEBUG_LOG(SINK_DATA,LL_INFO,"mp_entity::mp_task_data_switcher | m_pRcvFrameCB ptr_this[%p] proc start...",this);
                    m_pRcvRtpCB(this,m_pContext);
                    DEBUG_LOG(SINK_DATA,LL_INFO,"mp_entity::mp_task_data_switcher | m_pRcvFrameCB ptr_this[%p] proc end!",this);
                }
            }
            else
            {
                if(_transfer_video_byteblock() > 0)
                {
                    DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::mp_task_data_switcher | 组帧成功 _post_caster_task!ptr_this[%p]",this);
                    //投递数据转出任务
                    _post_caster_task();
                }
            }
        } while (false);
        this->release();

        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::mp_task_data_switcher | ptr_this[%p] end!",this);
    }

    void mp_entity::caster_data_out()
    {
        //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::caster_data_out | ptr_this[%p] start..",this);
        if (!m_msink_task_mutex.timed_lock(
            boost::get_system_time() +
            boost::posix_time::millisec(MP_MSINK_TASK_LOCK_TM)))
        {
            this->release();
            DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::caster_data_out | ptr_this[%p] m_msink_task_mutex.timed_lock fail!",this);
            return;
        }
        bool bOk = false;
        do
        {
            if(m_state != MP_OPEN_STATE || !m_bActive)
            {
                DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::caster_data_out | ptr_this[%p] on active!",this);
                break;
            }
            //此处代码不会被执行（mp_task_data_switcher 在抛投任务之前会处理） 20160308 注释 
            //             if(m_isDirectOutput)
            //             {
            //                 if(m_pRcvRtpCB != null)
            //                 {
            //                     DEBUG_LOG(SINK_DATA,LL_INFO,"mp_entity::caster_data_out | m_pRcvFrameCB1 ptr_this[%p] proc  start...",this);
            //                     m_pRcvRtpCB(this,m_pContext);
            //                     bOk = true;
            //                     DEBUG_LOG(SINK_DATA,LL_INFO,"mp_entity::caster_data_out | m_pRcvFrameCB1 ptr_this[%p] proc end!",this);
            //                 }
            //             }
            if(NULL != m_pRcvFrameCB)
            {
                DEBUG_LOG(SINK_DATA,LL_INFO,"mp_entity::caster_data_out | m_pRcvFrameCB ptr_this[%p] proc  start...",this);
                m_nFrame++;
                m_pRcvFrameCB(this,m_pContext);
                bOk = true;
                DEBUG_LOG(SINK_DATA,LL_INFO,"mp_entity::caster_data_out | m_pRcvFrameCB ptr_this[%p] proc end!",this);
            }
            else
            {
                DEBUG_LOG(SINK_DATA,LL_ERROE,"mp_entity::m_pRcvFrameCB is null ptr_this[%p]",this);
            }

            //四级流水，控制数据已写rtp_session方式输出
            if(m_mode == MP_BOTH_MSINK)
            {
                this->assign();
                if (!sink_inst::sink_singleton()->tp_inst()->schedule(boost::bind(&mp_entity::send_data_out,this)))
                {
                    this->release();
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::5| ptr_this[%p] sink_singleton()->tp_inst()->schedule fail!",this);
                }
            }
        } while (0);

        if(!bOk && this != null) m_sink.clear();

        m_msink_task_mutex.unlock();

        this->release();

        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::caster_data_out | ptr_this[%p] end!",this);
    }

    void mp_entity::send_data_out()
    {
        //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::send_data_out | ptr_this[%p] start...",this);
        if (!m_mforword_task_mutex.timed_lock(
            boost::get_system_time() + boost::posix_time::millisec(MP_FORWORD_TASK_LOCK_TM)))
        {
            DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::send_data_out | ptr_this[%p] m_mforword_task_mutex.timed_lock fail!",this);
            this->release();
            return;
        }
        do
        {
            if(m_state != MP_OPEN_STATE || !m_bActive)
            {
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"pump_rtp_in_sj-mp_entity::send_data_out not active ptr_this[%p]",this);
                break;
            }

            uint32_t size = m_snd_out_fifo.size();
            if(size <= 0)
            {
                DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::send_data_out() 发送帧数据 发送队列为空!ptr_this[%p]",this);
                break;
            }

            for(uint32_t i=0;i<size;i++)
            {
                rtp_packet_block * item = static_cast<rtp_packet_block *>(m_snd_out_fifo.pop());
                if(item == null) break;
                rv_rtp_param param;
                construct_rv_rtp_param(&param);
                param.sequenceNumber = item->get_sn();
                param.len = item->get_size()-item->get_sbyte();
                param.marker = item->get_marker();
                param.payload = item->get_payload_type();
                param.sByte = item->get_sbyte();
                param.sSrc = item->get_ssrc();
                param.timestamp = item->get_ts();

                //::EnterCriticalSection(&m_csSession);
                //pthread_mutex_lock(&m_csSession);
                rv_bool bRet = write_rtp(&m_handle,item->get_raw(),item->get_size(),&param);
                //::LeaveCriticalSection(&m_csSession);
                //pthread_mutex_unlock(&m_csSession);
                item->release();
                DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::send_data_out() ptr_this[%p] 发送帧数据ts[%d] pt[%d] ds[%d]",this,param.timestamp, param.payload, param.len);
            }
        } while (0);
        m_mforword_task_mutex.unlock();
        this->release();
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::send_data_out | ptr_this[%p] end!",this);
    }
	long mp_entity::mp_pump_out_rtp(void **p_rtp_block)
	{
		long ret = -1;
		do
		{
			itr_rtp_packet_t itr = map_rtp_packets.begin();
			if (itr==map_rtp_packets.end())
			{
				ret =-2;
				break;
			}

			rtp_block *rtp = itr->second;
			if (!rtp)
			{
				ret =-3;
				break;
			}
			*p_rtp_block = rtp;
			map_rtp_packets.erase(itr);
			if (rtp->m_rtp_param.marker == 1)
			{
				--wait_frames;
				ret = 1;
				break;
			}
			ret = 0;
		} while (0);
		return ret;
	}
    long mp_entity::mp_read_out_rtp( uint8_t *pDst,uint32_t size,rv_rtp_param *param)
    {
        long ret = -1;
        do
        {
			itr_rtp_packet_t itr = map_rtp_packets.begin();
			if (itr==map_rtp_packets.end())
			{
				ret =-2;
				break;
			}
			rtp_block *rtp = itr->second;
			map_rtp_packets.erase(itr);
			if (rtp == NULL)
			{
				ret =-3;
				break;
			}

			uint32_t data_size = rtp->m_rtp_param.len - rtp->m_rtp_param.sByte;
			if(data_size > size)
			{
				DEBUG_LOG(SINK_CALL,LL_ERROE, "mp_entity::mp_read_out_rtp| ptr_this[%p] mp_read_out_data fail:数据大小>拷贝缓冲区大小!",this);
				rtp->release();
				ret = -4;
				break;
			}
			::memcpy(param,&(rtp->m_rtp_param),sizeof(rv_rtp_param));
			rtp->read(pDst,size);

			//上层已做处理，读到mark就停止
			if(rtp->m_rtp_param.marker == 1)
			{
				--wait_frames;
				ret = 1;
				break;
			}
			ret = 0;
        } while (0);
        return ret;
    }

    long mp_entity::mp_read_out_data( uint8_t * pDst,uint32_t size,block_params * param )
    {
        long ret = -1;
        do
        {
            rtp_macro_block * macro = m_sink.pop();
            if(macro == null)
            {
                ret = -2;
                break;
            }
            uint32_t data_size = macro->get_data_size();
            if(data_size > size)
            {
                DEBUG_LOG(SINK_CALL,LL_ERROE,"mp_entity::mp_read_out_data| ptr_this[%p] fail:数据大小>拷贝缓冲区大小!",this);
                macro->release();
                ret = -3;
                break;
            }
            param->payload_type = macro->get_payload_type();
            param->size = data_size;
            param->ssrc = macro->get_ssrc();
            param->timestamp = macro->get_timestamp();
            macro->read(pDst,data_size);
            macro->release();

            ret = 0;
        } while (0);
        return ret;
    }
    long mp_entity::mp_read_out_data2( uint8_t * pDst,uint32_t size,block_params * param ,uint32_t *frame)
    {
        long ret = -1;
        do
        {
            rtp_macro_block * macro = m_sink.pop();
            if(macro == null)
            {
                ret=-2;
                break;
            }
            uint32_t data_size = macro->get_data_size();
            if(data_size > size)
            {
                DEBUG_LOG(SINK_CALL,LL_ERROE,"mp_entity::mp_read_out_data2|ptr_this[%p] fail:数据大小>拷贝缓冲区大小!",this);
                macro->release();
                //added by lichao, 20151127 当外入的缓存不足时 返回实际数据大小
                ret = static_cast<long>(data_size);
                break;
            }
            param->payload_type = macro->get_payload_type();
            param->size = data_size;
            param->ssrc = macro->get_ssrc();
            param->timestamp = macro->get_timestamp();
            macro->read(pDst,data_size);

            //////////////////////////////////////////////////////////////////////////
            rtp_packet_block *rtp = macro->get_first_block();
            if (rtp && rtp->m_bFrame && frame)
            {
                ::memcpy(frame, rtp->m_frame, sizeof(rtp->m_frame));
            }
            //////////////////////////////////////////////////////////////////////////

            macro->release();
            ret = 0;
        } while (false);
        return ret;
    }

    void mp_entity::_clear()
    {
        while (m_sink.pop(true))
        {
        }

        while (m_snd_out_fifo.pop(true))
        {
        }

        while (m_rtp_fifo.pop(true))
        {
        }
		wait_frames = 0;
		map_rtp_packets.clear();
        m_source.clear();
        m_listAck.clear();
        m_bInitLast = false;
        m_lastFrameMarkerSN = LAST_FRAME_SN_INIT;
    }

    long mp_entity::mp_add_rtp_remote_address( mp_address_descriptor * address )
    {
        rv_net_address rv_address;
        _construct_rv_address(address->ip_address,address->port,&rv_address);

        // 保存发送端地址
        _construct_rv_address(address->ip_address,address->port,&m_srcaddr.rtpaddr);
        m_srcaddr.multiplex = false;

        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = add_rtp_remote_address(&m_handle,&rv_address);
        //::LeaveCriticalSection(&m_csSession);
        /*//////////////////////////////////////////////////////////////////////////
        char natbuf[96];
        rv_rtp_param rtp_param;
        ::construct_rv_rtp_param(&rtp_param);
        rtp_param.sByte = 12;
        rtp_param.sequenceNumber = 0;
        rtp_param.payload = 96;
        rtp_param.timestamp = 0;
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        //////////////////////////////////////////////////////////////////////////*/

        return ret;
    }

    long mp_entity::mp_add_mult_rtp_remote_address( mp_address_descriptor * address, uint32_t multid )
    {
        rv_net_address rv_address;
        _construct_rv_address(address->ip_address,address->port,&rv_address);

        // 保存发送端地址
        _construct_rv_address(address->ip_address,address->port,&m_srcaddr.rtpaddr);
        m_srcaddr.multiplex = true;
        m_srcaddr.multid = multid;

        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = add_rtp_mult_remote_address(&m_handle,&rv_address, multid);
        //::LeaveCriticalSection(&m_csSession);
        /*//////////////////////////////////////////////////////////////////////////
        char natbuf[96];
        rv_rtp_param rtp_param;
        ::construct_rv_rtp_param(&rtp_param);
        rtp_param.sByte = 16;
        rtp_param.sequenceNumber = 0;
        rtp_param.payload = 96;
        rtp_param.timestamp = 0;		
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        write_rtp(&m_handle, natbuf, 32, &rtp_param);
        //////////////////////////////////////////////////////////////////////////*/

        return ret;
    }

    long mp_entity::mp_del_rtp_remote_address( mp_address_descriptor * address )
    {
        rv_net_address rv_address;
        _construct_rv_address(address->ip_address,address->port,&rv_address);

        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = del_rtp_remote_address(&m_handle,&rv_address);
        //::LeaveCriticalSection(&m_csSession);
        //pthread_mutex_unlock(&m_csSession);

        return ret;
    }

    long mp_entity::mp_clear_rtp_remote_address()
    {
        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = clear_rtp_remote_address(&m_handle);
        //::LeaveCriticalSection(&m_csSession);
        //pthread_mutex_unlock(&m_csSession);

        return ret;
    }

    long mp_entity::mp_add_rtcp_remote_address( mp_address_descriptor * address )
    {
        rv_net_address rv_address;
        _construct_rv_address(address->ip_address,address->port,&rv_address);

        // 保存发送端地址
        _construct_rv_address(address->ip_address,address->port,&m_srcaddr.rtcpaddr);
        m_srcaddr.multiplex = false;

        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = add_rtcp_remote_address(&m_handle,&rv_address);
        //::LeaveCriticalSection(&m_csSession);
        //pthread_mutex_unlock(&m_csSession);

        return ret;
    }

    long mp_entity::mp_del_rtcp_remote_address( mp_address_descriptor * address )
    {
        rv_net_address rv_address;
        _construct_rv_address(address->ip_address,address->port,&rv_address);

        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = del_rtcp_remote_address(&m_handle,&rv_address);
        //::LeaveCriticalSection(&m_csSession);
        //pthread_mutex_unlock(&m_csSession);

        return ret;
    }

    long mp_entity::mp_add_mult_rtcp_remote_address( mp_address_descriptor * address, uint32_t multid )
    {
        rv_net_address rv_address;
        _construct_rv_address(address->ip_address,address->port,&rv_address);

        // 保存发送端地址
        _construct_rv_address(address->ip_address,address->port,&m_srcaddr.rtcpaddr);
        m_srcaddr.multiplex = true;
        m_srcaddr.multid = multid;

        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = add_rtcp_mult_remote_address(&m_handle,&rv_address, multid);
        //::LeaveCriticalSection(&m_csSession);
        //pthread_mutex_unlock(&m_csSession);

        return ret;
    }

    long mp_entity::mp_clear_rtcp_remote_address()
    {
        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = clear_rtcp_remote_address(&m_handle);
        //::LeaveCriticalSection(&m_csSession);
        //pthread_mutex_unlock(&m_csSession);
        return ret;
    }

    long mp_entity::mp_query_rcv_rtcp(rtcp_receive_report * rtcp)
    {
        long ret = -1;
        do
        {
            if(rtcp == null)
            {
                ret=-2;
                break;
            }
            if(m_rcv_rtcp_fifo.size() <= 0)
            {
                ret=-3;
                break;
            }
            rtcp_receive_report& obj = m_rcv_rtcp_fifo.front();
            rtcp->cumulativeLost = obj.cumulativeLost;
            rtcp->dlSR = obj.dlSR;
            rtcp->fractionLost = obj.fractionLost;
            rtcp->jitter = obj.jitter;
            rtcp->lSR = obj.lSR;
            rtcp->sequenceNumber = obj.sequenceNumber;
            rtcp->ssrc = obj.ssrc;
            m_rcv_rtcp_fifo.pop();
            ret = 0;
        } while (false);

        return ret;
    }

    long mp_entity::mp_query_snd_rtcp( rtcp_send_report * rtcp )
    {
        long ret = -1;
        do
        {
            if(rtcp == null) break;
            if(m_snd_rtcp_fifo.size() <= 0) break;
            rtcp_send_report& obj = m_snd_rtcp_fifo.front();
            rtcp->lNTPtimestamp = obj.lNTPtimestamp;
            rtcp->mNTPtimestamp = obj.mNTPtimestamp;
            rtcp->octets = obj.octets;
            rtcp->packets = obj.packets;
            rtcp->ssrc = obj.ssrc;
            rtcp->timestamp = obj.timestamp;
            m_snd_rtcp_fifo.pop();
            ret = 0;
        } while (false);

        return ret;
    }

    long mp_entity::mp_manual_send_rtcp_sr( uint32_t pack_size,uint32_t pack_ts )
    {
        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        uint32_t ret = manual_send_rtcp_sr(&m_handle,pack_size,pack_ts);
        //::LeaveCriticalSection(&m_csSession);
        //pthread_mutex_unlock(&m_csSession);

        return ret;
    }

    long mp_entity::mp_manual_send_rtcp_rr( uint32_t ssrc,uint32_t local_ts,uint32_t ts, uint32_t sn )
    {
        //::EnterCriticalSection(&m_csSession);
        //pthread_mutex_lock(&m_csSession);
        long ret = manual_send_rtcp_rr(&m_handle,ssrc,local_ts,ts,sn);
        //::LeaveCriticalSection(&m_csSession);
        //pthread_mutex_unlock(&m_csSession);

        return ret;
    }

    long mp_entity::mp_rtcp_send_dar( uint32_t rate, uint32_t level)
    {
        RtcpAppMessage msg;
        uint8_t name[4] = {'D','A','R',' '};
        msg.subtype = 2;
        ::memcpy(msg.name, name, sizeof(name));

        uint32_t data[2];
        ::memcpy(data, &rate, sizeof(uint32_t));
        ::memcpy(data+sizeof(uint32_t), &level, sizeof(uint32_t));
        msg.userData = (uint8_t*)&data;
        msg.userDataLength = 2*sizeof(uint32_t);

        return RtcpSendApps(&m_handle, &msg, 1, false);
    }

    /*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |V=2|P|   FMT   |       PT      |          length               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                  SSRC of packet sender                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                  SSRC of media source                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    :            Feedback Control Information (FCI)                 ://*/
	//rtcp发送I帧请求
    long mp_entity::rtcp_send_fir()
    {
        const int num = 1;//FIR entry num
        const int len = 2*num+2;
        const int size = 12 + 8*num;
        unsigned char fir[size] = {0};
        unsigned char *ssrc_self = (unsigned char *)&m_rtcp_ssrc;
        unsigned char *ssrc_sender = (unsigned char *)&m_ssrc;

        fir[0] = bitfieldSet(fir[0], 2, 6, 2);//V=2
        fir[0] = bitfieldSet(fir[0], 0, 5, 1);//PAD=0
        fir[0] = bitfieldSet(fir[0], 4, 0, 5);//FMT=4
        fir[1] = bitfieldSet(fir[1], 206, 0, 8);//PT=PSFB
        fir[3] = bitfieldSet(fir[3], len, 0, 8);
        fir[16] = bitfieldSet(fir[16], m_fir_seq++, 0, 8);//seq

        fir[4] = bitfieldSet(fir[4], *(ssrc_self+3), 0, 8);
        fir[5] = bitfieldSet(fir[5], *(ssrc_self+2), 0, 8);
        fir[6] = bitfieldSet(fir[6], *(ssrc_self+1), 0, 8);
        fir[7] = bitfieldSet(fir[7], *ssrc_self, 0, 8);
        fir[8] = bitfieldSet(fir[8], *(ssrc_sender+3), 0, 8);
        fir[9] = bitfieldSet(fir[9], *(ssrc_sender+2), 0, 8);
        fir[10] = bitfieldSet(fir[10], *(ssrc_sender+1), 0, 8);
        fir[11] = bitfieldSet(fir[11], *ssrc_sender, 0, 8);
        fir[12] = bitfieldSet(fir[12], *(ssrc_sender+3), 0, 8);
        fir[13] = bitfieldSet(fir[13], *(ssrc_sender+2), 0, 8);
        fir[14] = bitfieldSet(fir[14], *(ssrc_sender+1), 0, 8);
        fir[15] = bitfieldSet(fir[15], *ssrc_sender, 0, 8);		

        return rtcp_send_rawdata(&m_handle, fir, size);
    }

    //查询SR报告
    long mp_entity::mp_get_xtsr(int &nDeviceType,
        int &nLinkRet,
        int &nFrameType,
        XTSSendReport *rtcp)
    {
        if (!rtcp)
        {
            return -1;
        }

        if (!m_bManualSR_cfg)
        {
            rtcp_send_report sr;
            ::memset(&sr, 0, sizeof(rtcp_send_report));
            mp_query_snd_rtcp(&sr);

            nDeviceType = 0;
            nLinkRet = 0;
            nFrameType = 0;
            rtcp->lNTPtimestamp = sr.lNTPtimestamp;
            rtcp->mNTPtimestamp = sr.mNTPtimestamp;
            rtcp->nOctets = sr.octets;
            rtcp->nPackets = sr.packets;
            rtcp->nSSRC = sr.ssrc;
            rtcp->nRtpTimestamp = sr.timestamp;
        }
        else
        {
            nDeviceType = m_xtsr.nDeviceType;
            nLinkRet = m_xtsr.nLinkRet;
            nFrameType = m_xtsr.nFrameType;
            rtcp->lNTPtimestamp = m_xtsr.sr.lNTPtimestamp;
            rtcp->mNTPtimestamp = m_xtsr.sr.mNTPtimestamp;
            rtcp->nOctets = m_xtsr.sr.nOctets;
            rtcp->nPackets = m_xtsr.sr.nPackets;
            rtcp->nSSRC = m_xtsr.sr.nSSRC;
            rtcp->nRtpTimestamp = m_xtsr.sr.nRtpTimestamp;
        }

        return 0;
    }

    bool mp_entity::get_multinfo(bool *multiplex, uint32_t *multid)
    {
        if (!multiplex || !multid)
        {
            return false;
        }

        *multiplex = m_bMultiplex;
        *multid = m_multID;
        return true;
    }

    int  mp_entity::RegistSendReportEvent(FPSendReportOutput fpSendReportOutput, void *objUserContext)
    {
        m_funcSR = fpSendReportOutput;
        m_puserdata = objUserContext;
        return 0;
    }

    // 添加发送端地址
    void mp_entity::add_srcaddr()
    {
        if (m_srcaddr.multiplex)
        {
            add_rtp_mult_remote_address(&m_handle, &m_srcaddr.rtpaddr, m_srcaddr.multid);
            add_rtcp_mult_remote_address(&m_handle, &m_srcaddr.rtcpaddr, m_srcaddr.multid);
        }
        else
        {
            add_rtp_remote_address(&m_handle, &m_srcaddr.rtpaddr);
            add_rtcp_remote_address(&m_handle, &m_srcaddr.rtcpaddr);
        }
    }

     void mp_entity::update_srcaddr_rtp(rv_net_address &addr)
    {
        rv_net_ipv4 ip1,ip2;
        convert_rvnet_to_ipv4(&ip1, &addr);
        convert_rvnet_to_ipv4(&ip2, &m_srcaddr.rtpaddr);
        if (ip1.port == ip2.port &&
            ip1.ip == ip2.ip)
        {
            return;
        }

        ::memcpy(&m_srcaddr.rtpaddr, &addr, sizeof(rv_net_address));
        if (m_srcaddr.multiplex)
        {
            clear_rtp_remote_address(&m_handle);
            add_rtp_mult_remote_address(&m_handle, &m_srcaddr.rtpaddr, m_srcaddr.multid);
        }
        else
        {
            clear_rtp_remote_address(&m_handle);
            add_rtp_remote_address(&m_handle, &m_srcaddr.rtpaddr);
        }
    }

    void mp_entity::update_srcaddr_rtcp(rv_net_address &addr)
    {
        rv_net_ipv4 ip1,ip2;
        convert_rvnet_to_ipv4(&ip1, &addr);
        convert_rvnet_to_ipv4(&ip2, &m_srcaddr.rtcpaddr);
        if (ip1.port == ip2.port &&
            ip1.ip == ip2.ip)
        {
            return;
        }

        ::memcpy(&m_srcaddr.rtcpaddr, &addr, sizeof(rv_net_address));
        if (m_srcaddr.multiplex)
        {
            clear_rtcp_remote_address(&m_handle);
            add_rtcp_mult_remote_address(&m_handle, &m_srcaddr.rtcpaddr, m_srcaddr.multid);
        }
        else
        {
            clear_rtcp_remote_address(&m_handle);
            add_rtcp_remote_address(&m_handle, &m_srcaddr.rtcpaddr);
        }
    }

    //句柄管理
    ////////////////////////////////////////////////////////////////////
    void mp_entity::add_entity(void *entity)
    {
        boost::recursive_mutex::scoped_lock lock(m_mEntity);
        m_setSinkHandleMrg.insert(entity);
    }

    void mp_entity::del_entity(void *entity)
    {
        boost::recursive_mutex::scoped_lock lock(m_mEntity);
        m_setSinkHandleMrg.erase(entity);
    }

    int mp_entity::size_entity()
    {
        int count = 0;

        boost::recursive_mutex::scoped_lock lock(m_mEntity);
        count = m_setSinkHandleMrg.size();

        return count;
    }

    bool mp_entity::is_valid(void *entity)
    {
        if (size_entity() == 0)
        {
            return false;
        }

        boost::recursive_mutex::scoped_lock lock(m_mEntity);
        if (m_setSinkHandleMrg.end() == m_setSinkHandleMrg.find(entity))
        {
            return false;
        }

        return true;
    }
    ///////////////////////////////////////////////////////////////////////
	int mp_entity::do_resend(bool resend,rv_rtp_param param)
	{
		//丢包重传群发导致的不请自来的rtp包
		if (m_nReSendCfg <= 0 && resend)
		{
			DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in_sj| ptr_this[%p] drop1",this);
			return -1;
		}

		if (m_nReSendCfg>0)
		{
			//sn在标准rtp中为有序，中间有缺失作为丢包的计算
			if (CalDif(m_lastSn, param.sequenceNumber)>=MAX_DROP)
			{
				m_lastSn = param.sequenceNumber;
			}
			vector<uint16_t> vt_lost_sn;
			CalLost(vt_lost_sn, m_lastSn, param.sequenceNumber);
			lost_packet_count += vt_lost_sn.size();
			if (lost_packet_count > MAX_DROP)
			{
				DEBUG_LOG("sink_calcerr",LL_ERROE,"mp_entity::pump_rtp_in_sj| ptr_this[%p]SN[%d] - [%d]",this, param.sequenceNumber, m_lastSn);
			}
			if (IsSmaller(m_lastSn, param.sequenceNumber))
			{
				m_lastSn = param.sequenceNumber;
			}
			boost::recursive_mutex::scoped_lock lock(m_mLost);
			// 请求数据
			std::vector<uint16_t>::iterator itr = vt_lost_sn.begin();
			char list_sn_buffer[1024];
			uint32_t sn_buffer_len =0;
			for (int nR = 0;nR<MAX_RESEND && itr!=vt_lost_sn.end();++itr,++nR)
			{
				// 屏蔽rtcp信道不发送数据bug(临时方案)
				//////////////////////////////////////////////////////////////////////////
				if (GetTickCount()-m_tmSrcAddr > m_tmSrcAddrInterval)
				{
					m_tmSrcAddr = GetTickCount();
					add_srcaddr();
				}
				//////////////////////////////////////////////////////////////////////////
				//每次发送一个丢包sn
				if (m_nReSendCfg == 1)
				{
					RtcpAppMessage msg;
					uint8_t name[4] = {'N','A','C','K'};
					msg.subtype = 1;
					::memcpy(msg.name, name, sizeof(name));

					uint32_t sn = *itr;
					msg.userData = (uint8_t*)&sn;
					msg.userDataLength = sizeof(uint32_t);
					RtcpSendApps(&m_handle, &msg, 1, false);
				}
				else
				{
					uint32_t sn = *itr;
					::memcpy(list_sn_buffer+sn_buffer_len,(uint8_t*)&sn,sizeof(sn));
					sn_buffer_len+=sizeof(sn);
				}        
			}
			//一次aapmsg发送很多丢包sn
			if( sn_buffer_len > 0 )
			{
				RtcpAppMessage msg;
				uint8_t name[4] = {'N','A','C','K'};
				msg.subtype = 3;
				::memcpy(msg.name, name, sizeof(name));

				boost::recursive_mutex::scoped_lock lock2(xt_mp_sink::mp_entity::m_mEntityClose);
				msg.userData = (uint8_t*)&list_sn_buffer;
				msg.userDataLength = sn_buffer_len;

				if (xt_mp_sink::mp_entity::is_valid(this))
				{
					RtcpSendApps(&m_handle, &msg, 1, false);
				}
			}
		}

		return 0;
	}

	void mp_entity::erase_front_frame()
	{
		while ( !map_rtp_packets.empty() )
		{
			itr_rtp_packet_t itr = map_rtp_packets.begin();
			if (itr==map_rtp_packets.end())
			{
				break;
			}
			rtp_block *rtp = itr->second;
			map_rtp_packets.erase(itr++);
			if (!rtp) break;
			if (rtp->m_rtp_param.marker == 1)
			{
				m_lastFrameMarkerSN = rtp->m_rtp_param.sequenceNumber;
				--wait_frames;
				break;
			}
		}

		if (map_rtp_packets.empty())
		{
			m_lastFrameMarkerSN = LAST_FRAME_SN_INIT;
		}
	}
	
	int mp_entity::check_jarless(uint16_t rtp_sn)
	{
		_lastseq++;//3,4,5//上一个为4，可能来3或者5
		if ( _lastseq != rtp_sn )
		{
			_lastseq--;//只是打日志用
			DEBUG_LOG("sink_乱序",LL_ERROE,"mp_task_data_switcher_sj | ptr_this[%p] last_seq = %d   current_seq = %d",this,_lastseq, rtp_sn );
		}
		_lastseq = rtp_sn;
		m_nIncoming++;//记录到来了一个数据包
		//允许的乱序差阈值
		if ( m_jarlessPackets > 0 )//1
		{
			if ( m_jPackets > 0 )//0
			{
				if ( m_nIncoming < m_jPackets)
				{
					return -1;
				}
				else
				{
					m_nIncoming = 0;
				}
			}
			else
			{
				// 达到同步包数 重置抖动缓冲包数
				if ( m_nIncoming > m_syncPackets )//300
				{
					m_jPackets  = m_jarlessPackets;
					m_nIncoming = 0;
				}
			}
		}

		return 0;
	}

	int mp_entity::deal_rtp_packet(uint16_t in_sn,bool in_mark,bool resend,rtp_block* block)
	{
		// 不知道上一帧的情况下，等待第一帧的mark包
		if ( m_lastFrameMarkerSN == LAST_FRAME_SN_INIT )
		{
			if ( in_mark )
			{
				m_nIncoming = 0;
				m_lastFrameMarkerSN = in_sn;
				_lastseq = in_sn;
				DEBUG_LOG("sink reset seq",LL_NORMAL_INFO,"mp_task_data_switcher_sj | ptr_this[%p] reset seq!",this);
			}
			return -1;
		}

		// sn跨度超过MP_SN_JUDGE_CONDITION的包丢弃
		if((uint16_t)(in_sn - m_lastFrameMarkerSN) > MP_SN_JUDGE_CONDITION ||
			in_sn == m_lastFrameMarkerSN)
		{
			return -2;
		}
		// 可能rtp包始终有 mark = 0 ( 声音静默 ?? ) 故此
		// 声音rtp包始终是 mark包

		//此处投递任务必须是串行的
		boost::unique_lock<boost::recursive_mutex> lock(m_member_variable_mutex);
		if (!(map_rtp_packets.insert(make_pair(in_sn, block)).second))
		{
			return -8;
		}
		//包抖动检测
		/*if (check_jarless(in_sn) < 0)
		{
			return -4;
		}*/
		
		//如果缓冲区满了就删掉最前面一帧
		if (map_rtp_packets.size() > LEN_RTP_CACHE)
		{
			printf("rtpnums:%d\n",map_rtp_packets.size());
			erase_front_frame();
		}
		//mark的投递任务，不让线程那么忙
		if (in_mark || resend)
		{
			if (in_mark) ++wait_frames;
			bool bflags = false;
			do 
			{
				bflags = false;
				uint16_t seq = m_lastFrameMarkerSN;
				itr_rtp_packet_t itr = map_rtp_packets.begin();
				for (;itr!=map_rtp_packets.end();++itr)
				{
					++seq;
					//遍历sn是否连续
					if ( (itr->second)->m_rtp_param.sequenceNumber != seq)  // seq 不连续 
					{
						bflags = false;
						break;
					}
					//抛出完整一帧
					if ( (itr->second)->m_rtp_param.marker == 1 )
					{
						m_lastFrameMarkerSN = (itr->second)->m_rtp_param.sequenceNumber;
						bflags = true;
						break;
					}
				}
				if (bflags)
				{
					m_pRcvRtpCB( this, m_pContext);// 回调事件
					continue;
				}
				if (wait_frames >= m_waitFrames)
				{
					printf("this:%p map_rtp_packets:%d\n",this,map_rtp_packets.size());
					printf("wait_frames sn:%d\n",in_sn);
					erase_front_frame();
					continue;
				}
			} while (bflags);
		}

		return 0;
	}
    //////////////////////////////////////////////////////////////////////////
	//标准非复用流
    void mp_entity::pump_rtp_in_sj( rv_handler hrv )
    {
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::pump_rtp_in_sj | ptr_this[%p] recv start ...",this);
        rv_rtp_param param;
        this->assign();
        do 
        {
            static uint32_t tickCount = GetTickCount();
            rtp_block* block = &m_block_pool[m_block_index];
            if (block == NULL)
            {
				m_pump_flags = false;
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in_sj | ptr_this[%p] get memory fail!",this);
                this->release();
                return;
            }

            rv_net_address addr;
            ::memset(&param, 0, sizeof(rv_rtp_param));
            rv_bool read_result = read_rtp(hrv,block->get_raw(),block->size(),&param,&addr);
			if (RV_ADAPTER_TRUE != read_result)
			{
				m_pump_flags = true;
				break;
			}
			else if(read_result == RV_ADAPTER_TRUE)
            {
                m_ssrc = param.sSrc;

                if (!m_bInitLast)
                {
                    m_bInitLast = true;
                    m_last_rtp_sn = param.sequenceNumber;
                }
                //////////////////////////////////////////////////////////////////////////
				bool isResend = IsReSend(param);
				if (do_resend(isResend,param) < 0)
				{
					continue;
				}
				if (!isResend )
				{
					uint32_t newTick = (GetTickCount()-tickCount)*MP_PSEUDO_TS_CLOCK;
					manual_send_rtcp_rr(&m_handle,param.sSrc,newTick,param.timestamp,param.sequenceNumber);

					unsigned long time = GetTickCount();
					if (time-m_tForceRR > m_tForceLevel)
					{
						m_tForceRR = time;
						force_send_rtcp_rr(&m_handle, false, NULL, 0);
					}
				}
				
                block->set_params(param.len-param.sByte,param.sByte);
                block->set_rtp_param(&param);
				//注意序号为1023,0,1的情况，每组帧完成一次，计数从0重新开始
				if (++m_block_index == LEN_RTP_CACHE)
				{
					m_block_index = 0;
				}
				///////////////////////////////////////////////////////////////////
				long ret = deal_rtp_packet(param.sequenceNumber,param.marker==1?true:false,isResend,block);
				if ( ret < 0 )
				{
				}
                DEBUG_LOG("sink_rtpin",LL_NORMAL_INFO,"mp_entity::pump_rtp_in_sj| ptr_this[%p]读取数据 - sn:%d ts:%d  pt:%d ds:%d fifozize:%d", this,param.sequenceNumber,param.timestamp, param.payload, param.len,m_rtp_fifo.size());
            }
        } while (true);

        this->release();
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::pump_rtp_in_sj | ptr_this[%p] recv data end!",this);
    }

	//标准非复用流
    void mp_entity::mp_task_data_switcher_sj()
    {
        if ( m_state == MP_OPEN_STATE && m_bActive )
        {
			// 抛出一完整首帧
			if (m_isDirectOutput)
			{
				if( m_pRcvRtpCB )
				{
					DEBUG_LOG(SINK_DATA,LL_INFO,"mp_entity::mp_task_data_switcher_sj | m_pRcvFrameCB ptr_this[%p] proc  start...",this);
					//该锁只是为了限速，后面处理速度必然小于数据接收速度
					boost::unique_lock<boost::recursive_mutex> lock(m_member_variable_mutex);
					m_pRcvRtpCB( this, m_pContext);// 回调事件
					--wait_frames;
					DEBUG_LOG(SINK_DATA,LL_INFO,"mp_entity::mp_task_data_switcher_sj | m_pRcvFrameCB ptr_this[%p] proc  end!",this);
				}
			}
		}
        this->release();
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::mp_task_data_switcher_sj | ptr_this[%p] recv data end!",this);
    }
	//标准复用流
    void mp_entity::pump_rtp_in_sj( RV_IN void *buf,
        RV_IN uint32_t buf_len,
        RV_IN rv_rtp_param *p,
        RV_IN rv_net_address *address )
    {
        //boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::pump_rtp_in_sj1 | ptr_this[%p] recv data start...",this);
        if (!buf || !p || !address)
        {
            DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in1 | ptr_this[%p] buf p address is empty pro fail!",this);
            return;
        }
        DEBUG_LOG("sink_demux_rtpin",LL_NORMAL_INFO,"mp_entity::pump_rtp_in_sj1 | ptr_this[%p] sn[%d] len[%d] sbyte[%d]",this,p->sequenceNumber, p->len, p->sByte );

        rv_rtp_param param;
        ::memcpy( &param, p, sizeof(rv_rtp_param) );
        this->assign();
        bool post_task = true;
        do 
        {
            if(m_state != MP_OPEN_STATE || !m_bActive)
            {
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in_sj1 | ptr_this[%p] not active!",this);
                post_task = false;
                break;
            }
            if (this->use_count() > usrcount_max_num)
            {
                std::size_t _tp_nums = sink_inst::sink_singleton()->tp_inst()->pending();
                std::size_t _post_tp_nums = sink_inst::sink_singleton()->post_tp_inst()->pending();
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in_sj1 | ptr_this[%p] _tp_nums[%d] _post_tp_nums[%d] use_count fail!",this,_tp_nums,_post_tp_nums);
                post_task = false;
                break;
            }
            //模拟丢包
            if (m_nLostCfg>0)
            {
                static int nN = 0;
				//每n包丢1包，1/n丢包率
                if (nN==m_nLostCfg)
                {
					nN = 0;
                    post_task = false;
                    break;
                }
                nN += 1;
            }

            if (m_rtp_fifo.size() > LEN_RTP_CACHE)
            {
                m_rtp_fifo.pop(true);
            }
            if (m_snd_out_fifo.size() > LEN_RTP_CACHE)
            {
                m_snd_out_fifo.pop(true);
            }
            if (m_sink.size() > LEN_RTP_CACHE)
            {
                m_sink.pop(true);
            }

            static uint32_t   tickCount = GetTickCount();
            rtp_packet_block *block     = alloc_rtp_block();// 分配块
            if ( block == NULL || block->size() < param.len )
            {
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::pump_rtp_in_sj1 | block == NULL || block->size() < param.len");
                post_task = false;
                break;
            }

            block->assign();
            block->m_bFrame = false;
            ::memcpy( block->get_raw(), buf, param.len );

            boost::unique_lock<boost::recursive_mutex> lock(m_member_variable_mutex);
            m_ssrc = param.sSrc;

            // 值校验
            if (m_rcheck_sum_cfg > 0)
            {
                unsigned char *crc = block->get_raw()+ param.len-4;

                unsigned long sum1 = 0;
                ::memcpy(&sum1, crc, 4);

                unsigned long sum2 = CRC_32(block->get_raw()+param.sByte, param.len-param.sByte-4);
                if (sum1 != sum2)
                {
                    block->release();
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"checksum fail!");
                    continue;
                }
                param.len -= 4;
            }
            if (!m_bInitLast)
            {
                m_bInitLast = true;
                m_last_rtp_sn = param.sequenceNumber;
            }
            if (!param.extensionBit)
            {
                param.extensionLength = 0;
                param.extensionData = NULL;
            }
            //////////////////////////////////////////////////////////////////////////
            if (param.extensionBit && param.extensionData)
            {
                uint32_t info[3];

                if (IsReSend(param))
                {
                    if (param.extensionLength >= 4)
                    {
                        block->m_bFrame = true;
                        info[0] = param.extensionData[1];
                        info[1] = param.extensionData[2];
                        info[2] = param.extensionData[3];
                    }
                }
                else
                {
                    if (param.extensionLength >= 3)
                    {
                        block->m_bFrame = true;
                        info[0] = param.extensionData[0];
                        info[1] = param.extensionData[1];
                        info[2] = param.extensionData[2];
                    }
                }
                if (block->m_bFrame)
                {
                    block->m_frame[0] = ::ntohl(info[0]);
                    block->m_frame[1] = ::ntohl(info[1]);
                    block->m_frame[2] = ::ntohl(info[2]);
                }
            }
            //////////////////////////////////////////////////////////////////////////*/

            // RESEND
            if (IsReSend(param))
            {
                if (m_nReSendCfg <= 0)
                {
                    block->release();
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"drop1");
                    continue;
                }
            }

            if (m_nReSendCfg>0)
            {
                boost::recursive_mutex::scoped_lock lock(m_mLost);
                if (!IsReSend(param))
                {
                    if (CalDif(m_lastSn, param.sequenceNumber)>=MAX_DROP)
                    {
                        m_lastSn = param.sequenceNumber;
                    }
                    //CalLost(m_listAck, m_lastSn, param.sequenceNumber);
                    if (m_listAck.size() > MAX_DROP)
                    {
                        DEBUG_LOG("sink_calcerr",LL_ERROE,"mp_entity::pump_rtp_in_sj | ptr_this[%p] SN[%d] - [%d]",this, param.sequenceNumber, m_lastSn);
                    }
                    while (m_listAck.size() > MAX_DROP)
                    {
                        m_listAck.erase(m_listAck.begin());
                    }

                    if (IsSmaller(m_lastSn, param.sequenceNumber))
                    {
                        m_lastSn = param.sequenceNumber;
                    }
                }

                // 更新已请求数据
                bool bReSend= false;
                std::vector<RcvSeg>::iterator it = m_listAck.begin();
                for (;it != m_listAck.end();)
                {
                    RcvSeg &segA = *it;
                    if (segA.sn == param.sequenceNumber)
                    {
                        it = m_listAck.erase(it);
                        bReSend = true;
                        continue;
                    }
                    if (IsSmaller(segA.sn, m_last_rtp_sn))
                    {
                        it = m_listAck.erase(it);
                        continue;
                    }
                    if (segA.time == 0)
                    {
                        segA.time = WAIT_RESEND-5;
                    }
                    else
                    {
                        segA.time += 1;
                    }
                    ++it;
                }

                if (IsReSend(param) && !bReSend)
                {
                    block->release();
                    DEBUG_LOG("sink_resend",LL_NORMAL_INFO,"mp_entity::pump_rtp_in_sj1 | ptr_this[%p]-:drop2",this);
                    continue;
                }
            }

            if (m_nReSendCfg > 0)
            {
                boost::recursive_mutex::scoped_lock lock(m_mLost);
                // 请求数据
                std::vector<RcvSeg>::iterator it = m_listAck.begin();
                char list_sn_buffer[1024];
                memset(list_sn_buffer,0x0,1024);
                uint32_t snsize =0;
                for (int nR = 0;nR<MAX_RESEND && it!=m_listAck.end();++it,++nR)
                {
                    RcvSeg &segA = *it;

                    if (segA.time >= WAIT_RESEND)
                    {
                        segA.time = 1;

                        // 屏蔽rtcp信道不发送数据bug(临时方案)
                        //////////////////////////////////////////////////////////////////////////
                        if (GetTickCount()-m_tmSrcAddr > m_tmSrcAddrInterval)
                        {
                            m_tmSrcAddr = GetTickCount();
                            add_srcaddr();
                        }
                        //////////////////////////////////////////////////////////////////////////
                        
                        if (m_nReSendCfg == 1)
                        {
                            RtcpAppMessage msg;
                            uint8_t name[4] = {'N','A','C','K'};
                            msg.subtype = 1;
                            ::memcpy(msg.name, name, sizeof(name));

                            uint32_t sn = segA.sn;
                            msg.userData = (uint8_t*)&sn;
                            msg.userDataLength = sizeof(uint32_t);
                            RtcpSendApps(&m_handle, &msg, 1, false);
                        }
                        else
                        {
                            uint32_t sn = segA.sn;
                            ::memcpy(list_sn_buffer+snsize,(uint8_t*)&sn,sizeof(sn));
                            snsize+=sizeof(sn);
                        }                        
                    }
                }
                //不能发送空的重传请求
                if( snsize > 0 &&list_sn_buffer)
                {
                    RtcpAppMessage msg;
                    uint8_t name[4] = {'N','A','C','K'};
                    msg.subtype = 3;
                    ::memcpy(msg.name, name, sizeof(name));

                    boost::recursive_mutex::scoped_lock lock2(xt_mp_sink::mp_entity::m_mEntityClose);
                    msg.userData = (uint8_t*)&list_sn_buffer;
                    msg.userDataLength = snsize;

                    if (xt_mp_sink::mp_entity::is_valid(this))
                    {
                        RtcpSendApps(&m_handle, &msg, 1, false);
                    }
                }
            }

            uint32_t newTick = ( GetTickCount() - tickCount ) * MP_PSEUDO_TS_CLOCK;
            if (!IsReSend(param))
            {
                manual_send_rtcp_rr(&m_handle,param.sSrc,newTick,param.timestamp,param.sequenceNumber);
                unsigned long time = GetTickCount();
                if (time-m_tForceRR > m_tForceLevel)
                {
                    m_tForceRR = time;
                    force_send_rtcp_rr(&m_handle, false, NULL, 0);
                }
            }
            block->set_size( param.len );
            block->set_marker(param.marker);
            block->set_payload_type( param.payload );
            block->set_sbyte( param.sByte );
            block->set_sn( param.sequenceNumber );
            block->set_ssrc( param.sSrc );
            block->set_ts( param.timestamp );
            block->set_params( param.len-param.sByte,param.sByte );
            block->set_head_param( param );
            //入队 m_rtp_fifo
            m_rtp_fifo.push( block );
            block->release();
        } while (0);

        if (post_task)
        {
            this->assign();
            if (!sink_inst::sink_singleton()->tp_inst()->schedule( boost::bind(&mp_entity::mp_task_data_switcher_sj, this)))
            {
                this->release();
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_entity::7| ptr_this[%p] sink_singleton()->tp_inst()->schedule fail!",this);
            }
        }
        this->release();
        DEBUG_LOG(SINK_DATA,LL_NORMAL_INFO,"mp_entity::pump_rtp_in_sj1 | ptr_this[%p] recv data end!",this);
    }



}
