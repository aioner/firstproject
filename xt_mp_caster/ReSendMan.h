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

    // ���黺��
    void addSeg(rtp_block *rtp);

    // ��ջ���
    void clrSeg();

    // �����ط�
    void reSend(uint16_t sn);

private:
    // �������
    vector<rtp_block*> m_vecSegment;
    int m_size_seg;

    // ���峤��
    unsigned long m_nSeg;

    // �̱߳���(�������)
    boost::recursive_mutex	m_mSeg;

    // ����·
    void *m_sink;

    // �ط�Ȩ��
    unsigned int m_nAu;

    // ���Ȩ��
    unsigned int m_nMaxAu;

    // �ش�������
    int m_nPackResend;
};
