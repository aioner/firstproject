#ifndef _PORTS_MGR_H_INCLUDED
#define _PORTS_MGR_H_INCLUDED

#include <stdint.h>
#include "novtable.h"

namespace xt_media_client
{
    class MEDIA_CLIENT_NO_VTABLE ports_mgr_t
    {
    public:
        virtual bool get_ports(uint32_t num, uint16_t *ports, bool demux,int32_t opt) = 0;
        virtual void free_ports(uint32_t num, uint16_t *ports) = 0;
        virtual bool update_demux_port()=0;

    protected:
        virtual ~ports_mgr_t() {}
    };
}

#endif //_PORTS_MGR_H_INCLUDED
