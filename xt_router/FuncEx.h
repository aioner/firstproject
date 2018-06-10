#ifndef FUNCEX_H_
#define FUNCEX_H_
#include <boost/noncopyable.hpp>
#include "framework/task.h"
#include "XTEngine.h"

//断线监测
#define USE_BreakMonitor_FUNC_
class BreakMonitorTask :public framework::task_base
{
public:
    uint32_t run();

private:
    void SaveCtrlInfo(const src_info& src,const long lChId,long lRet);
};

//专线保障
class ZLineTask : public framework::task_base
{
public:
    uint32_t run();

private:
    void ClearBreakLinePlayInfo();
};

//状态监控
class StateMonitorTask : public framework::task_base
{
public:
    uint32_t run();

private:
    static int MsgID;
    static int GetMsgID();
    void OnStateMonitor();
    void BreakDownToMany(const std::string &origMsg, std::vector<std::string> &vMsgList, int limit = 880);
    bool CheckSameMsgWithLastOne(const std::string &newMsg,const std::string &oldMsg);
    void FilterSpaceCharFromMsg(const std::string &origMsg , std::string &newMsg);
};
//扩展功能管理类
class CFuncEx : boost::noncopyable
{

    //单例相关
    ////////////////////////////////////////////////
protected:
    CFuncEx(void);
    ~CFuncEx(void);

public:
    static CFuncEx* instance()
    {
        return &m_Obj;
    }

private:
    static CFuncEx m_Obj;
    ////////////////////////////////////////////////
public:
    int InitFuncEx();
    int UnitFuncEx();
private:
    ZLineTask          *m_pZlineTask;      //专线保障任务
    BreakMonitorTask   *m_pBreakMonitor;   //断线监测任务
    StateMonitorTask   *m_pStateMonitor;   //状态监控任务
};

#endif// end FUNCEX_H_

