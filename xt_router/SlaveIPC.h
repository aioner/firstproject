#ifndef _SLAVEIPC_H_INCLUDED
#define _SLAVEIPC_H_INCLUDED
#ifdef USE_SNMP_
#ifdef _WIN32
#pragma comment(lib,"StatusSlaveInterface.lib")
#pragma comment(lib,"DevicePerformance.lib")
#endif //#ifdef _WIN32
#include "StatusSlaveInterface.h"
#include "StatusCommonDefine.h"
#include "DevicePerformance.h"
#include "XTEngine.h"
#include "Router_config.h"
#include "InfoTypeDef.h"
#include "InfoMgr.h"
#include <boost/noncopyable.hpp>
#include <iostream>

#ifndef _WIN32
#define _stdcall
#endif

class SlaveIPC : boost::noncopyable
{
public:
    SlaveIPC(void);
    ~SlaveIPC(void);

public:
    static SlaveIPC* instance()
    {
        return &m_Obj;
    }
    static int _stdcall RequestGetTbl(report_category category, void *data, int *len, void *user_context);
    static int _stdcall LostHeartBeat(void *user_context);

private:
    static             SlaveIPC  m_Obj;
    request_tbl        m_fcbRequestGetTblCB;
    request_tbl        m_fcbRequestSetTblCB;
    lost_heart_beat    m_fcbLostHeartBeatCB;
    SDeviceStatus      *m_pDeviceStatus;
    SDevicePerformance *m_pDevicePerformance;
    SLinkTraffic       *m_pLinkTraffic;
    SLinkLossAndDelay  *m_pLinkLossAndDelay;
    vector<SLinkTraffic>      listLinkTraffic;
    vector<SLinkLossAndDelay> listLinkLossAndDelay;

public:
    int EstablishSlaveIPC(int device_type);
    SDeviceStatus* GetDeviceStatus();
    SDevicePerformance* GetDevicePerformance();
    vector<SLinkTraffic>& GetLinkTraffic();
    vector<SLinkLossAndDelay>& GetLinkLossAndDelay();
    int SendNotification(trap_type type);
    int Exit();
};
#endif //#ifdef USE_SNMP_
#endif //_SLAVEIPC_H_INCLUDED