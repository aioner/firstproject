#ifndef _RV_RTSP_CLIENT_ADAPTER_H_INCLUED
#define _RV_RTSP_CLIENT_ADAPTER_H_INCLUED

#include "RvRtspClient.h"
#include "RvRtspClientConnection.h"
#include "RvRtspClientSession.h"
#include "RvRtspUtils.h"
#include "xt_rtsp_client_types.h"
#include "spinlock.h"
#include "packaged_task.h"
#include "closed_flag.h"

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/thread/future.hpp>
#include <memory>
#include <set>

#define RV_RTSP_MAINLOOP_TIMEOUT             5

namespace xt_rtsp_client
{
    class rtsp_client_info_t : public closed_flag_t
    {
    public:
        rtsp_client_info_t();

        RvStatus client_init(const RvRtspConfiguration *config);
        void client_end();
        bool schdule_one_task();

        template<typename F>
        boost::unique_future<RvStatus> add_task(F f)
        {
            return task_queue_.add_task(f);
        }

        RvRtspHandle native_handle() const { return handle_; }
    private:
        void thread_worker(boost::promise<RvStatus> &promise, const RvRtspConfiguration *config);

        std::auto_ptr<boost::thread> thread_;
        packaged_task_queue_t<RvStatus> task_queue_;
        RvRtspHandle handle_;
    };

    class rtsp_connection_info_t
    {
    public:
        rtsp_connection_info_t(RvRtspConnectionHandle handle = NULL, rtsp_client_info_t *client = NULL, bool connected = false, xt_rtsp_disconnect_callback_t cb = NULL, void *ctx = NULL)
            :handle_(handle),
            client_(client),
            cb_(cb),
            ctx_(ctx),
            connected_(connected)
        {}

        void init(RvRtspConnectionHandle handle, rtsp_client_info_t *client)
        {
            handle_ = handle;
            client_ = client;
        }

        void set_connected()
        {
            connected_ = true;
        }
        RvRtspConnectionHandle native_handle() const { return handle_; }
        rtsp_client_info_t *get_client() { return client_; }
    private:
        RvRtspConnectionHandle handle_;
        rtsp_client_info_t *client_;

        xt_rtsp_disconnect_callback_t cb_;
        void *ctx_;
        bool connected_;
    };

    class rtsp_session_info_t
    {
    public:
        rtsp_session_info_t(RvRtspSessionHandle handle = NULL, rtsp_connection_info_t *connection = NULL)
            :handle_(handle),
            connection_(connection)
        {}

        void init(RvRtspSessionHandle handle, rtsp_connection_info_t *connection)
        {
            handle_ = handle;
            connection_ = connection;
        }

        RvRtspSessionHandle native_handle() const { return handle_; }

        rtsp_connection_info_t *get_connection() { return connection_; }
        rtsp_client_info_t *get_client() { return get_connection()->get_client(); }
    private:
        RvRtspSessionHandle handle_;
        rtsp_connection_info_t *connection_;
    };

    class rv_rtsp_client_adapter
    {
    public:
        RvStatus client_init(const RvRtspConfiguration *config, rtsp_client_info_t *&client);
        RvStatus client_end(rtsp_client_info_t *client);

        RvStatus connection_init(rtsp_client_info_t *client, const char *uri, const char *local_ip, uint16_t local_port, const RvRtspConnectionConfiguration *pConfiguration, int8_t *connected, rtsp_connection_info_t *&connection);
        RvStatus connection_end(rtsp_connection_info_t *connection);

        RvStatus get_next_cseq(rtsp_connection_info_t *connection, RvUint16 *seq);

        RvStatus connect(rtsp_connection_info_t *connection);
        RvStatus disconnect(rtsp_connection_info_t *connection);
        RvStatus describe(rtsp_connection_info_t *connection, RvUint16 seq, const char *uri);
        RvStatus get_addr(rtsp_connection_info_t *connection, char ip[RTSP_CLIENT_IP_LEN], uint16_t *port);

        RvStatus session_init(rtsp_connection_info_t *connection, RvRtspSessionConfiguration *pConfiguration, rtsp_session_info_t *&session);
        RvStatus session_end(rtsp_session_info_t *session);

        RvStatus setup(rtsp_session_info_t *session, RvUint16 seq, RvRtspTransportHeader *pTransportHeader);
        RvStatus set_uri(rtsp_session_info_t *session, const char *uri);
        RvStatus play(rtsp_session_info_t *session, RvUint16 seq, RvRtspNptTime *pNptTime, float scale);
        RvStatus pause(rtsp_session_info_t *session, RvUint16 seq);
        RvStatus teardown(rtsp_session_info_t *session, RvUint16 seq);
    };
}

#endif //_RV_RTSP_CLIENT_ADAPTER_H_INCLUED
