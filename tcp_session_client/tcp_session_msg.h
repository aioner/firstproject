#ifndef _TCP_SESSION_MSG_H_INCLUDED
#define _TCP_SESSION_MSG_H_INCLUDED

#include<stdint.h>

//tcpЭ��Ĳ������б�
#define TCP_SESSION_OPID_LOGOUT                         0
#define TCP_SESSION_OPID_LOGIN                          1
#define TCP_SESSION_OPID_PLAY                           2
#define TCP_SESSION_OPID_STOP                           3
#define TCP_SESSION_OPID_MULTICAST_INFO                 4
#define TCP_SESSION_OPID_KEY_INFO                       5
#define TCP_SESSION_OPID_PORT_INFO                      6       //�˿���Ϣ(���ն�ʹ�õĶ˿���Ϣ�ӷ��Ͷ˻�ȡ)
#define TCP_SESSION_OPID_CLEAR                          7       //������������ص����в���

#define TCP_SESSION_OPID_EXT_FLAG                       16      //��չ������������չ����ָ�
#define TCP_SESSION_OPID_EXT2_FLAG                      17      //��չ������RTP�˿ڸ��ã�
#define TCP_SESSION_OPID_KEY_INFO2                      18

#define TCP_SESSION_OPID_CTRL_FLAG_SL                   21
#define TCP_SESSION_OPID_CTRL_PLAY_SL                   30
#define TCP_SESSION_OPID_CTRL_STOP_SL                   32


#define TCP_SESSION_MULTICAST_ADDR_BYTES                40      //�鲥��ַ����󳤶ȣ��ο���IPV6��ַ�ַ����ṹռ�õ����ֵ��
#define TCP_SESSION_KEY_LEN                             2048

#pragma pack(push, 8)

namespace tcp_session
{
    struct msg_header_t
    {
        uint16_t flag;           //��ʶֵ
        uint16_t op_id;          //������ʶ

        uint32_t data_size;      //���ݳ���(��Ч�غ�+�������(������ͷ����))
        uint32_t fill_size;      //������ݳ���
        uint32_t data_type;      //��������

        uint32_t length() const
        {
            return data_size - fill_size;
        }
    };

    struct login_msg_t
    {
        uint16_t op_id;
        uint16_t rtp_port;          //���������ö˿�
        uint16_t channel;           //Ҫ���յ�����ͨ����
        uint16_t mode;              //��������ģʽ(0,1: ������2���鲥)  //��RTP�ĵ㲥�����д˲�����ʾRctpSsrc
    };

    struct multicast_info_t
    {
        uint16_t port;
        char     addr[TCP_SESSION_MULTICAST_ADDR_BYTES];
        uint16_t rtp_start_port;                     //�����rtp���͵���ʼ�˿�
    };

    struct play_msg_t
    {
        uint16_t op_id;
        uint16_t op_id_ext;
        uint16_t audio_rtp_port;
        uint16_t rtp_port;          //���������ö˿�
        uint16_t channel;           //Ҫ���յ�����ͨ����
        uint16_t mode;              //��������ģʽ(0,1: ������2���鲥)  //��RTP�ĵ㲥�����д˲�����ʾRctpSsrc
        uint32_t ssrc;
    };

    struct demux_play_msg_t : play_msg_t
    {
        uint32_t multiplex;
        uint32_t multiplexID;
    };

    struct sdp_info_t
    {
        uint32_t length;
        char sdp[TCP_SESSION_KEY_LEN];

        uint32_t multiplex;
        uint32_t multiplexID;

        uint32_t data_type;

        uint16_t stop_flag;
        uint16_t rtp_port;          //�Ǹ��÷�ʽ�µķ���˵�rtp���Ͷ˿�
    };

    typedef login_msg_t stop_msg_t;

    typedef demux_play_msg_t demux_stop_msg_t;

    typedef login_msg_t stop_response_msg_t;

    struct ctrl_msg_t
    {
        uint16_t op_id;
        uint16_t op_id_ext;
        double npt;
        float scale;
        uint16_t rtp_port;          //���������ö˿�
        uint16_t channel;           //Ҫ���յ�����ͨ����
        uint16_t mode;              //��������ģʽ(0,1: ������2���鲥)  //��RTP�ĵ㲥�����д˲�����ʾRctpSsrc
    };
}

#pragma pack(pop)

#endif //_TCP_SESSION_MSG_H_INCLUDED

