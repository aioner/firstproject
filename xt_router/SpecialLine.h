#ifndef SPECIALLINE_H_
#define SPECIALLINE_H_

#include <list>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include <boost/atomic.hpp>

#include "xtXml.h"

#define SpLineLog "专线保障"

#define USE_SplecialLine_func

typedef struct _struct_SpecialLine //留做后用
{
	std::string m_strSIDS; //  专线中的源中心IDS
	std::string	m_strSName; //专线中的源中心名称
	std::string m_strSIP;  //发送端原始IP
	std::string	 m_strSZIP;  // 发送端专线IP
	std::string m_strDIDS;   //专线中的目的中心IDS;
	std::string m_strDName; //专线中的目的中心名称
	std::string m_strDIP; //接收端原始IP
	std::string m_strDZIP;   //专线中的目的IP
	std::string m_strTime;//最后一次状态变化时间 (年-月-日，时:分:秒) 视频交换机上的时间（以便查询）
	int m_iStat;//链接状态 (0:断线，1：在线)

	_struct_SpecialLine()
	{
		m_iStat = -1;
	}

} SPLINEINFO, *PSPLINEINFO;

class SpecialLine
{

//单例相关
protected:
	SpecialLine(void);
	~SpecialLine(void);

//单例接口
public:
	static SpecialLine* instance()
	{
		return &m_ins;
	}

//接口
public:
	void ChangeRouterSet(const char *pstrRouterSet, const long nSize);


	//专线检测
	void Check(std::string& strRetSPLine);

	//获取本地专线配置
	void GetLocalLineCfg(std::string & strLocalCfg);

	//获取断线专线
	void GetBreakSPLine(std::list<SPLINEINFO>& lstBreakLine);

protected:

	//检测专线状态
	int CheckState(const char *lpdest);

	//获取检测时间
	std::string GetCheckTime();

	//获取本地IP列表
	void GetLocalIpList(std::list<std::string>& lstIP);

	//解析XML到专线数据
	void Analyze();

//可配参数
private:

	int m_iPingPacklen;//Ping包长度
	int m_iPingtimeout;//Ping超时

//处理Data相关
private:

	xtXml m_xmlSPLineInfo;//输入专线信息	

	std::list<SPLINEINFO> m_lstLineInfo;

	boost::atomic<bool> m_bUpate; //中心是否发送更新的专线信息

	std::list<SPLINEINFO> m_lstBreakLine;//已经断线的专线

//线程安全相关
private:
	
	//线程安全
	boost::detail::spinlock  m_xmlSPLineInfoMutex;     //专线信息锁

    static SpecialLine m_ins;
};

#endif //SPECIALLINE_H_