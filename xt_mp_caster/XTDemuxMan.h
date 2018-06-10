#pragma once

#include <vector>
#include <../rv_adapter/rv_def.h>
#include <boost/thread/recursive_mutex.hpp>

using namespace std;

#define MAX_NUM_SESSIONS 1024 //复用会话数

// 复用信息
struct _XTDemux
{
    void* demux;								//复用句柄
    int numOfSessions;							//复用会话数
    rv_handler_s handle;						//复用会话主句柄
    rv_handler_s subHandle[MAX_NUM_SESSIONS];	//复用会话子句柄
    rv_handler_s bindHandle;					//复用会话子句柄(防止demux释放)
    rv_net_address addr;						//绑定地址
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
    // 复用信息数组
    std::vector<_XTDemux*> m_vecDemux;

    // mutex
    boost::recursive_mutex m_mutex;

    // 初始化
    bool m_bInit;

    int m_numOfDemuxs;
    int m_numOfSessions;
};
