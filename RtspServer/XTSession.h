#ifndef _XT_SINK_H
#define _XT_SINK_H

#include <map>

#include <boost/thread/recursive_mutex.hpp>

using namespace std;

#define MAX_TRACK  4

struct  rtsp_sink
{
    int trackid;
    string ip;
    unsigned short portA;
    unsigned short portB;
    bool multiplex;
    unsigned int multiplexID;
};

// 会话信息
struct xt_conn
{
    void* connection;
    std::string client_ip;

    xt_conn()
        :connection(NULL),
        client_ip("")
    {}
};

// 会话信息
struct xt_session
{
    long session_no;
    void* connection;
    void* session;
    int session_id;

    rtsp_sink sink[MAX_TRACK];    

    xt_session()
        :session_no(-1),
        connection(NULL),
        session(NULL),
        session_id(-1)
    {}
};

namespace XT_RTSP
{

    class rtsp_session
    {
    public:
        static rtsp_session* inst()
        {
            static rtsp_session self_;
            return &self_;
        }
    private:
        rtsp_session(void);
        ~rtsp_session(void);

    public:
        int add_conn(void* conn);
        int del_conn(void* conn);

        int set_conn_addr(void* conn, string &client_ip);
        int get_conn_addr(void* conn, string &client_ip);

        int add_session(void* conn, void* session, int session_id, int session_no);
        int del_session(void* conn);

        int get_session_addr(void* session, string &client_ip);

        int add_sink(void* session, rtsp_sink &sink);
        int del_sink(void* session, rtsp_sink &sink);

        int get_session(void* session, xt_session &session_);
        int get_connection(void* connection, std::map<void*, xt_session> &conn_);

    private:
        boost::recursive_mutex		m_mutex;	//mutex
        std::map<void*, xt_session>		m_mSessions;//会话信息
        std::map<void*, xt_conn>		m_mConns;	//链接信息
    };

}

#endif
