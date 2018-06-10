#ifndef _XT_CONFIG_API_H_INCLUDED
#define _XT_CONFIG_API_H_INCLUDED

#ifdef _WIN32
    #ifdef XT_CONFIG_EXPORTS
        #define XT_CONFIG_API __declspec(dllexport)
    #else
        #define XT_CONFIG_API __declspec(dllimport)
    #endif
#else
    #ifdef XT_CONFIG_EXPORTS
        #define XT_CONFIG_API __attribute__((visibility("default")))
    #else
        #define XT_CONFIG_API
    #endif
#endif


#define XT_CONFIG_STATUS_OK                 0
#define XT_CONFIG_INVALID_PTREE             NULL

#define XT_CONFIG_XML_NO_CONCAT_TEXT        0x1
#define XT_CONFIG_XML_NO_COMMENTS           0x2
#define XT_CONFIG_XML_TRIM_WHITESPACE       0x4


#define XT_CONFIG_MODULE_ROUTER             0x0
#define XT_CONFIG_MODULE_MAX                1

#ifdef __cplusplus
extern "C"
{
#endif
    typedef void *xt_config_ptree_t;
    typedef int xt_config_status_t;

    XT_CONFIG_API xt_config_ptree_t xt_config_create_ptree();
    XT_CONFIG_API xt_config_ptree_t xt_config_create_iptree();
    XT_CONFIG_API void xt_config_destroy_ptree(xt_config_ptree_t ptree);

    XT_CONFIG_API xt_config_status_t xt_config_ptree_get(xt_config_ptree_t ptree, const char *path, char *value, unsigned int *len);
    XT_CONFIG_API xt_config_status_t xt_config_ptree_put(xt_config_ptree_t ptree, const char *path, const char *value);
    XT_CONFIG_API xt_config_status_t xt_config_ptree_add(xt_config_ptree_t ptree, const char *path, const char *value);

    XT_CONFIG_API xt_config_status_t xt_config_read_xml(xt_config_ptree_t ptree, const char *filename, int flags);
    XT_CONFIG_API xt_config_status_t xt_config_write_xml(xt_config_ptree_t ptree, const char *filename, char indent_char, unsigned int indent_count, const char *encoding);

    XT_CONFIG_API xt_config_status_t xt_config_read_ini(xt_config_ptree_t ptree, const char *filename);
    XT_CONFIG_API xt_config_status_t xt_config_write_ini(xt_config_ptree_t ptree, const char *filename);

    XT_CONFIG_API xt_config_status_t xt_config_read_json(xt_config_ptree_t ptree, const char *filename);
    XT_CONFIG_API xt_config_status_t xt_config_write_json(xt_config_ptree_t ptree, const char *filename, int pretty);

    XT_CONFIG_API xt_config_status_t xt_config_module_init(int module, xt_config_ptree_t ptree);
    XT_CONFIG_API void xt_config_module_term_all();
    XT_CONFIG_API xt_config_ptree_t xt_config_module_get_ptree(int module);

#ifdef __cplusplus
};
#endif

#endif //_XT_CONFIG_API_H_INCLUDED
