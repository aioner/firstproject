#ifndef _H_XT_ROUTER_
#define _H_XT_ROUTER_

#if defined(WIN32) || defined(WIN64)
#ifdef XT_ROUTER_EXPORTS
#define XT_ROUTER_API __declspec(dllexport)
#else
#define XT_ROUTER_API __declspec(dllimport)
#endif

#define XT_ROUTER_CALLBACK __stdcall
#else   //ֻ����linux
#define XT_ROUTER_API __attribute__((visibility("default")))
#define XT_ROUTER_CALLBACK
#endif

#include <stdint.h>

#define RT_OK                               0
#define RT_ERR_BAD_PARAM                    -1
#define RT_ERR_CREATE_SRC_FAIL              -2
#define RT_ERR_START_LINK_FAIL              -3
#define RT_ERR_GET_SDP_FAIL                 -4
#define RT_ERR_QUERT_PROF_FAIL              -5
#define RT_ERR_INVALID_HANDLE               -6
#define RT_ERR_PARSE_SDP_FAIL               -7
#define RT_ERR_SET_PARAM_FAIL               -8

typedef struct _rt_rtcp_sr_t
{
    uint32_t    mNTPtimestamp;  /* Most significant 32bit of NTP timestamp */
    uint32_t    lNTPtimestamp;  /* Least significant 32bit of NTP timestamp */
    uint32_t    timestamp;      /* RTP timestamp */
    uint32_t    packets;        /* Total number of RTP data packets transmitted by the sender
                                since transmission started and up until the time this SR packet
                                was generated. */
    uint32_t    octets;     /* The total number of payload octets (not including header or padding */
} rt_rtcp_sr_t;

typedef struct _rt_rtcp_rr_t
{
    uint32_t    fractionLost;   /* The fraction of RTP data packets from source specified by
                                SSRC that were lost since previous SR/RR packet was sent. */
    uint32_t    cumulativeLost; /* Total number of RTP data packets from source specified by
                                SSRC that have been lost since the beginning of reception. */
    uint32_t    sequenceNumber; /* Sequence number that was received from the source specified
                                by SSRC. */
    uint32_t    jitter;         /* Estimate of the statistical variance of the RTP data packet inter
                                arrival time. */
    uint32_t    lSR;            /* The middle 32 bits of the NTP timestamp received. */
    uint32_t    dlSR;           /* Delay since the last SR. */
} rt_rtcp_rr_t;

typedef void (XT_ROUTER_CALLBACK *rt_rtcp_report_callback_t)(void *ctx, uint32_t ssrc, const rt_rtcp_sr_t *sr, const rt_rtcp_rr_t *rr);


typedef void (XT_ROUTER_CALLBACK *media_cb)(void *ctx ,void *linkhandle ,void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint32_t ssrc);

typedef void *rt_handle_t;

//�����㲥
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_start_play(int device_type, const char* ip, unsigned short port, long channel, int *trans_chan, rt_handle_t *handle);

//ֹͣ�㲥
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_stop_play(rt_handle_t handle);

//��ȡͨ��
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_get_chan(rt_handle_t handle, int *chan);

//��ȡsdp
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_get_sdp(rt_handle_t handle, char *sdp, uint32_t *sdp_len);

//���ý��ص�
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_media_cb(rt_handle_t handle, media_cb cb , void* ctx);

//�û��Զ�����ݵ㲥
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_custom_play(const char *sdp, uint32_t sdp_len, uint32_t date_type, int *trans_chan, rt_handle_t *handle);

//�û��Զ�����ݵ㲥����
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_send_data(rt_handle_t handle, void *data, uint32_t length, uint32_t frame_type, uint32_t data_type, uint32_t timestamp, uint8_t priority = 0);

//��ѯ����
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_query_prof(rt_handle_t handle, uint32_t *kbfs, uint32_t *fraction_lost, uint32_t *cumulative_lost, uint32_t *jitter);

typedef int (XT_ROUTER_CALLBACK *rt_data_break_callback_t)(void *ctx, rt_handle_t handle);

XT_ROUTER_API void XT_ROUTER_CALLBACK rt_register_data_break_callback(uint32_t timeout, rt_data_break_callback_t cb, void *ctx);

//�����㲥
XT_ROUTER_API int XT_ROUTER_CALLBACK rt_restart_play(int device_type, const char* ip, unsigned short port, long channel, rt_handle_t old_handle, rt_handle_t *new_handle);

XT_ROUTER_API int XT_ROUTER_CALLBACK rt_register_rtcp_callback(rt_handle_t handle, rt_rtcp_report_callback_t cb, void *ctx);

#endif
