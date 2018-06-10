#ifndef _COMMON_MSG_H_INCLUDED
#define _COMMON_MSG_H_INCLUDED

#include<stdint.h>

#pragma pack(push, 1)

#define UDP_SESSION_MSG_VERSION   3
#define MAX_SDP_LENGTH  2048

namespace udp_session
{
    enum msg_code
    {
        get_sdp = 1,
        play = 2,
        stop = 3,
        heartbit = 4,
        get_sdp_and_play = 5,
        send_data = 6,
        send_regist=7,
        send_stop_regist=8,
        heartbit2 = 9,
        get_sdp_and_play_v1 = 10
    };

    struct msg_header_t
    {
        uint16_t code;             // ָ���
        uint16_t version;          // �汾��
        uint32_t sequence;         // ���
    };

    struct msg_request_t : msg_header_t
    {};

    struct msg_response_t : msg_header_t
    {
        int16_t error_code;
    };

    /*
    1.  ��ȡͷ����Ϣ(sdp)
    c->s :
    ָ���          2
    �汾��          2
    ���            4
    �㲥ͨ��        2

    s-c:
    ָ���          2
    �汾��          2
    ���            4

    ������          2  (0�ɹ��� else��������)
    ͷ����Ϣ����    2
    ����            n
    */

    struct get_sdp_request_msg_t : msg_request_t
    {
        uint16_t channel;
    };

    struct get_sdp_response_msg_t : msg_response_t
    {
        uint16_t length;
        uint8_t sdp[0];
    };

    /*
    2.  �����㲥
    c->s:
    ָ���          2
    �汾��          2
    ���            4
    �㲥ͨ��        2
    ����Э��        1    (1��׼���� 2�����)
    ���ñ�־        1
    ����id          4
    RTP���ն˿�     2
    RTCP���ն˿�    2 

    s->c:
    ָ���          2
    �汾��          2
    ���            4
    ������          2 (0�ɹ��� else��������)
    ����Э��        1    (1��׼���� 2�����)
    ���ñ�־        1
    ����id          4
    RTP���ն˿�     2
    RTCP���ն˿�    2
    */
    struct play_request_msg_t : msg_request_t
    {
        uint16_t channel;
        uint8_t proto;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t client_rtp_port;
        uint16_t client_rtcp_port;
    };

    struct play_response_msg_t : msg_response_t
    {
        uint8_t proto;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t server_rtp_port;
        uint16_t server_rtcp_port;
    };

    /*
    3.  ֹͣ�㲥
    c->s:
    ָ���          2
    �汾��          2
    ���            4
    �㲥ͨ��        2

    ����Э��        1
    ���ñ�־        1
    ����id          4
    RTP���ն˿�     2
    RTCP���ն˿�    2

    s->c:
    ָ���          2
    �汾��          2
    ���            4
    ������          2(0�ɹ��� else��������)
    */
    struct stop_request_msg_t : msg_request_t
    {
        uint16_t channel;
        uint8_t proto;
        uint8_t demux_flag;
        uint32_t demux_id;
        uint16_t client_rtp_port;
        uint16_t client_rtcp_port;
    };

    struct stop_response_msg_t : msg_response_t
    {};

    /*
    4.  ����
    c->s:
    ָ���          2
    �汾��          2
    ���            4
    ͨ��            2
    */
    struct heartbit_request_msg_t : msg_request_t
    {
        uint16_t channel;
    };

    /*
    5.  ���ٵ㲥
        c->s:  (��2ָ��ͬ)
        ָ���          2
        �汾��          2
        ���            4

        �㲥ͨ��        2
        ����Э��        1    (1��׼���� 2�����)
        ���ñ�־        1
        ����id          4
        RTP���ն˿�     2
        RTCP���ն˿�    2

        s->c:
        ָ���          2
        �汾��          2
        ���            4
        ������          2 (0�ɹ��� else��������)
        ����Э��        1 (1��׼����2�����)
        ���ñ�־        1
        ����id          4
        RTP���ն˿�     2
        RTCP���ն˿�    2
        ͷ����Ϣ����    2
        ����            n
     */

    struct get_sdp_and_play_request_msg_t : play_request_msg_t
    {};

    /*
    ���ٵ㲥������ͬ����5 get_sdp_and_play_request_msg_t��
    ��ԭ��������չcode���������ֶ� 1���ֽڣ�֧�ֶ�����������
    */
    struct get_sdp_and_play_request_msg_v1_t : play_request_msg_t
    {
        uint8_t code;
    };

    struct get_sdp_and_play_response_msg_t : play_response_msg_t
    {
        uint16_t length;
        uint8_t sdp[0];
    };

    /*
    6.	͸������
    c->s:
    ָ���          2
    �汾��          2
    ���            4
    �㲥ͨ��        2
    ���ݳ���        2
    ����            n

    s->c:
    ָ���          2
    �汾��          2
    ���            4
    ������          2(0�ɹ��� else��������)
    ����            2
    ����            n
    */

    struct send_data_request_msg_t : msg_request_t
    {
        uint16_t channel;
        uint16_t length;
        uint8_t content[0];
    };

    struct send_data_response_msg_t : msg_response_t
    {
        uint16_t length;
        uint8_t data[0];
    };


    struct send_regist_request_msg_t:msg_request_t
    {
        uint16_t length;
        uint8_t ids[0];
    };
    struct send_regist_reponse_msg_t:msg_response_t
    {

    };

    /*
    9.  ��ת����ַ������
    c->s:
    ָ���          2
    �汾��          2
    ���            4
    ip��ַ          4
    �˿�            2
    ͨ��            2
    */
    struct heartbit2_request_msg_t : msg_request_t
    {
        uint32_t ip;
        uint16_t port;
        uint16_t channel;
    };
}


#pragma pack(pop)

#endif //_COMMON_MSG_H_INCLUDED
