#include "XTSingleSrc.h"
#include <boost/thread/lock_types.hpp>

extern int _xt_destroy_src(int srcno);
extern void MEDIA_SVR_PRINT(const xt_log_level ll,const char* format,...);
XTSingleSrc  XTSingleSrc::self;

XTSingleSrc::XTSingleSrc(void)
{
}

XTSingleSrc::~XTSingleSrc(void)
{
}

int XTSingleSrc::add_src(int srcno, int srcno_prime)
{
    boost::unique_lock<boost::shared_mutex> lock(m_srcs_mutex);
    m_srcs[srcno].src_prime = srcno_prime;
    m_srcs[srcno].use = false;
    return 0;
}

int XTSingleSrc::del_src(int srcno_prime)
{
    boost::unique_lock<boost::shared_mutex> lock(m_srcs_mutex);
    int ret = -1;
    std::map<int, src_inf>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();)
    {
        if (itr->second.src_prime==srcno_prime && itr->second.use)
        {
            int srcno = itr->first;
            _xt_destroy_src(itr->first);
#ifdef _OS_WINDOWS
            itr = m_srcs.erase(itr);
#else
            m_srcs.erase(itr++);
#endif//#ifdef _OS_WINDOWS
            ret = 0;
            MEDIA_SVR_PRINT(level_info, "destroy_single_src: srcno_prime[%d] srcno[%d]", srcno_prime, srcno);
        }
        else
        {
            ++itr;
        }
    }

    return ret;
}

int XTSingleSrc::del_src2(int srcno)
{
    boost::unique_lock<boost::shared_mutex> lock(m_srcs_mutex); 

    map<int, src_inf>::iterator it = m_srcs.find(srcno);
    if (it==m_srcs.end() || !it->second.use)
    {
        return -1;
    }

    m_srcs.erase(it);

    return 0;
}

void XTSingleSrc::use_src(int srcno, bool use)
{
    boost::unique_lock<boost::shared_mutex> lock(m_srcs_mutex);
    map<int, src_inf>::iterator it = m_srcs.find(srcno);
    if (it == m_srcs.end())
    {
        return;
    }

    it->second.use = use;
}

int XTSingleSrc::get_freesrc(int srcno_prime, int &srcno)
{
    boost::unique_lock<boost::shared_mutex> lock(m_srcs_mutex);
    std::map<int, src_inf>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();++itr)
    {
        if (itr->second.src_prime==srcno_prime &&
            !itr->second.use)
        {
            srcno = itr->first;
            itr->second.use = true;
            return 0; 
        }
    }

    return -1;
}

bool XTSingleSrc::has_freesrc(int srcno_prime)
{
    boost::unique_lock<boost::shared_mutex> lock(m_srcs_mutex);
    std::map<int, src_inf>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();++itr)
    {
        if (itr->second.src_prime==srcno_prime &&
            !itr->second.use)
        {
            return true; 
        }
    }

    return false;
}

void XTSingleSrc::get_physical_srcno(const int srcno_prime,std::list<int>& srcnos)
{
    boost::unique_lock<boost::shared_mutex> lock(m_srcs_mutex);
    std::map<int, src_inf>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();++itr)
    {
        if (itr->second.src_prime == srcno_prime)
        {
            srcnos.push_back(itr->first);
        }
    }
}

int XTSingleSrc::get_src(int srcno_prime, std::vector<int> &vsrc)
{
    boost::unique_lock<boost::shared_mutex> lock(m_srcs_mutex);
    std::map<int, src_inf>::iterator itr = m_srcs.begin();
    for (;itr != m_srcs.end();++itr)
    {
        if (itr->second.src_prime==srcno_prime)
        {
            vsrc.push_back(itr->first);
        }
    }

    return 0;
}