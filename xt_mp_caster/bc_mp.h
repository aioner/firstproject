///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����bc_mp.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��23��
// ����������ý�����ݴ���Ԫ -- �㲥������msink_rv_rtp
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef BRODCAST_RTP_MP_ENTITY_
#define BRODCAST_RTP_MP_ENTITY_

#include "mp_caster_config.h"
#include "mp.h"
#include "mssrc_frame.h"
#include "mssrc_rtp.h"
#include "msink_rv_rtp.h"
#include "rtp_packet_block.h"
#ifdef _USE_RTP_SEND_CONTROLLER
#include "send_side_controller.h"
#include "xt_mp_caster_api.h"
#endif

namespace xt_mp_caster
{
    struct SHeader{
        unsigned uFourCC;       //ʶ���־ CHUNK_HEADER_FOURCC
        unsigned uHeaderSize;   //֡ͷ���� sizeof(SChunkHeader)
        unsigned uMediaType;    //֡����
        unsigned uChunkCount;   //��ˮ���
        unsigned uDataSize;     //���ݳ���(����ͷ�ṹ)
        unsigned uTimeStamp;    //֡ʱ���
    };

    class bc_mp : public mp
#ifdef _USE_RTP_SEND_CONTROLLER
        ,public bitrate_observer_t
#endif
    {
    public:
        bc_mp();
        virtual ~bc_mp();

    public:
        bool open(
            MP_IN bc_mp_descriptor * descriptor, 
            MP_OUT mssrc_h hmssrc, 
            MP_OUT msink_h hmsink,
            MP_OUT uint32_t *multID);

        //���¶����ش����� add by songlei 20150708
        void update_resend_flag(const int resend_flag);

        void close();
        virtual bool active(bool bActive);
        bool isAcitve(){ return m_active; }
		virtual bool pump_rtp_in(MP_IN mssrc *hmssrc,MP_IN void *rtp);
		//��������ģʽ������ִ�зְ�����
		virtual bool pump_frame_in(
			MP_IN mssrc * hmssrc,			//Ŀ��mssrc���
			MP_IN uint8_t *frame,			//����֡������
			MP_IN uint32_t framesize,		//����֡����
			MP_IN bool	   frameTS_opt,		//����֡ʱ����ⲿ������Ч�ԣ�falseΪ�ڲ�����ʱ���(mp_caster_config.h)
			MP_IN uint32_t frameTS,			//����֡ʱ��� == 0��ʾ�ڲ�����ʱ���
			MP_IN uint8_t  framePayload,	//����֡αװý������ 0xFF��ʾ��ȱʡ����(mp_caster_config.h)
			MP_IN XTFrameInfo &info,
            MP_IN uint8_t priority,//�����������ȼ�
            MP_IN bool is_std_data,
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc);

		mp_bool pump_frame_in_aac(
								MP_IN mssrc *hmssrc,			//Ŀ��mssrc���
								MP_IN uint8_t *frame,			//֡����(ȥ��˽��ͷ)
								MP_IN uint32_t framesize,		//����֡����
								MP_IN uint32_t frameTS,			//ʱ���(˽��ͷ���)			  
								MP_IN uint8_t  framePayload,    // 96
                                MP_IN uint8_t priority,
                                MP_IN bool use_ssrc,
                                MP_IN uint32_t ssrc);

        mp_bool frame_unpack_aac(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu);

		mp_bool pump_frame_in_s(
								MP_IN mssrc *hmssrc,			//Ŀ��mssrc���
								MP_IN uint8_t *frame,			//֡����(ȥ��˽��ͷ)
								MP_IN uint32_t framesize,		//����֡����
								MP_IN uint32_t frameTS,			//ʱ���(˽��ͷ���)			  
								MP_IN uint8_t  framePayload,    // 96
                                MP_IN uint8_t priority,         // �����������ȼ�
                                MP_IN uint8_t stream_mode,	    // ������������ģʽ��0Ĭ�ϱ�עRTP��1ΪPS
                                MP_IN bool use_ssrc,
                                MP_IN uint32_t ssrc);
		mp_bool pump_frame_in_hevc(
			MP_IN mssrc *hmssrc,			//Ŀ��mssrc���
			MP_IN uint8_t *frame,			//֡����(ȥ��˽��ͷ)
			MP_IN uint32_t framesize,		//����֡����
			MP_IN uint32_t frameTS,			//ʱ���(˽��ͷ���)			  
			MP_IN uint8_t  framePayload,	// 96
            MP_IN uint8_t priority,        // �����������ȼ�
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc);

        bool add_rtp_sink(rtp_sink_descriptor *descriptor, msink_h hsink);
		uint16_t get_sink_sn();
        virtual bool del_msink(msink *hmsink);

        // �Ƿ�h264
        bool is_h264(uint8_t *frame, uint32_t framesize);

		mp_bool frame_unpack(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu,MP_IN uint32_t frameTS,			//ʱ���(˽��ͷ���)			  
			MP_IN uint8_t  framePayload);
        mp_bool frame_unpack_ps(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu, uint32_t frameTS);
        mp_bool pes_unpack_rtp(rtp_mblock *mrtp, uint8_t *pes_slice, uint32_t slice_size, uint32_t mtu, uint32_t frameTS);
        mp_bool frame_unpack_hevc(rtp_mblock *mrtp, uint8_t *frame, uint32_t framesize, uint32_t mtu);
        mp_bool pump_frame_h264_to_ps(		
            MP_IN mssrc *hmssrc,			//Ŀ��mssrc���
            MP_IN uint8_t *frame,			//֡����(ȥ��˽��ͷ)
            MP_IN uint32_t framesize,		//����֡����
            MP_IN uint32_t frameTS,			//ʱ���(˽��ͷ���)			  
            MP_IN uint8_t  framePayload,    // 96
            MP_IN uint8_t priority,
            MP_IN uint8_t stream_mode,
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc);

        void set_raddr_cb(raddr_cb cb);

		void set_file_path(const char *file);

#ifdef _USE_RTP_SEND_CONTROLLER
        void register_network_changed_callback(mp_network_changed_callback_t cb, void *ctx);
#endif

    protected:
        //ִ��mssrc������
        //1��bwShaper = false�������ⲿ�û�ͨ��pump�ͷ�
        //2, bwShaper = true �����ɶ�ʱ���ͷ�
        virtual bool do_mssrc_task(uint32_t &delayMS);
        virtual bool do_msink_task();

    private:
        //do_mssrc_task�����������������
        //1��do_mssrc_task_frame������֡������Դ��������ȡ����
        //2��do_mssrc_task_rtp������RTP��������Դ��������ȡ����
        uint32_t do_mssrc_task_frame(
            mssrc_frame *hmssrc, msink_rv_rtp * hmsink, 
            uint32_t canSendSize);
        uint32_t do_mssrc_task_rtp(
            mssrc_rtp *hmssrc, msink_rv_rtp * hmsink, 
            uint32_t canSendSize);

        //������غ���ز���
        virtual void recycle_alloc_event();	
        virtual void recycle_release_event();


    private:
        bool m_active;
        uint32_t m_last_pesudo_ts;
        uint32_t m_last_pesudo_rtp_ts;
        uint16_t m_last_frame_in_rtp_sn;

#ifdef _USE_RTP_SEND_CONTROLLER
        std::auto_ptr<bitrate_controller_t> m_bitrate_controller;
        mp_network_changed_callback_t m_network_changed_callback;
        void *m_network_changed_callback_ctx;

        void on_network_change(uint32_t bitrate, uint8_t fraction_lost, uint32_t rtt);
#endif

    public:
        static raddr_cb m_raddr_cb; 
    };
}
#endif
