#include "BaseDevice.h"
#include "xt_media_client.h"
#include "logger.h"

#include <vector>
#include <string.h>
#include <boost/thread.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define LEN_LOG 4096
#define MEDIA_DEVICE "media_device"

const char* XT_MEDIA_DEVICE_LIB_INFO = "XT_Lib_Version: V_XT_Media_Device_4.26.2016.041100";

#ifdef _WIN32
#define LOG_PATH "d:\\log\\xt_router\\"
#else
#define LOG_PATH "/var/log/xtlog/xt_router/"
#endif

#define WRITE_LOG(logger_name, loglevel, format, ...) xt_log_write(std::string(LOG_PATH).append(logger_name).c_str(), loglevel, format, ##__VA_ARGS__)

namespace 
{
    class xt_media_device : public CBaseDevice
    {
    public:
        xt_media_device()
            :media_client_init_result_(MEDIA_CLIENT_STATUS_UNKNOWN),
            data_cb_(NULL),
            data_ctx_(NULL)
        {

        }

        ~xt_media_device()
        {

        }
        void regist_log()
        {
			//modify by panyu 20160310
			init_log_target(LOG_PATH"media_client");
			init_log_target(LOG_PATH"tcp_client");
			init_log_target(LOG_PATH"rtsp_client");
			init_log_target(LOG_PATH"media_device");
        }

        void init_media_client(xt_client_cfg_t cfg)
        {
            WRITE_LOG(MEDIA_DEVICE,ll_info, "%s", "init_media_client start");
            if (media_client_init_result_ != MEDIA_CLIENT_STATUS_OK)
            {
                regist_log();
                xt_media_client_cfg_t _cfg; 
                strncpy(_cfg.udp_session_bind_ip, cfg.udp_session_bind_ip, MEDIA_CLIENT_IP_LEN);
                _cfg.udp_session_bind_port = cfg.udp_session_bind_port;
                _cfg.udp_session_heartbit_proid = cfg.udp_session_heartbit_proid;
                _cfg.udp_session_request_try_count = cfg.udp_session_request_try_count;
                _cfg.udp_session_request_one_timeout = cfg.udp_session_request_one_timeout;

                strncpy(_cfg.tcp_session_bind_ip, cfg.tcp_session_bind_ip, MEDIA_CLIENT_IP_LEN);
                _cfg.tcp_session_bind_port = cfg.tcp_session_bind_port;
                _cfg.tcp_session_connect_timeout = cfg.tcp_session_connect_timeout;
                _cfg.tcp_session_login_timeout = cfg.tcp_session_login_timeout;
                _cfg.tcp_session_play_timeout = cfg.tcp_session_play_timeout;
                _cfg.tcp_session_stop_timeout = cfg.tcp_session_stop_timeout;

                _cfg.rtsp_session_connect_timeout = cfg.rtsp_session_connect_timeout;
                _cfg.rtsp_session_setup_timeout = cfg.rtsp_session_setup_timeout;
                _cfg.rtsp_session_describe_timeout = cfg.rtsp_session_describe_timeout;
                _cfg.rtsp_session_play_timeout = cfg.rtsp_session_play_timeout;
                _cfg.rtsp_session_pause_timeout = cfg.rtsp_session_pause_timeout;
                _cfg.rtsp_session_teardown_timeout = cfg.rtsp_session_teardown_timeout;

                uint32_t threads_num = boost::thread::hardware_concurrency();
                _cfg.rtp_rv_thread_num = threads_num;
                _cfg.rtp_sink_thread_num = threads_num;
                _cfg.rtp_post_thread_num = threads_num;

                _cfg.ports_mgr_get_callback = &xt_media_device::s_ports_mgr_get_cb;
                _cfg.ports_mgr_free_callback = &xt_media_device::s_ports_mgr_free_cb;
                _cfg.media_log_callback = xt_media_client_print_log;
                _cfg.ports_mgr_ctx = this;

                _cfg.session_enable_flags = MEDIA_CLIENT_USE_ALL_SESSION;

                media_client_init_result_ = xt_media_client_init(&_cfg);
            }

            WRITE_LOG(MEDIA_DEVICE,ll_info, "%s", "init_media_client end");
        }

        //初始化设备
        virtual long InitDevice(void* pParam)
        {
            xt_client_cfg_t* ptr_cfg = static_cast<xt_client_cfg_t*>(pParam);
            if (NULL == ptr_cfg)
            {
                return -1;
            }

            xt_client_cfg_t cfg = *ptr_cfg;
            init_media_client(cfg);
            return  media_client_init_result_ == MEDIA_CLIENT_STATUS_OK ? 0:-1;
        }

        //反初始化设备
        virtual long UnInitDevice(void* pParam)
        {
            if (MEDIA_CLIENT_STATUS_OK == media_client_init_result_)
            {
                xt_media_client_term();
                media_client_init_result_ = MEDIA_CLIENT_STATUS_UNKNOWN;

				delete_log_target(LOG_PATH"media_client");
				delete_log_target(LOG_PATH"tcp_client");
				delete_log_target(LOG_PATH"rtsp_client");
				delete_log_target(LOG_PATH"media_device");
            }
            return media_client_init_result_ == MEDIA_CLIENT_STATUS_UNKNOWN ? 0:-1;
        }

        long  StartLinkDevice(char *szDeviceIP,long nNetPort ,long nChannel,long nLinkType, long nMediaType, long sockethandle, const char *szMulticastIp, unsigned short nMulticastPort,const char *szLocalIP)
        {
            xt_media_link_handle_t link = NULL;
            xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;

			if (szMulticastIp!=NULL&&strlen(szMulticastIp)<=0)
			{
				WRITE_LOG(MEDIA_DEVICE,ll_error,"szMulticastIp:%s nMulticastPort:%d", szMulticastIp, nMulticastPort);
				szMulticastIp = NULL;
			}

            WRITE_LOG(MEDIA_DEVICE,ll_info,"StartLinkDevice:szDeviceIP[%s], nNetPort[%d], nChannel[%d], nLinkType[%d], nMediaType[%d], sockethandle[%d]", szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, sockethandle);
            if (MEDIA_CLIENT_STATUS_OK != media_client_init_result_)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:ret[-1] szDeviceIP[%s], nNetPort[%d], nChannel[%d], nLinkType[%d], nMediaType[%d], sockethandle[%d]", szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, sockethandle);
                return -1;
            }

            if (6 == nLinkType)
            {
                stat = xt_media_client_rtsp_link(szDeviceIP, nMediaType, false, &link,szLocalIP);
            }
            else if (8 == nLinkType)
            {
                char url[256] = "";
                sprintf(url, "rtsp://%s:%d/%d", szDeviceIP, nNetPort, nChannel);
                stat = xt_media_client_rtsp_link(url, nMediaType, true, &link,szLocalIP);
            }
            /*
            else if(5 == nLinkType)
            {
                stat = xt_media_client_priv_link(szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, true, &link);
            }
            else if (14 == nLinkType)
            {
                stat = xt_media_client_priv_link(szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, true, &link);
            }
            */
            else
            {
                if (NULL != szMulticastIp)
                {
                    stat = xt_media_client_priv_multicast_link(szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, szMulticastIp, nMulticastPort, &link);
                }
                else
                {
                    stat = xt_media_client_priv_link(szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, ((5 == nLinkType) || (14 == nLinkType)), &link);
                }
            }

            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:ret[-2] szDeviceIP[%s], nNetPort[%d], nChannel[%d], nLinkType[%d], nMediaType[%d], sockethandle[%d]", szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, sockethandle);
                return -1;
            }

            WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:link[%d] szDeviceIP[%s], nNetPort[%d], nChannel[%d], nLinkType[%d], nMediaType[%d], sockethandle[%d]", link, szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, sockethandle);
            return reinterpret_cast<long>(link);
        }

        long  StartLinkDevice(char *szDeviceIP,long nNetPort ,long nChannel,long nLinkType, long nMediaType, bool bDemuxFlag,long SocketHandle, const char *szMulticastIp, unsigned short nMulticastPort)
        {
            xt_media_link_handle_t link = NULL;
            xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_OK;


            WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:szDeviceIP[%s], nNetPort[%d], nChannel[%d], nLinkType[%d], nMediaType[%d], bDemuxFlag[%d], sockethandle[%d]", szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, bDemuxFlag, SocketHandle);

            if (MEDIA_CLIENT_STATUS_OK != media_client_init_result_)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:ret[-1] szDeviceIP[%s], nNetPort[%d], nChannel[%d], nLinkType[%d], nMediaType[%d], bDemuxFlag[%d], sockethandle[%d]", szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, bDemuxFlag, SocketHandle);
                return -1;
            }

            if (6 == nLinkType)
            {
                stat = xt_media_client_rtsp_link(szDeviceIP, nMediaType, bDemuxFlag, &link);
            }
            else
            {
                if (NULL != szMulticastIp)
                {
                    stat = xt_media_client_priv_multicast_link(szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, szMulticastIp, nMulticastPort, &link);
                }
                else
                {
                    stat = xt_media_client_priv_link(szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, bDemuxFlag, &link);
                }
            }

            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:ret[-2] szDeviceIP[%s], nNetPort[%d], nChannel[%d], nLinkType[%d], nMediaType[%d], bDemuxFlag[%d], sockethandle[%d]", szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, bDemuxFlag, SocketHandle);
                return -1;
            }

            WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:link[%d] szDeviceIP[%s], nNetPort[%d], nChannel[%d], nLinkType[%d], nMediaType[%d], bDemuxFlag[%d], sockethandle[%d]", link, szDeviceIP, nNetPort, nChannel, nLinkType, nMediaType, bDemuxFlag, SocketHandle);
            return reinterpret_cast<long>(link);
        }


        //开启设备链接 -URL
        virtual long StartLinkDevice(const char *szURL)
        {
            xt_media_link_handle_t link = NULL;

            WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:szURL[%s]", szURL);

            if (MEDIA_CLIENT_STATUS_OK != media_client_init_result_)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:ret[-1] szURL[%s]", szURL);
                return -1;
            }

            xt_media_client_status_t stat = xt_media_client_rtsp_link(szURL, 0, false, &link);
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkDevice:ret[-2] szURL[%s]", szURL);
                return -1;
            }

            WRITE_LOG(MEDIA_DEVICE,ll_info,"StartLinkDevice:link[%d] szURL[%s]", link, szURL);
            return reinterpret_cast<long>(link);
        }

        long GetSDP(long link_hadle,unsigned char *msg, long& length)
        {
            xt_media_link_handle_t link = reinterpret_cast<xt_media_link_handle_t>(link_hadle);

            WRITE_LOG(MEDIA_DEVICE,ll_info,"GetSDP:link_hadle[%d]", link_hadle);
            if (NULL == link)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "GetSDP:ret[-1] link_hadle[%d]", link_hadle);
                return -1;
            }

            xt_media_client_status_t stat = xt_media_client_get_header(link, (uint8_t *)msg, ((uint32_t*)&length));
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info,"GetSDP:ret[-2] link_hadle[%d]", link_hadle);
                return -1;
            }

            WRITE_LOG(MEDIA_DEVICE,ll_info, "GetSDP:ret[0] link_hadle[%d] length[%d] msg[%s]", link_hadle, length, msg);

            return 0;
        }
        //add by songlei 20150605
        virtual long request_iframe(long link_hadle)
        {
            xt_media_link_handle_t link = reinterpret_cast<xt_media_link_handle_t>(link_hadle);

            WRITE_LOG(MEDIA_DEVICE,ll_info, "request_iframe:link_hadle[%d]", link_hadle);
            if (NULL == link)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "request_iframe:ret[-1] link_hadle[%d]", link_hadle);
                return -1;
            }

            xt_media_client_status_t stat = xt_media_client_request_iframe(link);

            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "request_iframe:ret[-2] link_hadle[%d]", link_hadle);
                return -1;
            }
            WRITE_LOG(MEDIA_DEVICE,ll_info, "request_iframe:ret[0] link_hadle[%d]", link_hadle);
            return 0;
        }

        //关闭设备链接
        void StopLinkDevice(long lDeviceLinkHandle)
        {
            xt_media_link_handle_t link = reinterpret_cast<xt_media_link_handle_t>(lDeviceLinkHandle);

            WRITE_LOG(MEDIA_DEVICE,ll_info,"StopLinkDevice:link_hadle[%d]", lDeviceLinkHandle);
            if (NULL != link)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StopLinkDevice:ret[0] link_hadle[%d]", lDeviceLinkHandle);
                xt_media_client_close_link(link);

            }
            else
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info,"StopLinkDevice:ret[-1] link_hadle[%d]", lDeviceLinkHandle);
            }
        }

        //开启采集
        long StartLinkCapture(long lDeviceLinkHandle, OV_PRealDataCallback lpRealDatacallback,void* UserContext)
        {
            xt_media_link_handle_t link = reinterpret_cast<xt_media_link_handle_t>(lDeviceLinkHandle);

            WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkCapture:link_hadle[%d] UserContext[%d]", lDeviceLinkHandle, UserContext);

            if (NULL == link)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkCapture:ret[-1] link_hadle[%d]", lDeviceLinkHandle);
            }

            data_cb_ = lpRealDatacallback;
            data_ctx_ = UserContext;

            xt_media_client_status_t stat = xt_media_client_play(link, &xt_media_device::s_media_client_frame_cb, this);
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkCapture:ret[-2] link_hadle[%d]", lDeviceLinkHandle);
                return -1;
            }

            WRITE_LOG(MEDIA_DEVICE,ll_info, "StartLinkCapture:ret[0] link_hadle[%d] UserContext[%d]", lDeviceLinkHandle, UserContext);

            return 0;
        }

        //关闭采集
        long StopLinkCapture(long lDeviceLinkHandle)
        {
            return 0;
        }

        //文件流化接口
        long TcpPlayCtrl(long lDeviceLinkHandle, double npt, float scale, unsigned long *rtp_pkt_timestamp)
        {
            xt_media_link_handle_t link = reinterpret_cast<xt_media_link_handle_t>(lDeviceLinkHandle);
            if (NULL == link)
            {
                return -1;
            }

            uint32_t seq = 0;
            uint32_t timestamp = 0;
            xt_media_client_status_t stat = xt_media_client_seek(link, npt, scale, &seq, (uint32_t*)timestamp);
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                return -1;
            }

            if (NULL != rtp_pkt_timestamp)
            {
                *rtp_pkt_timestamp = timestamp;
            }

            return 0;
        }

        long TcpPauseCtrl(long lDeviceLinkHandle)
        {
            xt_media_link_handle_t link = reinterpret_cast<xt_media_link_handle_t>(lDeviceLinkHandle);
            if (NULL == link)
            {
                return -1;
            }

            xt_media_client_status_t stat = xt_media_client_pause(link);
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                return -1;
            }

            return 0;
        }

		long SetSSRCReport(long lDeviceLinkHandle, rtcp_report_callback_t reportFunc, void* ctx)
		{
			WRITE_LOG(MEDIA_DEVICE, ll_info, "SetSSRCReport: link_hadle[%d]", lDeviceLinkHandle);
			xt_media_link_handle_t link = reinterpret_cast<xt_media_link_handle_t>(lDeviceLinkHandle);
			if (NULL == link)
			{
				return -1;
			}

			xt_media_client_status_t stat = xt_media_client_register_rtcp_callback(link, (xt_media_client_rtcp_report_callback_t)reportFunc, ctx);
			if (MEDIA_CLIENT_STATUS_OK != stat)
			{
				return -1;
			}

			return 0;
		}

        static int32_t XT_MEDIA_CLIENT_STDCALL s_ports_mgr_get_cb(void *ctx, uint32_t num, uint16_t *ports, int32_t opt)
        {
            xt_media_device *pThis = static_cast<xt_media_device *>(ctx);
            if (NULL == pThis)
            {
                return -1;
            }

            std::vector<long> vports(num);
            if (!pThis->GetPorts(num, &vports[0], (1 == opt)))
            {
                return -2;
            }

            std::copy(vports.begin(), vports.end(), ports);

            WRITE_LOG(MEDIA_DEVICE,ll_info, "s_ports_mgr_get_cb::ports(%d,%d),num(%d)", ports[0],ports[1],num);

            return 0;
        }

        static void XT_MEDIA_CLIENT_STDCALL s_ports_mgr_free_cb(void *ctx, uint32_t num, uint16_t *ports)
        {
            xt_media_device *pThis = static_cast<xt_media_device *>(ctx);
            if (NULL != pThis)
            {
                std::vector<long> vports(num);
                std::copy(ports, ports + num, vports.begin());
                WRITE_LOG(MEDIA_DEVICE,ll_info, "s_ports_mgr_free_cb::ports(%d),num(%d)", ports[0] ,num);
                pThis->FreePorts(&vports[0], num);
            }
        }

        static void XT_MEDIA_CLIENT_STDCALL s_media_client_frame_cb(void *ctx, xt_media_link_handle_t link, void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc)
        {
            xt_media_device *pThis = static_cast<xt_media_device *>(ctx);
            if (NULL != pThis && pThis->data_cb_)
            {
                pThis->data_cb_(reinterpret_cast<long>(link), frame_type, (unsigned char *)data, length, data_type, pThis->data_ctx_, timestamp, ssrc);
            }
        }

        long create_recv(int track_num, bool demux)
        {
            xt_media_link_handle_t link = NULL;

            WRITE_LOG(MEDIA_DEVICE,ll_info, "create_recv:track_num[%d] demux[%d]", track_num, demux);

            if (MEDIA_CLIENT_STATUS_OK != media_client_init_result_)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "create_recv:ret[-1] track_num[%d] demux[%d]", track_num, demux);
                return -1;
            }

            xt_media_client_status_t stat = xt_media_client_create_recv(track_num, demux, &link);
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "create_recv:ret[-2] track_num[%d] demux[%d]", track_num, demux);
                return -1;
            }
            WRITE_LOG(MEDIA_DEVICE,ll_info, "create_recv:link[%d] track_num[%d] demux[%d]", link, track_num, demux);
            return reinterpret_cast<long>(link);
        }

        virtual long create_recvinfo(int track_num, bool demux, bool multicast, const char* multicastip, int* multiports)
        {
            xt_media_link_handle_t handle_link = NULL;

            WRITE_LOG(MEDIA_DEVICE,ll_info, "create_recvinfo:track_num[%d] demux[%d] multicast[%d] multicastip[%s] multiports[%d]", track_num, demux, multicast, multicastip, *multiports);
            do 
            { 
                if (MEDIA_CLIENT_STATUS_OK != media_client_init_result_)
                {
                    WRITE_LOG(MEDIA_DEVICE,ll_info, "create_recvinfo:ret[-1] track_num[%d] demux[%d] multicast[%d] multicastip[%s] multiports[%d]", track_num, demux, multicast, multicastip, *multiports);
                    break;
                }

                xt_media_client_status_t stat = xt_media_client_create_recv_2(track_num, demux,multicast,multicastip,multiports,&handle_link);
                if (MEDIA_CLIENT_STATUS_OK != stat)
                {
                    WRITE_LOG(MEDIA_DEVICE,ll_info, "create_recvinfo:ret[-2] track_num[%d] demux[%d] multicast[%d] multicastip[%s] multiports[%d]", track_num, demux, multicast, multicastip, *multiports);
                    break;
                }
            } while (0);

            if (NULL == handle_link)
            {
                return -1;
            }
            WRITE_LOG(MEDIA_DEVICE,ll_info, "create_recvinfo:link[%d] track_num[%d] demux[%d] multicast[%d] multicastip[%s] multiports[%d]", handle_link, track_num, demux, multicast, multicastip, *multiports);

            return reinterpret_cast<long>(handle_link);
        }

        long get_rcvinfo(long link, RCVINFO *infos, int &num)
        {
            xt_sink_info_t sinks[9];
            int num_sink = 9;
            WRITE_LOG(MEDIA_DEVICE,ll_info, "get_rcvinfo:link[%d]", link);


            xt_media_client_status_t stat = xt_media_client_getport((xt_media_link_handle_t)link, sinks, num_sink);
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "get_rcvinfo:ret[-1] link[%d]", link);
                return -1;
            }

            for (int i=0;i<num_sink&&i<num;++i)
            {
                infos[i].index = sinks[i].index;
                infos[i].port_rtp = sinks[i].port_rtp;
                infos[i].port_rtcp = sinks[i].port_rtcp;
                infos[i].demux = sinks[i].demux;
                infos[i].demuxid = sinks[i].demuxid;
                WRITE_LOG(MEDIA_DEVICE,ll_info, "get_rcvinfo:ret[0] link[%d] port[%d-%d] demux[%d] demuxid[%d]", link, infos[i].port_rtp, infos[i].port_rtcp, infos[i].demux, infos[i].demuxid);
            }

            if (num > num_sink)
            {
                num = num_sink;
            }

            return 0;
        }

        long set_sdp(long link, const char *sdp, uint32_t len_sdp)
        {
            if (len_sdp==0 || !sdp)
            {
                WRITE_LOG(MEDIA_DEVICE,ll_info, "set_sdp:ret[-1] link[%d] len_sdp[%d]", link, len_sdp);
                return -1;
            }

            WRITE_LOG(MEDIA_DEVICE,ll_info,"set_sdp:link[%d] len_sdp[%d] sdp[%s]", link, len_sdp, sdp);
            xt_media_client_status_t stat = xt_media_client_setsdp((xt_media_link_handle_t)link, sdp, len_sdp);
            if (MEDIA_CLIENT_STATUS_OK != stat)
            {
                return -1;
            }
            return 0;
        }

		long SetResend(int resend,int wait_resend, int max_resend, int vga_order)
		{
			return ::xt_media_client_setresend(resend, wait_resend, max_resend, vga_order);
		}

        long  set_regist_callback (regist_call_back_t func)
        {
            xt_media_client_regist_callback(func);
            return 0;
        }

        static void XT_MEDIA_CLIENT_STDCALL xt_media_client_print_log(const char* log_name,const media_client_log_level_t log_level,const char* log_ctx)
        {
            severity_level ll_level = ll_off;
            switch(log_level)
            {
            case md_log_info:
                {
                    ll_level = ll_info;
                    break;
                }

            case md_log_warn:
                {
                    ll_level = ll_warn;
                    break;
                }

            case md_log_error:
                {
                    ll_level = ll_error;
                    break;
                }
            case md_log_debug:
                {
                    ll_level = ll_debug;
                    break;
                }
            default:
                {
                    break;
                }
            }
            WRITE_LOG(log_name, ll_level, "%s", log_ctx);
        }
    private:
        xt_media_device(const xt_media_device&);
        void operator=(const xt_media_device&) const;

        xt_media_client_status_t media_client_init_result_;
        OV_PRealDataCallback data_cb_;
        regist_call_back_t  regist_cb_;
        void *data_ctx_;
    };
}

extern "C" 
#if !defined(WIN32) && !(defined(WIN64))
__attribute__((visibility("default")))
#endif
CBaseDevice  *AddMe(CDeviceCollect* pDeviceCollect, int nDeviceLinkType, int nDataType)
{
    return AddA(new xt_media_device, pDeviceCollect, nDeviceLinkType, nDataType);
}

