#include "XTChan.h"

XTChan XTChan::self;
XTChan::XTChan(void)
{
}

XTChan::~XTChan(void)
{
}

int XTChan::add_chan(unsigned long chanid, int num /* = -1*/)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    for (int c = 0;c < num;++c)
    {
        m_chans[chanid+c].active = false;
        m_chans[chanid+c].type = 0;
    }

    return 0;
}

bool XTChan::is_chan_used(const unsigned long chaid)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    map<unsigned long, xt_chan>::iterator it = m_chans.find(chaid);
    if (it == m_chans.end())
    {
        return false;
    }

    return it->second.active;
}

int XTChan::active_chan(unsigned long chanid, bool active)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    map<unsigned long, xt_chan>::iterator it = m_chans.find(chanid);
    if (it == m_chans.end())
    {
        return -1;
    }

    it->second.active = active;

    return 0;
}

int XTChan::set_stdchan(unsigned long chanid)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    map<unsigned long, xt_chan>::iterator it = m_chans.find(chanid);
    if (it == m_chans.end())
    {
        return -1;
    }

    it->second.type = 1;

    return 0;
}

int XTChan::set_candchan(unsigned long chanid)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    map<unsigned long, xt_chan>::iterator it = m_chans.find(chanid);
    if (it== m_chans.end())
    {
        return -1;
    }

    it->second.type = 2;

    return 0;
}

int XTChan::add_sink(unsigned long chanid)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    map<unsigned long, xt_chan>::iterator it = m_chans.find(chanid);
    if (it == m_chans.end())
    {
        return -1;
    }

    it->second.sink += 1;

    return 0;
}

int XTChan::del_sink(unsigned long chanid)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    map<unsigned long, xt_chan>::iterator it = m_chans.find(chanid);
    if (it == m_chans.end())
    {
        return -1;
    }

    it->second.sink -= 1;

    return 0;
}

int XTChan::get_freechan(unsigned long &chanid)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    std::map<unsigned long, xt_chan>::iterator itr = m_chans.begin();
    for (;itr != m_chans.end();++itr)
    {
        xt_chan &chan = itr->second;
        if (!chan.active && chan.type==0)
        {
            chan.active = true;
            chanid = itr->first;
            return 0;
        }
    }

    return -1;
}

int XTChan::get_freestdchan(unsigned long &chanid)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    std::map<unsigned long, xt_chan>::iterator itr = m_chans.begin();
    for (;itr != m_chans.end();++itr)
    {
        xt_chan &chan = itr->second;
        if (!chan.active && (chan.type==1||chan.type==2))
        {
            chan.active = true;
            chanid = itr->first;
            return 0;
        }
    }

    return -1;
}

int XTChan::get_freecandchan(unsigned long &chanid)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    std::map<unsigned long, xt_chan>::iterator itr = m_chans.begin();
    for (;itr != m_chans.end();++itr)
    {
        xt_chan &chan = itr->second;
        if (!chan.active && (chan.type==1||chan.type==2))
        {
            chan.active = true;
            chanid = itr->first;
            return 0;
        }
    }

    return -1;
}

int XTChan::get_chan_info(unsigned long chanid, xt_chan &chan)
{
    boost::unique_lock<boost::recursive_mutex> lock(m_mutex);
    map<unsigned long, xt_chan>::iterator it = m_chans.find(chanid);
    if (it == m_chans.end())
    {
        return -1;
    }

    xt_chan &ch = it->second;
    ::memcpy(&chan, &ch, sizeof(xt_chan));

    return 0;
}
