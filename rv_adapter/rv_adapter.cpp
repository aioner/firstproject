///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：rv_adapter.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月16日
// 内容描述：radvsion ARTP协议栈适配器
//
// 修订日志
// [2012-03-16]		创建基础版本
///////////////////////////////////////////////////////////////////////////////////////////


//Radvision ARTP toolsuit
#include <rvconfig.h>

#include <rvtypes.h>
#include <stdio.h>
#include <stdarg.h>
#include <rvansi.h>
#include <rtp.h>
#include <rtcp.h>
#include <rvrtpseli.h>
#include <payload.h>
#include <rvrtpnatfw.h>
#include <RtpDemux.h>

#include "rv_adapter.h"
#include "mem_check_on.h"
#include "rv_adapter_convert.h"
#include "rv_engine.h"

#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#else
#include <sys/resource.h>
#endif

#include <sys/stat.h>

const char* XT_ADAPTER_LIB_INFO = "XT_Lib_Version: V_XT_ADAPTER_4.26.2016.040800";
RvRtpLogger g_logManager;

#ifdef  _WIN32
void DeleteLogFile(const char *pstrFName, DWORD nMaxSize)
{
    long nFileSize = 0;
    int fh = _open( pstrFName, _S_IWRITE);

    if(fh != -1)
    {
        nFileSize = _filelength(fh);
        if(nFileSize<0)
            nFileSize = 0;
        _close( fh );
    }

    if((DWORD)nFileSize>=nMaxSize)
    {
        DeleteFile(pstrFName);
    }
}

void WriteLog(LPCSTR pStr, LPCSTR pstrIndex)
{	
    FILE *fp = fopen(pstrIndex, "a"); 

    if (fp)
    {
        fprintf(fp,"[%d] %s\n", GetTickCount()/1000, pStr);		
        fclose(fp);		
    }
}

void RVCALLCONV rvRtpLogPrintFunc(
                                  IN void*           context,
                                  IN RvRtpLoggerFilter filter,
                                  IN const RvChar   *formattedText)
{

        DeleteLogFile("D:\\log\\rv_adapter.txt", 20*1024*1024);
        WriteLog(formattedText, "D:\\log\\rv_adapter.txt");
}
#else
void RVCALLCONV rvRtpLogPrintFunc(
                                  IN void*           context,
                                  IN RvRtpLoggerFilter filter,
                                  IN const RvChar   *formattedText)
{
}
#endif //#ifdef  _WIN32



namespace rv
{
    //单件接口
    rv_adapter * rv_adapter::_instance = 0;
    static boost::shared_mutex rv_locker;
    rv_adapter *rv_adapter::init(RV_IN rv_adapter_descriptor *descriptor)
    {
        boost::unique_lock<boost::shared_mutex> lock(rv_locker);
        if (!_instance)
        {
            _instance = new rv_adapter();
            if (_instance)
                if (!_instance->init_adapter(descriptor))
                {
                    delete _instance;
                    _instance = 0;
                }
        }
        return _instance;
    }
    rv_adapter *rv_adapter::self()
    {
        return _instance;
    }

    void rv_adapter::end()
    {
        //added by lichao, 20150429 解决engine线程与此处写锁构成死锁的bug
        engine_end();

        boost::unique_lock<boost::shared_mutex> lock(rv_locker);
        if (_instance)
        {
            delete _instance;
            _instance = NULL;
        }
    }

    //added by lichao, 20150429 解决engine线程与此处写锁构成死锁的bug
    void rv_adapter::engine_end()
    {
        boost::shared_lock<boost::shared_mutex> lock(rv_locker);
        if (_instance)
        {
            //协议调度引擎停止
            _instance->m_core.close();
        }
    }

    void rv_adapter::share_lock()
    {
#if (RV_ADAPTER_SHARE_LOCK)
        rv_locker.lock_shared();
        //		rv_locker.lock();
#endif
    }
    void rv_adapter::share_unlock()
    {
#if (RV_ADAPTER_SHARE_LOCK)
        rv_locker.unlock_shared();
        //		rv_locker.unlock();
#endif
    }

    rv_adapter::rv_adapter() : m_bReady(false)
    {

    }

    rv_adapter::~rv_adapter()
    {
        end_adapter();
    }

    bool rv_adapter::init_adapter(RV_IN rv_adapter_descriptor *descriptor)
    {
        bool bRet = false;
        do
        {
            if (!descriptor) break;
            if (!descriptor->thread_nums) break;
            RvStatus      status     =  RV_ERROR_UNKNOWN;

#ifndef  _WIN32
			rlimit fdlimit;
			fdlimit.rlim_cur=65535;
			fdlimit.rlim_max=65535;
			setrlimit(RLIMIT_NOFILE,&fdlimit);
#endif
            status = RvRtpInit();
            if (status != RV_OK) break;

            status = RvRtcpInit();
            if (status != RV_OK) break;

            m_bReady = true;

            // 设置日志
            RvRtpCreateLogManager(&g_logManager);
            RvRtpSetPrintEntryCallback(rvRtpLogPrintFunc, (void*)&g_logManager);

            RvRtpSetLogModuleFilter(RVRTP_RTP_MODULE,
                RVRTP_LOG_INFO_FILTER|
                RVRTP_LOG_WARN_FILTER||
                RVRTP_LOG_ERROR_FILTER|
                RVRTP_LOG_EXCEP_FILTER|
                RVRTP_LOG_LOCKDBG_FILTER|
                RVRTP_LOG_ENTER_FILTER|
                RVRTP_LOG_LEAVE_FILTER);

            RvRtpSetLogModuleFilter(RVRTP_RTCP_MODULE,
                RVRTP_LOG_WARN_FILTER||
                RVRTP_LOG_ERROR_FILTER|
                RVRTP_LOG_EXCEP_FILTER|
                RVRTP_LOG_DEBUG_FILTER|
                RVRTP_LOG_INFO_FILTER);

            //启动调度引擎
            m_core.build(descriptor->thread_nums);
            bRet = true;
        } while (false);


        return bRet;
    }

    void rv_adapter::end_adapter()
    {
        if (!m_bReady) return;
        m_bReady = false;

        //deleted by lichao, 20150429 将线程调度引擎调到写锁外面，防止死锁
        RvRtcpEnd();
        RvRtpEnd();
    }

    //RTP/RTCP Session操作函数
    bool rv_adapter::open_session(RV_IN rv_session_descriptor *descriptor, RV_OUT rv_handler hrv)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!descriptor) break;
            if (!hrv) break;

            if(!(m_core.m_contexts.sel_context(hrv->hthread))) break;
            rv::open_session_event *event_open =
                static_cast<rv::open_session_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_OPEN_SESSION_EVENT));
            if(!event_open) break;
            event_open->assign();
            event_open->hrv = hrv;
            event_open->setparams(descriptor);
            event_open->set_wait_state();
            if(!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_open)))
            {
                event_open->release();
                break;
            }
            event_open->wait_event(500);
            if (event_open->bRetState)
            {
                m_core.m_contexts.add_ref(hrv->hthread);
                bRet = true;
            }
            event_open->release();
        } while (false);
        return bRet;
    }
    bool rv_adapter::open_session2(RV_IN rv_session_descriptor *descriptor, RV_OUT rv_handler hrv)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!descriptor) break;
            if (!hrv) break;

            if(!(m_core.m_contexts.sel_context(hrv->hthread))) break;
            rv::open_session_event *event_open =
                static_cast<rv::open_session_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_OPEN_SESSION_EVENT));
            if(!event_open) break;
            event_open->assign();
            event_open->hrv = hrv;
            event_open->setparams(descriptor);
            event_open->nDemux = 1;
            event_open->set_wait_state();
            if(!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_open)))
            {
                event_open->release();
                break;
            }
            event_open->wait_event(500);
            if (event_open->bRetState)
            {
                m_core.m_contexts.add_ref(hrv->hthread);
                bRet = true;
            }
            event_open->release();
        } while (false);
        return bRet;
    }

    bool rv_adapter::close_session(RV_IN rv_handler hrv)
    {
        bool bRet = false;
        do
        {
            rv::close_session_event *event_close =
                static_cast<rv::close_session_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_CLOSE_SESSION_EVENT));
            if (!event_close) break;
            event_close->assign();
            event_close->hrv = hrv;

            event_close->set_wait_state();
            if (!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_close))) break;
            event_close->wait_event(500);
            if (event_close->bRetState)
            {
                m_core.m_contexts.dec_ref(hrv->hthread);
                bRet = true;
            }
            event_close->release();
        } while (false);
        return bRet;
    }
	bool rv_adapter::close_session2(RV_IN rv_handler hrv)
	{
		bool bRet = false;
		do
		{
			rv::close_session_event *event_close =
				static_cast<rv::close_session_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_CLOSE_SESSION_EVENT));
			if (!event_close) break;
			event_close->assign();
			event_close->hrv = hrv;
			event_close->nDemux = 1;
			event_close->set_wait_state();
			if (!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_close))) break;
			event_close->wait_event(500);
			if (event_close->bRetState)
			{
				m_core.m_contexts.dec_ref(hrv->hthread);
				bRet = true;
			}
			event_close->release();
		} while (false);
		return bRet;
	}

    bool rv_adapter::open_demux_session(RV_IN rv_session_descriptor *descriptor, RV_IN void* rtpDemux, RV_IN uint32_t *multiplexID, RV_OUT rv_handler hrv)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!descriptor) break;
            if (!hrv) break;

            if(!(m_core.m_contexts.sel_context(hrv->hthread))) break;
            rv::open_session_event *event_open =
                static_cast<rv::open_session_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_OPEN_SESSION_EVENT));
            if(!event_open) break;
            event_open->assign();
            event_open->hrv = hrv;
            event_open->setparams(descriptor);
            event_open->nDemux = 2;
            event_open->demux = rtpDemux;
            event_open->multiplexID = multiplexID;
            event_open->set_wait_state();
            if(!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_open)))
            {
                event_open->release();
                break;
            }
            event_open->wait_event(500);
            if (event_open->bRetState)
            {
                m_core.m_contexts.add_ref(hrv->hthread);
                bRet = true;
            }
            event_open->release();
        } while (false);
        return bRet;
    }

    bool rv_adapter::close_demux_session(RV_IN rv_handler hrv)
    {
        bool bRet = false;
        do
        {
            rv::close_session_event *event_close =
                static_cast<rv::close_session_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_CLOSE_SESSION_EVENT));
            if (!event_close) break;
            event_close->assign();
            event_close->hrv = hrv;
            event_close->nDemux = 2;
            event_close->set_wait_state();
            if (!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_close))) break;
            event_close->wait_event(500);
            if (event_close->bRetState)
            {
                m_core.m_contexts.dec_ref(hrv->hthread);
                bRet = true;
            }
            event_close->release();
        } while (false);
        return bRet;
    }

    void* rv_adapter::demux_construct(RV_IN uint32_t numberOfSessions, RV_IN rv_handler hrv)
    {
        if (!hrv)
        {
            return NULL;
        }

        RvRtpDemux demux = NULL;
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hrv) break;

            if(!(m_core.m_contexts.sel_context(hrv->hthread))) break;
            rv::open_session_event *event_open =
                static_cast<rv::open_session_event *>(m_core.m_contexts.forceAllocEvent(rv::RV_OPEN_SESSION_EVENT));
            if(!event_open) break;
            event_open->assign();
            event_open->hrv = hrv;
            event_open->nDemux = 3;
            event_open->nSessions = numberOfSessions;
            event_open->set_wait_state();
            if(!(m_core.m_contexts.post_asyn_msg(hrv->hthread, event_open)))
            {
                event_open->release();
                break;
            }
            event_open->wait_event(500);
            if (event_open->bRetState)
            {
                m_core.m_contexts.add_ref(hrv->hthread);
                bRet = true;
                demux = (RvRtpDemux)event_open->demux;
            }
            event_open->release();
        } while (false);

        return (void*)demux;
    }

    void rv_adapter::demux_deconstruct(RV_IN void* demux)
    {
        if (demux)
        {
			RtpDemux *d = (RtpDemux*) demux;
			d->rtpSessionsCounter = 0;
			d->rtcpSessionsCounter = 0;

            RvRtpDemuxDestruct((RvRtpDemux)demux);
        }
    }


} /* end of namespace rv*/

