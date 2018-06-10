#include "rtsp_task.h"
#include <boost/pool/singleton_pool.hpp>
#include "RvRtspClientInc.h"

extern void RTSP_C_LOG(const rtsp_client_level_t log_level,const char* fmt,...);
namespace xt_rtsp_client
{
    typedef boost::singleton_pool<rtsp_method_async_task_allocator, sizeof(rtsp_connect_task_t)> rtsp_method_async_task_pool;

    void *rtsp_method_async_task_allocator::malloc()
    {
        return rtsp_method_async_task_pool::malloc();
    }

    void rtsp_method_async_task_allocator::free(void *p)
    {
        rtsp_method_async_task_pool::free(p);
    }

    void rtsp_connect_task_t::done(void *response)
    {
        if (NULL == response)
        {
            do_callback(RTSP_CLIENT_STATUS_NETWORK_PROBLEM);
        }
        else
        {
            response_->success = *(RvBool *)response;
            do_callback(RTSP_CLIENT_STATUS_OK);
        }
    }

    int32_t rtsp_describe_task_t::on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response)
    {
        if (NULL == response)
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        response_->status = (uint32_t)response->statusLine.status;

        if ((RV_TRUE != response->contentLengthValid) || (NULL == response->hBody))
        {
            return RTSP_CLIENT_STATUS_NETWORK_PROBLEM;
        }

        RvSize_t body_size = 0;
        ::RvRtspBloblen(rtsp_handle, response->hBody, &body_size);

        if ((0 == body_size) || (body_size > RTSP_CLIENT_SDP_LEN))
        {
            RTSP_C_LOG(rtsp_c_log_error, "bad sdp size(%d)", body_size);
            return RTSP_CLIENT_STATUS_NETWORK_PROBLEM;
        }

        response_->content_length = response->contentLength.value;
        RvRtspBlobcpy(rtsp_handle, response->hBody, RTSP_CLIENT_SDP_LEN, (RvChar *)response_->body);

        return RTSP_CLIENT_STATUS_OK;
    }

    int32_t rtsp_setup_task_t::on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response)
    {
        if (NULL == response)
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        response_->status = (uint32_t)response->statusLine.status;

        if (RV_TRUE != response->transportValid)
        {
            return RTSP_CLIENT_STATUS_NETWORK_PROBLEM;
        }

        response_->client_rtp_port = response->transport.clientPortA;
        response_->client_rtcp_port = response->transport.clientPortB;
        response_->server_rtp_port = response->transport.serverPortA;
        response_->server_rtcp_port = response->transport.serverPortB;
        response_->is_unicast = response->transport.isUnicast;

        // ¸´ÓÃ
        //////////////////////////////////////////////////////////////////////////
        bool demux = false;
        unsigned int demuxid = 0;
        do 
        {
            if (response->transport.additionalFields)
            {
                RvChar *app_header_fields[32];
                for (int jj = 0; jj < 32; ++jj)
                {
                    app_header_fields[jj] = (RvChar *)malloc(512);
                }

                RvRtspMsgAppHeader app_header;

                (void)memset(&app_header, 0, sizeof(RvRtspMsgAppHeader));
                app_header.headerName = (RvChar *)malloc(32);
                app_header.headerNameLen = 32;
                app_header.headerFieldsSize = 32;
                app_header.headerFieldStrLen = 512;
                app_header.headerFields = app_header_fields;

                ::RvRtspMsgGetHeaderFieldValues(rtsp_handle, response->transport.additionalFields, &app_header);

                for (RvUint32 jj = 0; jj<32 && jj<app_header.headerFieldsSize; ++jj)
                {
                    RvChar *field = app_header.headerFields[jj];//demuxid=xxx

                    std::string sdemuxid = field;
                    std::string sub = "demuxid=";
                    int offset = sdemuxid.find(sub);						
                    if (offset >= 0)
                    {
                        sdemuxid = sdemuxid.substr(offset+sub.length());
                        demux = true;
                        demuxid = atoi(sdemuxid.c_str());
                        break;
                    }
                }

                for (RvUint32 jj = 0; jj < 32; ++jj)
                {
                    free(app_header_fields[jj]);
                }

                free(app_header.headerName);
            }
        } while (false);

        response_->server_demux = demux;
        response_->server_demuxid = demuxid;
        ////////////////////////////////////////////////////////////////////////// 

#ifdef _WIN32
        strncpy_s(response_->destination, response->transport.destination, RTSP_CLIENT_IP_LEN);
#else
        strncpy(response_->destination, response->transport.destination, RTSP_CLIENT_IP_LEN);
#endif

        return RTSP_CLIENT_STATUS_OK;
    }

    int32_t rtsp_play_task_t::on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response)
    {
        if (NULL == response)
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        response_->status = (uint32_t)response->statusLine.status;

        if (RV_TRUE == response->rtpInfoValid)
        {
            RvSize_t rtp_info_num = 0;

            RvRtspArraySize(rtsp_handle, response->rtpInfo.hInfo, &rtp_info_num);
            if (rtp_info_num > 0)
            {
                RvRtspRtpInfo *rtp_info = NULL;
                RvRtspArrayGetFirst(rtsp_handle, response->rtpInfo.hInfo, (void **)&rtp_info);

                RvSize_t index = 0;
                while (NULL != rtp_info)
                {
                    if (NULL != rtp_info->hURI)
                    {
                        RvRtspStrcpy(rtsp_handle, rtp_info->hURI, RTSP_CLIENT_URI_LEN, response_->rtp_infos[index].uri);
                    }

                    if (RV_TRUE == rtp_info->rtpTimeValid)
                    {
                        response_->rtp_infos[index].timestamp = rtp_info->rtpTime;
                    }

                    if (RV_TRUE == rtp_info->seqValid)
                    {
                        response_->rtp_infos[index].seq = rtp_info->seq;
                    }

                    index++;

                    if ((index >= rtp_info_num) || (index >= RTSP_CLIENT_RTPINFO_MAX))
                    {
                        break;
                    }

                    RvRtspArrayGetNext(rtsp_handle, response->rtpInfo.hInfo, rtp_info, (void **)&rtp_info);
                }

                response_->rtp_info_num = index;
            }
        }

        return RTSP_CLIENT_STATUS_OK;
    }

    int32_t rtsp_pause_task_t::on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response)
    {
        if (NULL == response)
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        response_->status = (uint32_t)response->statusLine.status;

        return RTSP_CLIENT_STATUS_OK;
    }

    int32_t rtsp_teardown_task_t::on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response)
    {
        if (NULL == response)
        {
            return RTSP_CLIENT_STATUS_NULLPTR;
        }

        response_->status = (uint32_t)response->statusLine.status;

        return RTSP_CLIENT_STATUS_OK;
    }
}
