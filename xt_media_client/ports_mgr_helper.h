#ifndef _PORTS_MGR_HELPER_H_INCLUDED
#define _PORTS_MGR_HELPER_H_INCLUDED

#include <vector>
extern void md_log(const media_client_log_level_t log_level,const char *fmt, ...);
namespace xt_media_client
{
    template<typename IPortsMgr>
    class ports_mgr_helper
    {
    public:
        explicit ports_mgr_helper(IPortsMgr *ports_mgr = NULL)
            :p_ports_mgr_(ports_mgr)
        {}

        ~ports_mgr_helper()
        {
            free_used_ports();
            p_ports_mgr_ = NULL;
        }

        void reset_ports_mgr(IPortsMgr *ports_mgr)
        {
            p_ports_mgr_ = ports_mgr;
        }

        IPortsMgr *get_ports_mgr()
        {
            return p_ports_mgr_;
        }

        bool get_ports(uint32_t num, uint16_t *ports,bool demux, int32_t opt = 0)
        {
            return (NULL == p_ports_mgr_) ? false : p_ports_mgr_->get_ports(num, ports,demux, opt);
        }

        bool update_demux_port()
        {
            return (NULL == p_ports_mgr_) ? false : p_ports_mgr_->update_demux_port();
        }

        void ports_used(uint32_t num, uint16_t *ports)
        {
            while (num > 0)
            {
                port_used(ports[num - 1]);
                num--;
            }
        }

        void port_used(uint16_t port)
        {
            used_ports_.push_back(port);
        }

        void free_ports(uint32_t num, uint16_t *ports)
        {
            if (NULL != p_ports_mgr_)
            {
               md_log(md_log_info, "ports_mgr_helper:free_ports num[%d]",num);
                p_ports_mgr_->free_ports(num, ports);
            }
        }

        void free_used_ports()
        {
            if (used_ports_.size() > 0)
            {
                free_ports((uint32_t)used_ports_.size(), &(used_ports_.front()));
                md_log(md_log_info, "ports_mgr_helper:free_used_ports");
                used_ports_.clear();
            }
        }

    protected:
        std::vector<uint16_t> used_ports_;
        IPortsMgr *p_ports_mgr_;
    };


    template<typename IPortsMgr>
    class scoped_ports_mgr_helper : public ports_mgr_helper<IPortsMgr>
    {
    public:
        explicit scoped_ports_mgr_helper(IPortsMgr *ports_mgr)
            :ports_mgr_helper<IPortsMgr>(ports_mgr)
        {}

        explicit scoped_ports_mgr_helper(ports_mgr_helper<IPortsMgr> &that)
            :ports_mgr_helper<IPortsMgr>(that.get_ports_mgr())
        {}

        void ports_failed(uint32_t num, uint16_t *ports)
        {
            this->ports_used(num, ports);
        }

        void port_failed(uint16_t ports)
        {
            this->port_used(ports);
        }
    private:
        using ports_mgr_helper<IPortsMgr>::port_used;
        using ports_mgr_helper<IPortsMgr>::free_used_ports;
    };
}


#endif //_PORTS_MGR_HELPER_H_INCLUDED
