#include "gw_join_sip_session_mgr.h"
#include "XTEngine.h"
#include "XTRouterLog.h"

gw_join_sip_session_mgr gw_join_sip_session_mgr::self_;

gw_join_sip_session_mgr::gw_join_sip_session_mgr()
{

}
gw_join_sip_session_mgr::~gw_join_sip_session_mgr()
{

}

long gw_join_sip_session_mgr::get_session_by_ids(const std::string&ids, session_inf_t& outinf)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.begin();
    for (;gw_jion_sip_session_mgr.end() != itr;)
    {
        if (ids == itr->second.recv_ids
            || ids == itr->second.send_ids)
        {
            outinf = itr->second;
            return 0;
        }
        else
        {
            ++itr;
        }
    }
    return -1;
}

void gw_join_sip_session_mgr::del_session_by_ids(const std::string& ids)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::del_session_by_ids :-: ids[%s] start...",ids.c_str());
    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.begin();
    for (;gw_jion_sip_session_mgr.end() != itr;)
    {
        if (ids == itr->second.recv_ids
            || ids == itr->second.send_ids)
        {
            DEBUG_LOG(DBLOGINFO,ll_info,"del_session_by_ids sessionid[%d] ids[%s] end!",itr->second.sessionid,ids.c_str());
            gw_jion_sip_session_mgr.erase(itr++);
        }
        else
        {
            ++itr;
        }
    }
}

void gw_join_sip_session_mgr::clear_session_all()
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    if (!gw_jion_sip_session_mgr.empty())
    {
        gw_jion_sip_session_mgr.clear();
    }
}

void gw_join_sip_session_mgr::del_session(const long sessionid)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::del_session sessionid[%d] start",sessionid);
    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.find(sessionid);
    if (gw_jion_sip_session_mgr.end() == itr)
    {
        return;
    }
    gw_jion_sip_session_mgr.erase(itr++);
    DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::del_session sessionid[%d] end!",sessionid);
}

long gw_join_sip_session_mgr::save_session(const session_inf_t& inf)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);

    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.find(inf.sessionid);
    if (gw_jion_sip_session_mgr.end() != itr)
    {
        //呼叫转点播
        if (MSG_SIP_2_SIP_OPEN_CALL == itr->second.type
            && MSG_OPEN_RECV == inf.type)
        {
            //清理当前会话的转发机制
            long ret = XTEngine::instance()->destroy_src(itr->second.srcno);
            if (ret < 0)
            {
                return -2;
            }
        }
        if (MSG_SIP_2_SIP_OPEN_CALL == itr->second.type
            && MSG_SIP_2_SIP_PLAY == inf.type)
        {
            long ret_code = XTEngine::instance()->rtp_close_recv(itr->second.dev_handle);
            if (ret_code < 0)
            {
                return -3;
            }
        }
        //如果点播已存在则更新
        itr->second = inf;
        DEBUG_LOG(DBLOGINFO,ll_info,"save_session |会话存在更新会话 sessionid[%d] session_type[%d] recv_ids[%s] send_ids[%s] session_buf[%d]",
            inf.sessionid,inf.type,inf.recv_ids.c_str(),inf.send_ids.c_str(),gw_jion_sip_session_mgr.size());
        return 1;
    }

    gw_jion_sip_session_mgr[inf.sessionid] = inf;

    DEBUG_LOG(DBLOGINFO,ll_info,"save_session |保存会话 gw_jion_sip_session_mgr_buf[%d] sessionid[%d] recv_ids[%s] send_ids[%s]",
        gw_jion_sip_session_mgr.size(),inf.sessionid,inf.recv_ids.c_str(),inf.send_ids.c_str());

    return 0;
}
long gw_join_sip_session_mgr::get_session(const long sessionid,session_inf_t& outinf)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.find(sessionid);
    if (gw_jion_sip_session_mgr.end() == itr)
    {
        return -1;
    } 
    outinf = itr->second;
    return 0; 
}

long gw_join_sip_session_mgr::clear_session(const long sessionid)
{
    DEBUG_LOG(DBLOGINFO,ll_info,"XTEngine::clear_session start sessionid[%d]",sessionid);
    long ret_code =0;
    do 
    {
        session_inf_t info;
        long ret_code = get_session(sessionid,info);
        if (ret_code < 0)
        {
            ret_code = 0;
            break;
        }
        /*
        switch(info.type)
        {
        case MSG_OPEN_RECV:
            {
                ret_code = XTEngine::_()->gw_join_sip_close_r(sessionid);
                if (ret_code < 0)
                {
                    ret_code = -1;
                    break;
                }
                break;
            }
        case MSG_OPEN_SEND:
            {
                ret_code = XTEngine::_()->gw_join_sip_close_s(sessionid);
                if (ret_code  < 0)
                {
                    ret_code = -2;
                    break;
                }
                break;
            }
        case MSG_OPEN_CALL:
            {
                ret_code = XTEngine::_()->gw_join_sip_close_s(sessionid);
                if (ret_code  < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"clear_session | gw_join_sip_close_s fail sessionid[%d]",sessionid);
                }
                ret_code = XTEngine::_()->gw_join_sip_close_r(sessionid);
                if (ret_code < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"clear_session | gw_join_sip_close_r fail sessionid[%d]",sessionid);
                    ret_code = -3;
                    break;
                }

                break;
            }
        case MSG_SIP_2_SIP_OPEN_CALL:
        case MSG_SIP_2_SIP_PLAY:
            {
                //清理接收机制
                ret_code = XTEngine::_()->rtp_close_recv(info.dev_handle);
                if (ret_code < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"clear_session | rtp_close_recv fail ret_code[%d] recvids[%s] sendids[%s] sessionid[%d]",
                        ret_code,info.recv_ids.c_str(),info.send_ids.c_str(),sessionid);
                }

                //清理转发机制
                ret_code = XTEngine::_()->destroy_src(info.srcno);
                if (ret_code < 0)
                {
                    DEBUG_LOG(DBLOGINFO,ll_error,"clear_session | destroy_src fail ret_code[%d] recvids[%s] sendids[%s] sessionid[%d]",
                        ret_code,info.recv_ids.c_str(),info.send_ids.c_str(),sessionid);
                    ret_code = -5;
                    break;
                }
                break;
            }
        default:
            {
                break;
            }
        }*/
        ret_code = 0;
    } while (0);

    del_session(sessionid);

    DEBUG_LOG(DBLOGINFO,ll_error,"XTEngine::clear_session end sessionid[%d]",sessionid);
    return ret_code;
}

long gw_join_sip_session_mgr::get_session(const std::string& recvids,const std::string& sendids,session_inf_t& outinf)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.begin();
    for(;gw_jion_sip_session_mgr.end() != itr;++itr)
    {
        if (itr->second.recv_ids == recvids
            && itr->second.send_ids == sendids)
        {
            outinf = itr->second;
            return 0;
        }
    }
    return -1;
}

long gw_join_sip_session_mgr::save_sdp_to_session(const long sessionid,const char *sdp, const long len_sdp)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.find(sessionid);
    if (gw_jion_sip_session_mgr.end() == itr)
    {
        return -1;
    }
    itr->second.sdp = sdp;
    itr->second.sdp_len = len_sdp;
    return 0;
}

long gw_join_sip_session_mgr::save_send_dst(const long sessionid,const rtp_dst_info_t& dst)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.find(sessionid);
    if (gw_jion_sip_session_mgr.end() == itr)
    {
        return -1;
    }
    itr->second.send_dsts.push_back(dst);
    return 0;
}

long gw_join_sip_session_mgr::update_session(const session_inf_t& session)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    std::map<long,session_inf_t>::iterator itr = gw_jion_sip_session_mgr.find(session.sessionid);
    if (gw_jion_sip_session_mgr.end() == itr)
    {
        return -1;
    }
    itr->second = session;
    return 0;
}

void gw_join_sip_session_mgr::get_session_all(std::map<long,session_inf_t>& session)
{
    boost::unique_lock<boost::recursive_mutex> lock(mutex_sessionid);
    session = gw_jion_sip_session_mgr;
}

