#ifndef _RTSP_TASK_H_INCLUDED
#define _RTSP_TASK_H_INCLUDED

#include "async_task.h"
#include "xt_rtsp_client.h"
#include "rv_rtsp_client_adapter.h"

namespace xt_rtsp_client
{
    struct rtsp_method_async_task_allocator
    {
        static void *malloc();
        static void free(void *);
    };

    template<typename RequestT, typename ResponseT>
    class rtsp_method_async_task_t : public async_task_t
    {
    public:
        typedef RequestT request_type;
        typedef ResponseT response_type;

        void done(void *response)
        {
            int32_t stat = RTSP_CLIENT_STATUS_INVOKE_TIMEOUT;

            if (NULL != response)
            {
                stat = on_rtsp_response(rtsp_handle_, (RvRtspResponse *)response);
            }

            do_callback(stat);
        }

        void release()
        {
            this->~rtsp_method_async_task_t<RequestT, ResponseT>();
            rtsp_method_async_task_allocator::free(this);
        }
    protected:
        rtsp_method_async_task_t(RvRtspHandle rtsp_handle, const RequestT *request, ResponseT *response, xt_rtsp_client_callback_t cb, void *ctx, uint32_t timeout)
            :async_task_t(async_task_t::time_traits::milliseconds(timeout)),
            request_(request),
            response_(response),
            cb_(cb),
            ctx_(ctx),
            rtsp_handle_(rtsp_handle)
        {}

        ~rtsp_method_async_task_t()
        {}

        friend class rtsp_task_factory;

        virtual int32_t on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response) { return RTSP_CLIENT_STATUS_OK; }

        void do_callback(int32_t stat)
        {
            if (NULL != cb_)
            {
                cb_(stat, ctx_);
            }
        }

        const RequestT *request_;
        ResponseT *response_;

        xt_rtsp_client_callback_t cb_;
        void *ctx_;

        RvRtspHandle rtsp_handle_;
    };

    #define DEFINE_RTSP_TASK_CONSTRUCT(method)  \
        rtsp_##method##_task_t(RvRtspHandle rtsp_handle, const rtsp_client_##method##_request_t *request, rtsp_client_##method##_response_t *response, xt_rtsp_client_callback_t cb, void *ctx, uint32_t timeout)    \
            :rtsp_method_async_task_t<rtsp_client_##method##_request_t, rtsp_client_##method##_response_t>(rtsp_handle, request, response, cb, ctx, timeout)   \
        {}

    class rtsp_connect_task_t : public rtsp_method_async_task_t<rtsp_client_connect_request_t, rtsp_client_connect_response_t>
    {
    public:
        DEFINE_RTSP_TASK_CONSTRUCT(connect)
        void done(void *response);
    };

    class rtsp_describe_task_t : public rtsp_method_async_task_t<rtsp_client_describe_request_t, rtsp_client_describe_response_t>
    {
    public:
        DEFINE_RTSP_TASK_CONSTRUCT(describe)
        int32_t on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response);
    };

    class rtsp_setup_task_t : public rtsp_method_async_task_t<rtsp_client_setup_request_t, rtsp_client_setup_response_t>
    {
    public:
        DEFINE_RTSP_TASK_CONSTRUCT(setup)
        int32_t on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response);
    };

    class rtsp_play_task_t : public rtsp_method_async_task_t<rtsp_client_play_request_t, rtsp_client_play_response_t>
    {
    public:
        DEFINE_RTSP_TASK_CONSTRUCT(play)
        int32_t on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response);
    };

    class rtsp_pause_task_t : public rtsp_method_async_task_t<rtsp_client_pause_request_t, rtsp_client_pause_response_t>
    {
    public:
        DEFINE_RTSP_TASK_CONSTRUCT(pause)
        int32_t on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response);
    };

    class rtsp_teardown_task_t : public rtsp_method_async_task_t<rtsp_client_teardown_request_t, rtsp_client_teardown_response_t>
    {
    public:
        DEFINE_RTSP_TASK_CONSTRUCT(teardown)
        int32_t on_rtsp_response(RvRtspHandle rtsp_handle, RvRtspResponse *response);
    };


    class rtsp_task_factory
    {
    public:
        template<typename RtspTaskT, typename ArgT1, typename ArgT2, typename ArgT3, typename ArgT4, typename ArgT5, typename ArgT6>
        static RtspTaskT *create_async_task(const ArgT1& arg1, const ArgT2& arg2, const ArgT3& arg3, const ArgT4& arg4, const ArgT5& arg5, const ArgT6& arg6)
        {
            return new (rtsp_method_async_task_allocator::malloc()) RtspTaskT(arg1, arg2, arg3, arg4, arg5, arg6);
        }
    };
}

#endif //_RTSP_TASK_H_INCLUDED
