///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����xt_mp_caster_def.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��23��
// ������������ͼ�¿ƹ�˾mp�㲥�����
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef XT_MP_CASTER_DEFINE_
#define XT_MP_CASTER_DEFINE_

#include<stdint.h>

#ifdef XT_MP_CASTER_EXPORTS
#include <xt_mp_def.h>
#include <rv_adapter/rv_def.h>
#else
#include <xt_mp_def.h>
#include <rv_adapter/rv_def.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct caster_descriptor_
    {
        uint32_t thread_nums;				//caster �̳߳�����
        uint32_t rv_adapter_thread_nums;	//rv_adapter�̳߳�����
        uint32_t numberOfSessions;			//���ûỰ��
        mp_bool use_traffic_shapping;       //�Ƿ�����������
    } caster_descriptor;

    //mp��������
    typedef enum mp_type_
    {
        META_MP,			//����ʹ������
        BROADCAST_MP,		//�㲥��MP 1:N
        PROXY_MP,			//������MP 1:N
        MIXER_MP,			//�����MP M:N
        TRANSLATOR_MP,		//ת����MP 1:1
    } mp_type;

    //mssrc��������
    typedef enum mssrc_type_
    {
        META_MSSRC,
        FRAME_MSSRC,
        RTP_MSSRC,
        RV_RTP_MSSRC,		//radvision Э��ջ����ssrc
    } mssrc_type;

    //msink��������
    typedef enum msink_type_
    {
        META_MSINK,
        RTP_MSINK,
        STREAM_RTP_MSINK,
        MEMORY_MSINK,
        RV_RTP_MSINK,		//radvision Э��ջ����sink
    } msink_type;

    //����������
    typedef enum stream_mode_
    {
        STD_RTP_STREAM = 0,  //��׼RTP��
        PS_RTP_STREAM,       //����GB28181��PS��
    } stream_mode;

    typedef struct bc_mp_descriptor_	//�㲥��MP������
    {
        //rv_adapater ������Ϣ
        //	rv_adapaterҪ���RTP���յ�ַ����������Ϊż��
        //	Ĭ�Ͽ�����RTCP���յ�ַΪRTP��ַ�˿�+1
        //	manual_rtcp���Կ��ǿ�����ر�rtcp�����
        //	��Ϊ����������Ĵ�����
        rv_net_address local_address;
        mp_bool manual_rtcp;
        //media stream������Ϣ
        uint32_t max_bandwidth;		// ý���ƽ������	(unit. bytes/s), 0 -- ��ʾ�رմ�������
        mssrc_type ssrc_type;		// ý��Դ����
        mp_bool active_now;			// ���̼����־

        uint8_t msink_multicast_rtp_ttl;			//�鲥���RTP��TTLֵ
        uint8_t msink_multicast_rtcp_ttl;			//�鲥���RTCP��TTLֵ

        mp_bool multiplex;                      //�˿ڸ���ģʽ
    } bc_mp_descriptor;

    typedef struct proxy_mp_descriptor_	//������MP������
    {
        //mssrc_rv_rtp
        rv_net_address listen_address;
        mp_bool mssrc_manual_rtcp;
        //msink_rv_rtp
        rv_net_address local_address;
        mp_bool msink_manual_rtcp;
        //media stream������Ϣ
        uint32_t max_bandwidth;		// ý���ƽ������	(unit. bytes/s), 0 -- ��ʾ�رմ�������
        mp_bool active_now;			// ���̼����־

        //�鲥�������
        mp_bool mssrc_multicast_rtp_opt;				//�Ƿ�����鲥RTP����
        rv_net_address listen_mulitcast_rtp_address;	//�鲥RTP������ַ
        mp_bool mssrc_multicast_rtcp_opt;				//�Ƿ�����鲥RTCP����
        rv_net_address listen_mulitcast_rtcp_address;	//�鲥RTCP������ַ

        uint8_t msink_multicast_rtp_ttl;			//�鲥���RTP��TTLֵ
        uint8_t msink_multicast_rtcp_ttl;			//�鲥���RTCP��TTLֵ

        mp_bool multiplex;                      //�˿ڸ���ģʽ
    } proxy_mp_descriptor;

    typedef struct mixer_mp_descriptor_	//�����MP������
    {
        //unsupported
    } mixer_mp_descriptor;

    typedef struct tanslator_mp_descriptor_	//ת����MP������
    {
        //unsupported
    } tanslator_mp_descriptor;


    typedef struct rtp_sink_descriptor_
    {
        rv_net_address rtp_address;		//rtp�����ַ
        mp_bool rtcp_opt;				//rtcp������Чѡ��
        rv_net_address rtcp_address;	//rtcp�����ַ
        mp_bool multiplex;				//�Ƿ�˿ڸ���
        uint32_t multiplexID;			//����ID
    } rtp_sink_descriptor;

    typedef struct stream_rtp_sink_descriptor_
    {
        //unsupported
    } stream_rtp_sink_descriptor;

    typedef void (*MemorySinkEventHandler_CB)(
        MP_IN mp_h		hmp,			//�����������mp���
        MP_IN msink_h	hsink,			//�������������msink���
        MP_IN mp_context context,		//�û�����������
        MP_IN uint8_t * buf,			//���ݿռ�
        MP_IN uint32_t size);			//���ݿռ��С

    typedef void (*MemorySinkEventHandlerEx_CB)(
        MP_IN mp_h		hmp,			//�����������mp���
        MP_IN msink_h	hsink,			//�������������msink���
        MP_IN mp_context context,		//�û�����������
        MP_IN msink_block buf			//���ݿռ����byte_block������û�������֧���ڴ��,������ת������Ȩ
        );

    typedef struct memory_sink_descriptor_
    {
        /*
        ������onMemorySinkExEvent��msink����ʹ�øûص��ӿ�
        */
        MemorySinkEventHandler_CB	onMemorySinkEvent;		//�¼��ص��ӿ�
        MemorySinkEventHandlerEx_CB onMemorySinkExEvent;	//�¼��ص��ӿڣ����ȼ�����onMemorySinkEvent
        mp_context context;
    } memory_sink_descriptor;

    typedef struct _XTFrameInfo
    {
        unsigned int verify;
        unsigned int frametype;     //֡�������ͣ���ʾΪH264��H265��AAC������һ��
        unsigned int datatype;      //�豸����
        unsigned int streamtype;    //�����ͣ�0Ϊ��׼RTP����1Ϊ����PS��������������
    }XTFrameInfo;

    typedef void (*raddr_cb)(void *hmp,rv_net_address *addr);
#ifdef __cplusplus
}
#endif

#endif
