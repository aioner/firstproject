#ifndef _MTS_FRAMEWORK_DETAIL_SINGLETON_H_INCLUDED
#define _MTS_FRAMEWORK_DETAIL_SINGLETON_H_INCLUDED

#include "../../framework/config.h"
#include <stddef.h>

namespace framework
{
    namespace detail
    {
        template<typename T>
        class singleton
        {
        public:
            static void new_instance()
            {
                s_inst = new T;
            }

            static void delete_instance()
            {
                if (NULL != s_inst)
                {
                    delete s_inst;
                    s_inst = NULL;
                }
            }

            static T *instance()
            {
                return s_inst;
            }
        private:
            static T *s_inst;
        };

        //static
        template<typename T>  T *singleton<T>::s_inst;
    }
}
#endif //_MTS_FRAMEWORK_DETAIL_SINGLETON_H_INCLUDED
