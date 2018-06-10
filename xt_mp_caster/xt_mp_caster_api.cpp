///////////////////////////////////////////////////////////////////////////////////////////
// 文 件 名：xt_mp_caster_api.cpp
// 创 建 者：汤戈
// 创建时间：2012年03月23日
// 内容描述：兴图新科公司mp广播服务库
///////////////////////////////////////////////////////////////////////////////////////////

#include "mp_caster_config.h"
#include "xt_mp_caster_def.h"
#include "xt_mp_caster_api.h"
#include "mp_caster.h"
#include "bc_mp.h"
#include <../rv_adapter/rv_api.h>
#include "msink_rv_rtp.h"
#include <stdarg.h>
#include <stdio.h>

#ifdef	_OS_WINDOWS
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define sprintf_s snprintf
#endif

#ifndef MAX_PATH 
#define MAX_PATH 1024
#endif//#ifndef MAX_PATH
const char* XT_CASTER_LIB_INFO = "XT_Lib_Version: V_XT_CASTER_4.26.2016.041400";

void writeLogV(int nLogLevel, const char* szLogName,const char* szLogFmt, va_list vArgList)
{
#ifndef _ANDROID
    if (nLogLevel < config::_()->logLevel(-1))
    {
        return;
    }
#endif //_ANDROID

    FILE * pFile = NULL;
    char szFile[MAX_PATH] = "";
    sprintf_s(szFile, MAX_PATH, "d:\\Log\\%s.txt", szLogName);

    //开启文件
    pFile = fopen(szFile,  "a");
    if (pFile == NULL)
        return;
    struct tm * timeinfo;
    time_t  rawtime;
    time(&rawtime);
    timeinfo = localtime (&rawtime);
    ::fprintf(pFile, "%02d-%02d-%02d %02d-%02d-%02d ", timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

    //写入数据
    vfprintf(pFile, szLogFmt, vArgList);
    fprintf(pFile, "\n");
    fclose(pFile);
}

void writeLog(int nLevel, const char* szLogName, const char* szLogFmt, ...)
{
//      va_list argptr;
//      va_start(argptr, szLogFmt);
//      writeLogV(nLevel, szLogName, szLogFmt, argptr);
//      va_end(argptr);
}

mp_bool init_mp_caster(MP_IN caster_descriptor *descriptor)
{
#ifndef _ANDROID
    int ret = config::_()->init_config();
    if (ret < 0)
    {
        writeLog(0, "caster-call", "caster初始化配置文件失败!");
    }
#endif

    if (xt_mp_caster::caster::init(descriptor))
    {
        writeLog(0, "caster-call", "caster库初始化成功");
        return MP_TRUE;
    }
    else
    {
        writeLog(0, "caster-call", "caster库初始化失败");
        return MP_FALSE;
    }
}

void end_mp_caster(void)
{
    writeLog(0, "caster-call", "caster库释放");
    xt_mp_caster::caster::end();
}

//获取当前版本号
void get_mp_caster_version(uint32_t *v1, uint32_t *v2, uint32_t *v3)
{
    if(v1) *v1 = MP_CASTER_VER1;
    if(v2) *v2 = MP_CASTER_VER2;
    if(v3) *v3 = MP_CASTER_VER3;
}

//创建一个广播MP
mp_bool open_bc_mp(
                   MP_IN bc_mp_descriptor* descriptor,
                   MP_OUT mp_h hmp,						//mp实体句柄
                   MP_OUT mssrc_h hmssrc,					//用于pump_frame_in/pump_rtp_in
                   MP_OUT msink_h hmsink,					//用于read_rtcp_sr/read_rtcp_rr，不可调用del_sink释放
                   MP_OUT uint32_t *multID)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;

        if (!caster->open_bc_mp(descriptor, hmp, hmssrc, hmsink, multID))
        {
            break;
        }

        bRet = MP_TRUE;
    } while (false);

    xt_mp_caster::caster::share_unlock();
    return bRet;
}

//更新丢包重传开关 add by songlei 20150708
void update_resend_flag(MP_IN mp_h hmp,const int resend_flag)
{
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;
        caster->update_resend_flag(hmp,resend_flag);
    } while (false);
    xt_mp_caster::caster::share_unlock();
}

//析构指定MP
mp_bool close_mp(MP_IN mp_h hmp)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;

        if (!caster->close_mp(hmp))
        {
            break;
        }

        bRet = MP_TRUE;
    } while (false);

    writeLog(0, "caster-call", "关闭mp:ret[%d]", (int)bRet);

    xt_mp_caster::caster::share_unlock();
    return bRet;
}
//激活或禁用指定MP，禁用的MP将不接纳外部数据输入，也不输出数据
mp_bool active_mp(MP_IN mp_h hmp, mp_bool bActive)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;

        if (!caster->active_mp(hmp, (MP_TRUE == bActive)))
        {
            break;
        }

        bRet = MP_TRUE;
    } while (false);
    xt_mp_caster::caster::share_unlock();

    writeLog(0, "caster-call", "激活mp:ret[%d] active[%d]", (int)bRet, (int)bActive);

    return bRet;
}

//对指定MP增加rtp输出接点，rtp_sink_descriptor中的地址操作使用rv_adapter中的相关函数
mp_bool add_rtp_sink(
                     MP_IN mp_h hmp,
                     MP_IN rtp_sink_descriptor* descriptor,
                     MP_OUT msink_h hsink)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;

        if (!caster->add_rtp_sink(hmp, descriptor, hsink))
        {
            break;
        }

        bRet = MP_TRUE;
    } while (false);
    xt_mp_caster::caster::share_unlock();

    if (descriptor)
    {
        rv_net_ipv4 dst1, dst2;
        ::memset(&dst1, 0, sizeof(dst1));
        ::memset(&dst2, 0, sizeof(dst2));
        convert_rvnet_to_ipv4(&dst1, &descriptor->rtp_address);
        convert_rvnet_to_ipv4(&dst2, &descriptor->rtcp_address);
        in_addr addr1,addr2;
        addr1.s_addr = dst1.ip;
        addr2.s_addr = dst2.ip;
        writeLog(0, "caster-call", "增加rtp输出点:ret[%d] multiplex[%d] multid[%d] trtcp[%d] ip[%s] port[%d] rtcpip[%s] rtcpport[%d]", (int)bRet, descriptor->multiplex, descriptor->multiplexID, descriptor->rtcp_opt, inet_ntoa(addr1), dst1.port, inet_ntoa(addr2), dst2.port);
    }

    return bRet;
}
//对指定MP删除指定sink输出点
mp_bool del_sink(MP_IN mp_h hmp, MP_IN msink_h hsink)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;
        if (!caster->del_sink(hmp, hsink))
        {
            break;
        }

        bRet = MP_TRUE;
    } while (false);
    xt_mp_caster::caster::share_unlock();

    writeLog(0, "caster-call", "删除mem输出点:ret[%d]", (int)bRet);

    return bRet;
}
//读取bc_mp的rtcp发送报告，其中hsink参数由open_bc_mp获得
mp_bool read_rtcp_sr_from_sender(
                                 MP_IN msink_h hsink,
                                 MP_OUT rtcp_send_report *sr)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;
        if (!caster->read_rtcp_sr(hsink, sr)) break;
        bRet = MP_TRUE;
    } while (false);
    xt_mp_caster::caster::share_unlock();
    return bRet;
}
//读取bc_mp的rtcp接收报告，其中hsink参数由open_bc_mp获得
mp_bool read_rtcp_rr_from_sender(
                                 MP_IN msink_h hsink,
                                 MP_OUT rtcp_receive_report *rr)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;
        if (!caster->read_rtcp_rr(hsink, rr)) break;
        bRet = MP_TRUE;
    } while (false);
    xt_mp_caster::caster::share_unlock();
    return bRet;
}

//读取rv_ssrc的rtcp发送报告，其中hssrc参数由open_xxx_mp获得
mp_bool read_rtcp_sr_from_receiver(
                                   MP_IN mssrc_h hssrc,
                                   MP_OUT rtcp_send_report *sr)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;
        if (!caster->read_rtcp_sr(hssrc, sr)) break;
        bRet = MP_TRUE;
    } while (false);
    xt_mp_caster::caster::share_unlock();
    return bRet;
}
//读取rv_ssrc的rtcp接收报告，其中hsink参数由open_xxx_mp获得
mp_bool read_rtcp_rr_from_receiver(
                                   MP_IN mssrc_h hssrc,
                                   MP_OUT rtcp_receive_report *rr)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;
        if (!caster->read_rtcp_rr(hssrc, rr)) break;
        bRet = MP_TRUE;
    } while (false);
    xt_mp_caster::caster::share_unlock();
    return bRet;
}
//对指定MP的MSSRC端口写入数据帧
mp_bool pump_frame_in(
                       MP_IN mp_h	hmp,				
                       MP_IN mssrc_h hmssrc,			
                       MP_IN uint8_t *frame,			
                       MP_IN uint32_t framesize,		
                       MP_IN mp_bool  frameTS_opt,		
                       MP_IN uint32_t frameTS,			
                       MP_IN uint8_t  framePayload,		
                       MP_IN XTFrameInfo &info,
                       MP_IN uint8_t priority,
                       MP_IN bool std,
                       MP_IN bool use_ssrc,
                       MP_IN uint32_t ssrc)
{
    mp_bool bRet = MP_FALSE;
    xt_mp_caster::caster::share_lock();
    do 
    {
        xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
        if (!caster) break;
        if (!caster->pump_frame_in(
            hmp, hmssrc, frame, framesize, 
            (MP_TRUE == frameTS_opt),
            frameTS, framePayload, info,priority,std,use_ssrc, ssrc)) break;	

        bRet = MP_TRUE;
    } while (false);
    xt_mp_caster::caster::share_unlock();

    return bRet;
}

mp_bool pump_rtp_in2(
									  MP_IN mp_h	hmp,				//目标mp句柄
									  MP_IN mssrc_h hmssrc,			//目标mssrc句柄
									  MP_IN void *rtp)				//RTP完整信息包
{
	mp_bool bRet = MP_FALSE;
	xt_mp_caster::caster::share_lock();
	do 
	{
		xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
		if (!caster) break;
		if (!caster->pump_rtp_in2(
			hmp, hmssrc, rtp)) break;	

		bRet = MP_TRUE;
	} while (false);
	xt_mp_caster::caster::share_unlock();

	return bRet;
}

mp_bool get_sink_sn(MP_IN mp_h hmp, unsigned short *sn)
{
	mp_bool bRet = MP_FALSE;
	xt_mp_caster::caster::share_lock();
	do 
	{
		xt_mp_caster::caster * caster = xt_mp_caster::caster::self();
		if (!caster) break;
		if (!caster->get_sink_sn(hmp, sn)) break;	

		bRet = MP_TRUE;
	} while (false);
	xt_mp_caster::caster::share_unlock();

	return bRet;
}

void setSink_RtcpCB(mp_handle sink, pSink_RtcpCB func)
{
    xt_mp_caster::msink_rv_rtp *rv_sink = static_cast<xt_mp_caster::msink_rv_rtp *>(sink);
    if (rv_sink)
    {
        rv_sink->setSink_RtcpCB(func);
    }
}

void setSink_SID(mp_handle sink, long sid)
{
    xt_mp_caster::msink_rv_rtp *rv_sink = static_cast<xt_mp_caster::msink_rv_rtp *>(sink);
    if (rv_sink)
    {
        rv_sink->m_nSID = sid;
    }
}

void setSink_AppMsgCB(mp_handle sink, pRtcpAppMsgCB func)
{
    xt_mp_caster::msink_rv_rtp *rv_sink = static_cast<xt_mp_caster::msink_rv_rtp *>(sink);
    if (rv_sink)
    {
        rv_sink->setSink_AppMsgCB(func);
    }
}

void mp_rtcp_set_rawdata_cb(mp_handle sink, pRtcpRawDataCB func)
{
    xt_mp_caster::msink_rv_rtp *rv_sink = static_cast<xt_mp_caster::msink_rv_rtp *>(sink);
    if (rv_sink)
    {
        rv_sink->rtcp_set_rawcb(func);
    }
}

void set_raddr_cb(mp_h	hmp, raddr_cb cb)
{
    if (!hmp)
    {
        return;
    }

    bc_mp *mp = (bc_mp*)hmp->hmp;
    mp->set_raddr_cb(cb);
}

void mp_set_file_path(MP_IN mp_h	hmp,				
				   MP_IN const char *file)
{
	if (!hmp)
	{
		return;
	}

	bc_mp *mp = (bc_mp*)hmp->hmp;
	mp->set_file_path(file);
}

#ifdef _USE_RTP_SEND_CONTROLLER
void mp_register_network_changed_callback(mp_handle hmp, mp_network_changed_callback_t cb, void *ctx)
{
    bc_mp *mp = static_cast<bc_mp*>(hmp);
    if (NULL != mp)
    {
        mp->register_network_changed_callback(cb, ctx);
    }
}
#endif