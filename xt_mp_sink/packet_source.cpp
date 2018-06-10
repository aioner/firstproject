//#include "stdafx.h"
#include "packet_source.h"
#include <algorithm>
#include "sink_singleton.h"

extern int g_log;
namespace xt_mp_sink
{
    bool rtp_packet_source::_insert( rtp_packet_block *item )
    {
        DEBUG_LOG(SINK_CALL,LL_NORMAL_INFO,"rtp_packet_source::_insert start..");
        bool ret = false;
        if(m_queue.empty())
        {
            m_queue.push_back(item);
            DEBUG_LOG(SINK_CALL,LL_NORMAL_INFO,"_insert | push_back emp:%d ", item->get_sn());
            return true;
        }

        rtp_packet_block* last_item = m_queue.back();

        if (IsSmaller(last_item->get_sn(), item->get_sn()))
        {
            m_queue.push_back(item);
            DEBUG_LOG(SINK_CALL,LL_NORMAL_INFO,"_insert | push_back:%d - [%d] ", last_item->get_sn(), item->get_sn());

            return true;
        }

        SOURCE_ITER itr = m_queue.begin();
        for (;itr != m_queue.end();++itr)
        {
            rtp_packet_block *frm = *itr;
            if (frm->get_sn() == item->get_sn())
            {
                //item->release();  // 此行为 2014-08-06 增 ( 重传包A 发送到接收端C1, C2...时, 可能到此; 如无此行则会出现内存泄露 )
                break;
            }

            if (IsSmaller(item->get_sn(), frm->get_sn()))
            {
                ret = true;
                m_queue.insert(itr, item);
                DEBUG_LOG(SINK_CALL,LL_INFO,"insert:[%d] - [%d] ", item->get_sn(), frm->get_sn());

                break;
            }
        }
        DEBUG_LOG(SINK_CALL,LL_NORMAL_INFO,"rtp_packet_source::_insert end!");

        return ret;
    }

    // SN比较
    bool rtp_packet_source::IsSmaller(uint16_t sn1, uint16_t sn2)
    {
        if (sn1<sn2 && (sn2-sn1)<MP_SN_JUDGE_CONDITION)
        {
            return true;
        }
        else if (sn1>sn2 && (sn1-sn2)>MP_SN_JUDGE_CONDITION)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    bool rtp_packet_source::_push( rtp_packet_block *item ,long& lastSeqNum)
    {
        DEBUG_LOG("sink_overflow",LL_NORMAL_INFO,"rtp_packet_source::_push start...");
        bool bRet = true;
        if(!item)
        {
            return false;
        }

        uint16_t seqNum = item->get_sn();
        if((uint16_t)(seqNum - lastSeqNum) > MP_SN_JUDGE_CONDITION)
        {
            return false;
        }

        if (seqNum == lastSeqNum)
        {
            return false;
        }

        bool bMark = true;
        if(m_queue.size() >= m_max_size)
        {
            SOURCE_ITER it = m_queue.begin();

            if (IsSmaller(seqNum, (*it)->get_sn()))
            {
                return false;
            }

            while(m_queue.size())
            {
                rtp_packet_block* pkt = m_queue.front();
                if (pkt->is_marker1())
                {
                    bMark = true;

                    lastSeqNum = pkt->get_sn();
                    DEBUG_LOG("sink_overflow",LL_NORMAL_INFO,"丢弃[%d]", lastSeqNum);

                    m_queue.erase(m_queue.begin());
                    pkt->release();

                    break;
                }
                else
                {
                    bMark = false;
                }
                DEBUG_LOG("sink_overflow",LL_NORMAL_INFO,"丢弃[%d]", item->get_sn());
                m_queue.erase(m_queue.begin());
                pkt->release();
            }
        }

        bRet = _insert(item); 

		//mark标记的意思是如果由于queue满出队了一个包，该帧的其他包都要删掉
        if (!bMark)
        {
            lastSeqNum = -1;
        }

        DEBUG_LOG("sink_overflow",LL_NORMAL_INFO,"rtp_packet_source::_push end!");
        return bRet;
    }

    rtp_packet_block * rtp_packet_source::_pop( bool bRelease )
    {
        rtp_packet_block * item = 0;

        if ( !m_queue.empty() )
        {
            item = m_queue.front();
            m_queue.erase(m_queue.begin());

            if (bRelease)
                item->release();
        }

        return item;
    }

    void rtp_packet_source::_erase(SOURCE_ITER pos,bool bRelease)
    {
        rtp_packet_block * item = *pos;
        m_queue.erase(pos);
        if(bRelease)
            item->release();
    }

    bool xt_mp_sink::rtp_packet_source::push( rtp_packet_block *item ,long& lastSeqNum)
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return _push(item,lastSeqNum);
    }

    rtp_packet_block * rtp_packet_source::pop( bool bRelease /*= false*/ )
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return _pop(bRelease);
    }

    void rtp_packet_source::erase( SOURCE_ITER pos,bool bRelease /*= false*/ )
    {
        boost::mutex::scoped_lock lock(m_mutex);
        _erase(pos,bRelease);
    }

    void rtp_packet_source::clear()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        std::vector<rtp_packet_block *>::iterator it;
        for (it = m_queue.begin(); it != m_queue.end(); ++it)
        {
            rtp_packet_block * item = *it;
            if (item)
            {
                if(0 > item->release())
                    utility::destruct_ptr(item);
            }
        }
        m_queue.clear();
    }

    bool rtp_packet_source::_is_sequence_sn( SOURCE_ITER pos )
    {
        if(pos == m_queue.begin())
            return true;
        SOURCE_ITER	titer = pos;
        if(pos != m_queue.end())
            titer = pos++;
        uint32_t n = 0;
        for (SOURCE_ITER iter=m_queue.begin();iter!=titer;iter++)
        {
            if(iter != m_queue.begin())
            {
                if (uint16_t((*iter)->get_sn() - n) != 1)
                {
                    return false;
                }
            }
            n = (*iter)->get_sn();
        }
        return true;
    }

    uint32_t rtp_packet_source::_output_data( int _distance,pump_param& param )
    {
        DEBUG_LOG("sink_snout",LL_NORMAL_INFO,"rtp_packet_source::_output_data start...");
        uint32_t ret = 0;
        uint32_t macro_real_size = 0;
        uint32_t macro_data_size = 0;
        rtp_macro_block * block = param.mblock;
        for (int i=0;i<=_distance;i++)
        {
            rtp_packet_block * item = _pop(false);
            if (!item)
            {
                break;
            }

            macro_data_size += (item->get_size() - item->get_sbyte());
            macro_real_size += item->get_size();
            block->set_payload_type(item->get_payload_type());
            block->set_ssrc(item->get_ssrc());
            block->set_timestamp(item->get_ts());

            //////////////////////////////////////////////////////////////////////////
            DEBUG_LOG("sink_snout",LL_NORMAL_INFO,"_output_data | %d", item->get_sn());
            //////////////////////////////////////////////////////////////////////////

            if(i == _distance)
                memcpy(&m_last_param,item->get_head(),sizeof(rv_rtp_param));

            switch (param.mode)
            {
            case MP_MEMORY_MSINK:
                {
                    block->push_byte_block(item);
                    ret ++;
                }
                break;
            case MP_RV_RTP_MSINK:
                {
                    param.queue->push(item);
                    ret ++;
                }
                break;
            case MP_BOTH_MSINK:
                {
                    block->push_byte_block(item);
                    param.queue->push(item);
                    ret ++;
                }

                break;
            default:
                break;
            }
            item->release();
        }

        block->set_data_size(macro_data_size);
        block->set_real_size(macro_real_size);

        DEBUG_LOG("sink_snout",LL_NORMAL_INFO,"rtp_packet_source::_output_data end");
        return ret;
    }

    uint32_t rtp_packet_source::size()
    {
        boost::mutex::scoped_lock lock(m_mutex);
        return m_queue.size();
    }

    ////////////////xy////////////////
    bool xt_mp_sink::rtp_packet_source::dataRecombine(rtp_packet_block* item,long& lastSeqNum,pump_param& param)
    {
        DEBUG_LOG("sink_snin",LL_NORMAL_INFO,"rtp_packet_source::dataRecombine start...");
        bool bRet = true;

        if (!item)
        {
            goto Lable;
        }

        //////////////////////////////////////////////////////////////////////////
        DEBUG_LOG("sink_snin",LL_NORMAL_INFO,"dataRecombine |sn:%d - lastSN:%d - queueSize:%d", item->get_sn(), lastSeqNum,m_queue.size());
        //////////////////////////////////////////////////////////////////////////

        //删除首帧，记录帧尾的序列号
        if(lastSeqNum == -1)
        {
            if(item->is_marker1() == 1)
            {
                lastSeqNum = item->get_sn();
            }

            item->release();
            return false;
        }

        bRet = push(item,lastSeqNum);

        if(!bRet)
        {
            //////////////////////////////////////////////////////////////////////////
            DEBUG_LOG("sink_push",LL_NORMAL_INFO,"dataRecombine | push fail sn:%d  -lastSn:%d", item->get_sn(), lastSeqNum);
            //////////////////////////////////////////////////////////////////////////
            item->release();

            boost::mutex::scoped_lock lock(m_mutex);
            checkTimeout(lastSeqNum);
            return false;
        }

Lable:
        {
            boost::mutex::scoped_lock lock(m_mutex);
            int distance = -1;
            if(m_queue.size() <= 0)
            {
                return false;
            }

            SOURCE_ITER ite = m_queue.end();
            SOURCE_ITER its = m_queue.end();
            SOURCE_ITER it = m_queue.begin();

            //找到帧尾
            SOURCE_ITER itr = m_queue.begin();
            for(;itr != m_queue.end();++itr)
            {
                if ( NULL == (*itr))
                {
                    DEBUG_LOG(SINK_ERROE,LL_ERROE,"dataRecombine | *itr[%p]m_queue_size[%d]",*itr,m_queue.size());
                    continue;
                }

                if((*itr)->is_marker1() == 1)
                {
                    ite = itr;
                    break;
                }
            }
            if(ite == m_queue.end())
            {
                checkTimeout(lastSeqNum);
                return false;
            }

            its = m_queue.begin();
            if ( NULL == (*its) )
            {
                return false;
            }

            //判断是否为一帧中的首包
            if (lastSeqNum == MP_SOURCE_SN_MAX_NUM-1)
            {
                if((*its)->get_sn() != 0)
                {
                    checkTimeout(lastSeqNum);
                    return false;
                }
            }
            else if((*its)->get_sn() != lastSeqNum +1)
            {
                checkTimeout(lastSeqNum);
                return false;
            }

            //判断SN是否连续
            int nSN = 0;
            for(;it!=m_queue.end();++it)
            {
                if ( NULL == (*it))
                {
                     DEBUG_LOG(SINK_ERROE,LL_ERROE,"dataRecombine eer 2| *itr[%p]m_queue_size[%d]",*it,m_queue.size());
                    continue;
                }

                if(it == its)
                {
                    nSN = (*it)->get_sn();
                    if(nSN == 65535)
                    {
                        nSN = -1;
                    }
                }
                else if(nSN+1 != (*it)->get_sn())
                {
                    bRet = false;
                    break;
                }
                else if((*ite)->is_smaller(*it))
                {
                    break;
                }
                else
                {
                    if(nSN == 65535)
                    {
                        nSN = -1;
                    }
                    else
                    {
                        nSN +=1;
                    }
                }
            }

            if(!bRet)
            {
                checkTimeout(lastSeqNum);
                return false;
            }

            if (NULL == (*ite))
            {
                DEBUG_LOG(SINK_ERROE,LL_ERROE,"dataRecombine eer 3| *its[%p] *ite[%p]m_queue_size[%d]",*its,*ite,m_queue.size());
                return false;
            }

            distance = std::distance(m_queue.begin(),ite);

            lastSeqNum = (*ite)->get_sn();

            _output_data(distance,param);
        }

        DEBUG_LOG("sink_snin",LL_NORMAL_INFO,"rtp_packet_source::dataRecombine end");
        return true;
    }

    void xt_mp_sink::rtp_packet_source::_dropData(long& lastSeqNum)
    {
        DEBUG_LOG("sink_drop",LL_NORMAL_INFO,"rtp_packet_source::_dropData start...");
        bool bMark = false;
        while(m_queue.size())
        {
            rtp_packet_block* item = m_queue.front();
            DEBUG_LOG("sink_drop",LL_NORMAL_INFO,"_dropData | sn:%d",item->get_sn());

            if (item->is_marker1())
            {
                bMark = true;

                lastSeqNum = item->get_sn();

                item->release();
                m_queue.erase(m_queue.begin());
                break;
            }
            else
            {
                bMark = false;
            }

            item->release();
            m_queue.erase(m_queue.begin());
        }
        if(!bMark)
        {
            lastSeqNum = -1;
        }

        DEBUG_LOG("sink_drop",LL_NORMAL_INFO,"rtp_packet_source::_dropData end!");
    }

    //判断是否超时
    void xt_mp_sink::rtp_packet_source::checkTimeout(long& lastSeqNum)
    {
        //找到帧尾
        int nCount = 0;
        SOURCE_ITER itr = m_queue.begin();
        for(;itr != m_queue.end();++itr)
        {
            if (nCount >= m_frame_cache)
            {
                break;
            }
            if((*itr)->is_marker1() == 1)
            {
                nCount++;
            }
        }

        if (nCount >= m_frame_cache)
            for (int i=0;i<nCount/2;++i)
            {
                _dropData(lastSeqNum);
            }
    }
}
