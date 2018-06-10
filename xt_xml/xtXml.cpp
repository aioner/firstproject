#include "xtXml.h"
#include "tinyxml/tinyxml.h"

#ifdef _WIN32
	#include <io.h>
#else
	#include <unistd.h>
#endif

class xtXml::XmlImplType
{
public:
    //XML文档
    TiXmlDocument m_doc;

    //XML打印
    TiXmlPrinter m_printer;
};

xtXml::xtXml(void)
:m_pXml(new XmlImplType)
{}

xtXml::~xtXml(void)
{
    if (NULL != m_pXml)
    {
        delete m_pXml;
        m_pXml = NULL;
    }
}

xtXml& xtXml::operator=( const xtXml& base )
{
    m_pXml->m_doc = base.m_pXml->m_doc;
    m_pXml->m_printer = base.m_pXml->m_printer;

    return *this;
}

bool xtXml::create(const char*  path,  const char* _version /*="1.0" */, const char* _encoding /*="UTF-8" */, const char* _standalone/*="no" */)
{
    if (! m_pXml->m_doc.LoadFile(path, TIXML_ENCODING_UTF8))
    {
        return false;
    }

    TiXmlDeclaration delar(_version, _encoding, _standalone);
    m_pXml->m_doc.InsertEndChild(delar);

    TiXmlElement root("root");
    m_pXml->m_doc.InsertEndChild(root);

    return true;
}

bool xtXml::open( const char* path )
{
    return m_pXml->m_doc.LoadFile(path, TIXML_ENCODING_UTF8);
}

bool xtXml::save(const char* path)
{
    bool ret;

    if(NULL == path) ret = m_pXml->m_doc.SaveFile();
    else ret = m_pXml->m_doc.SaveFile(path);

    return ret;
}

void xtXml::LoadXMLStr(const char* pXMLStr)
{
    m_pXml->m_doc.Parse(pXMLStr);
}

xtXmlNodePtr xtXml::AddRoot(const char* pszRoot)
{
    m_pXml->m_doc.Parse(pszRoot);

    return m_pXml->m_doc.RootElement();
}

const char* xtXml::GetXMLStrEx()
{
    m_pXml->m_doc.Accept(&m_pXml->m_printer);
    return m_pXml->m_printer.CStr();
}

bool xtXml::GetXMLStr(char* pszXMLStrBuffer, const size_t uiBufferSize)
{
    TiXmlPrinter printer;
    m_pXml->m_doc.Accept(&printer);
    if (printer.Size() > uiBufferSize)
    {
        return false;
    }
#ifdef _WIN32
    if (0 != ::strcpy_s(pszXMLStrBuffer,uiBufferSize, printer.CStr()))
#else
	if (0 != ::strcpy(pszXMLStrBuffer, printer.CStr()))
#endif
    {
        return false;
    }

    //成功返回true
    return true;
}

xtXmlNodePtr  xtXml::getRoot()
{
    return m_pXml->m_doc.RootElement();
}

bool xtXml::isExist( const char* path )
{
#ifdef _WIN32
    if(-1 != _access(path, 0))
#else
	if(-1 != access(path, 0))
#endif
    {
        return true;
    }
    else
    {
        return false;
    }
}

xtXmlNodePtr xtXml::getNode(const char *name)
{
    xtXmlNodePtr node;
    xtXmlNodePtr tmp = getRoot();

    node = getNode(tmp, name);

    return node;
}

xtXmlNodePtr xtXml::getNode(xtXmlNodePtr node, const char *name)
{
    xtXmlNodePtr node1;

    if (node.IsNull() || !name)
    {
        return node1;
    }

    node1 = node.GetFirstChild(name);

    return node1;
}

const char *xtXml::getValue(xtXmlNodePtr node)
{
    return node.IsNull() ? "" : node.GetValue();
}

const char *xtXml::getValue(xtXmlNodePtr node, const char *attr)
{
    return node.IsNull() ? "" : node.GetAttribute(attr);
}

void xtXml::TestPrintStdOut()
{
    m_pXml->m_doc.Print();
}

class xtXmlNodePtr::XmlElementImplType : public TiXmlElement
{};

xtXmlNodePtr::xtXmlNodePtr(void *pElement)
:m_pElement((XmlElementImplType *)(TiXmlElement *)pElement)
{}

xtXmlNodePtr::~xtXmlNodePtr()
{
    m_pElement = NULL;
}

xtXmlNodePtr& xtXmlNodePtr::operator=(const xtXmlNodePtr& base)
{
    m_pElement = base.m_pElement;
    return *this;
}

bool xtXmlNodePtr::IsNull()
{
    return NULL==m_pElement;
}

void xtXmlNodePtr::SetAttribute( const char* name, const char* value )
{
    m_pElement->SetAttribute(name, value);
}

void xtXmlNodePtr::SetAttribute( const char * name, int value )
{
    m_pElement->SetAttribute(name, value);
}

void xtXmlNodePtr::SetDoubleAttribute( const char * name, double value )
{
    m_pElement->SetDoubleAttribute(name, value);
}

void xtXmlNodePtr::DelAttribute( const char * name )
{
    m_pElement->RemoveAttribute(name);
}

int xtXmlNodePtr::QueryIntAttribute( const char* name, int* ival ) const
{
    return m_pElement->QueryIntAttribute(name,ival);
}

const char * xtXmlNodePtr::GetAttribute( const char* name )
{
	const char *pvalue = m_pElement->Attribute(name);
	if (pvalue)
	{
		return pvalue;
	}
	return "";
}

void xtXmlNodePtr::SetName( const char*name )
{
    m_pElement->SetValue(name);
}

const char *xtXmlNodePtr::GetName()
{
	const char *pvalue = m_pElement->Value();
	if (pvalue)
	{
		return pvalue;
	}
    return "";
}

void xtXmlNodePtr::SetValue( const char* value )
{
    TiXmlText txt(value);
    m_pElement->InsertEndChild(txt);
}

const char *xtXmlNodePtr::GetValue()
{
	const char* pvalue = m_pElement->GetText();
	if (pvalue)
	{
		return pvalue;
	}
    return "";
}

xtXmlNodePtr xtXmlNodePtr::GetFirstChild( const char* name/*=NULL*/ )
{
    if(NULL == name)
    {
        return m_pElement->FirstChildElement();
    }
    else
    {
        return m_pElement->FirstChildElement(name);
    }
}

xtXmlNodePtr xtXmlNodePtr::NextSibling( const char* name/*=NULL*/ )
{
    if(NULL == name)
    {
        return m_pElement->NextSiblingElement();
    }
    else
    {
        return m_pElement->NextSiblingElement(name);
    }
}

xtXmlNodePtr xtXmlNodePtr::PreSibling( const char* name/*=NULL*/ )
{
    if(NULL == name)
    {
        return m_pElement->PreviousSibling();
    }
    else
    {
        return m_pElement->PreviousSibling(name);
    }
}

bool xtXmlNodePtr::Destory()
{
    if(*this == Root())
    {
        return false;
    }
    return	m_pElement->Parent()->RemoveChild((TiXmlNode*)m_pElement);
}

xtXmlNodePtr xtXmlNodePtr::Root()
{
    TiXmlElement *pElement = m_pElement;
    TiXmlElement *pRoot = NULL;

    int nType = pElement->Type();
    while (0 != nType)
    {
        pRoot    = pElement;
        pElement = (TiXmlElement*)pElement->Parent();
        nType    = pElement->Type();
    }

    return pRoot;
}

bool xtXmlNodePtr::operator==(const xtXmlNodePtr& node )
{
    return this->m_pElement == node.m_pElement;
}

void xtXmlNodePtr::DestoryAllChildren()
{
    m_pElement->Clear();
}

xtXmlNodePtr xtXmlNodePtr::NewChild(const char* name)
{
    return (TiXmlElement*)m_pElement->InsertEndChild(TiXmlElement(name));
}

class xtXmlAttribute::XmlAttributeImplType : public TiXmlAttribute
{};

xtXmlAttribute::xtXmlAttribute(void *pAttribute)
:m_pAttribute((XmlAttributeImplType *)(TiXmlAttribute *)pAttribute)
{}

xtXmlAttribute::~xtXmlAttribute()
{
    m_pAttribute = NULL;
}

xtXmlAttribute xtXmlNodePtr::GetFirstAttribute()
{
    return m_pElement->FirstAttribute();
}

xtXmlAttribute xtXmlNodePtr::LastAttribute()
{
    return m_pElement->LastAttribute();
}

xtXmlAttribute xtXmlAttribute::Next()
{
    return	m_pAttribute->Next();
}

bool xtXmlAttribute::IsNull()
{
    return (NULL == m_pAttribute);
}

xtXmlAttribute xtXmlAttribute::Pre()
{
    return m_pAttribute->Previous();
}

const char *xtXmlAttribute::Name()
{
    return m_pAttribute->Name();
}

const char *xtXmlAttribute::Value()
{
    return m_pAttribute->Value();
}

void xtXmlAttribute::SetName(const char* name)
{
    m_pAttribute->SetName(name);
}

void xtXmlAttribute::SetValue(const char* value)
{
    m_pAttribute->SetValue(value);
}

double xtXmlAttribute::DoubleValue()
{
    return m_pAttribute->DoubleValue();
}

int xtXmlAttribute::IntValue()
{
    return m_pAttribute->IntValue();
}