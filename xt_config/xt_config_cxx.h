#ifndef _XT_CONFIG_CXX_H_INCLUDED
#define _XT_CONFIG_CXX_H_INCLUDED

#ifndef __cplusplus
    #error "this tool file need cxx complier."
#endif

#include "xt_config_api.h"

#include <sstream>
#include <stdexcept>

namespace xt_config
{
    inline xt_config_ptree_t xinit(bool nocase = false)
    {
        xt_config_ptree_t ptree = XT_CONFIG_INVALID_PTREE;

        if (nocase)
        {
            ptree = xt_config_create_iptree();
        }
        else
        {
            ptree = xt_config_create_ptree();
        }

        return ptree;
    }

    template<typename T, typename U>
    T xget(xt_config_ptree_t ptree, const char *path, const U& default_value)
    {
        char buf[512];
        unsigned int len = sizeof(buf);

        if (XT_CONFIG_STATUS_OK != xt_config_ptree_get(ptree, path, buf, &len))
        {
            return default_value;
        }

        class raw_streambuf : public std::streambuf
        {
        public:
            raw_streambuf(char *buf_start, char *buf_end)
            {
                setg(buf_start, buf_start, buf_end);
            }
        } sbf(buf, buf + len);

        std::istream istrm(&sbf);

        T v;
        istrm >> v;

        if (!istrm)
        {
            return default_value;
        }

        return v;
    }

    template<typename T>
    void xput(xt_config_ptree_t ptree, const char *path, const T& value)
    {
        std::ostringstream oss;
        oss << value;

        xt_config_ptree_put(ptree, path, oss.str().c_str());
    }

    class ptree_t
    {
    public:
        explicit ptree_t(bool nocase = false)
        {
            if (!init(nocase))
            {
                throw std::runtime_error("xt_config.ptree_t::init fail!");
            }
        }

        ~ptree_t()
        {
            term();
        }

        bool init(bool nocase = false)
        {
            impl_ = xinit(nocase);
            return (XT_CONFIG_INVALID_PTREE != impl_);
        }

        void term()
        {
            if (XT_CONFIG_INVALID_PTREE != impl_)
            {
                xt_config_destroy_ptree(impl_);
                impl_ = XT_CONFIG_INVALID_PTREE;
            }
        }

        xt_config_ptree_t detach()
        {
            xt_config_ptree_t ptree = impl_;
            impl_ = XT_CONFIG_INVALID_PTREE;
            return ptree;
        }

        operator bool () const
        {
            return (XT_CONFIG_INVALID_PTREE != impl_);
        }

        template<typename T, typename U>
        T get(const char *path, const U& default_value)
        {
            return xget<T>(impl_, path, default_value);
        }

        template<typename T>
        void put(const char *path, const T& value)
        {
            xput(path, value);
        }

        bool read_xml(const char *filename, int flags = 0)
        {
            return (XT_CONFIG_STATUS_OK == xt_config_read_xml(impl_, filename, flags));
        }

        bool write_xml(const char *filename, char indent_char = ' ', unsigned int indent_count = 0, const char *encoding = "utf-8")
        {
            return (XT_CONFIG_STATUS_OK == xt_config_write_xml(impl_, filename, indent_char, indent_count, encoding));
        }

        bool read_ini(const char *filename)
        {
            return (XT_CONFIG_STATUS_OK == xt_config_read_ini(impl_, filename));
        }

        bool write_ini(const char *filename)
        {
            return (XT_CONFIG_STATUS_OK == xt_config_write_ini(impl_, filename));
        }

        bool read_json(const char *filename)
        {
            return (XT_CONFIG_STATUS_OK == xt_config_read_json(impl_, filename));
        }

        bool write_json(const char *filename, bool pretty = false)
        {
            return (XT_CONFIG_STATUS_OK == xt_config_write_json(impl_, filename, (int)pretty));
        }

        xt_config_ptree_t natite_handle()
        {
            return impl_;
        }

    private:
        ptree_t(const ptree_t&);
        const ptree_t& operator =(const ptree_t&) const;

        xt_config_ptree_t impl_;

        enum xml_parser_flag
        {
            no_concat_text = XT_CONFIG_XML_NO_CONCAT_TEXT,
            no_comments = XT_CONFIG_XML_NO_COMMENTS,
            trim_whitespace = XT_CONFIG_XML_TRIM_WHITESPACE
        };
    };

    template<unsigned int M>
    struct modules
    {
        static bool init(bool nocase = false)
        {
            try
            {
                ptree_t ptree(nocase);
                xt_config_module_init(M, ptree.detach());
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        static bool init_from_xml(const char *xml, bool nocase = false)
        {
            try
            {
                ptree_t ptree(nocase);
                if (!ptree.read_xml(xml))
                {
                    return false;
                }
                xt_config_module_init(M, ptree.detach());
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        static bool init_from_ini(const char *ini, bool nocase = false)
        {
            try
            {
                ptree_t ptree(nocase);
                if (!ptree.read_ini(ini))
                {
                    return false;
                }
                xt_config_module_init(M, ptree.detach());
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        static bool init_from_json(const char *json, bool nocase = false)
        {
            try
            {
                ptree_t ptree(nocase);
                if (!ptree.read_json(json))
                {
                    return false;
                }
                xt_config_module_init(M, ptree.detach());
            }
            catch (...)
            {
                return false;
            }

            return true;
        }

        template<typename T, typename U>
        static T get(const char *path, const U& default_value)
        {
            return xget<T>(xt_config_module_get_ptree(M), path, default_value);
        }

        template<typename T>
        static void put(const char *path, const T& value)
        {
            xput(xt_config_module_get_ptree(M), path, value);
        }

        static void init_and_put(const char *path, const char *value)
        {
            xt_config_ptree_t ptree = xt_config_module_get_ptree(M);
            if (XT_CONFIG_INVALID_PTREE == ptree)
            {
                init();
            }

            ptree = xt_config_module_get_ptree(M);
            if (XT_CONFIG_INVALID_PTREE != ptree)
            {
                xt_config_ptree_put(ptree, path, value);
            }
        }
    };

    struct router_module : modules<XT_CONFIG_MODULE_ROUTER> {};
}


#endif //_XT_CONFIG_CXX_H_INCLUDED
