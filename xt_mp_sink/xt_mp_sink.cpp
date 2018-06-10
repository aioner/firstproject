//#include "stdafx.h"
#include "sink_singleton.h"
#include "xt_mp_sink_api.h"
#include "mp_entity.h"
#include "XTDemuxMan.h"

extern int VGA_ORDER;
const char* XT_SINK_LIB_INFO = "XT_Lib_Version: V_XT_Sink_4.26.2016.041400";

void nomorememory()
{
    DEBUG_LOG(SINK_ERROE,LL_ERROE,"%s","ERR:-nomorememory!");
}

long mp_open( xt_mp_descriptor* mp_des,p_msink_handle handle )
{
    if (!mp_des || !handle) return -1;

     DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open| ip[%s] port[%d] handle[%p] open start ....", mp_des->local_address.ip_address, mp_des->local_address.port,handle);

    int32_t ret = -1;
    handle->h_mp = NULL;
    xt_mp_sink::mp_entity * entity = NULL;
#ifdef use_recycle_entity_pool_func__
    entity = xt_mp_sink::mp_entity::malloc_entity();
    if (NULL == entity)
    {
        return -1;
    }
#else
    entity = new xt_mp_sink::mp_entity;
    if (entity == NULL)
    {
        return -1;
    }
#endif //#ifdef use_recycle_entity_pool_func__

    handle->h_mp = static_cast<mp_handle>(entity);
    if (NULL == handle->h_mp)
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"handle->h_mp = static_cast<mp_handle>(entity) fail!");
        return -1;
    }

    xt_mp_sink::mp_entity::add_entity(entity);

    ret = entity->mp_open(mp_des,handle,0,0,false);
    if (ret == 0)
    {
        return -1;
    }
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open| ret[%d] ip[%s] port[%d] handle[%p] handle->h_mp[%p] open end!", ret, mp_des->local_address.ip_address, mp_des->local_address.port,handle,handle->h_mp);
    return ret;
}

long mp_open_mult( xt_mp_descriptor* mp_des, p_msink_handle handle, uint32_t *multid)
{
    if (!mp_des || !handle) return -1;
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open| ip[%s] port[%d] handle[%p] open start ....", mp_des->local_address.ip_address, mp_des->local_address.port,handle);

    int32_t ret = -1;
    handle->h_mp = NULL;

    xt_mp_sink::mp_entity * entity = NULL;
#ifdef use_recycle_entity_pool_func__
    entity = xt_mp_sink::mp_entity::malloc_entity();
    if (NULL == entity)
    {
        return -1;
    }
#else
    entity = new xt_mp_sink::mp_entity;
    if (entity == NULL)
    {
        return -1;
    }
#endif //#ifdef use_recycle_entity_pool_func__ 
    handle->h_mp = entity;
    xt_mp_sink::mp_entity::add_entity(entity);

    ret = entity->mp_open(mp_des,handle,true,multid,false);
    if (ret == 0)
    {
        return -1;
    }
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_open_mult| ret[%d] ip[%s] port[%d] handle[%p] handle->h_mp[%p] end!", ret, mp_des->local_address.ip_address, mp_des->local_address.port,handle,handle->h_mp);
    return ret;
}

void mp_setdemux_handler(rv_context func)
{
    xt_mp_sink::mp_entity::mp_setdemux_handler(func);
}

bool mp_read_demux_rtp(
                       RV_IN void* demux,
                       RV_IN void * buf,
                       RV_IN uint32_t buf_len,
                       RV_INOUT rv_rtp *rtpH,
                       RV_INOUT void **context,
                       RV_INOUT rv_rtp_param * p,
                       RV_INOUT rv_net_address *address)
{
    return RV_ADAPTER_TRUE == read_demux_rtp(demux, buf, buf_len, rtpH, context, p, address) ? true : false;
}

bool mp_pump_demux_rtp(
                       RV_IN p_msink_handle handle,
                       RV_IN void *demux,
                       RV_IN void *buf,
                       RV_IN uint32_t buf_len,
                       RV_INOUT rv_rtp *rtpH,
                       RV_INOUT void **context,
                       RV_INOUT rv_rtp_param *p,
                       RV_INOUT rv_net_address *address)
{
    if (!handle || !buf || !rtpH || !p || !address)
    {
        return false;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(!entity)
    {
        return false;
    }

    bool ret = false;
    if (entity->m_handle.hrtp==*rtpH)
    {
        if (entity->m_isDirectOutput && VGA_ORDER>0)
        {
            entity->pump_rtp_in_sj(buf, buf_len, p, address);
        }
        else
        {
            entity->pump_rtp_in(buf, buf_len, p, address);
        }

        ret = true;
    }

    return ret;
}

bool mp_get_multinfo(p_msink_handle handle, bool *multiplex, uint32_t *multid)
{
    if (!handle || !multiplex || !multid)
    {
        return false;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(!entity)
    {
        return false;
    }

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_get_multinfo entity valide entity[%p]",entity);
        return false;
    }
    return entity->get_multinfo(multiplex, multid);
}

long mp_active( p_msink_handle handle,uint32_t bActive )
{
    if (!handle)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == NULL) return -1;
#ifdef use_recycle_entity_pool_func__
    if (!xt_mp_sink::mp_entity::check_entity(entity))
    {
        return -1;
    }
#endif //#ifdef use_recycle_entity_pool_func__
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_active");
    return entity->mp_active(bActive);
}

long mp_directoutput(p_msink_handle handle, bool direct)
{
    if (!handle)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

#ifdef use_recycle_entity_pool_func__
    if (!xt_mp_sink::mp_entity::check_entity(entity))
    {
        return -1;
    }
#endif //#ifdef use_recycle_entity_pool_func__

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_directoutput entity valide entity[%p]",entity);
        return -1;
    }

    entity->mp_directoutput(direct);
    entity->_clear();

    return 0;
}

long mp_close( p_msink_handle handle )
{
    if (!handle) return -1;
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_close| handle[%p] handle->h_mp[%p]",handle,handle->h_mp);

    int32_t ret = -1;
    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1; 
#ifdef use_recycle_entity_pool_func__
    if (!xt_mp_sink::mp_entity::check_entity(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_close| check_entity fail! handle[%p] handle->h_mp[%p]",handle,handle->h_mp);
        return -1;
    }
#endif //#ifdef use_recycle_entity_pool_func__

    do 
    {
        boost::recursive_mutex::scoped_lock lock2(xt_mp_sink::mp_entity::m_mEntityClose);
        if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
        {
            DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_close| mp_entity::is_valid fail! handle[%p] handle->h_mp[%p]",handle,handle->h_mp);
            return -1;
        }
        else
        {
            xt_mp_sink::mp_entity::del_entity(handle->h_mp);
            DEBUG_LOG(SINK_CALL,LL_INFO,"mp_close| mp_entity::del_entity handle[%p] handle->h_mp[%p]",handle,handle->h_mp);
        }
//         ret = entity->mp_close(true);
//         DEBUG_LOG(SINK_CALL,LL_INFO,"mp_close| entity->mp_close handle[%p] handle->h_mp[%p] ret[%d]",handle,handle->h_mp,ret);
        handle->h_mp = null;
    } while (0);

    ret = entity->mp_close();
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_close| entity->mp_close handle[%p] ret[%d]",handle,ret);

    entity->_clear();
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_close| entity->_clear() handle[%p]",handle);
#ifndef use_recycle_entity_pool_func__
    utility::destruct_ptr(entity);
#endif //#ifndef use_recycle_entity_pool_func__
    return ret;
}

long mp_sink_init( sink_init_descriptor * sink_des )
{
    if (!sink_des)
    {
        return -1;
    }

    // new失败防止抛出异常
    set_new_handler(nomorememory);

	sink_des->sink_thread_num *= 2;
    long ret = xt_mp_sink::sink_inst::sink_singleton()->mp_sink_init(sink_des);

#ifdef use_recycle_entity_pool_func__
    xt_mp_sink::mp_entity::init_recycle_entity_pool();
#endif //#ifdef use_recycle_entity_pool_func__

    return ret;
}

long mp_sink_end()
{
    #ifdef use_recycle_entity_pool_func__
	xt_mp_sink::mp_entity::free_entity_all();
#endif //#ifdef use_recycle_entity_pool_func__

    xt_mp_sink::sink_inst::sink_singleton()->mp_sink_end();

    xt_mp_sink::sink_inst::sink_singleton_end();

    return 0;
}

long mp_read_out_data( p_msink_handle handle,uint8_t * pDst,uint32_t size,block_params * param )
{
    if (!handle || !pDst || !param)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null)
    {
        return -1;
    }
    long nRet = entity->mp_read_out_data(pDst,size,param);
    if (nRet != -1)
    {
        DEBUG_LOG(SINK_CALL,LL_INFO,"mp_read_out_data - ret:%d ts:%d  pt:%d ds:%d", nRet, param->timestamp, param->payload_type, param->size);
    }

    return nRet;
}
long mp_read_out_data2( p_msink_handle handle,uint8_t * pDst,uint32_t size,block_params * param ,XTFrameInfo &frame)
{
    if (!handle || !pDst || !param)
    {
        DEBUG_LOG("sink_frame",LL_ERROE,"mp_read_out_data2 fail handle:%p dst:%p param:%p", handle, pDst, param);
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null)
    {
        DEBUG_LOG("sink_frame",LL_ERROE,"mp_read_out_data2 fail handle:%p", handle);
        return -1;
    }
    uint32_t info[3];
    long nRet = entity->mp_read_out_data2(pDst,size,param,info);
    if (nRet != -1)
    {
        frame.verify = info[0];
        frame.frametype = info[1];
        frame.datatype = info[2];
        //DEBUG_LOG(SINK_CALL,LL_INFO,
        //    "mp_read_out_data - ret:%d ts:%d  pt:%d ds:%d frame[%d] data[%d]", nRet, param->timestamp, param->payload_type, param->size, frame.frametype, frame.datatype);
    }

    return nRet;
}

long mp_pump_out_rtp( p_msink_handle handle,void **p_rtp_block)
{
	if (!handle)
	{
		return -1;
	}

	xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
	if(entity == null)
	{
		return -1;
	}

	long nRet = entity->mp_pump_out_rtp(p_rtp_block);
	if (nRet != -1)
	{
		//DEBUG_LOG(SINK_CALL,LL_INFO,"mp_read_out_rtp:ret - ret:%d ts:%d  pt:%d ds:%d",nRet, param->timestamp, param->payload, param->len);
	}

	return nRet;
}

long mp_read_out_rtp( p_msink_handle handle,uint8_t *pDst,uint32_t size,rv_rtp_param *param)
{
    if (!handle || !pDst || !param)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null)
    {
        return -1;
    }

    long nRet = entity->mp_read_out_rtp(pDst,size,param);
    if (nRet != -1)
    {
        //DEBUG_LOG(SINK_CALL,LL_INFO,"mp_read_out_rtp:ret - ret:%d ts:%d  pt:%d ds:%d",nRet, param->timestamp, param->payload, param->len);
    }

    return nRet;
}

long mp_query_rcv_rtcp( p_msink_handle handle,rtcp_receive_report * rtcp )
{
    if (!handle || !rtcp)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_query_rcv_rtcp entity valide entity[%p]",entity);
        return -1;
    }

    return entity->mp_query_rcv_rtcp(rtcp);
}

long mp_query_snd_rtcp( p_msink_handle handle,rtcp_send_report * rtcp )
{
    if (!handle || !rtcp)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_query_snd_rtcp entity valide entity[%p]",entity);
        return -1;
    }

    return entity->mp_query_snd_rtcp(rtcp);
}

long mp_add_rtp_remote_address( p_msink_handle handle,mp_address_descriptor * address )
{
    if (!handle || !address)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_add_rtp_remote_address entity valide entity[%p]",entity);
        return -1;
    }
    long nRet = entity->mp_add_rtp_remote_address(address);
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_add_rtp_remote_address:ret:%d ip:%s port:%d", nRet, address->ip_address, address->port);

    return nRet;
}

long mp_add_mult_rtp_remote_address( p_msink_handle handle,mp_address_descriptor * address, uint32_t multid )
{
    if (!handle || !address)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_add_mult_rtp_remote_address entity valide entity[%p]",entity);
        return -1;
    }
    long nRet = entity->mp_add_mult_rtp_remote_address(address, multid);
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_add_mult_rtp_remote_address:ret:%d ip:%s port:%d multid:%d", nRet, address->ip_address, address->port, multid);

    return nRet;
}

long mp_del_rtp_remote_address( p_msink_handle handle,mp_address_descriptor * address )
{
    if (!handle || !address)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_del_rtp_remote_address entity valide entity[%p]",entity);
        return -1;
    }

    long nRet = entity->mp_del_rtp_remote_address(address);
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_del_rtp_remote_address:ret:%d ip:%s port:%d", nRet, address->ip_address, address->port);

    return nRet;
}

long mp_clear_rtp_remote_address( p_msink_handle handle )
{
    if (!handle)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_clear_rtp_remote_address entity valide entity[%p]",entity);
        return -1;
    }

    long nRet = entity->mp_clear_rtp_remote_address();
    DEBUG_LOG(SINK_CALL,LL_INFO, "mp_clear_rtp_remote_address:%d", nRet);

    return nRet;
}

long mp_add_rtcp_remote_address( p_msink_handle handle,mp_address_descriptor * address )
{
    long ret = -1;
    if (!handle || !address)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_add_rtcp_remote_address entity valide entity[%p]",entity);
        return -1;
    }

    ret =  entity->mp_add_rtcp_remote_address(address);
    DEBUG_LOG(SINK_CALL,LL_INFO, "mp_add_rtcp_remote_address:ret:%d ip:%s port:%d", ret, address->ip_address, address->port);

    return ret;
}

long mp_del_rtcp_remote_address( p_msink_handle handle,mp_address_descriptor * address )
{
    if (!handle || !address)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_del_rtcp_remote_address entity valide entity[%p]",entity);
        return -1;
    }

    return entity->mp_del_rtcp_remote_address(address);
}

long mp_add_mult_rtcp_remote_address( p_msink_handle handle,mp_address_descriptor * address, uint32_t multid)
{
    long ret = -1;
    if (!handle || !address)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;
    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_add_mult_rtcp_remote_address entity valid entity[%p]",entity);
        return -1;
    }
    ret =  entity->mp_add_mult_rtcp_remote_address(address, multid);
    DEBUG_LOG(SINK_CALL,LL_INFO,"mp_add_mult_rtcp_remote_address:ret:%d ip:%s port:%d multid:%d", ret, address->ip_address, address->port, multid);

    return ret;
}

long mp_clear_rtcp_remote_address( p_msink_handle handle,mp_address_descriptor * address )
{
    if (!handle || !address)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_clear_rtcp_remote_address entity valide entity[%p]",entity);
        return -1;
    }

    return entity->mp_clear_rtcp_remote_address();
}

long mp_manual_send_rtcp_sr( p_msink_handle handle,uint32_t pack_size,uint32_t pack_ts )
{
    if (!handle)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_manual_send_rtcp_sr entity valide entity[%p]",entity);
        return -1;
    }

    return entity->mp_manual_send_rtcp_sr(pack_size,pack_ts);
}

long mp_manual_send_rtcp_rr( p_msink_handle handle,uint32_t ssrc,uint32_t local_ts,uint32_t ts, uint32_t sn )
{
    if (!handle)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_manual_send_rtcp_rr entity valide entity[%p]",entity);
        return -1;
    }

    return entity->mp_manual_send_rtcp_rr(ssrc,local_ts,ts,sn);
}

long mp_get_version( uint32_t * v1,uint32_t * v2,uint32_t * v3 )
{
    *v1 = MP_VERSION_MAIN;
    *v2 = MP_VERSION_SUB;
    *v3 = MP_VERSION_TEMP;

    return 0;
}

long mp_rtcp_send_dar(p_msink_handle handle, uint32_t rate, uint32_t level)
{
    return 0;
    if (!handle)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;
    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_rtcp_send_dar entity valid entity[%p]",entity);
        return -1;
    }

    return entity->mp_rtcp_send_dar(rate, level);
}

long mp_directoutput_order(bool order)
{
    return 0;
}

long mp_rtcp_send_fir(p_msink_handle handle)
{
    if (!handle)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_rtcp_send_fir entity valide entity[%p]",entity);
        return -1;
    }

    return entity->rtcp_send_fir();
}

long mp_get_xtsr(p_msink_handle handle, int &nDeviceType, int &nLinkRet, int &nFrameType,XTSSendReport *rtcp)
{
    if (!handle)
    {
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_get_xtsr entity valide entity[%p]",entity);
        return -1;
    }

    return entity->mp_get_xtsr(nDeviceType, nLinkRet, nFrameType, rtcp);
}

int mp_set_resend(int resend,int wait_resend, int max_resend, int vga_order)
{
	return xt_mp_sink::mp_entity::mp_set_resend(resend, wait_resend, max_resend, vga_order);
}

int mp_RegistSendReportEvent(p_msink_handle handle, FPSendReportOutput fpSendReportOutput, void *objUserContext)
{
    if (!handle)
    {
        return -1;
    }

    if (!xt_mp_sink::mp_entity::is_valid(handle->h_mp))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_RegistSendReportEvent entity valid entity[%p]",handle->h_mp);
        return -1;
    }

    xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(handle->h_mp);
    if(entity == null) return -1;

    if (!xt_mp_sink::mp_entity::is_valid(entity))
    {
        DEBUG_LOG(SINK_ERROE,LL_ERROE,"mp_RegistSendReportEvent entity valide entity[%p]",entity);
        return -1;
    }

    return entity->RegistSendReportEvent(fpSendReportOutput, objUserContext);
}

rv_rtp mp_query_rtp_handle(p_msink_handle h)
{
	xt_mp_sink::mp_entity * entity = static_cast<xt_mp_sink::mp_entity *>(h->h_mp);
	if(!entity)
	{
		return NULL;
	}

	return entity->m_handle.hrtp;
}