#pragma once

#include <vector>
#include <../rv_adapter/rv_def.h>
#include <boost/thread/recursive_mutex.hpp>

using namespace std;

#define MAX_NUM_SESSIONS 1024 //���ûỰ��

// ������Ϣ
struct _XTDemux
{
    void* demux;								//���þ��
    int numOfSessions;							//���ûỰ��
    rv_handler_s handle;						//���ûỰ�����
    rv_handler_s subHandle[MAX_NUM_SESSIONS];	//���ûỰ�Ӿ��
    rv_handler_s bindHandle;					//���ûỰ�Ӿ��(��ֹdemux�ͷ�)
    rv_net_address addr;						//�󶨵�ַ
};
class XTDemuxMan
{
private:
    XTDemuxMan(void);
    ~XTDemuxMan(void);

private:
    static XTDemuxMan m_self;

    bool create_demux(rv_session_descriptor *descriptor_session, int numOfSessions);
    void delete_demux(_XTDemux *demux);

    _XTDemux* get_demux();
    void add_session(_XTDemux *demux, rv_handler_s *hrv);
    void del_session(rv_handler_s *hrv);
    int get_session(_XTDemux* demux);

public:
    static XTDemuxMan* instance(){return &m_self;}

    void init(int numOfDemux, int numOfSessions);
    void uninit();

    bool open_session(rv_session_descriptor *descriptor, uint32_t *multiplexID, rv_handler_s *hrv);
    void close_session(rv_handler_s *hrv);

private:
    // ������Ϣ����
    std::vector<_XTDemux*> m_vecDemux;

    // mutex
    boost::recursive_mutex m_mutex;

    // ��ʼ��
    bool m_bInit;

    int m_numOfDemuxs;
    int m_numOfSessions;
};
