///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����mp_caster.h
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��23��
// ����������ý�����ݹ㲥��
//
// 1�����нӿ����Ϊ�̰߳�ȫ
// 2������boost���е�mutex����ʵ���߳�ͬ��
//
// �޶���־
// [2012-03-23]		���������汾
///////////////////////////////////////////////////////////////////////////////////////////

#ifndef MP_CASTER_
#define MP_CASTER_

#include "mp_caster_config.h"
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include "xt_mp_caster_def.h"
#include "mp.h"
#include "caster_engine.h"
#include "xt_av_check.h"

#ifdef _USE_RTP_TRAFFIC_SHAPING
#include "msec_timer.h"
#endif

namespace xt_mp_caster
{
    class caster
    {
        /*
        ��������ʽ����ӿں��๤������
        */
    private:

        caster();
        ~caster();

        bool init_caster(MP_IN caster_descriptor *descriptor);
        void end_caster();

    public:
        static caster * _instance;
    public:
        //ȫ�ֹ�����
        // 1����ģ���һ���ú������ɹ�����caster���
        static caster *init(MP_IN caster_descriptor *descriptor);
        // 2����ģ�����һ�����ú���
        static void end();
        // 3��ȡcaster���
        static caster *self();
        // 4�����������ȷ��caster�����Ч��
        static void share_lock();
        // 5�����������ȷ��caster�����Ч��
        static void share_unlock();

        //mp�๤������
    public:
        bool open_bc_mp(
            MP_IN bc_mp_descriptor* descriptor, 
            MP_OUT mp_h hmp,						//mpʵ����
            MP_OUT mssrc_h hmssrc,					//����pump_frame_in/pump_rtp_in
            MP_OUT msink_h hmsink,					//����read_rtcp_sr/read_rtcp_rr�����ɵ���del_sink�ͷ�
            MP_OUT uint32_t *multID);

        //���¶����ش����� add by songlei 20150708
        void update_resend_flag(MP_IN mp_h hmp,const int resend_flag);

        bool open_proxy_mp(	
            MP_IN proxy_mp_descriptor* descriptor, 
            MP_OUT mp_h hmp,						//mpʵ����
            MP_OUT mssrc_h hmssrc,					//����read_rtcp_sr/read_rtcp_rr
            MP_OUT msink_h hmsink);					//����read_rtcp_sr/read_rtcp_rr�����ɵ���del_sink�ͷ�
        bool open_mixer_mp(	//unsupported
            MP_IN mixer_mp_descriptor* descriptor, 
            MP_OUT mp_h hmp) { return false; }
        bool open_tanslator_mp(
            MP_IN tanslator_mp_descriptor* descriptor, 
            MP_OUT mp_h hmp) { return false; }

        bool close_mp(MP_IN mp_h hmp);
        bool active_mp(MP_IN mp_h hmp, bool bActive);

        bool add_rtp_sink(
            MP_IN mp_h hmp, 
            MP_IN rtp_sink_descriptor* descriptor, 
            MP_OUT msink_h hsink);

        bool add_stream_sink(	//unsupported
            MP_IN mp_h hmp,
            MP_IN stream_rtp_sink_descriptor* descriptor,
            MP_OUT msink_h hsink){ return false; }

        bool add_mem_sink(		
            MP_IN mp_h hmp,
            MP_IN memory_sink_descriptor* descriptor,
            MP_IN bool bActive,
            MP_OUT msink_h hsink);

        bool del_sink(MP_IN mp_h hmp, MP_IN msink_h hsink);

        bool read_rtcp_sr(
            MP_IN msink_h hsink, 
            MP_OUT rtcp_send_report *sr);
        bool read_rtcp_rr(
            MP_IN msink_h hsink, 
            MP_OUT rtcp_receive_report *rr);

        bool read_rtcp_sr(
            MP_IN mssrc_h hssrc, 
            MP_OUT rtcp_send_report *sr);
        bool read_rtcp_rr(
            MP_IN mssrc_h hssrc, 
            MP_OUT rtcp_receive_report *rr);

		bool get_sink_sn(MP_IN mp_h hmp,
			MP_OUT uint16_t *sn);

        //��������ģʽ������ִ�зְ�����
        bool pump_frame_in(
            MP_IN mp_h	hmp,				//Ŀ��mp���
            MP_IN mssrc_h hmssrc,			//Ŀ��mssrc���
            MP_IN uint8_t *frame,			//����֡������
            MP_IN uint32_t framesize,		//����֡����
            MP_IN bool	   frameTS_opt,		//����֡ʱ����ⲿ������Ч�ԣ�falseΪ�ڲ�����ʱ���(mp_caster_config.h)
            MP_IN uint32_t frameTS,			//����֡ʱ��� == 0��ʾ�ڲ�����ʱ���
            MP_IN uint8_t  framePayload,	//����֡αװý������ 0xFF��ʾ��ȱʡ����(mp_caster_config.h)
            MP_IN XTFrameInfo &info,
			MP_IN uint8_t priority,          //�����������ȼ�
            bool is_std_data,
            MP_IN bool use_ssrc,
            MP_IN uint32_t ssrc);
		//��ָ��MP��MSSRC�˿�д��RTP���ݰ�������0����ģʽ�����û���ʹ��tghelper����ڴ��ͳ�ϵͳ
		// �ڴ����ʽ����ѭ˭����˭�ͷŵ�ԭ��ֻ����������ָ�뼼�����ڴ���ύ�ɹ������ü���������
		mp_bool pump_rtp_in2(
			MP_IN mp_h	hmp,				//Ŀ��mp���
			MP_IN mssrc_h hmssrc,			//Ŀ��mssrc���
			MP_IN void *rtp);				//RTP������Ϣ��,
		//�ⲿ����byte_block���ݿ飬��ʹ���귵��Դbyte_pool
		//�������óɹ���rtp�����ü��������ӣ��û�������˺�����
		//������rtp->assign()����rtp->release()

#ifdef _USE_RTP_TRAFFIC_SHAPING
        deadline_timer_mgr_t& get_timer_mgr();
        bool use_traffic_shapping() const;
#endif
    public:
        mp_pools m_objpools;
        caster_engine *m_engine;
    private:
        bool m_bReady;

#ifdef _USE_RTP_TRAFFIC_SHAPING
        deadline_timer_mgr_t m_timer_mgr;
        bool m_use_traffic_shapping;
#endif

        private:
            std::map<mp*, mp*> m_mEntity;
            char m_license[128];
    };
}


#endif

