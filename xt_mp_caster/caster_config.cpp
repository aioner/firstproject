#ifndef _ANDROID
#include "caster_config.h"

config config::self;

config::config(void)
:m_fPath(),m_valid(false)
{
}

int config::init_config()
{
#ifdef _OS_WINDOWS
    m_fPath.assign("D:/NetMCSet/xtrouter_config.xml");
#else
    m_fPath.assign("/etc/xtconfig/d/netmcset/xtrouter_config.xml");
#endif//#ifdef _OS_WINDOWS
    m_valid = m_config.open(m_fPath.c_str());
    if (!m_valid)
    {
        return -1;
    }
    return 0;
}

config::~config(void)
{
}

xtXmlNodePtr config::caster_cfg()
{
    xtXmlNodePtr root = m_config.getRoot();
    if (root.IsNull())
    {
        return root;
    }

    xtXmlNodePtr router = m_config.getNode(root, "caster_cfg");

    return router;
}

long config::MaxSize(const long val_default)
{
    return get_node_value<long>(val_default,"MaxSize");
}

int config::resend(const int val_default)
{
    return get_node_value<int>(val_default,"resend");
}

int config::pri_pkt(const int val_default)
{
    return get_node_value<int>(val_default,"pri_pkt");
}

int config::sndbuf(const int val_default)
{
    return get_node_value<int>(val_default,"sndbuf");
}

int config::packresend(const int val_default)
{
    return get_node_value<int>(val_default,"packresend");
}

unsigned int config::ReSendAu(const unsigned int val_default)
{
    return get_node_value<unsigned int>(val_default,"ReSendAu");
}

unsigned long config::ReSendLen(const unsigned long val_default)
{
    return get_node_value<unsigned long>(val_default,"ReSendLen");
}

int config::multiplex_s_num(const int val_default)
{
    return get_node_value<int>(val_default,"multiplex_s_num");
}
int config::multiplex_s_sub(const int val_default)
{
    return get_node_value<int>(val_default,"multiplex_s_sub");
}

uint32_t config::min_bitrate(const uint32_t val_default)
{
    return get_node_value<uint32_t>(val_default,"min_bitrate");
}

uint32_t config::max_bitrate(const uint32_t val_default)
{
    return get_node_value<uint32_t>(val_default,"max_bitrate");
}

uint32_t config::max_rtt_thr(const uint32_t val_default)
{
    return get_node_value<uint32_t>(val_default,"max_rtt_thr");
}

uint32_t config::max_rtcp_priod_thr(const uint32_t val_default)
{
    return get_node_value<uint32_t>(val_default,"max_rtcp_priod_thr");
}

int config::logLevel(const int val_default)
{
    return get_node_value<int>(val_default,"logLevel");
}

#endif //_ANDROID

