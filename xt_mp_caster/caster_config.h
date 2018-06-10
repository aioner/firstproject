#ifndef CASTER_CONFIG_H__
#define CASTER_CONFIG_H__

#ifndef _ANDROID
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include<stdint.h>
#include "xtXml.h"

class config:private boost::noncopyable
{
private:
    config(void);
    ~config(void);

public:
    static config* instance()
    {
        return &self;
    }
    static config* _()
    {
        return &self;
    }

    const std::string& get_config_path() const
    {
        return m_fPath;
    }
    bool valid() const
    {
        return m_valid;
    }

    xtXmlNodePtr caster_cfg();

    int init_config();

    template< typename T>
    T get_node_value(const T val_default,const char* node_name)
    {
        xtXmlNodePtr node = m_config.getNode(caster_cfg(),node_name);
        if (node.IsNull())
        {
            return val_default;
        }

        const char *val = m_config.getValue(node);
        if (NULL == val)
        {
            return val_default;
        }
        return boost::lexical_cast<T>(val);
    }

public:
    long MaxSize(const long val_default);

    //<!--重传开关-->
    int resend(const int val_default);
    int pri_pkt(const int val_default);
    int sndbuf(const int val_default);
    int packresend(const int val_default);
    unsigned int ReSendAu(const unsigned int val_default);
    unsigned long ReSendLen(const unsigned long val_default);
    int multiplex_s_num(const int val_default);
    int multiplex_s_sub(const int val_default);
    uint32_t min_bitrate(const uint32_t val_default);
    uint32_t max_bitrate(const uint32_t val_default);
    uint32_t max_rtt_thr(const uint32_t val_default);
    uint32_t max_rtcp_priod_thr(const uint32_t val_default);

public:
    int logLevel(const int val_default);

private:
    // xml
    xtXml m_config;
    bool m_valid;
    // 文件路径
    std::string m_fPath;

    //作为全局静态变量来初始化，一次初始化写操作，后面全是读操作
    static config self;
};

#endif //_ANDROID

#endif // #ifndef CASTER_CONFIG_H__