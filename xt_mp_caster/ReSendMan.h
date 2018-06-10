#pragma once
#include <map>
#include <boost/thread/recursive_mutex.hpp>
#include "mp.h"

using namespace std;
using namespace xt_mp_caster;

class ReSendMan
{
public:
    ReSendMan(void);
    ~ReSendMan(void);

public:
    void setSink(void *sink){m_sink = sink;}

    // 分组缓冲
    void addSeg(rtp_block *rtp);

    // 清空缓冲
    void clrSeg();

    // 数据重发
    void reSend(uint16_t sn);

private:
    // 缓冲队列
    vector<rtp_block*> m_vecSegment;
    int m_size_seg;

    // 缓冲长度
    unsigned long m_nSeg;

    // 线程保护(缓冲队列)
    boost::recursive_mutex	m_mSeg;

    // 绑定链路
    void *m_sink;

    // 重发权标
    unsigned int m_nAu;

    // 最大权标
    unsigned int m_nMaxAu;

    // 重传包概率
    int m_nPackResend;
};
