#ifndef _XT_TCP_H_INCLUDED
#define _XT_TCP_H_INCLUDED

#ifdef _WIN32
    #ifdef XT_TCP_EXPORTS
        #define XT_TCP_API __declspec(dllexport)
    #else
        #define XT_TCP_API __declspec(dllimport)
    #endif

    #define XT_TCP_STDCALL __stdcall
#else
    #ifdef XT_TCP_EXPORTS
        #define XT_TCP_API __attribute__((visibility("default")))
    #else
        #define XT_TCP_API 
    #endif

    #define XT_TCP_STDCALL
#endif

#include <stdint.h>
#include <stdlib.h>

#define XT_TCP_INVALID_HANDLE                   NULL
#define XT_TCP_STATUS_OK                        0

#ifdef __cplusplus
extern "C"
{
#endif
    typedef void *xt_tcp_service_t;
    typedef void *xt_tcp_acceptor_t;
    typedef void *xt_tcp_socket_t;

    typedef int xt_tcp_status_t;

    enum
    {
        XT_TCP_MSG_PEEK = 1,
        XT_TCP_MSG_OOB,
        XT_TCP_MSG_MSG_DONTROUTE,
        XT_TCP_MSG_MSG_MSG_EOR
    };

    typedef int xt_tcp_message_flag;

    typedef enum _xt_tcp_shudown_type
    {
        XT_TCP_SHUT_SEND,
        XT_TCP_SHUT_RECEIVE,
        XT_TCP_SHUT_BOTH
    } xt_tcp_shutdown_type;

    typedef enum _xt_tcp_socket_opt
    {
        XT_TCP_OPT_BROADCAST,
        XT_TCP_OPT_DEBUG,
        XT_TCP_OPT_NOT_ROUTE,
        XT_TCP_OPT_KEEP_ALIVE,
        XT_TCP_OPT_SEND_BUF_SIZE,
        XT_TCP_OPT_SEND_LOW_WATERMARK,
        XT_TCP_OPT_RECV_BUF_SIZE,
        XT_TCP_OPT_RECV_LOW_WATERMARK,
        XT_TCP_OPT_REUSE_ADDR,
        XT_TCP_OPT_LINGER,
        XT_TCP_OPT_ENABLE_CONNECTION_ABORTED,
    } xt_tcp_socket_opt;

    typedef struct _xt_tcp_linger_t
    {
        uint16_t onoff;
        uint16_t linger;
    } xt_tcp_linger_t;

    typedef struct _xt_tcp_keepalive_t
    {
        uint32_t onoff;
        uint32_t idle;          //单位:s
        uint32_t interval;      //单位:s
        uint32_t count;     //windows上只能是1
    } xt_tcp_keepalive_t;

    typedef void (XT_TCP_STDCALL *xt_tcp_connect_handler_t)(void *ctx, xt_tcp_status_t stat);
    typedef void (XT_TCP_STDCALL *xt_tcp_accept_handler_t)(void *ctx, xt_tcp_status_t stat, xt_tcp_socket_t socket);
    typedef void(XT_TCP_STDCALL *xt_tcp_send_handler_t)(void *ctx, xt_tcp_status_t stat, const void *buf, size_t bytes_transferred);
    typedef void(XT_TCP_STDCALL *xt_tcp_receive_handler_t)(void *ctx, xt_tcp_status_t stat, void *buf, size_t bytes_transferred);

    XT_TCP_API xt_tcp_status_t xt_tcp_create_service(xt_tcp_service_t *pservice);
    XT_TCP_API xt_tcp_status_t xt_tcp_destroy_service(xt_tcp_service_t service);
    XT_TCP_API xt_tcp_status_t xt_tcp_service_run(xt_tcp_service_t service);
    XT_TCP_API xt_tcp_status_t xt_tcp_service_poll(xt_tcp_service_t service);
    XT_TCP_API xt_tcp_status_t xt_tcp_service_stop(xt_tcp_service_t service);
    XT_TCP_API xt_tcp_status_t xt_tcp_service_stoped(xt_tcp_service_t service);

    XT_TCP_API xt_tcp_status_t xt_tcp_create_acceptor(xt_tcp_service_t service, const char *ip, uint16_t port, uint8_t reuse_addr, xt_tcp_acceptor_t *pacceptor);
    XT_TCP_API xt_tcp_status_t xt_tcp_destroy_acceptor(xt_tcp_acceptor_t acceptor);
    XT_TCP_API xt_tcp_status_t xt_tcp_acceptor_accept(xt_tcp_acceptor_t acceptor, xt_tcp_socket_t socket, xt_tcp_accept_handler_t handler, void *ctx);
    XT_TCP_API xt_tcp_status_t xt_tcp_create_socket(xt_tcp_service_t service, int8_t open, xt_tcp_socket_t *psocket);
    XT_TCP_API xt_tcp_status_t xt_tcp_attach_socket(xt_tcp_service_t service, int native_socket, xt_tcp_socket_t *psocket);
    XT_TCP_API xt_tcp_status_t xt_tcp_destroy_socket(xt_tcp_socket_t socket);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_bind(xt_tcp_socket_t socket, const char *ip, uint16_t port);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_connect(xt_tcp_socket_t socket, const char *ip, uint16_t port, xt_tcp_connect_handler_t handler, void *ctx);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_close(xt_tcp_socket_t socket);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_cancel(xt_tcp_socket_t socket);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_set_opt(xt_tcp_socket_t socket, xt_tcp_socket_opt opt, void *val, size_t len);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_get_opt(xt_tcp_socket_t socket, xt_tcp_socket_opt opt, void *val, size_t len);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_shutdown(xt_tcp_socket_t socket, xt_tcp_shutdown_type what);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_get_remote(xt_tcp_socket_t socket, char *ip, uint16_t *port);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_get_local(xt_tcp_socket_t socket, char *ip, uint16_t *port);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_send(xt_tcp_socket_t socket, const void *buf, size_t len, xt_tcp_message_flag flags, xt_tcp_send_handler_t handler, void *ctx);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_receive(xt_tcp_socket_t socket, void *buf, size_t len, xt_tcp_message_flag flags, xt_tcp_receive_handler_t handler, void *ctx);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_write_some(xt_tcp_socket_t socket, const void *buf, size_t len, xt_tcp_send_handler_t handler, void *ctx);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_read_some(xt_tcp_socket_t socket, void *buf, size_t len, xt_tcp_receive_handler_t handler, void *ctx);
    XT_TCP_API xt_tcp_status_t xt_tcp_socket_set_keepalive(xt_tcp_socket_t socket, const xt_tcp_keepalive_t *opt);

#ifdef __cplusplus
}
#endif

#endif //_XT_TCP_H_INCLUDED
