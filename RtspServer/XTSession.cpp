#include "XTSession.h"

extern void  free_sessionid(int sid);

namespace XT_RTSP
{

    rtsp_session::rtsp_session(void)
    {
    }

    rtsp_session::~rtsp_session(void)
    {
    }

    int rtsp_session::add_conn(void* conn)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        if (m_mConns.find(conn) != m_mConns.end())
        {
            return -1;
        }

        m_mConns[conn].connection = conn;

        return 0;
    }

    int rtsp_session::del_conn(void* conn)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        if (m_mConns.find(conn) == m_mConns.end())
        {
            return -1;
        }

        del_session(conn);
        m_mConns.erase(conn);

        return 0;
    }

    int rtsp_session::set_conn_addr(void* conn, string &client_ip)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        if (m_mConns.find(conn) == m_mConns.end())
        {
            return -1;
        }

        m_mConns[conn].client_ip = client_ip;

        return 0;
    }

    int rtsp_session::get_conn_addr(void* conn, string &client_ip)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        if (m_mConns.find(conn) == m_mConns.end())
        {
            return -1;
        }

        client_ip = m_mConns[conn].client_ip;

        return 0;
    }

    int rtsp_session::add_session(void* conn, void* session, int session_id, int session_no)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        m_mSessions[session].session = session;
        m_mSessions[session].session_id = session_id;
        m_mSessions[session].connection = conn;
        m_mSessions[session].session_no = session_no;

        return 0;
    }

    int rtsp_session::del_session(void* conn)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        map<void*, xt_session>::iterator itr = m_mSessions.begin();
        for (;itr != m_mSessions.end();++itr)
        {
            xt_session &s = itr->second;
            if (s.connection == conn)
            {
                free_sessionid(s.session_id);
                s.session = 0;
                s.session_id = -1;
                s.connection = 0;
                s.session_no = -1;
            }
        }

        return 0;
    }

    int rtsp_session::get_session_addr(void* session, string &client_ip)
    {
        int ret = -1;

        void *conn_ = NULL;
        do 
        {
            boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

            map<void*, xt_session>::iterator itr = m_mSessions.begin();
            for (;itr != m_mSessions.end();++itr)
            {
                xt_session &s = itr->second;
                if (s.session == session)
                {
                    conn_ = s.connection;
                    break;
                }
            }
        } while (false);

        if (conn_)
        {
            return get_conn_addr(conn_, client_ip);
        }	

        return ret;
    }

    int rtsp_session::get_session(void* session, xt_session &session_)
    {
        int ret = -1;
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        map<void*, xt_session>::iterator itr = m_mSessions.begin();
        for (;itr != m_mSessions.end();++itr)
        {
            xt_session &s = itr->second;
            if (s.session == session)
            {
                ret = 0;

                session_ = s;
                ::memcpy(session_.sink, s.sink, sizeof(s.sink));
                break;
            }
        }

        return ret;
    }

    int rtsp_session::get_connection(void* connection, std::map<void*, xt_session> &conn_)
    {
        int ret = 0;
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        map<void*, xt_session>::iterator itr = m_mSessions.begin();
        for (;itr != m_mSessions.end();++itr)
        {
            xt_session &s = itr->second;
            if (s.connection == connection)
            {
                conn_[s.session] = s;
                ::memcpy(conn_[s.session].sink, s.sink, sizeof(s.sink));
            }
        }

        return ret;
    }

    int rtsp_session::add_sink(void* session, rtsp_sink &sink)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        if (m_mSessions.find(session) == m_mSessions.end())
        {
            return -1;
        }

        rtsp_sink *sinks = m_mSessions[session].sink;
        for (int nI = 0;nI < MAX_TRACK;++nI)
        {
            rtsp_sink &s = sinks[nI];
            if (s.ip.length() == 0)
            {
                s.ip = sink.ip;
                s.trackid = sink.trackid;
                s.portA = sink.portA;
                s.portB = sink.portB;
                s.multiplex = sink.multiplex;
                s.multiplexID = sink.multiplexID;

                break;
            }
        }

        return 0;
    }

    int rtsp_session::del_sink(void* session, rtsp_sink &sink)
    {
        boost::unique_lock<boost::recursive_mutex> lock(m_mutex); 

        if (m_mSessions.find(session) == m_mSessions.end())
        {
            return -1;
        }

        rtsp_sink *sinks = m_mSessions[session].sink;
        for (int nI = 0;nI < MAX_TRACK;++nI)
        {
            rtsp_sink &s = sinks[nI];
            if (s.ip == sink.ip &&
                s.trackid == sink.trackid &&
                s.portA == sink.portA &&
                s.portB == sink.portB &&
                s.multiplex == sink.multiplex &&
                s.multiplexID == sink.multiplexID)
            {
                s.ip = "";
                s.trackid = -1;
                s.portA = 0;
                s.portB = 0;
                s.multiplex = 0;
                s.multiplexID = 0;
                break;
            }
        }

        return 0;
    }

}