#include "webconfig.h"

#ifdef _WIN32
#define CFG_FILE_PATH "D:/NetMCSet/"
#else
#define CFG_FILE_PATH "/etc/xtconfig/d/netmcset/"
#endif //#ifdef _WIN32

webconfig webconfig::m_obj;

webconfig::webconfig(void)
{
#ifdef _WIN32
	m_fPath.assign(CFG_FILE_PATH"dps_cfg.xml");
#else
	m_fPath.assign(CFG_FILE_PATH"dps_cfg.xml");
#endif//#ifdef _WIN32
}

webconfig::~webconfig(void)
{
}

int webconfig::get_dps_cfg(char *buff, int len)
{
	xtXml dps_cfg;
	bool valid = dps_cfg.open(m_fPath.c_str());
	if (!valid)
	{
		return -1;
	}
	const char *xmlstr = dps_cfg.GetXMLStrEx();
	if (!xmlstr)
	{
		return -1;
	}
	::memcpy(buff,xmlstr,strlen(xmlstr)+1);

	return 0;
}

int webconfig::set_dps_cfg(const char *buff, int len)
{
	xtXml xml;
	xml.LoadXMLStr(buff);

	xtXml dps_cfg;
	bool valid = dps_cfg.open(m_fPath.c_str());
	if (!valid)
	{
		return -1;
	}

	//web->cfg
	xtXmlNodePtr root = dps_cfg.getRoot();
	xtXmlNodePtr root2 = xml.getRoot();
	if (root.IsNull()||root2.IsNull())
	{
		return -1;
	}

	//一一对应赋值
	xtXmlNodePtr f_node = root.GetFirstChild();//system
	xtXmlNodePtr f_node2 = root2.GetFirstChild();
	while (!f_node.IsNull() && !f_node2.IsNull() &&
		strcmp(f_node.GetName(),f_node2.GetName()) == 0)
	{
		xtXmlNodePtr s_node = f_node.GetFirstChild();//link_center
		xtXmlNodePtr s_node2 = f_node2.GetFirstChild();
		while (!s_node.IsNull() && !s_node2.IsNull() &&
			strcmp(s_node.GetName(),s_node2.GetName()) == 0)
		{
			//node
			const char *val = s_node2.GetValue();
			if (val!=NULL && strlen(val)!=0)
			{
				s_node.SetValue2(val);
			}
			//dev
			xtXmlAttribute att = s_node2.GetFirstAttribute();
			while (!att.IsNull())
			{
				const char *str = att.Value();
				if (str!=NULL && strlen(str)!=0)
				{
					s_node.SetAttribute(att.Name(), str);
				}
				att = att.Next();
			}
			s_node = s_node.NextSibling();//link_type
			s_node2 = s_node2.NextSibling();
		}
		f_node = f_node.NextSibling();//access
		f_node2 = f_node2.NextSibling();
	}
	//printf(strbuf.c_str());
	dps_cfg.save();

	return 0;
}