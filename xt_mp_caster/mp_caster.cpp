///////////////////////////////////////////////////////////////////////////////////////////
// �� �� ����mp_caster.cpp
// �� �� �ߣ�����
// ����ʱ�䣺2012��03��23��
// ����������ý�����ݹ㲥��
///////////////////////////////////////////////////////////////////////////////////////////

#include "rv_adapter/rv_def.h"
#include "rv_adapter/rv_api.h"
#include <boost/thread/shared_mutex.hpp>
#include "mp_caster.h"
#include "mp.h"
#include "mssrc_rv_rtp.h"
#include "msink_rv_rtp.h"
#include "msink_rtp.h"
#include "msink_memory.h"
#include "bc_mp.h"
#include "XTDemuxMan.h"
#include "xt_av_check.h"

#ifdef _ANDROID
#include "xt_config_cxx.h"
#endif

namespace xt_mp_caster
{
    //�����ӿ�
    caster * caster::_instance = 0;

    static boost::shared_mutex caster_locker;
    caster *caster::init(RV_IN caster_descriptor *descriptor)
    {
        boost::unique_lock<boost::shared_mutex> lock(caster_locker);
        if (!_instance)
        {
            _instance = new caster();
            if (_instance)
                if (!_instance->init_caster(descriptor))
                {
                    delete _instance;
                    _instance = 0;
                }
        }
        return _instance;
    }
    caster *caster::self()
    {
        return _instance;
    }

    void caster::end()
    {
        boost::unique_lock<boost::shared_mutex> lock(caster_locker);
        if (_instance)
        {
            delete _instance;
            _instance = NULL;
        }
    }

    void caster::share_lock()
    {
#if (MP_CASTER_SHARE_LOCK)
        caster_locker.lock_shared();
#endif
    }
    void caster::share_unlock()
    {
#if (MP_CASTER_SHARE_LOCK)
        caster_locker.unlock_shared();
#endif
    }

    caster::caster() : m_bReady(false), m_engine(0)
#ifdef _USE_RTP_TRAFFIC_SHAPING
        , m_timer_mgr(), m_use_traffic_shapping(false)
#endif
    {

    }

    caster::~caster()
    {
        end_caster();
    }

    bool caster::init_caster(MP_IN caster_descriptor *descriptor)
    {
        bool bRet = false;
        do
        {
            if (!descriptor) break;
            if (!descriptor->thread_nums) break;

            //::memset(&m_hrvMult, 0, sizeof(rv_handler_s));

            //��ʼ��Э��ջ
            rv_adapter_descriptor rv_descriptor;
            rv_descriptor.thread_nums = descriptor->rv_adapter_thread_nums;

            if (!init_rv_adapter(&rv_descriptor)) break;

            //��ʼ���ڲ�ȫ����Դ
			//�޸Ĵ˴��߳��������Դﵽ�������
            m_engine = new caster_engine(descriptor->thread_nums, CASTER_ENGINE_TIMER_SLICE);

            //��ʼ������
#ifdef _ANDROID
            int num = xt_config::router_module::get<int>("config.caster_cfg.multiplex_s_num",1);
            int sub = xt_config::router_module::get<int>("config.caster_cfg.multiplex_s_sub",1024);
#else
            int num = config::_()->multiplex_s_num(1);
            int sub = config::_()->multiplex_s_sub(1024);
#endif
            XTDemuxMan::instance()->init(num, sub);

#ifdef _USE_RTP_TRAFFIC_SHAPING
            m_use_traffic_shapping = descriptor->use_traffic_shapping;
            if (m_use_traffic_shapping)
            {
                m_timer_mgr.start(1);
            }
#endif  //_USE_RTP_TRAFFIC_SHAPING

            m_bReady = true;
            bRet = true;
        } while (false);

#ifdef _WIN32
        strcpy(m_license, "D:/NetMCSet/license");
#else
        strcpy(m_license, "/etc/xtconfig/d/netmcset/license");
#endif//#ifdef _WIN32

        return bRet;
    }

    void caster::end_caster()
    {
        if (!m_bReady) return;
        m_bReady = false;
#ifdef _USE_RTP_TRAFFIC_SHAPING
        if (m_use_traffic_shapping)
        {
            m_timer_mgr.stop();
        }
#endif
        //�����ڲ�ȫ����Դ
        if (m_engine) delete m_engine;
        m_engine = 0;

        XTDemuxMan::instance()->uninit();

        //����Э��ջ
        end_rv_adapter();
    }


    bool caster::open_bc_mp(
        MP_IN bc_mp_descriptor* descriptor,
        MP_OUT mp_h hmp,						//mpʵ����
        MP_OUT mssrc_h hmssrc,					//����pump_frame_in/pump_rtp_in
        MP_OUT msink_h hmsink,					//����read_rtcp_sr/read_rtcp_rr�����ɵ���del_sink�ͷ�
        MP_OUT uint32_t *multID)
    {
        bool bRet = false;

        {
            if (xt_av_check::inst()->check_sn(m_license)<0 &&
                m_mEntity.size() >= 12)
            {
                return false;
            }
        }

        do
        {
            if (!m_bReady) break;
            if (!descriptor) break;
            if (!hmp) break;
            if (!hmssrc) break;
            if (!hmsink) break;

            bc_mp * mp =
                static_cast<bc_mp *>(
                m_objpools.forceAllocObject((EMP_OBJECT_ID)(EMP_OBJ_MP + BROADCAST_MP)));

            if (!mp) break;
            mp->assign();
            if (!mp->open(descriptor, hmssrc, hmsink, multID))
            {
                mp->release();
                break;
            }
            hmp->hmp = mp;
            bRet = true;

            {
                m_mEntity.insert(std::make_pair(mp, mp));
            }                                                                  
        } while (false);

        return bRet;
    }

    //���¶����ش����� add by songlei 20150708
    void caster::update_resend_flag(MP_IN mp_h hmp,const int resend_flag)
    {
        do 
        {
            if (!hmp) break;
            mp_object * obj = (mp_object *)(hmp->hmp);
            if (!obj->isMP()) break;
            mp * mpObj = (mp *)obj;
            if (!mpObj) break;

            mpObj->update_resend_flag(resend_flag);

        } while (0);
    }

    bool caster::close_mp(MP_IN mp_h hmp)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hmp) break;
            mp_object * obj = (mp_object *)(hmp->hmp);
            if (!obj->isMP()) break;
            mp * mpObj = (mp *)obj;
            mpObj->active(false);

            switch(mpObj->get_type())
            {
            case BROADCAST_MP:
            case MIXER_MP:
            case TRANSLATOR_MP:
            case PROXY_MP:
                break;
            }

            //��ֹû�еȴ�mp��ʵ�˳����ڴ�й¶�������
            while(obj->use_count() > 1)
                boost::this_thread::yield();
            obj->release();
            //����û��������ֹ�ͷź�ľ��Σ��ʹ��
            hmp->hmp = 0;
            bRet = true;

            {
                if (m_mEntity.find(mpObj)!=m_mEntity.end())
                {
                    m_mEntity.erase(mpObj);
                }                                             
            }

        } while (false);

        return bRet;
    }

    bool caster::active_mp(MP_IN mp_h hmp, bool bActive)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hmp) break;
            mp_object * obj = (mp_object *)(hmp->hmp);
            if (!obj->isMP()) break;
            bRet = static_cast<mp *>(obj)->active(bActive);
        } while (false);

        return bRet;
    }

    bool caster::add_rtp_sink(
        MP_IN mp_h hmp,
        MP_IN rtp_sink_descriptor* descriptor,
        MP_OUT msink_h hsink)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hmp) break;
            if (!descriptor) break;
            if (!hsink) break;
            mp_object * obj = (mp_object *)(hmp->hmp);
            if (!obj->isMP()) break;
            mp_type mpType = static_cast<mp *>(obj)->get_type();
            switch(mpType)
            {
            case BROADCAST_MP:
                {
                    static_cast<mp *>(obj)->unique_lock();
                    bRet = static_cast<bc_mp *>(obj)->add_rtp_sink(descriptor, hsink);
                    static_cast<mp *>(obj)->unique_unlock();
                }
                break;
            case PROXY_MP:
                break;
            case MIXER_MP:
            case TRANSLATOR_MP:
                mpType = META_MP;
                break;
            }
            if (META_MP == mpType) break;

        } while (false);

        return bRet;
    }

    bool caster::add_mem_sink(
        MP_IN mp_h hmp,
        MP_IN memory_sink_descriptor* descriptor,
        MP_IN bool bActive,
        MP_OUT msink_h hsink)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hmp) break;
            if (!descriptor) break;
            if (!hsink) break;
            mp_object * obj = (mp_object *)(hmp->hmp);
            if (!obj->isMP()) break;
            msink_memory *sink =
                static_cast<msink_memory *>(caster::self()->m_objpools.forceAllocObject(
                (EMP_OBJECT_ID)(EMP_OBJ_SINK + MEMORY_MSINK)));
            if (!sink) break;
            if (!sink->open(descriptor, bActive))
            {
                sink->release();
                break;
            }
            static_cast<mp *>(obj)->unique_lock();
            static_cast<mp *>(obj)->add_msink(sink);
            static_cast<mp *>(obj)->unique_unlock();
            hsink->hmsink = sink;
            bRet = true;
        } while (false);

        return bRet;
    }

    bool caster::del_sink(MP_IN mp_h hmp, MP_IN msink_h hsink)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hmp) break;
            if (!hsink) break;

            mp_object * obj = (mp_object *)(hmp->hmp);
            if (!obj->isMP()) break;
            mp_object * sink = (mp_object *)(hsink->hmsink);
            if (!sink->isMSINK()) break;
            //msink_rv_rtp����ͨ���������ɾ�����������bc_mpͬ������
            if (RV_RTP_MSINK == static_cast<msink *>(sink)->get_type()) break;

            static_cast<mp *>(obj)->unique_lock();
            bRet = static_cast<mp *>(obj)->del_msink(static_cast<msink *>(sink));
            static_cast<mp *>(obj)->unique_unlock();

            bRet = true;
        } while (false);

        return bRet;
    }

    bool caster::read_rtcp_sr(
        MP_IN msink_h hsink,
        MP_OUT rtcp_send_report *sr)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hsink) break;
            if (!sr) break;

            mp_object * obj = (mp_object *)(hsink->hmsink);
            if (!obj->isMSINK()) break;
            if (RV_RTP_MSINK != static_cast<msink *>(obj)->get_type()) break;
            bRet = static_cast<msink_rv_rtp *>(obj)->read_rtcp_sr(sr);
        } while (false);
        return bRet;
    }
    bool caster::read_rtcp_rr(
        MP_IN msink_h hsink,
        MP_OUT rtcp_receive_report *rr)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hsink) break;
            if (!rr) break;

            mp_object * obj = (mp_object *)(hsink->hmsink);
            if (!obj->isMSINK()) break;
            if (RV_RTP_MSINK != static_cast<msink *>(obj)->get_type()) break;
            bRet = static_cast<msink_rv_rtp *>(obj)->read_rtcp_rr(rr);
        } while (false);
        return bRet;
    }

    bool caster::read_rtcp_sr(
        MP_IN mssrc_h hssrc,
        MP_OUT rtcp_send_report *sr)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hssrc) break;
            if (!sr) break;

            mp_object * obj = (mp_object *)(hssrc->hmssrc);
            if (!obj->isMSSRC()) break;
            if (RV_RTP_MSSRC != static_cast<msink *>(obj)->get_type()) break;
            bRet = static_cast<mssrc_rv_rtp *>(obj)->read_rtcp_sr(sr);
        } while (false);
        return bRet;
    }
    bool caster::read_rtcp_rr(
        MP_IN mssrc_h hssrc,
        MP_OUT rtcp_receive_report *rr)
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hssrc) break;
            if (!rr) break;

            mp_object * obj = (mp_object *)(hssrc->hmssrc);
            if (!obj->isMSSRC()) break;
            if (RV_RTP_MSSRC != static_cast<msink *>(obj)->get_type()) break;
            bRet = static_cast<mssrc_rv_rtp *>(obj)->read_rtcp_rr(rr);
        } while (false);
        return bRet;
    }

	bool caster::get_sink_sn(MP_IN mp_h hmp,
		MP_OUT uint16_t *sn)
	{
		bool bRet = false;
		do 
		{
			if (!hmp) break;
			mp_object * mpObj = (mp_object *)(hmp->hmp);
			if (!mpObj->isMP()) break;
			bc_mp * mp_obj = static_cast<bc_mp *>(mpObj);
			*sn = mp_obj->get_sink_sn();
			bRet = true;
		} while (false);

		return bRet;
	}
    bool caster::pump_frame_in(
        MP_IN mp_h	hmp,				//Ŀ��mp���
        MP_IN mssrc_h hmssrc,			//Ŀ��mssrc���
        MP_IN uint8_t *frame,			//����֡������
        MP_IN uint32_t framesize,		//����֡����
        MP_IN bool	   frameTS_opt,		//����֡ʱ����ⲿ������Ч�ԣ�falseΪ�ڲ�����ʱ���(mp_caster_config.h)
        MP_IN uint32_t frameTS,			//����֡ʱ��� == 0��ʾ�ڲ�����ʱ���
        MP_IN uint8_t  framePayload,	//����֡αװý������ 0xFF��ʾ��ȱʡ����(mp_caster_config.h)
        MP_IN XTFrameInfo &info,
        MP_IN uint8_t priority,          //�����������ȼ�
        MP_IN bool is_std_data,
        MP_IN bool use_ssrc,
        MP_IN uint32_t ssrc)//ssrc��Ϊ�ⲿ����sn����Ϊ֡���������֡�첽ѹ��
    {
        bool bRet = false;
        do
        {
            if (!m_bReady) break;
            if (!hmp) break;
            if (!hmssrc) break;
            if (!frame) break;
            if (!framesize) break;

            mp_object * mpObj = (mp_object *)(hmp->hmp);
            if (!mpObj->isMP()) break;
            mp_object * ssrcObj = (mp_object *)(hmssrc->hmssrc);
            if (!ssrcObj->isMSSRC()) break;
            mp * mp_obj = static_cast<mp *>(mpObj);
            mssrc *mssrc_obj = static_cast<mssrc *>(ssrcObj);
            bRet = mp_obj->pump_frame_in(mssrc_obj, frame, framesize, frameTS_opt, frameTS, framePayload, info,priority, is_std_data, use_ssrc, ssrc);

            //����������ȹ���
            if (!bRet) break;
#ifdef USE_POST_TASK
			m_engine->post_task(mp_obj, ECASTER_TASK_MSSRC);
#endif
        } while (false);

        return bRet;
    }

	//��ָ��MP��MSSRC�˿�д��RTP���ݰ�������0����ģʽ�����û���ʹ��tghelper����ڴ��ͳ�ϵͳ
	// �ڴ����ʽ����ѭ˭����˭�ͷŵ�ԭ��ֻ����������ָ�뼼�����ڴ���ύ�ɹ������ü���������
    mp_bool caster::pump_rtp_in2(
		MP_IN mp_h	hmp,				//Ŀ��mp���
		MP_IN mssrc_h hmssrc,			//Ŀ��mssrc���
		MP_IN void *rtp)				//RTP������Ϣ��
	{
		bool bRet = false;
		do
		{
			if (!m_bReady) break;
			if (!hmp) break;
			if (!hmssrc) break;

			mp_object * mpObj = (mp_object *)(hmp->hmp);
			if (!mpObj->isMP()) break;
			mp_object * ssrcObj = (mp_object *)(hmssrc->hmssrc);
			if (!ssrcObj->isMSSRC()) break;
			mp * mp_obj = static_cast<mp *>(mpObj);
			mssrc *mssrc_obj = static_cast<mssrc *>(ssrcObj);
			bRet = mp_obj->pump_rtp_in(mssrc_obj, rtp);
			//���ڰ�ȫ���ǣ��˴�ֱ�ӷ���
		} while (false);

		return bRet;
	}

#ifdef _USE_RTP_TRAFFIC_SHAPING
    deadline_timer_mgr_t& caster::get_timer_mgr()
    {
        return m_timer_mgr;
    }

    bool caster::use_traffic_shapping() const
    {
        return m_use_traffic_shapping;
    }
#endif
}


