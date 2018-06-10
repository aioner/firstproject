#include "xt_router.h"
#include "sdp.h"
#include "parse_buffer.h"

#include <assert.h>
#include <memory>
#include <android/log.h>

#define DEVICE_TYPE_ROUTER              9
#define DEVICE_TYPE_VGA                 172

#define XT_VIDEO_FRAME_TYPE             0
#define XT_AUDIO_FRAME_TYPE             1

namespace xt_router
{
    //static
    route_info_mgr_t route_info_mgr_t::s_inst_;

    int get_frame_type_by_name(const std::string& name)
    {
        if ("video" == name)
        {
            return XT_VIDEO_FRAME_TYPE;
        }
        else if ("audio" == name)
        {
            return XT_AUDIO_FRAME_TYPE;
        }
        else
        {
            return -1;
        }
    }

    enum ov_frame_type
    {
        OV_FRAME_TYPE_UNKNOWN  = 0xffffffff,
        OV_VIDEO_I            = 0x00000000,
        OV_AUDIO              = 0x00000001,
        HC_HEADE              = 0x68,
        HC_AUDIO              = 0x69,
        OV_HEADE              = 0x80,
        OV_VIDEO_P            = 0x00010000,
        OV_VIDEO_B            = 0x00020000,
        OV_VIDEO_SEI          = OV_VIDEO_P,
        OV_VIDEO_SPS          = OV_VIDEO_I,
        OV_VIDEO_PPS          = OV_VIDEO_I,
        OV_AAC                = 0x00120000,
        OV_H264              = 0x00100000,
        OV_H264_I          = OV_H264+1,
        OV_H264_P          = OV_H264+2,
        OV_H264_B         = OV_H264+3,
        OV_H264_SEI        = OV_H264+4,
        OV_H264_SPS            = OV_H264+5,
        OV_H264_PPS        = OV_H264+6,
        OV_G711           = 0x00110000,
        OV_H265           = 0x00130000,
    };

    int get_frame_type_index(uint32_t frame_type)
    {
        switch (frame_type)
        {
        case OV_VIDEO_I:
        case OV_VIDEO_P:
        case OV_VIDEO_B:
        case OV_H264:
        case OV_H264_I:
        case OV_H264_P:
        case OV_H264_B:
        case OV_H264_SEI:
        case OV_H264_SPS:
        case OV_H264_PPS:
        case OV_H265:
            return XT_VIDEO_FRAME_TYPE;
            break;
        case OV_AUDIO:
        case OV_AAC:
        case OV_G711:
            return XT_AUDIO_FRAME_TYPE;
            break;
        default:
            return -1;
            break;
        }
        assert(false);
    }

    int create_src_adapter(const char *sdp, size_t sdp_len, int& srcno, int &chan, int (&frame_type_to_trackids)[MAX_TRACK_NUM])
    {
        srcno = -1;
        std::fill_n(frame_type_to_trackids, MAX_TRACK_NUM, -1);

        //解析sdp
        xt_sdp::sdp_session_t sdp_session;

        try
        {
            xt_sdp::parse_buffer_t pb(sdp, sdp_len);
            sdp_session.parse(pb);
        }
        catch (const std::exception& e)
        {
            __android_log_print(ANDROID_LOG_ERROR, "create_src_adatper", "parse sdp failed(%s)", e.what());
            chan = -1;
            return RT_ERR_PARSE_SDP_FAIL;
        }

        src_track_info_t track_info = { 0 };

        uint32_t index = 0;
        for (xt_sdp::sdp_session_t::medium_container_t::iterator it = sdp_session.media().begin(); (sdp_session.media().end() != it) && (index < MAX_TRACK); ++it)
        {
            track_info.trackids[index] = index + 1;
            (void)strncpy(track_info.tracknames[index], it->name().c_str(), sizeof(track_info.tracknames[index]));
            __android_log_print(ANDROID_LOG_ERROR, "create_src_adatper", "fill param.id(%u),name(%s)", track_info.trackids[index], track_info.tracknames[index]);

            int frame_type = get_frame_type_by_name(it->name());
            if ((frame_type < 0) || (frame_type >= MAX_TRACK_NUM))
            {
                __android_log_print(ANDROID_LOG_ERROR, "create_src_adatper", "bad frame_type(%d)", frame_type);
                chan = -1;
                return RT_ERR_PARSE_SDP_FAIL;
            }

            frame_type_to_trackids[frame_type] = track_info.trackids[index];

            index++;
        }

        if (0 == index)
        {
            __android_log_print(ANDROID_LOG_ERROR, "create_src_adatper", "parse sdp failed(%.*s)", sdp_len, sdp);
            chan = -1;
            return RT_ERR_PARSE_SDP_FAIL;
        }

        track_info.tracknum = index;

        int ret = xt_create_src(track_info, srcno, chan);
        if (ret < 0)
        {
            __android_log_print(ANDROID_LOG_ERROR, "create_src_adatper", "xt_create_src failed(%d)", ret);
            chan = -1;
            return RT_ERR_CREATE_SRC_FAIL;
        }

        chan = srcno;

        return RT_OK;
    }
}

using namespace xt_router;

static void XT_MEDIA_CLIENT_STDCALL data_cb(void *ctx, xt_media_link_handle_t link, void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc)
{
    route_info_t *rt = static_cast<route_info_t *>(ctx);

    if (rt)
    {
        if (rt->cb)
        {
            rt->cb(rt->ctx, link, data, length, frame_type, data_type, timestamp, ssrc);
        }

        route_info_mgr_t::instance().update_rt_deadline(rt);
    }

    if(0 == frame_type)
    {
        rt->priority = 0;
    }
    else if(0x00010000 == frame_type)
    {
        rt->priority++;
    }

    rt_send_data(ctx, data, length, frame_type, data_type, timestamp, rt->priority);
}

int XT_ROUTER_CALLBACK rt_start_play(int device_type, const char*ip, unsigned short port, long channel, int *trans_chan, rt_handle_t *handle)
{
    if ((NULL == ip) || (NULL == trans_chan) || (NULL == handle))
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_start_play", "bad param.ip(%p),trans_chan(%p),handle(%p)", ip, trans_chan, handle);
        return RT_ERR_BAD_PARAM;
    }

    *handle = NULL;

    xt_media_link_handle_t link = NULL;
    xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_UNKNOWN;

    if (DEVICE_TYPE_VGA == device_type)
    {
        stat = xt_media_client_rtsp_link(ip, 0, false, &link);
    }
    else
    {
        stat = xt_media_client_priv_link(ip, port, channel, LT_UDP_RTP_PRI, 0, false, &link);
    }

    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        *trans_chan = -1;
        __android_log_print(ANDROID_LOG_ERROR, "rt_start_play", "xt_media_client_priv_link failed(%d),ip(%s),port(%d),channel(%ld),device_type(%d)", stat, ip, port, channel, device_type);
        return RT_ERR_START_LINK_FAIL;
    }

    char sdp[2048];
    uint32_t sdp_len = sizeof(sdp);

    stat = xt_media_client_get_header(link, (uint8_t *)sdp, &sdp_len);
    if ((MEDIA_CLIENT_STATUS_OK != stat) || (0 == sdp_len))
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_start_play", "xt_media_client_get_header failed(%d),ip(%s),port(%d),channel(%ld),sdp(%u)", stat, ip, port, channel, sdp_len);
        xt_media_client_close_link(link);
        *trans_chan = -1;
        return RT_ERR_GET_SDP_FAIL;
    }

    int srcno = -1;
    int frame_type_to_trackids[MAX_TRACK_NUM] = { 0 };

    int ret = create_src_adapter(sdp, sdp_len, srcno, *trans_chan, frame_type_to_trackids);
    if (RT_OK != ret)
    {
        xt_media_client_close_link(link);
        return ret;
    }

    std::auto_ptr<route_info_t> info(new route_info_t);
    assert(NULL != info.get());

    info->chan = *trans_chan;
    info->srcno = srcno;
    info->link = link;
    std::copy(frame_type_to_trackids, frame_type_to_trackids + MAX_TRACK_NUM, info->frame_type_to_trackids);

    stat = xt_media_client_play(link, data_cb, info.get());
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_start_play", "xt_media_client_play failed(%d),ip(%s),port(%d),channel(%ld)", stat, ip, port, channel);
    }

    xt_set_key(srcno, (char*)sdp, sdp_len, 172);
    xt_send_data(srcno, -1, (char*)sdp, sdp_len, 128, 172);

    __android_log_print(ANDROID_LOG_ERROR, "rt_start_play", "new info(%p)", info.get());
    *handle = static_cast<void *>(info.get());

    route_info_mgr_t::instance().add_route_info(info.release());

    return RT_OK;
}

int XT_ROUTER_CALLBACK rt_stop_play(rt_handle_t handle)
{
    route_info_t *info = static_cast<route_info_t *>(handle);
    if (NULL == info)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_stop_play", "bad param.handle(%p)", handle);
        return RT_ERR_BAD_PARAM;
    }

    if (NULL != info->link)
    {
        xt_media_link_handle_t link = info->link;
        info->link = NULL;
        xt_media_client_close_link(link);
    }

    if (info->srcno >= 0)
    {
        int srcno = info->srcno;
        info->srcno = -1;
        xt_destroy_src(srcno);
    }

    __android_log_print(ANDROID_LOG_ERROR, "rt_stop_play", "delete info(%p)", info);
    route_info_mgr_t::instance().del_route_info(info);
    __android_log_print(ANDROID_LOG_ERROR, "rt_stop_play", "delete finish");

    return RT_OK;
}

//��ȡͨ��
int  XT_ROUTER_CALLBACK rt_get_chan(rt_handle_t handle, int *chan)
{
    route_info_t *info = static_cast<route_info_t *>(handle);
    if (NULL == info)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_media_cb", "bad param.handle(%p)", handle);
        return RT_ERR_BAD_PARAM;
    }

    *chan = info->chan;

    return RT_OK;
}

//��ȡsdp
int XT_ROUTER_CALLBACK rt_get_sdp(rt_handle_t handle, char *sdp, uint32_t *sdp_len)
{
    route_info_t *info = static_cast<route_info_t *>(handle);
    if ((NULL == info) || (NULL == sdp) || (NULL == sdp_len))
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_get_sdp", "bad param.handle(%p)", handle);
        return RT_ERR_BAD_PARAM;
    }

    if (NULL == info->link)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_get_sdp", "get sdp not supported.handle(%p)", handle);
        return RT_ERR_INVALID_HANDLE;
    }

    xt_media_client_status_t stat = xt_media_client_get_header(info->link, (uint8_t *)sdp, sdp_len);
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_get_sdp", "xt_media_client_get_header failed(%d),handle(%p),sdp_len(%d)", stat, handle, *sdp_len);
        return RT_ERR_GET_SDP_FAIL;
    }

    return RT_OK;
}

int XT_ROUTER_CALLBACK rt_media_cb(rt_handle_t handle, media_cb cb , void* ctx)
{
    route_info_t *info = static_cast<route_info_t *>(handle);
    if (NULL == info)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_media_cb", "bad param.handle(%p)", handle);
        return RT_ERR_BAD_PARAM;
    }

    info->cb = cb;
    info->ctx = ctx;

    __android_log_print(ANDROID_LOG_ERROR, "rt_media_cb", "handle(%p), cb(%p)", handle, info->cb);

    return RT_OK;
}

//�û��Զ�����ݵ㲥
int XT_ROUTER_CALLBACK rt_custom_play(const char *sdp, uint32_t sdp_len, uint32_t date_type, int *trans_chan, rt_handle_t *handle)
{
    if ((NULL == sdp) || (NULL == trans_chan) || (NULL == handle))
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_custom_play", "bad param.sdp(%p),trans_chan(%p),handle(%p)", sdp, trans_chan, handle);
        return RT_ERR_BAD_PARAM;
    }

    *handle = NULL;

    int srcno = -1;
    int frame_type_to_trackids[MAX_TRACK_NUM] = { 0 };

    int ret = create_src_adapter(sdp, sdp_len, srcno, *trans_chan, frame_type_to_trackids);
    if (RT_OK != ret)
    {
        return ret;
    }

    std::auto_ptr<route_info_t> info(new route_info_t);
    assert(NULL != info.get());

    info->chan = *trans_chan;
    info->srcno = srcno;
    std::copy(frame_type_to_trackids, frame_type_to_trackids + MAX_TRACK_NUM, info->frame_type_to_trackids);

    xt_set_key(srcno, (char*)sdp, sdp_len, date_type);
    xt_send_data(srcno, -1, (char*)sdp, sdp_len, 128, date_type);

    *handle = static_cast<void *>(info.get());

    __android_log_print(ANDROID_LOG_ERROR, "rt_custom_play", "handle=%p", *handle);
    //�Զ������ת����û�ж��߼��
    route_info_mgr_t::instance().add_route_info(info.release(), false);

    return RT_OK;
}

//�û��Զ�����ݵ㲥����
int XT_ROUTER_CALLBACK rt_send_data(rt_handle_t handle, void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint8_t priority)
{
    route_info_t *info = static_cast<route_info_t *>(handle);
    if (NULL == info)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_send_data", "bad param.handle(%p)", handle);
        return RT_ERR_BAD_PARAM;
    }

    if (128 == frame_type)
    {
        xt_set_key(info->srcno, (char*)data, length, data_type);
    }

    int trackid = -1;
    //处理标准流
    int index = get_frame_type_index(frame_type);
    if ((index >= 0) && (index < MAX_TRACK_NUM) && (info->frame_type_to_trackids[index] >= 0))
    {
        trackid = info->frame_type_to_trackids[index];
    }
    else
    {
        trackid = -1;
    }

    xt_send_data_in_stamp_p(info->srcno, trackid, (char*)data, length, frame_type, data_type, true, timestamp, priority);

    return RT_OK;
}

//��ѯ����
int XT_ROUTER_CALLBACK rt_query_prof(rt_handle_t handle, uint32_t *kbfs, uint32_t *fraction_lost, uint32_t *cumulative_lost, uint32_t *jitter)
{
    route_info_t *info = static_cast<route_info_t *>(handle);
    if (NULL == info)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_query_prof", "bad param.handle(%p)", handle);
        return RT_ERR_BAD_PARAM;
    }

    if (NULL == info->link)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_query_prof", "get sdp not supported.handle(%p)", handle);
        return RT_ERR_INVALID_HANDLE;
    }

    xt_rtp_prof_info_t rtp_prof;
    xt_media_client_status_t stat = xt_media_client_query_prof(info->link, &rtp_prof);
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_query_prof", "xt_media_client_query_prof failed.handle(%p), stat(%d)", handle, stat);
        return RT_ERR_QUERT_PROF_FAIL;
    }

    if (NULL != kbfs)
    {
        *kbfs = rtp_prof.recv_Bps;
    }

    if (NULL != fraction_lost)
    {
        *fraction_lost = rtp_prof.recv_packet_fraction_lost;
    }

    if (NULL != cumulative_lost)
    {
        *cumulative_lost = rtp_prof.recv_packet_cumulative_lost;
    }

    if (NULL != jitter)
    {
        *jitter = rtp_prof.recv_jitter;
    }

    return RT_OK;
}

void XT_ROUTER_CALLBACK rt_register_data_break_callback(uint32_t timeout, rt_data_break_callback_t cb, void *ctx)
{
    route_info_mgr_t::instance().register_data_break_callback(timeout, cb, ctx);
}

int XT_ROUTER_CALLBACK rt_restart_play(int device_type, const char* ip, unsigned short port, long channel, rt_handle_t old_handle, rt_handle_t *new_handle)
{
    __android_log_print(ANDROID_LOG_ERROR, "rt_restart_play", "entry ip(%s),port(%u),old_info(%p)", ip, port, old_handle);

    route_info_t *old_info = static_cast<route_info_t *>(old_handle);
    if ((NULL == ip) || (NULL == old_info))
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_restart_play", "ip(%s),old_rt_info(%p)", ip, old_info);
        return RT_ERR_BAD_PARAM;
    }

    if (NULL != old_info->link)
    {
        xt_media_client_close_link(old_info->link);
        old_info->link = NULL;
    }

    xt_media_link_handle_t link = NULL;
    xt_media_client_status_t stat = MEDIA_CLIENT_STATUS_UNKNOWN;
    if (DEVICE_TYPE_VGA == device_type)
    {
        stat = xt_media_client_rtsp_link(ip, 0, false, &link);
    }
    else
    {
        stat = xt_media_client_priv_link(ip, port, channel, LT_UDP_RTP_PRI, 0, false, &link);
    }

    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_restart_play", "xt_media_client_priv_link failed(%d),ip(%s),port(%d),channel(%ld)", stat, ip, port, channel);
        return RT_ERR_START_LINK_FAIL;
    }

    char sdp[2048];
    uint32_t sdp_len = sizeof(sdp);

    stat = xt_media_client_get_header(link, (uint8_t *)sdp, &sdp_len);
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        xt_media_client_close_link(link);
        __android_log_print(ANDROID_LOG_ERROR, "rt_restart_play", "xt_media_client_get_header failed(%d),ip(%s),port(%d),channel(%ld)", stat, ip, port, channel);
        return RT_ERR_GET_SDP_FAIL;
    }

    route_info_t *info = NULL;
    if (NULL != new_handle)
    {
        info = new (std::nothrow) route_info_t;
        info->chan = old_info->chan;
        info->srcno = old_info->srcno;
        route_info_mgr_t::instance().del_route_info(old_info);
    }
    else
    {
        info = old_info;
    }

    info->cb = NULL;
    info->ctx = NULL;
    info->link = link;

    stat = xt_media_client_play(link, data_cb, info);
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_restart_play", "xt_media_client_play failed(%d),ip(%s),port(%d),channel(%ld)", stat, ip, port, channel);
    }

    xt_set_key(info->srcno, (char*)sdp, sdp_len, 172);
    //xt_send_data(info->srcno, -1, (char*)sdp, sdp_len, 128, 172);
    xt_send_data_in_stamp_p(info->srcno,-1,(char*)sdp,sdp_len,128,172,false,-1,0);

    __android_log_print(ANDROID_LOG_ERROR, "rt_restart_play", "new info(%p)", info);

    if (NULL != new_handle)
    {
        *new_handle = info;
        route_info_mgr_t::instance().add_route_info(info);
    }

    return RT_OK;
}

int XT_ROUTER_CALLBACK rt_register_rtcp_callback(rt_handle_t handle, rt_rtcp_report_callback_t cb, void *ctx)
{
    route_info_t *info = static_cast<route_info_t *>(handle);
    if (NULL == info)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_register_rtcp_callback", "bad param.handle(%p)", handle);
        return RT_ERR_BAD_PARAM;
    }

    xt_media_client_status_t stat = ::xt_media_client_register_rtcp_callback(info->link, (xt_media_client_rtcp_report_callback_t)cb, ctx);
    if (MEDIA_CLIENT_STATUS_OK != stat)
    {
        __android_log_print(ANDROID_LOG_ERROR, "rt_register_rtcp_callback", "xt_media_client_register_rtcp_callback(%d)", stat);
        return RT_ERR_SET_PARAM_FAIL;
    }

    return RT_OK;
}
