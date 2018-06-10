#ifndef _XT_RTSP_CLIENT_TYPES_H_INCLUDED
#define _XT_RTSP_CLIENT_TYPES_H_INCLUDED


#ifdef _WIN32
    #ifdef XT_RTSP_CLIENT_EXPORTS
        #define XT_RTSP_CLIENT_API __declspec(dllexport)
    #else
        #define XT_RTSP_CLIENT_API __declspec(dllimport)
    #endif

    #define XT_RTSP_CLIENT_STDCALL  __stdcall

    #pragma pack(push, 8)
#else  //else linux
    #define XT_RTSP_CLIENT_API __attribute__((visibility("default")))
    #define XT_RTSP_CLIENT_STDCALL 
#endif

#include<stdint.h>

#define RTSP_CLIENT_IP_LEN              16
#define RTSP_CLIENT_URI_LEN             128
#define RTSP_CLIENT_SDP_LEN             1024
#define RTSP_CLIENT_RTPINFO_MAX         4

#ifdef __cplusplus
extern "C"
{
#endif

    typedef void *rtsp_client_handle_t;
    typedef void *rtsp_connection_handle_t;
    typedef void *rtsp_session_handle_t;

    typedef enum _xt_rtsp_client_status_t
    {
        RTSP_CLIENT_STATUS_OK = 0,
        RTSP_CLIENT_STATUS_UNKNOWN,
        RTSP_CLIENT_STATUS_OUTOFRESOURCES,
        RTSP_CLIENT_STATUS_BADPARAM,
        RTSP_CLIENT_STATUS_NULLPTR,
        RTSP_CLIENT_STATUS_OUTOFRANGE,
        RTSP_CLIENT_STATUS_DESTRUCTED,
        RTSP_CLIENT_STATUS_NOTSUPPORTED,
        RTSP_CLIENT_STATUS_UNINITIALIZED,
        RTSP_CLIENT_STATUS_TRY_AGAIN,
        RTSP_CLIENT_STATUS_ILLEGAL_ACTION,
        RTSP_CLIENT_STATUS_NETWORK_PROBLEM,
        RTSP_CLIENT_STATUS_INVLIAD_HANDLE,
        RTSP_CLIENT_STATUS_NOT_FOUND_EXT,
        RTSP_CLIENT_STATUS_INSUFFICIENT_BUFFER,
        RTSP_CLIENT_STATUS_INVOKE_TIMEOUT,
        RTSP_CLIENT_STATUS_NOT_DISCONNECT,
        RTSP_CLIENT_STATUS_SESSION_BAD_STATE,
        RTSP_CLIENT_STATUS_BAD_ADDR,
        RTSP_CLIENT_STATUS_SET_AUTH_FAIL
    } xt_rtsp_client_status_t;

    typedef enum _rtsp_npt_format
    {
        RTSP_NPT_FORMAT_NOT_EXISTS,      /* The structure is null.            */
        RTSP_NPT_FORMAT_NOW,             /* NPT is set to the "now" constant. */
        RTSP_NPT_FORMAT_SEC,             /* NPT is in seconds.                */
        RTSP_NPT_FORMAT_HHMMSS           /* NPT is in the hh:mm:ss format.    */
    } rtsp_npt_format;

    typedef struct _rtsp_client_config_t
    {
        uint32_t max_connections;
        char dns_ip_address[RTSP_CLIENT_IP_LEN];
    } rtsp_client_config_t;

    typedef struct _rtsp_client_connection_config_t
    {
        uint16_t max_sessions;              /* Max number of sessions per connection*/
        uint16_t waiting_describe_requests; /* Max number of Describe requests in   */
        /* the hWaitingDescribeRequests Array   */
        uint16_t transmit_queue_size;       /* Max number of messages waiting to be */
        /* sent on the connection               */
        uint16_t max_headers_in_msg;        /* Max number of headers in a message   */
        uint16_t max_urls_in_msg;           /* Max number of URLs in a connection   */
        uint16_t dns_max_results;           /* Max number of IP results from DNS    */
        uint32_t describe_response_timeout; /* in milliseconds units                */
    } rtsp_client_connection_config_t;

    typedef struct _rtsp_client_session_config_t
    {
        uint32_t response_timeout;          /* in milliseconds units*/
        uint32_t ping_transmission_timeout; /* in milliseconds units*/
    } rtsp_client_session_config_t;

    typedef struct _rtsp_npt_time_t
    {
        double seconds;
        rtsp_npt_format format;
        uint8_t hours;
        uint8_t mintues;
    } rtsp_npt_time_t;

    typedef void (XT_RTSP_CLIENT_STDCALL *xt_rtsp_disconnect_callback_t)(void *ctx, rtsp_connection_handle_t connection);

    typedef struct _rtsp_client_connect_request_t
    {
        xt_rtsp_disconnect_callback_t disconnect_callback;
        void *ctx;
    } rtsp_client_connect_request_t;

    typedef struct _rtsp_client_connect_response_t
    {
        int32_t success;
    } rtsp_client_connect_response_t;

    typedef struct _rtsp_client_describe_request_t
    {
        char uri[RTSP_CLIENT_URI_LEN];
    } rtsp_client_describe_request_t;

    typedef struct _rtsp_client_describe_response_t
    {
        uint32_t status;
        uint32_t content_length;
        uint8_t body[RTSP_CLIENT_SDP_LEN];
    } rtsp_client_describe_response_t;

    typedef struct _rtsp_client_setup_request_t
    {
        char uri[RTSP_CLIENT_URI_LEN];
        uint16_t client_rtp_port;
        uint16_t client_rtcp_port;
		uint8_t  client_demux;
		uint32_t client_demuxid;
        uint16_t server_rtp_port;
        uint16_t server_rtcp_port;
		uint8_t  server_demux;
		uint32_t server_demuxid;
        char destination[RTSP_CLIENT_IP_LEN];
        uint8_t is_unicast;
    } rtsp_client_setup_request_t;

    typedef struct _rtsp_client_setup_response_t
    {
        uint32_t status;
        uint16_t client_rtp_port;
        uint16_t client_rtcp_port;
		uint8_t  client_demux;
		uint32_t client_demuxid;
        uint16_t server_rtp_port;
        uint16_t server_rtcp_port;
		uint8_t  server_demux;
		uint32_t server_demuxid;
        char destination[RTSP_CLIENT_IP_LEN];
        uint8_t is_unicast;
    } rtsp_client_setup_response_t;

    typedef struct _rtsp_client_play_request_t
    {
        char uri[RTSP_CLIENT_URI_LEN];
        rtsp_npt_time_t range;
        float scale;
    } rtsp_client_play_request_t;

    typedef struct _rtsp_client_rtp_info_t
    {
        uint32_t seq;
        uint32_t timestamp;
        char uri[RTSP_CLIENT_URI_LEN];
    } rtsp_client_rtp_info_t;

    typedef struct _rtsp_client_play_response_t
    {
        uint32_t status;
        uint32_t rtp_info_num;
        rtsp_client_rtp_info_t rtp_infos[RTSP_CLIENT_RTPINFO_MAX];
    } rtsp_client_play_response_t;

    typedef struct _rtsp_client_pause_request_t
    {
        char uri[RTSP_CLIENT_URI_LEN];
    } rtsp_client_pause_request_t;

    typedef struct _rtsp_client_pause_response_t
    {
        uint32_t status;
    } rtsp_client_pause_response_t;

    typedef struct _rtsp_client_teardown_request_t
    {
        char uri[RTSP_CLIENT_URI_LEN];
    } rtsp_client_teardown_request_t;

    typedef struct _rtsp_client_teardown_response_t
    {
        uint32_t status;
    } rtsp_client_teardown_response_t;

#ifdef __cplusplus
}
#endif

#ifdef _WIN32
    #pragma pack(pop)
#endif

#endif //_XT_RTSP_CLIENT_TYPES_H_INCLUDED
