#ifndef _RPC_TYPES_H_INCLUDED
#define _RPC_TYPES_H_INCLUDED

#include "rpc_utility.h"
#include <stdint.h>

namespace rpc
{
    typedef uint32_t iid_t;
    typedef uint32_t dispid_t;

    enum { invalid_dispid = ~0 };
}

#define BEGIN_RPC_FUNC(id, x, drivedT)  \
    static rpc::iid_t get_register_iid() { return id; }   \
    typedef x this_type;    \
    typedef drivedT drived_type;    \
    typedef void (drived_type::*func_type)(rpc::control_t&, void *, void *, rpc::closure_t *);  \
    typedef void (drived_type::*event_type)(rpc::control_t&, void *);   \
    struct func_info_t  \
    {   \
        rpc::dispid_t dispid;    \
        const char *name;   \
        union   \
        {   \
            func_type func; \
            event_type event;   \
        };   \
        void *(*create_request)(); \
        void (*destroy_request)(void *); \
        bool (*out_request)(void *, rpc::serialization::type, std::ostream&); \
        bool (*in_request)(void *, rpc::serialization::type, std::istream&); \
        void *(*create_response)(); \
        void (*destroy_response)(void *); \
        bool (*out_response)(void *, rpc::serialization::type, std::ostream&); \
        bool (*in_response)(void *, rpc::serialization::type, std::istream&); \
    };  \
    static const func_info_t *get_register_func_info(uint32_t *num = NULL) \
    {   \
        static func_info_t s_func_info[] = \
        {

#define END_RPC_FUNC()  \
            {rpc::invalid_dispid, NULL, NULL} \
        };  \
        if (NULL != num) { *num = sizeof(s_func_info) / sizeof(func_info_t); }  \
        return s_func_info; \
    }   \
    static rpc::dispid_t get_func_dispid(const char *name)   \
    {   \
        const func_info_t *info = get_register_func_info();  \
        while (NULL != info->name)  \
        {   \
            if (0 == strcmp(name, info->name))  \
            {   \
                return info->dispid;    \
            }   \
            info++; \
        }   \
        return rpc::invalid_dispid;  \
    }\
    static const func_info_t *get_func_info(rpc::dispid_t dispid) \
    {   \
        const func_info_t *info = get_register_func_info();  \
        while (NULL != info->name)  \
        {   \
            if (dispid == info->dispid)  \
            {   \
                return info;    \
            }   \
            info++; \
        }   \
        return NULL;  \
    }

#define RPC_FUNC(dispid, func, C1, D1, O1, I1,  C2, D2, O2, I2) \
{ dispid, #func, rpc::utility::bitwise_cast<func_type>(&drived_type::func), C1, D1, O1, I1, C2, D2, O2, I2 },

#define RPC_FUNC2(dispid, func, request, response) \
    RPC_FUNC(dispid, func, \
    rpc::utility::allocate_wrapper_t<request>::allocate, rpc::utility::allocate_wrapper_t<request>::deallocate, \
    rpc::utility::allocate_wrapper_t<request>::out_object, rpc::utility::allocate_wrapper_t<request>::in_object, \
    rpc::utility::allocate_wrapper_t<response>::allocate, rpc::utility::allocate_wrapper_t<response>::deallocate, \
    rpc::utility::allocate_wrapper_t<response>::out_object, rpc::utility::allocate_wrapper_t<response>::in_object)

#define RPC_STUB_FUNC_IMPL(stype, func,  Request, Response) \
    bool func(rpc::control_t& control, Request *request, Response *response, rpc::closure_t *done)    \
    {   \
        return invoke<rpc::serialization::stype>(#func, control, request, response, done);  \
    }


#define RPC_EVENT(dispid, event, request)   \
    RPC_FUNC(dispid, event, \
    rpc::utility::allocate_wrapper_t<request>::allocate, rpc::utility::allocate_wrapper_t<request>::deallocate, \
    rpc::utility::allocate_wrapper_t<request>::out_object, rpc::utility::allocate_wrapper_t<request>::in_object, \
    NULL, NULL, NULL, NULL)

#define RPC_PROXY_EVENT_IMPL(stype, event,  Request)    \
    void event(rpc::control_t& control, Request *request)    \
    {   \
        fire_event<rpc::serialization::stype>(#event, control, request);  \
    }

#endif //_RPC_TYPES_H_INCLUDED
