//
// @created by lichao, 20140903
// @tinyxml��ļ�ª��װ��xml��̬��
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

    // ��һ������
    xtXmlAttribute Next();
    // ǰһ������
    xtXmlAttribute Pre();

    // ������������
    const char *Name();

    // ������������
    void SetName(const char* name);

    // ��������ֵconst char *����
    const char *Value();

    // ��������ֵint����
    int IntValue();

    // ��������ֵdouble����
    double DoubleValue();

    // ��������ֵ
    void SetValue(const char* value); 

    //�����Ƿ�Ϊ��
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

    // ����µĽڵ�
    xtXmlNodePtr   NewChild(const char* name);
    // ��ȡ��һ�����ӽڵ㣬Ĭ�Ϸ��ص�һ�����ӽڵ�
    xtXmlNodePtr   GetFirstChild(const char* name=NULL);
    // ��ȡ��һ���ֵܽڵ㣬Ĭ�Ϸ����±ߵ�һ���ֵܽڵ�
    xtXmlNodePtr   NextSibling(const char* name=NULL);
    // ��ȡ��һ���ֵܽڵ㣬Ĭ�Ϸ����ϱߵ�һ���ֵܽڵ�
    xtXmlNodePtr   PreSibling(const char* name=NULL);
    // ��������
    bool            Destory();
    // �������к��ӽڵ�
    void            DestoryAllChildren();

    // ��������
    void SetAttribute(const char* name, const char* value);

    void SetAttribute( const char * name, int value );


    void SetAttribute(const char *name, const std::string&value)
    {
        SetAttribute(name, value.c_str());
    }

    void SetDoubleAttribute( const char * name, double value );

    //ɾ������
    void DelAttribute( const char * name );

    // ��ȡ����ֵ
    const char* GetAttribute(const char* name);
    xtXmlAttribute GetFirstAttribute();
    xtXmlAttribute LastAttribute();

    int QueryIntAttribute( const char* name, int* ival ) const;

    // ���ýڵ�����
    void SetName(const char*name);
    //  ��ȡ�ڵ�����
    const char *GetName();

    // ���ýڵ�ֵ
    void SetValue(const char* value);
    // ��ȡ�ڵ�ֵ
    const char *GetValue();

    // �жϸýڵ��Ƿ�Ϊ��
    bool IsNull();

    // ���ظ��ڵ�
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
    // ����xml�ļ� Ĭ������Ϊ<?xml version="1.0" encoding="UTF-8" standalone="no"?>
    bool  create(const char* path, const char* version="1.0", const char*  encoding="UTF-8", const char*  standalone="no");

    // ���ļ�
    bool  open(const char* path);

    // �����ļ�
    bool  save(const char* path=NULL);

    //����XML�ַ���
    void LoadXMLStr(const char* pXMLStr);

    //��ȡXML�ַ��� 
    bool GetXMLStr(char* pszXMLStrBuffer, const size_t uiBufferSize);

    //��ȡXML�ַ��� 
    const char* GetXMLStrEx();

    // ��ȡ���ڵ�
    xtXmlNodePtr  getRoot();

    //���Ӹ��ڵ�
    xtXmlNodePtr AddRoot(const char* pszRoot);

    // �жϸ��ļ��Ƿ����
    bool  isExist(const char* path);

    // ��ýڵ�
    xtXmlNodePtr getNode(const char *name);
    xtXmlNodePtr getNode(xtXmlNodePtr node, const char *name);

    // ��ýڵ�ֵ
    const char *getValue(xtXmlNodePtr node);

    // �������ֵ
    const char *getValue(xtXmlNodePtr node, const char *attr);

    void TestPrintStdOut();

private:
    class XmlImplType;
    XmlImplType *m_pXml;
};

#endif 

#endif //_XT_XML_H_INCLUDED
