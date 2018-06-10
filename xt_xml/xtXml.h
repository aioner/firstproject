//
// @created by lichao, 20140903
// @tinyxml库的简陋封装的xml静态库
//
#ifndef _XT_XML_H_INCLUDED
#define _XT_XML_H_INCLUDED

#ifdef __cplusplus

#include <string>

class xtXmlAttribute
{
public:
    xtXmlAttribute(void *pAttribute = NULL);
    ~xtXmlAttribute();

    // 下一个属性
    xtXmlAttribute Next();
    // 前一个属性
    xtXmlAttribute Pre();

    // 返回属性名称
    const char *Name();

    // 设置属性名称
    void SetName(const char* name);

    // 返回属性值const char *类型
    const char *Value();

    // 返回属性值int类型
    int IntValue();

    // 返回属性值double类型
    double DoubleValue();

    // 设置属性值
    void SetValue(const char* value); 

    //检验是否为空
    bool IsNull();

private:
    class XmlAttributeImplType;
    XmlAttributeImplType *m_pAttribute;
};

class xtXmlNodePtr
{
public:
    xtXmlNodePtr(void *pElement = NULL);
    ~xtXmlNodePtr();

    xtXmlNodePtr& operator=(const xtXmlNodePtr& base );

    bool operator==(const xtXmlNodePtr &node);

    // 添加新的节点
    xtXmlNodePtr   NewChild(const char* name);
    // 获取第一个孩子节点，默认返回第一个孩子节点
    xtXmlNodePtr   GetFirstChild(const char* name=NULL);
    // 获取下一个兄弟节点，默认返回下边第一个兄弟节点
    xtXmlNodePtr   NextSibling(const char* name=NULL);
    // 获取上一个兄弟节点，默认返回上边第一个兄弟节点
    xtXmlNodePtr   PreSibling(const char* name=NULL);
    // 自我销毁
    bool            Destory();
    // 销毁所有孩子节点
    void            DestoryAllChildren();

    // 设置属性
    void SetAttribute(const char* name, const char* value);

    void SetAttribute( const char * name, int value );


    void SetAttribute(const char *name, const std::string&value)
    {
        SetAttribute(name, value.c_str());
    }

    void SetDoubleAttribute( const char * name, double value );

    //删除属性
    void DelAttribute( const char * name );

    // 读取属性值
    const char* GetAttribute(const char* name);
    xtXmlAttribute GetFirstAttribute();
    xtXmlAttribute LastAttribute();

    int QueryIntAttribute( const char* name, int* ival ) const;

    // 设置节点名称
    void SetName(const char*name);
    //  获取节点名称
    const char *GetName();

    // 设置节点值
    void SetValue(const char* value);
    // 获取节点值
    const char *GetValue();

    // 判断该节点是否为空
    bool IsNull();

    // 返回根节点
    xtXmlNodePtr   Root();

public:
    class XmlElementImplType;
    XmlElementImplType *m_pElement;
};

class xtXml
{
public:
    xtXml(void);
    ~xtXml(void);

    xtXml& operator=( const xtXml& base );

public:
    // 创建xml文件 默认声明为<?xml version="1.0" encoding="UTF-8" standalone="no"?>
    bool  create(const char* path, const char* version="1.0", const char*  encoding="UTF-8", const char*  standalone="no");

    // 打开文件
    bool  open(const char* path);

    // 保存文件
    bool  save(const char* path=NULL);

    //加载XML字符串
    void LoadXMLStr(const char* pXMLStr);

    //获取XML字符串 
    bool GetXMLStr(char* pszXMLStrBuffer, const size_t uiBufferSize);

    //获取XML字符串 
    const char* GetXMLStrEx();

    // 获取根节点
    xtXmlNodePtr  getRoot();

    //增加根节点
    xtXmlNodePtr AddRoot(const char* pszRoot);

    // 判断该文件是否存在
    bool  isExist(const char* path);

    // 获得节点
    xtXmlNodePtr getNode(const char *name);
    xtXmlNodePtr getNode(xtXmlNodePtr node, const char *name);

    // 获得节点值
    const char *getValue(xtXmlNodePtr node);

    // 获得属性值
    const char *getValue(xtXmlNodePtr node, const char *attr);

    void TestPrintStdOut();

private:
    class XmlImplType;
    XmlImplType *m_pXml;
};

#endif 

#endif //_XT_XML_H_INCLUDED
