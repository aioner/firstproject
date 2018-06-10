
// JkMainOcxClient2Dlg.cpp : 实现文件
//

#include "JkMainClient.h"
#include "config.h"
#include "JKMainClientLog.h"
#include <ostream>
#include<sstream>

JkMainClient::JkMainClient():
centerid_(0),center_port_(0),
server_num_(0),center_ip_(""),
local_name_(""),local_ip_(""),
auto_start_router_(false),center_link_type_(2)
{
    //加载配置
    load_cfg();
}

void JkMainClient::load_cfg()
{
    center_ip_ = config::instance()->center_ip("0.0.0.0");
    center_port_ = config::instance()->center_port(18000);
    centerid_ = config::instance()->center_id(1);
    local_name_ = config::instance()->local_name("XTRouterV428");
    local_ip_ = config::instance()->local_ip("0.0.0.0");
    server_num_ = config::instance()->chan_num(128);
    auto_start_router_ = config::instance()->auto_start_router(true);
    center_link_type_ = config::instance()->center_link_type(2);

    std::ostringstream oss;
    oss << "load_cfg:center_ip_ = " << center_ip_ << ",center_port_=" <<center_port_<< ",centerid_=" << centerid_
        << ",local_name_=" << local_name_ << ",local_ip_=" <<  local_ip_ << ",server_num_=" << server_num_
        <<",auto_start_router_=" << auto_start_router_ << ",center_link_type_=" << center_link_type_<<std::endl;
    std::cout<<oss.str();
     
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
}

//dll
///////////////////////////////////////////////////////////////////////////////
void JkMainClient::init_jk()
{
    m_jk.init();

    m_jk.SetCallBackOnTellLocalIDS(OnTellLocalIDS,this);
    m_jk.SetCallBackOnLinkServer(OnLinkServer,this);
    m_jk.SetCallBackOnUserInOut(OnUserInOut,this);	
    m_jk.SetCallBackOnDBImageCenter(OnDBImageCenter,this);
    m_jk.SetCallBackOnEventGetMsg(OnEventGetMsg,this);
    m_jk.SetCallBackOnTransparentCommand(OnTransparentCommandCB,this);
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JkMainClient::init_jk");

}
void JkMainClient::uninit_jk()
{
    m_jk.uninit();

}

//回调事件
///////////////////////////////////////////////////////////////////////////////
// 通知本机的IDS 
void JkMainClient::OnTellLocalIDS(void* pUser, char* LocalIDS, char* sRes1, long iRes1)
{
    JkMainClient* pjk = (JkMainClient*)pUser;
    if (pjk == NULL)
    {
        return;
    }
    
    std::ostringstream oss;
    oss << "JkMainClient::OnTellLocalIDS" << ",LocalIDS=" <<LocalIDS << ",sRes1=" << sRes1 <<",iRes1=" << iRes1 <<std::endl;
    std::cout<<oss;
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    pjk->OnTellLocalIDSJkEvent(LocalIDS, sRes1, iRes1);
}

// 通知本机的IDS  
void JkMainClient::OnLinkServer(void* pUser, long sNum, long bz)
{
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JkMainClient::OnLinkServer");
    JkMainClient* pjk = (JkMainClient*)pUser;
    if (pjk == NULL)
    {
        return;
    }
    pjk->OnLinkServerJkEvent(sNum, bz);
}

// 链路连接成功 
void JkMainClient::OnUserInOut(void* pUser, char* sIDS, char* sName, long sType, char* sIPS, long bz, long iRes1, long iRes2, char* sRes1, char* sRes2)
{
    //std::cout<<"OnUserInOut"<<std::endl;
    JkMainClient* pjk = (JkMainClient*)pUser;
    if (pjk == NULL)
    {
        return;
    }
    std::ostringstream oss;
    oss << "JkMainClient::OnUserInOut" << ",sIDS=" <<sIDS <<",sName=" << sName << ",sType=" << sType <<",sIPS=" << sIPS
        << ",bz=" << bz << ",iRes1=" << iRes1 << ",iRes2=" << iRes2 << ",sRes1= " << sRes1 << ",sRes2 =" << sRes2 << std::endl;
    std::cout<<oss;
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    pjk->OnUserInOutJkEvent(sIDS, sName, sType, sIPS, bz, iRes1, iRes2, sRes1, sRes2);
}

//专线保障
void JkMainClient::OnEventGetMsg(void* pUser, char* sSrcIDS, char* sData,long nDataLen, long nOrderbz)
{
    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JkMainClient::OnEventGetMsg");
    JkMainClient* pjk = (JkMainClient*)pUser;
    if (pjk == NULL)
    {
        return;
    }
    pjk->OnGetMsgJkEvent(sSrcIDS, sData, nDataLen, nOrderbz);
}

//通知服务器，本机的某个图像被点播 
void JkMainClient::OnDBImageCenter(void* pUser, char* sIDS, long sCH, long successbz, char* fIDS, char* fIPS, long DBMode, long localVPort,long localAPort, long destVPort, long destAPort, long iRes1, char* sRes1)
{
  
    JkMainClient* pjk = (JkMainClient*)pUser;
    if (pjk == NULL)
    {
        return;
    }
    std::ostringstream oss;
    oss << "JkMainClient::OnDBImageCenter" << ",sIDS=" <<sIDS <<",sCH=" << sCH << ",successbz=" << successbz <<",fIDS=" << fIDS
        << ",DBMode=" << DBMode << ",localVPort=" << localVPort << ",localAPort=" << localAPort << ",destAPort= " << destAPort
        << ",iRes1 =" << iRes1 << ",sRes1="<< sRes1 <<std::endl;
    std::cout<<oss;

     WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,oss.str().c_str());
    pjk->OnDBImageCenterJkEvent(sIDS, sCH, successbz, fIDS, fIPS, DBMode, localVPort, localAPort, destVPort, destAPort, iRes1, sRes1);
}

//透传指令
void JkMainClient::OnTransparentCommandCB(void* pUser, char* fIDS, char* sCommands)
{
    JkMainClient* pjk = (JkMainClient*)pUser;
    if (pjk == NULL)
    {
        return;
    }

    pjk->OnTransparentCommandJkEvent(fIDS,sCommands);
}

bool JkMainClient::LinkCenter()
{
   WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JkMainClient::LinkCenter start");
   std::cout<<"JkMainClient::LinkCenter start"<<std::endl;
    m_jk.SetMainHwnd(0);

    m_jk.NewSetLocalType(Center_TYPE);
    m_jk.CheckPassword(0x1000, "", "", "", server_num_);//将交换机的交换能力提交给中心
   WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JkMainClient::LinkCenter  XTRouter Max Transmit Channel(%d)",server_num_);
   printf("JkMainClient::LinkCenter  XTRouter Max Transmit Channel(%d)\n",server_num_);
    
     //0：dricplay  1:tcp 2:udx
    m_jk.SetLinkType(centerid_,center_link_type_);
    m_jk.SetServerInfo((short)centerid_, center_ip_.c_str(), center_port_);

   WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"LinkCenter  CenterID(%d),CenterIP(%s),CenterPort(%d)",centerid_, center_ip_.c_str(), center_port_);
   printf("LinkCenter  CenterID(%d),CenterIP(%s),CenterPort(%d)\n",centerid_, center_ip_.c_str(), center_port_);
    m_jk.SendInfo(local_name_.c_str(), local_ip_.c_str());


   WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JkMainClient::LinkCenter  Local Name(%s),Local IP(%s)",local_name_.c_str(), local_ip_.c_str());
   printf("JkMainClient::LinkCenter  Local Name(%s),Local IP(%s)",local_name_.c_str(), local_ip_.c_str());
    m_jk.StartLinkServer((short)centerid_);

    WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JkMainClient::LinkCenter end ");
    printf("JkMainClient::LinkCenter end\n");
    return true;
}

void JkMainClient::ExitCenter()
{
    m_jk.StopLinkServer((short)centerid_);
   WRITE_LOG(JK_MIAN_CLIENT_INFO,ll_info,"JkMainClient::StopLinkServer  centerid(%d) ",centerid_);
   printf("JkMainClient::StopLinkServer  centerid(%d)\n",centerid_);
}