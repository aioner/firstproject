#ifndef _SINGLETON_H_INCLUDED
#define _SINGLETON_H_INCLUDED

#include <stdlib.h>
namespace xt_utility
{
template<typename DrivedT>
class singleton
{
public:
    typedef DrivedT drived_type;

    static drived_type *instance()
    {
        return s_self;
    }

    static void new_instance()
    {
        s_self = new drived_type;
    }

    static void delete_instance()
    {
        if (NULL != s_self)
        {
            delete s_self;
            s_self = NULL;
        }
    }

    static bool is_initialized()
    {
        return (NULL != s_self);
    }

private:
    static drived_type *s_self;
};

// !static
template<typename DrivedT>
DrivedT *singleton<DrivedT>::s_self = NULL;

template<typename ImplT>
class singleton_impl : public ImplT, public singleton<singleton_impl<ImplT> > {};
}//namespace xt_utility
#endif //_SINGLETON_H_INCLUDED
