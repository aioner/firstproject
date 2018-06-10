#include "xt_config_api.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/noncopyable.hpp>


namespace
{
    class i_ptree_t
    {
    public:
        virtual ~i_ptree_t() {}

        virtual xt_config_status_t get(const char *path, char *value, unsigned int *len) = 0;
        virtual xt_config_status_t put(const char *path, const char *value) = 0;
        virtual xt_config_status_t add(const char *path, const char *value) = 0;

        virtual xt_config_status_t read_xml(const char *filename, int flags) = 0;
        virtual xt_config_status_t write_xml(const char *filename, char indent_char, unsigned int indent_count, const char *encoding) = 0;

        virtual xt_config_status_t read_ini(const char *filename) = 0;
        virtual xt_config_status_t write_ini(const char *filename) = 0;

        virtual xt_config_status_t read_json(const char *filename) = 0;
        virtual xt_config_status_t write_json(const char *filename, int pretty) = 0;
    };

    template<typename ImplT>
    class ptree_t : public i_ptree_t, private boost::noncopyable
    {
    public:
        typedef ImplT impl_t;

        explicit ptree_t(impl_t *pimpl, bool need_delete = false)
            :pimpl_(pimpl),
            need_delete_(need_delete)
        {}

        ptree_t()
            :pimpl_(new (std::nothrow) impl_t),
            need_delete_(true)
        {}

        ~ptree_t()
        {
            if (need_delete_ && (NULL != pimpl_))
            {
                delete pimpl_;
                pimpl_ = NULL;
            }
        }

        impl_t *detach()
        {
            impl_t *impl = pimpl_;
            pimpl_ = NULL;
            return impl;
        }

        xt_config_status_t get(const char *path, char *value, unsigned int *len)
        {
            BOOST_AUTO(child, pimpl_->get_child_optional(path));
            if (!child)
            {
                return -1;
            }

            typename impl_t::data_type& v = child->data();
            if (v.empty() || (v.length() > *len))
            {
                return -1;
            }

            (void)memcpy(value, v.c_str(), v.length());
            *len = v.length();

            return 0;
        }

        xt_config_status_t put(const char *path, const char *value)
        {
            pimpl_->put(path, value);
            return 0;
        }

        xt_config_status_t add(const char *path, const char *value)
        {
            pimpl_->put(path, value);
            return 0;
        }

        xt_config_status_t read_xml(const char *filename, int flags)
        {
            try
            {
                boost::property_tree::read_xml(filename, *pimpl_, flags);
            }
            catch (...)
            {
                return -1;
            }

            return 0;
        }

        xt_config_status_t write_xml(const char *filename, char indent_char, unsigned int indent_count, const char *encoding)
        {
            try
            {
                boost::property_tree::write_xml(filename, *pimpl_);
            }
            catch (...)
            {
                return -1;
            }

            return 0;
        }

        xt_config_status_t read_ini(const char *filename)
        {
            try
            {
                boost::property_tree::read_ini(filename, *pimpl_);
            }
            catch (...)
            {
                return -1;
            }

            return 0;
        }

        xt_config_status_t write_ini(const char *filename)
        {
            try
            {
                boost::property_tree::write_ini(filename, *pimpl_);
            }
            catch (...)
            {
                return -1;
            }

            return 0;
        }

        xt_config_status_t read_json(const char *filename)
        {
            try
            {
                boost::property_tree::read_json(filename, *pimpl_);
            }
            catch (...)
            {
                return -1;
            }

            return 0;
        }

        xt_config_status_t write_json(const char *filename, int pretty)
        {
            try
            {
                boost::property_tree::write_json(filename, *pimpl_, std::locale(), 0 != pretty);
            }
            catch (...)
            {
                return -1;
            }

            return 0;
        }

    private:
        impl_t *pimpl_;
        bool need_delete_;
    };

    xt_config_ptree_t _config_modules[XT_CONFIG_MODULE_MAX] = { NULL };
}

xt_config_ptree_t xt_config_create_ptree()
{
    return static_cast<void *>(new (std::nothrow) ptree_t<boost::property_tree::ptree>);
}

xt_config_ptree_t xt_config_create_iptree()
{
    return static_cast<void *>(new (std::nothrow) ptree_t<boost::property_tree::iptree>);
}

void xt_config_destroy_ptree(xt_config_ptree_t ptree)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    delete impl;
}

xt_config_status_t xt_config_ptree_get(xt_config_ptree_t ptree, const char *path, char *value, unsigned int *len)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->get(path, value, len);
}

xt_config_status_t xt_config_ptree_put(xt_config_ptree_t ptree, const char *path, const char *value)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->put(path, value);
}

xt_config_status_t xt_config_ptree_add(xt_config_ptree_t ptree, const char *path, const char *value)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->add(path, value);
}

xt_config_status_t xt_config_read_xml(xt_config_ptree_t ptree, const char *filename, int flags)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->read_xml(filename, flags);
}

xt_config_status_t xt_config_write_xml(xt_config_ptree_t ptree, const char *filename, char indent_char, unsigned int indent_count, const char *encoding)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->write_xml(filename, indent_char, indent_count, encoding);
}

xt_config_status_t xt_config_read_ini( xt_config_ptree_t ptree, const char *filename)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->read_ini(filename);
}

xt_config_status_t xt_config_write_ini(xt_config_ptree_t ptree, const char *filename)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->write_ini(filename);
}

xt_config_status_t xt_config_read_json( xt_config_ptree_t ptree, const char *filename)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->read_json(filename);
}

xt_config_status_t xt_config_write_json(xt_config_ptree_t ptree, const char *filename, int pretty)
{
    i_ptree_t *impl = static_cast<i_ptree_t *>(ptree);
    if (NULL == impl)
    {
        return -1;
    }
    return impl->write_json(filename, pretty);
}

xt_config_status_t xt_config_module_init(int module, xt_config_ptree_t ptree)
{
    if (module >= XT_CONFIG_MODULE_MAX)
    {
        return -1;
    }

    if (NULL != _config_modules[module])
    {
        return -1;
    }

    _config_modules[module] = ptree;

    return 0;
}

xt_config_ptree_t xt_config_module_get_ptree(int module)
{
    if (module >= XT_CONFIG_MODULE_MAX)
    {
        return XT_CONFIG_INVALID_PTREE;
    }

    return _config_modules[module];
}

void xt_config_module_term_all()
{
    for (unsigned int n = 0; n < XT_CONFIG_MODULE_MAX; ++n)
    {
        xt_config_destroy_ptree(_config_modules[n]);
        _config_modules[n] = XT_CONFIG_INVALID_PTREE;
    }
}
