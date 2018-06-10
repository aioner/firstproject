#ifndef _MTS_UTILITY_NONCOPYABLE_H_INCLUDED
#define _MTS_UTILITY_NONCOPYABLE_H_INCLUDED

namespace utility
{
    class noncopyable
    {
    protected:
        noncopyable() {}
        ~noncopyable() {}
    private:
        noncopyable(const noncopyable &);
        noncopyable& operator=(const noncopyable &);
    };
}

#endif //_MTS_UTILITY_DETAIL_NONCOPYABLE_H_INCLUDED
