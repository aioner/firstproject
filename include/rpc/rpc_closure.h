#ifndef _RPC_CLOSURE_H_INCLUDED
#define _RPC_CLOSURE_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

namespace rpc
{
    class closure_t
    {
    public:
        virtual void done() = 0;
        virtual closure_t *clone() { return NULL; }
    protected:
        virtual ~closure_t() {}
    };

    class recv_msg_closure_t
    {
    public:
        virtual void done(uint8_t *data, uint32_t data_bytes) = 0;
    protected:
        virtual ~recv_msg_closure_t() {}
    };
}

#endif //_RPC_CLOSURE_H_INCLUDED
