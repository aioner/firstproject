#ifndef FUNCEX_H_
#define FUNCEX_H_
#include <boost/noncopyable.hpp>
#include "framework/task.h"
#include "XTEngine.h"

//���߼��
#define USE_BreakMonitor_FUNC_
class BreakMonitorTask :public framework::task_base
{
public:
    uint32_t run();

private:
    void SaveCtrlInfo(const src_info& src,const long lChId,long lRet);
};

//ר�߱���
class ZLineTask : public framework::task_base
{
public:
    uint32_t run();

private:
    void ClearBreakLinePlayInfo();
};

//״̬���
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
//��չ���ܹ�����
class CFuncEx : boost::noncopyable
{

    //�������
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
    ZLineTask          *m_pZlineTask;      //ר�߱�������
    BreakMonitorTask   *m_pBreakMonitor;   //���߼������
    StateMonitorTask   *m_pStateMonitor;   //״̬�������
};

#endif// end FUNCEX_H_

