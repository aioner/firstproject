#include "SlaveIPC.h"
#ifdef USE_SNMP_
int _stdcall SlaveIPC::RequestGetTbl(report_category category, void *data, int *len, void *user_context)
{
    switch(category)
    {
    case Category_DeviceStatus:
        {
            SDeviceStatus *pDS = ((SlaveIPC*)user_context )->GetDeviceStatus();
            memcpy(data, pDS, sizeof(*pDS));
            break;
        }
    case Category_DevicePerformance:
        { 
            SDevicePerformance *pDP = ((SlaveIPC*)user_context)-> GetDevicePerformance();
            memcpy(data, pDP, sizeof(*pDP));
            break;
        }
    case Category_LinkTraffic:
        {
//             std::cout << "Category_LinkTraffic test! "<<std::endl;
//             vector<SLinkTraffic> vecLinkTraffic = ((SlaveIPC*)user_context)-> GetLinkTraffic();
//             memcpy(data, &vecLinkTraffic.front(), sizeof(vecLinkTraffic.front()));
//             vector<SLinkTraffic>::iterator ite = vecLinkTraffic.begin();
//             vecLinkTraffic.erase(ite); //ɾ����һ��Ԫ��
// 
//             if ( 0 == vecLinkTraffic.size() ) return -1;
// 
//             return  vecLinkTraffic.size();
        }
    case Category_LinkLossAndDelay:
        {
//             std::cout << "Category_LinkLossAndDelay test! "<<std::endl;
//             vector<SLinkLossAndDelay> vecLossAndDelay = ((SlaveIPC*)user_context)-> GetLinkLossAndDelay();
//             memcpy(data, &vecLossAndDelay.front(), sizeof(vecLossAndDelay.front()));
//             vector<SLinkLossAndDelay>::iterator ite = vecLossAndDelay.begin();
//             vecLossAndDelay.erase(ite); //ɾ����һ��Ԫ��
// 
//             if ( 0 == vecLossAndDelay.size() ) return -1;
// 
//             return  vecLossAndDelay.size();
        }
    case Category_CodecRecord:
    case Category_CommonConfiguration:
    case Category_MeetingConfiguration:
    case Category_MeetingsBasic:
    case Category_MeetingsPeople:
    case Category_MeetingsTerminals:
    case Category_ServerTerminals:
    case Category_MediaServerStatistic:
    case Category_StoreServerStatistic:
        break;
    default:
        break;	
    }

    return 0;
}

int _stdcall SlaveIPC::LostHeartBeat(void *user_context)
{
    return 0;
}

SlaveIPC SlaveIPC::m_Obj;

SlaveIPC::SlaveIPC(void)
{
    this->m_fcbLostHeartBeatCB = LostHeartBeat;
    this->m_fcbRequestGetTblCB = RequestGetTbl;
    this->m_fcbRequestSetTblCB = NULL;
    this->m_pDeviceStatus      = new SDeviceStatus;
    this->m_pDevicePerformance = new SDevicePerformance;
    this->m_pLinkTraffic       = new SLinkTraffic;
    this->m_pLinkLossAndDelay  = new SLinkLossAndDelay;
}

SlaveIPC::~SlaveIPC(void)
{

}

int SlaveIPC::EstablishSlaveIPC(int device_type)
{
    int iRetVal = StatusSlave_Startup(this->m_fcbLostHeartBeatCB,this);
    if( iRetVal < 0 ) return iRetVal;

    iRetVal = StatusSlave_ConnectMaster(NULL,0,device_type);
    if( iRetVal < 0 ) return iRetVal; 

    iRetVal = StatusSlave_SetRequestCallbackEx(this->m_fcbRequestGetTblCB, this->m_fcbRequestGetTblCB, this->m_fcbRequestSetTblCB, this); 
    if( iRetVal < 0 ) return iRetVal;

    return 0;
}

SDeviceStatus* SlaveIPC::GetDeviceStatus()
{

	std::string strIP       = config::instance()->local_sndip("0.0.0.0");

    //�豸����
    this->m_pDeviceStatus->dsIndex = config::instance()->device_status_index(0);

	 //�豸����
    strcpy(this->m_pDeviceStatus->dsName, "XTRouter"); 

    //�豸��ϵ�绰
    strcpy(this->m_pDeviceStatus->dsContact,  config::instance()->device_status_contact("").c_str());

    //�豸��������
    strcpy(this->m_pDeviceStatus->dsManufactroy, config::instance()->device_status_manufactory("").c_str());

    //����汾
    strcpy(this->m_pDeviceStatus->dsSoftwareVersion,config::instance()->device_status_software_version("").c_str());

    //Ӳ���汾
    strcpy(this->m_pDeviceStatus->dsHardwareVersion, config::instance()->device_status_hardware_version("").c_str());

    //�豸IP��ַ
    strcpy(this->m_pDeviceStatus->dsDeviceIPAddr,  strIP.c_str());

    //�豸MAC��ַ
	std::string strMac = get_mac_by_ip(strIP);
    strcpy(this->m_pDeviceStatus->dsDeviceMac,  strMac.c_str());

    //�豸�ͺ�
    strcpy(this->m_pDeviceStatus->dsDeviceHX, config::instance()->device_status_xh("").c_str());

    //ϵͳΨһ��ʶ
    strcpy(this->m_pDeviceStatus->dsCode, config::instance()->device_status_code("").c_str());

    //�豸����
    this->m_pDeviceStatus->dsDeviceType      = config::instance()->device_status_type(1);

    //�豸����ʱ��
    std::string strTime = info_mgr::GetCurTime();
    this->m_pDeviceStatus->dsUptime          = info_mgr::ToTMTime(strTime.c_str());

    //�豸�Ƿ�����
    this->m_pDeviceStatus->dsOnline          = true;

    return this->m_pDeviceStatus;
}

SDevicePerformance* SlaveIPC::GetDevicePerformance()
{
    //�豸����
    this->m_pDevicePerformance->devicepfIndex = config::instance()->device_status_index(0);

    std::string strErrMsg   = "";
    std::string strSystemId = "";
    std::string strIP       = config::instance()->local_sndip("0.0.0.0");

    //ϵͳΨһ��ʶ
    int id = get_system_id(strSystemId, strErrMsg);
    strcpy(this->m_pDevicePerformance->devicepfCode, strSystemId.c_str()); 

    //CPUʹ����
    int cpuVal = get_cpu_usage(this->m_pDevicePerformance->devicepfCpuUsage, strErrMsg);

    //�ڴ�ʹ����
    int memVal = get_memory_usage(this->m_pDevicePerformance->devicepfMemUsage,strErrMsg);

    //����ʹ����
    int diskVal = get_disk_usage(this->m_pDevicePerformance->devicepfDiskUsage, strErrMsg);

    //CPU �¶�
    this->m_pDevicePerformance->devicepfTemperature = 0; 
#ifndef _WIN32
	string strTempFile = config::instance()->device_status_temperature_file("/sys/class/hwmon/hwmon0/temp1_input");
    int val = get_device_temperature(this->m_pDevicePerformance->devicepfTemperature,strTempFile, strErrMsg);
#else
    int val = get_device_temperature(this->m_pDevicePerformance->devicepfTemperature,"", strErrMsg);
#endif

    //���������ֽ���
    unsigned long send = 0;
    int sendVal = get_device_send_bytes_by_ip(send,strIP,strErrMsg);
    this->m_pDevicePerformance->devicepfSendBytes = static_cast<long>(send);

    //���������ֽ���
    unsigned long recv = 0;
    int recvVal = get_device_recv_bytes_by_ip(recv,strIP,strErrMsg);
    this->m_pDevicePerformance->devicepfRecvBytes = static_cast<long>(recv);

    //����������
    unsigned long bandwidth = 0;
    int bandwidthVal = get_device_bandwidth_by_ip(bandwidth,strIP,strErrMsg);
    this->m_pDevicePerformance->devicepfBindth = static_cast<long>(bandwidth);

    return this->m_pDevicePerformance;
}

vector<SLinkTraffic>& SlaveIPC::GetLinkTraffic()
{
//     //Clear 
//     vector<SLinkTraffic>().swap(listLinkTraffic);
// 
//     //�������
//     unsigned long bandwidth = 0;
//     std::string strErrMsg   = "";
//     std::string strGUID     = config::instance()->network_interface_guid("");
//     int bandwidthVal        = get_device_bandwidth(bandwidth,strGUID,strErrMsg);
// 
//     //�㲥��·��Ϣ
//     SLinkTraffic  linkTraffic;
//     std::list<info_mgr::INFO_PLAY> lstPlayInfo;
//     CInfoMgr::instance()->GetPlayInfo(lstPlayInfo);
//     std::list<info_mgr::INFO_PLAY>::iterator itr = lstPlayInfo.begin();
//     int  linkCount = 0;
//     for (; lstPlayInfo.end() != itr; ++itr)
//     {
//         //��·����
//         linkTraffic.linkIndex        = linkCount;
// 
//         //ÿ������ֽ���
//         linkTraffic.linkNetInOctets  = itr->m_lRCVBytePS * 1024;
// 
//         //ÿ�뷢���ֽ���
//         linkTraffic.linkNetOutOctets = 0;
// 
//         //�������
//         linkTraffic.linkBindth       = static_cast<long>(bandwidth);
// 
//         //���豸Ψһ��ʶ 
//         strcpy(linkTraffic.localDeviceCode,"XTRouter");
// 
//         //��·��ϵͳ��ͳһ����
//         strcpy(linkTraffic.linkCode , itr->m_sDevIDS.c_str());
// 
//         //�Զ��豸Ψһ��ʶ
//         strcpy(linkTraffic.remoteDeviceCode , itr->m_sDBUrl.c_str());
// 
//         listLinkTraffic.push_back(linkTraffic);
//         linkCount++;
//     }
// 
//     //ת����·��Ϣ
//     list<src_info> lstSrc; //ת��Դ��Ϣ
//     XTEngine::instance()->get_all_src(lstSrc);
//     list<src_info>::iterator itrSrc = lstSrc.begin();
//     int count = 0; //Defined for ����ͨ����
//     for (; lstSrc.end() != itrSrc; ++itrSrc)
//     {
//         //if( itr->m_lStreamId == itrSrc->srcno) 
//         //{
//         std::list<info_mgr::INFO_TRANS> lstTransInfoOut;
//         CInfoMgr::instance()->GetConnectInfo(lstTransInfoOut);
// 
//         if ( lstTransInfoOut.size() > 0 )
//         {
//             std::list<info_mgr::INFO_TRANS>::iterator itrTransInfo = lstTransInfoOut.begin();
// 
//             for ( ; lstTransInfoOut.end() != itrTransInfo; itrTransInfo++ )
//             {
//                 if ( itrTransInfo->m_lChID == count )  //�ж��Ƿ�Ϊ��ͬ�Ľ���ͨ��
//                 {
//                     //��·����
//                     linkTraffic.linkIndex        = linkCount;
// 
//                     //ÿ������ֽ���
//                     linkTraffic.linkNetInOctets  = 0;
// 
//                     //ÿ�뷢���ֽ���
//                     time_t beginTime   = info_mgr::ToUnixTimestamp(itrTransInfo->m_pszCreateTime);
//                     time_t currentTime = info_mgr::ToUnixTimestamp( info_mgr::GetCurTime().c_str() );
//                     linkTraffic.linkNetOutOctets = itrTransInfo->m_uiOctets / ( currentTime - beginTime );
// 
//                     //�������
//                     linkTraffic.linkBindth  = static_cast<long>(bandwidth);
// 
//                     //���豸Ψһ��ʶ 
//                     strcpy(linkTraffic.localDeviceCode,"XTRouter");
// 
//                     //��·��ϵͳ��ͳһ����
//                     strncpy_s(linkTraffic.linkCode, itrTransInfo->m_pszSrcIDS, sizeof(linkTraffic.linkCode) - 1);
// 
//                     //�Զ��豸Ψһ��ʶ
//                     strncpy_s(linkTraffic.remoteDeviceCode , itrTransInfo->m_pszDestIP, sizeof(linkTraffic.remoteDeviceCode) - 1);
// 
//                     listLinkTraffic.push_back(linkTraffic);
//                     linkCount++;
// 
//                 }
//             }
//         }
// 
//         //}
//         count++;     
//     }

    return listLinkTraffic;

}

vector<SLinkLossAndDelay>& SlaveIPC::GetLinkLossAndDelay()
{
//     //Clear 
//     vector<SLinkLossAndDelay>().swap(listLinkLossAndDelay);
// 
//     //ȡת���Ķ���ʱ��
//     SLinkLossAndDelay  linkLossAndDelay;
//     std::list<info_mgr::INFO_TRANS> lstTransInfoOut;
//     CInfoMgr::instance()->GetConnectInfo(lstTransInfoOut);
//     int count = 0;
//     if ( lstTransInfoOut.size() > 0 )
//     {
//         std::list<info_mgr::INFO_TRANS>::iterator itrTransInfo = lstTransInfoOut.begin();
// 
//         for ( ; lstTransInfoOut.end() != itrTransInfo; itrTransInfo++ )
//         {
//             //��·����
//             linkLossAndDelay.linkIndex       = count;
// 
//             //�ɼ����
//             linkLossAndDelay.collectInterval = 0;
// 
//             //���ӻ�δ����
//             linkLossAndDelay.linkStatus      = true;
// 
//             //������·�Ķ�����
//             linkLossAndDelay.linkPacketLoss  = itrTransInfo->m_uiFractionLost;
// 
//             //������·��ʱ�Ӵ�С
//             linkLossAndDelay.linkTimeDelay   = itrTransInfo->m_uiDlSR;
// 
//             //�ɼ�ʱ��
//             linkLossAndDelay.collectTime     = 0;
// 
//             //��״̬
//             linkLossAndDelay.linkRowStatus   = 1;
// 
//             listLinkLossAndDelay.push_back(linkLossAndDelay);
// 
//             count++;
//         }
// 
//     }
// 
//     //ȡ�㲥�Ķ���ʱ��
//     std::list<info_mgr::INFO_PLAY> lstPlayInfo;
//     CInfoMgr::instance()->GetPlayInfo(lstPlayInfo);
//     if(lstPlayInfo.size() > 0 )
//     {
//         std::list<info_mgr::INFO_PLAY>::iterator itr = lstPlayInfo.begin();
//         for ( ; lstPlayInfo.end() != itr; itr++ )
//         {
//             //��·����
//             linkLossAndDelay.linkIndex       = count;
// 
//             //�ɼ����
//             linkLossAndDelay.collectInterval = 0;
// 
//             //���ӻ�δ����
//             linkLossAndDelay.linkStatus      = true;
// 
//             //������·�Ķ�����
//             linkLossAndDelay.linkPacketLoss  = itr->m_lLost;
// 
//             //������·��ʱ�Ӵ�С
//             linkLossAndDelay.linkTimeDelay   = itr->m_lDelay;
// 
//             //�ɼ�ʱ��
//             linkLossAndDelay.collectTime     = 0;
// 
//             //��״̬
//             linkLossAndDelay.linkRowStatus   = 1;
// 
//             listLinkLossAndDelay.push_back(linkLossAndDelay);
// 
//             count++;
//         }
//     }

    return listLinkLossAndDelay;
}

int SlaveIPC::SendNotification(trap_type type)
{
    char dsCode[20];
    strcpy(dsCode,config::instance()->device_status_code("").c_str());

    int dsIndex      = config::instance()->device_status_index(0);
    int iRetVal      = StatusSlave_SendNotification(dsCode, dsIndex, type);
    if( iRetVal < 0 ) return iRetVal;

    return 0;
}

int SlaveIPC::Exit()
{
    if(this->m_pDeviceStatus != NULL )
    {
        delete this->m_pDeviceStatus;
        this->m_pDeviceStatus = NULL;
    }

    if(this->m_pDevicePerformance != NULL )
    {
        delete this->m_pDevicePerformance;
        this->m_pDevicePerformance = NULL;
    }

    if(this->m_pLinkTraffic != NULL )
    {
        delete this->m_pLinkTraffic;
        this->m_pLinkTraffic = NULL;
    }

    if(this->m_pLinkLossAndDelay != NULL )
    {
        delete this->m_pLinkLossAndDelay;
        this->m_pLinkLossAndDelay = NULL;
    }

    int iRetVal = StatusSlave_DisconectMaster();
    if( iRetVal < 0 ) return iRetVal;

    iRetVal = StatusSlave_Cleanup();
    if( iRetVal < 0 ) return iRetVal;

    return 0;
}
#endif //#ifdef USE_SNMP_