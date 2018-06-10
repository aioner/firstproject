#ifndef _PORTS_MGR_IMPL_H_INCLUDED
#define _PORTS_MGR_IMPL_H_INCLUDED

#include "xt_media_client.h"
#include "ports_mgr.h"
#include "utility/singleton.h"
#include <vector>
extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{
    class ports_mgr_impl : public ports_mgr_t, public xt_utility::singleton<ports_mgr_impl>
    {
    public:
        ports_mgr_impl(xt_ports_mgr_get_callback_t get_ports_cb = NULL, xt_ports_mgr_free_callback_t free_ports_cb = NULL, void *ctx = NULL)
            :get_ports_cb_(get_ports_cb),
            free_ports_cb_(free_ports_cb),
            ctx_(ctx),
            demux_port1(0),
            demux_port2(0)
        {
        }

        void register_callbacks(xt_ports_mgr_get_callback_t get_ports_cb, xt_ports_mgr_free_callback_t free_ports_cb, void *ctx)
        {
            get_ports_cb_ = get_ports_cb;
            free_ports_cb_ = free_ports_cb;
            ctx_ = ctx;
        }

        bool update_demux_port()
        {
            uint16_t ports[2]={0};
            int32_t ret = get_ports_cb_(ctx_,2, ports, 1);
            if (0 > ret)
            {
                return false;
            }
            demux_port1 = ports[0];
            demux_port2 = ports[1];
            return true;
        }

        void init_demux_ports()
        {
            uint16_t ports[2] = {0};
            get_multiplex_ports(ports,2,1);
           md_log(md_log_info, "init_demux_ports port1[%d] port2[%d]",ports[0],ports[1]);
        }

        bool get_multiplex_ports( uint16_t *ports,uint32_t num,int32_t opt)
        {
            bool ret_code = true;
            do 
            {
                if (0 == demux_port1 || 0 == demux_port2)
                {
                    int32_t ret = get_ports_cb_(ctx_,num, ports, opt);
                    if (0 > ret)
                    {
                        ret_code = false;
                        break;
                    }
                    demux_port1 = ports[0];
                    demux_port2 = ports[1];

                }
                else
                {
                    if (1 == num)
                    {
                        ports[0] = demux_port1;
                    }
                    if (2 == num)
                    {
                        ports[0] = demux_port1;
                        ports[1] = demux_port2;
                    }
                }
            } while (0);
            return ret_code;
        }

        bool is_demux_ports(uint16_t port)
        {
            if (port == demux_port1
                || port == demux_port2)
            {
                return true;
            }

            return false;
        }

        bool get_ports(uint32_t num, uint16_t *ports, bool demux,int32_t opt)
        {
            bool ret_code = true;
            do 
            {
                if (NULL == get_ports_cb_)
                {
                    ret_code = false;
                    break;
                }
                if (demux)
                {
                    ret_code = get_multiplex_ports(ports,num,opt);
                    break;
                }
                else
                {
                    ret_code = (0 <= get_ports_cb_(ctx_,num, ports, opt) ) ? true :false;
                    break;
                } 
            } while (0);

           md_log(md_log_info, "ports_mgr_impl:get_ports port(%d,%d),demux(%d)", ports[0], ports[1], demux);
            return ret_code;
        }

        void free_ports(uint32_t num, uint16_t *ports)
        {
            for (uint32_t i=0;i<num;++i)
            {
                if (NULL != free_ports_cb_ && !is_demux_ports(ports[i]))
                {
                   md_log(md_log_info, "ports_mgr_impl:free_ports port(%d)", ports[i]);
                    free_ports_cb_(ctx_, 1, &ports[i]);
                }
            }
        }

    private:
        xt_ports_mgr_get_callback_t get_ports_cb_;
        xt_ports_mgr_free_callback_t free_ports_cb_;
        //std::vector<uint16_t> multiplex_ports;
        uint16_t demux_port1;
        uint16_t demux_port2;

        void *ctx_;
    };
}

#endif //_PORTS_MGR_IMPL_H_INCLUDED
