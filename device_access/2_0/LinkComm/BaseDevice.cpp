#include "BaseDevice.h"
#include "LinkInfo.h"

#include "tinyxml/tinyxml.h"

#ifdef WIN32
#else
#include <dlfcn.h>
#include <unistd.h>
#include <string>
#include <time.h>

using namespace std;

const char* LINKCOMMON_VER_INFO = "XT_Lib_Version: V_LC_1.00.1208.0";

#endif

int logLevel = 1;
void WriteLog(int level, const char* title, const char* content)
{
	if (level < 1)
	{
		return;
	}

	char timeBuf[100];
	char fileName[100];
	char titleBuf[100]={0};

#ifdef WIN32
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	sprintf(fileName, "d:\\Log\\MediaDeviceLog\\Log_%d%02d%02d.txt", sysTime.wYear, sysTime.wMonth,sysTime.wDay);
	sprintf(timeBuf, "[%d-%02d-%02d %02d:%02d:%02d.%03d]   ", sysTime.wYear, sysTime.wMonth,sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
#else
	struct timeval tv;
	struct timezone tz;
	struct tm *ptm = NULL;

	gettimeofday(&tv, &tz);
	//ptm = gmtime((const time_t*)&tv.tv_sec);
	ptm = localtime((const time_t*)&tv.tv_sec);
	sprintf(fileName, "/var/log/MediaDeviceLog/Log_%d%02d%02d.txt", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
	sprintf(timeBuf, "%d-%02d-%02d %02d:%02d:%02d  ", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
#endif

	FILE *fl = fopen(fileName, "a+b");
	if (fl == NULL)
	{
		return;
	}

	_snprintf(titleBuf, 27, "%- 30s", title);

	fwrite(timeBuf, sizeof(char), strlen(timeBuf), fl);
	fwrite(titleBuf, sizeof(char), strlen(titleBuf), fl);
	fwrite(content, sizeof(char), strlen(content), fl);
#ifdef WIN32
	fwrite("\r\n", sizeof(char), 2, fl);
#else
	fwrite("\n", sizeof(char), 1, fl);
#endif

	fclose(fl);
}


CBaseDevice::CBaseDevice(void)
{
    m_pDeviceCollect = NULL;
    m_Links = new LINKINFO[MAX_LINKS];
    ::memset(m_Links, 0, sizeof(m_Links));
    //InitializeCriticalSection(&m_lockAddLink);
}

CBaseDevice::~CBaseDevice(void)
{
//     for (LINKMAP::iterator iter=m_mapLink.begin(); iter!=m_mapLink.end(); iter++)
//     {
//         if (iter->second != NULL)
//         {
//             delete iter->second;
//             iter->second = NULL;
//         }
//     }
//     m_mapLink.clear();


    //DeleteCriticalSection(&m_lockAddLink);
}

long CBaseDevice::GetLoginHandle(char* szIp)
{
//     map<long,LINKINFO*>::iterator iter;
// 
//     for(iter = m_mapLink.begin(); iter != m_mapLink.end(); iter++)
//     {
//         if(strcmp(iter->second->szIp, szIp) == 0)
//         {
//             return  iter->second->nLoginHandle;
//         }
//     }

    for (int i=0;i < MAX_LINKS;++i)
    {
        if(m_Links[i].use && strcmp(m_Links[i].szIp, szIp) == 0)
         {
             return  m_Links[i].nLoginHandle;
         }
    }

    return -1;
}

bool  CBaseDevice::GetPorts(long portnum, long *pPorts, bool bIsSort)
{
    if(!m_pDeviceCollect)
        return false;

    return m_pDeviceCollect->GetPorts(portnum, pPorts, bIsSort);
}

void CBaseDevice::SetLinkType(long nLinkType, long nDataType)
{
    m_nLinkType = nLinkType; 
    m_nDataType = nDataType;
}

void CBaseDevice::FreePorts(long *pPorts, long portnum)
{
    if(!m_pDeviceCollect)
        return;

    m_pDeviceCollect->FreePorts(pPorts, portnum);
}

//获取媒体类型
long  CBaseDevice::GetMediaType(long lLinkHandle)
{
//     LINKMAP::iterator iter =  m_mapLink.find(lLinkHandle);
//     if (iter != m_mapLink.end())
//     {
//         return iter->second->nMediaType;
//     }
//     else
//     {
//         return -1;
//     }

    for (int i=0;i < MAX_LINKS;++i)
    {
        if(m_Links[i].use && m_Links[i].nLinkHandle == lLinkHandle)
        {
            return  m_Links[i].nMediaType;
        }
    }

    return -1;
}
 LINKINFO* CBaseDevice::GetLinkMap()
 {
     return m_Links;
 }

int CBaseDevice::GetLinkInfo(long handle, LINKINFO **link)
{
    for (int i=0;i < MAX_LINKS;++i)
    {
        if(m_Links[i].use && m_Links[i].nLinkHandle == handle)
        {
            //::memcpy(link, &m_Links[i], sizeof(LINKINFO));
			*link = &m_Links[i];
            return  0;
        }
    }

    return -1;
}

//获取链接数
long   CBaseDevice::GetLinkCount()
{
    long count = 0;
    for (int i=0;i < MAX_LINKS;++i)
    {
        if(m_Links[i].use)
        {
            count += 1;
        }
    }

    return count;
}

//添加链接
void* CBaseDevice::AddLinkInfo(LINKINFO *pLinkinfo)
{
    for (int i=0;i < MAX_LINKS;++i)
    {
        if(!m_Links[i].use)
        {
            ::memcpy(&m_Links[i], pLinkinfo, sizeof(LINKINFO));
             m_Links[i].use = true;
            return &m_Links[i];
        }
    }

    return NULL;
    //EnterCriticalSection(&m_lockAddLink);
    //m_mapLink.insert(pair<long, LINKINFO*>(pLinkinfo->nLinkHandle, pLinkinfo));
    //LeaveCriticalSection(&m_lockAddLink);
}

//删除链接
void CBaseDevice::DelLinkInfo(long linkhandle)
{
    for (int i=0;i < MAX_LINKS;++i)
    {
        if(m_Links[i].use && m_Links[i].nLinkHandle==linkhandle)
        {
            m_Links[i].use = false;
            ::memset(&m_Links[i], 0, sizeof(LINKINFO));
            return;
        }
    }

    return;
    //EnterCriticalSection(&m_lockAddLink);
    //m_mapLink.insert(pair<long, LINKINFO*>(pLinkinfo->nLinkHandle, pLinkinfo));
    //LeaveCriticalSection(&m_lockAddLink);
}


void  CBaseDevice::SetDeviceCollect(CDeviceCollect *pDeviceCollect)
{
    m_pDeviceCollect = pDeviceCollect;
}

//通知数据头
long  CBaseDevice::NotifyHeadData(long lDeviceLinkHandle, unsigned char* szHead, long nSize)
{
	return 0;
}



CDeviceCollect::CDeviceCollect(void)
{
    char szBuf[MAX_PATH]={0};

#ifdef _WIN32
    HMODULE  hModule = GetModuleHandle("MediaDevice2.0.dll");
    ::GetModuleFileNameA(hModule,szBuf,MAX_PATH);

    long len = strlen(szBuf);

    for(long i = len - 1;i>=0;i--)
    {
        if(szBuf[i] == '\\')
        {
            szBuf[i] = 0;
            break;
        }
    }
#else
    Dl_info dlInfo;
    int ret = dladdr("", &dlInfo);
    if (ret == 0)
    {
        printf("Error: Get current filepath failed");
    }
    else
    {
        for (int i=strlen(dlInfo.dli_fname)-1; i>0; i--)
        {
            if (dlInfo.dli_fname[i] == '/')
            {
                strncpy(szBuf, dlInfo.dli_fname, i);
                break;
            }
        }
		if (szBuf[0]=='\0')
		{
			getcwd(szBuf, MAX_PATH);
		}
        char pathInfo[500];
        sprintf(pathInfo, "Get current filepath: path=%s\n", szBuf);
        printf(pathInfo);
    }
#endif//#ifdef _WIN32

    m_pPortStatus = NULL;
    LoadDevices(szBuf);
}

CDeviceCollect::~CDeviceCollect(void)
{
    UnLoadDevices();

    if(m_pPortStatus)
    {
        delete []m_pPortStatus;
        m_pPortStatus = NULL;
    }
}

long CDeviceCollect::SetPortRange(long startPort, long portNum)
{
    if (portNum < 1)
    {
        m_nStartPort =16000;
        m_nEndPort   = 17000;
    }
    else
    {
        m_nStartPort = startPort;
        m_nEndPort   = startPort + portNum;
    }

    if (m_pPortStatus != NULL)
    {
        delete [] m_pPortStatus;
    }

    m_pPortStatus = new BYTE[m_nEndPort - m_nStartPort];
    memset(m_pPortStatus, 0, m_nEndPort - m_nStartPort);

    return 0;
}

/***
nFlag - 0/1  无条件/排序
***/
bool  CDeviceCollect::GetPorts(long portnum, long *pPorts, bool bIsSort)
{
    if(pPorts == NULL)
        return false;

    if(!bIsSort)
    {
        for(long i = 0; i<m_nEndPort - m_nStartPort; i++)
        {
            if(m_pPortStatus[i] == 0)
            {
                pPorts[--portnum] = i + m_nStartPort;
                m_pPortStatus[i] = 1;
            }

            if(portnum == 0)
            {
                return true;
            }
        }
    }
    else
    {
        /*		int nSpaceUnit = 0;
        for(int i = 0; i<m_nEndPort - m_nStartPort - portnum; i+= nSpaceUnit)
        {
        for(int j = 0; j<portnum;j++)
        {
        pPorts[j] = i + j + m_nStartPort;
        if(m_pPortStatus[i + j] != 0)
        {
        nSpaceUnit = j;
        continue;
        }
        }

        for(int j = 0; j<portnum;j++)
        {
        m_pPortStatus[pPorts[j] - m_nStartPort] = 1;
        }

        return true;

        }*/
        long nSpaceCnt = 0;
        long i  = 0;
        for(i = 0; i<m_nEndPort - m_nStartPort; ++i)
        {
            if (0 != m_pPortStatus[i])
            {
                nSpaceCnt = 0;
                continue;
            }

            nSpaceCnt++;

            if (nSpaceCnt >= portnum)
            {
                break;
            }
        }

        if(i == m_nEndPort - m_nStartPort)
            return false;

        while (nSpaceCnt > 0)
        {
            m_pPortStatus[i] = 1;
            pPorts[nSpaceCnt - 1] = m_nStartPort + i;
            nSpaceCnt--;
            i--;
        }

        return true;
    }

    return false;
}

//释放端口
void CDeviceCollect::FreePorts(long *pPorts, long portnum)
{
    if(!pPorts)
        return;

    for(long i = 0; i<portnum; i++)
    {
        if(pPorts[i]>m_nEndPort||pPorts[i]<m_nStartPort)
            continue;

        m_pPortStatus[pPorts[i] - m_nStartPort] = 0;
    }
}


bool CDeviceCollect::LoadDevices(char* szWorkPath)
{
    char szXmlPath[256];
    char szDllDir[256];

#ifdef WIN32	
    sprintf_s(szXmlPath, "%s\\deviceinfo.xml", szWorkPath);

    OutputDebugStringA(szXmlPath);
#else
    sprintf(szXmlPath, "%s/deviceinfo.xml", szWorkPath);
#endif	

    TiXmlDocument  tiXmlDocument;
    if(!tiXmlDocument.LoadFile(szXmlPath))
    {
#ifdef WIN32
#else
        printf(">>>Open configuration file failed!\n");	
#endif
        return false;
    }
    TiXmlElement*  tiXmlElement = tiXmlDocument.FirstChildElement("mediaDevice")->FirstChildElement("devlinkinfo")->FirstChildElement("devitem");

    typedef CBaseDevice* (FUN_AddMe)(CDeviceCollect* pDeviceCollect, long nDeviceLinkType, long nDataType);
    while(tiXmlElement != NULL)
    {
        int     nDevLinkType = -1;
        int     nDataType    = -1;
#ifdef WIN32
        HMODULE  hInstance = NULL;
#else
        void*  hInstance = NULL;
#endif

        FUN_AddMe*	 pLoadFun   =	NULL;

        tiXmlElement->Attribute("linktype", &nDevLinkType);
        tiXmlElement->Attribute("DataType", &nDataType);
        string	strKeyDLLName = tiXmlElement->Attribute("maindll");
        string	strDir        = tiXmlElement->Attribute("path");

#ifdef WIN32
        sprintf(szDllDir, "%s\\dll\\%s\\", szWorkPath,strDir.c_str());
#else
        sprintf(szDllDir, "%s/dll/%s/", szWorkPath,strDir.c_str());
        printf(">>>Load %s .....\n", szDllDir);
#endif

        long t = 0;
        char szDllName[256];

        do
        {
            t = strKeyDLLName.find('|');

            if(t>=0)
            {
                sprintf(szDllName ,"%s", szDllDir);
                memcpy(szDllName + strlen(szDllDir) , strKeyDLLName.c_str(), t);
                szDllName[ + strlen(szDllDir) + t] = 0;
#ifdef WIN32
                hInstance = LoadLibraryEx(szDllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
                printf("  >>Open %s .....\n", szDllName);
                hInstance = dlopen(szDllName, RTLD_LAZY|RTLD_GLOBAL);
#endif	
                strKeyDLLName.erase(0, t+1);
#ifdef WIN32
#else
                if (hInstance != NULL)
                {
                    printf("    OK.\n\n");
                }
                else
                {
                    printf("    Failed. %s\n\n", dlerror());
                }
#endif
            }
            else
            {
                sprintf(szDllName , szDllDir);
                memcpy(szDllName + strlen(szDllDir), strKeyDLLName.c_str(), strKeyDLLName.length());
                szDllName[strlen(szDllDir)+strKeyDLLName.length()] = 0;
                //memcpy(szDllName , strKeyDLLName.c_str(), strKeyDLLName.length());
                //szDllName[strKeyDLLName.length()] = 0;

#ifdef WIN32
                hInstance = LoadLibraryEx(szDllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
                printf("  >>Open %s .....\n", szDllName);
                hInstance = dlopen(szDllName, RTLD_LAZY|RTLD_GLOBAL);
                if (hInstance != NULL)
                {
                    printf("    OK.\n\n");
                }
                else
                {
                    printf("    Failed. %s\n\n", dlerror());
                }
#endif	
            }

        }while(t > 0);
        //加载主库,生成devicemap
        //HINSTANCE hInstance = ::LoadLibraryA((LPCSTR)strKeyDLLName.c_str());

		char szLog[512];
        if (hInstance == NULL)
        {
#ifdef WIN32
			sprintf(szLog, "Error: %d类型库加载失败，请检查%s是否存在", nDevLinkType, szDllName);
#else
			sprintf(szLog, "Error: Load the library of type %d failed, please check the %s", nDevLinkType, szDllName);
#endif
			WriteLog(logLevel,  "[LoadDevices]", szLog);
        }
		else
		{
#ifdef WIN32
			sprintf(szLog, "Info:  %d类型库加载成功", nDevLinkType);
			pLoadFun = (FUN_AddMe*)::GetProcAddress(hInstance,  "AddMe" );
#else
			sprintf(szLog, "Info: Load the library of type %d successful", nDevLinkType);
			pLoadFun = (FUN_AddMe*)dlsym(hInstance,"AddMe");
#endif	
			WriteLog(logLevel,  "[LoadDevices]", szLog);

			CBaseDevice* pDevice  = (*pLoadFun)(this, nDevLinkType, nDataType);

			pDevice->SetDeviceCollect(this);

			//SetCurrentDirectoryA(szWorkPath);

			//生成linktype map
			TiXmlElement*  tiXmlDevItem  = tiXmlElement->FirstChildElement("devgroup")->FirstChildElement("devsbum");

			while(tiXmlDevItem)
			{
				int nDevType = -1;
				int nNetLinkType = -1;

				tiXmlDevItem->Attribute("devtype", &nDevType);
				tiXmlDevItem->Attribute("netlinktype", &nNetLinkType);

				DEVINFO devinfo;
				devinfo.nDevLinkType = nDevLinkType;
				devinfo.nNetLinkType = nNetLinkType;

				m_mapLinkType.insert(pair<long,DEVINFO>(nDevType, devinfo));

				tiXmlDevItem   = tiXmlDevItem->NextSiblingElement("devsbum");
			}
			//生成linktype map end
		}

        tiXmlElement  = tiXmlElement->NextSiblingElement("devitem");
    }

    //加载端口配置
    TiXmlElement*  tiXmlPortElement = tiXmlDocument.FirstChildElement("mediaDevice")->FirstChildElement("portinfo")->FirstChildElement("linkitem");

    while(tiXmlPortElement)
    {
        PORTINFO  portinfo;

        tiXmlPortElement->Attribute("devtype", &portinfo.nDevType);
        tiXmlPortElement->Attribute("netlinktype", &portinfo.nNetLinkType);
        tiXmlPortElement->Attribute("port", &portinfo.nPort);

        m_PortInfo.push_back(portinfo);

        tiXmlPortElement  = tiXmlPortElement->NextSiblingElement("linkitem");
    }

    return true;

}

bool CDeviceCollect::QueryPort(long nDevType, long nNetLinkType, long& nPort)
{
    list<PORTINFO>::iterator iter;

    for(iter = m_PortInfo.begin(); iter != m_PortInfo.end(); iter++)
    {
        if(iter->nDevType == nDevType && iter->nNetLinkType == nNetLinkType)
        {
            nPort = iter->nPort;
            return true;
        }
    }

    return false;

}

//注销设备
bool   CDeviceCollect::UnLoadDevices()
{
    long nCount = 0;
    map<long,CBaseDevice*>::iterator  iter;

    for(iter = begin(); iter != end(); iter++)
    {
        CBaseDevice * pBaseDevice = iter->second;

        if (pBaseDevice != NULL)
        {
            delete pBaseDevice;
        }
    }

    return true;
}

//通过设备类型获取设备对象
CBaseDevice* CDeviceCollect::GetDevice(long nDeviceType)
{
    DEVINFO devinfo = m_mapLinkType[nDeviceType];

    map<long,CBaseDevice*>::iterator iter = this->find(devinfo.nDevLinkType);
    if (iter == this->end())
    {
        return NULL;
    }
    else
    {
        return iter->second;
    }

    //return operator [](devinfo.nDevLinkType);
}

//获取加载的总设备类型数
long   CDeviceCollect::GetDeviceCount()
{
    return m_mapLinkType.size();
}

//获取链接数
long  CDeviceCollect::GetLinkCount()
{
    long nCount = 0;
    map<long,CBaseDevice*>::iterator  iter;

    for(iter = begin(); iter != end(); iter++)
    {
        CBaseDevice *dev = iter->second;
        if (dev)
        {
            nCount += dev->GetLinkCount();
        }
        //LINKMAP * pLinkMap = iter->second->GetLinkMap();
        //nCount += pLinkMap->size();
    }
    return nCount;
}

//判断是否已登录
long CDeviceCollect::GetLoginHandle(char* szIP, long nDeviceType)
{
    CBaseDevice* pDevice = GetDevice(nDeviceType);
    if (pDevice == NULL)
    {
        return -1;
    }

    return GetLoginHandle(szIP, pDevice);
}

//判断是否已登录
long  CDeviceCollect::GetLoginHandle(char* szIP,CBaseDevice *pDevice)
{
//     map<long,LINKINFO*>::iterator iter;
// 
//     for(iter = pDevice->GetLinkMap()->begin(); iter != pDevice->GetLinkMap()->end(); iter++)
//     {
//         if(strcmp(iter->second->szIp, szIP) == 0)
//         {
//             return  iter->second->nLoginHandle;
//         }
//     }

    if (pDevice)
    {
        return pDevice->GetLoginHandle(szIP);
    }

    return -1;
}

//判断是否已链接
long  CDeviceCollect::GetLinkHandle(char* szIP, long nChannel, long nMediaType, CBaseDevice *pDevice)
{
//     map<long,LINKINFO*>::iterator iter;
// 
//     for(iter = pDevice->GetLinkMap()->begin(); iter != pDevice->GetLinkMap()->end(); iter++)
//     {
//         if(strcmp(iter->second->szIp, szIP) == 0 && iter->second->nChannel == nChannel && iter->second->nMediaType == nMediaType)
//         {
//             return  iter->second->nLinkHandle;
//         }
//     }

    if (pDevice)
    {
        LINKINFO *links = pDevice->GetLinkMap();
        for(int i=0;i<MAX_LINKS;++i)
        {
            LINKINFO &l = links[i];
            if(strcmp(l.szIp, szIP) == 0 && l.nChannel == nChannel && l.nMediaType == nMediaType)
            {
                return l.nLinkHandle;
            }
        }
    }

    return -1;
}

//获取链接信息
int CDeviceCollect::GetLinkInfo(long nLinkHandle, CBaseDevice *pDevice, LINKINFO **link)
{
    if (pDevice == NULL)
    {
        return -1;
    }

    return pDevice->GetLinkInfo(nLinkHandle, link);

//     map<long,LINKINFO*>::iterator iter;
//     iter = pDevice->GetLinkMap()->find(nLinkHandle);
//     if (iter == pDevice->GetLinkMap()->end())
//     {
//         return NULL;
//     }
//     else
//     {
//         return iter->second;
//     }
}

bool    CDeviceCollect::IsDeviceLink(long lLoginHandle, CBaseDevice *pDevice)
{
//     map<long,LINKINFO*>::iterator iter;
//     LINKMAP* pMapLink= pDevice->GetLinkMap();
//     if (pMapLink == NULL)
//     {
//         return false;
//     }
// 
//     for(iter = pDevice->GetLinkMap()->begin(); iter != pDevice->GetLinkMap()->end(); iter++)
//     {
//         if(iter->second->nLoginHandle == lLoginHandle)
//         {
//             return  true;
//         }
//     }
// 
//     return false;

    if (pDevice)
    {
        LINKINFO *links = pDevice->GetLinkMap();
        for(int i=0;i<MAX_LINKS;++i)
        {
            LINKINFO &l = links[i];
            if(l.use && l.nLoginHandle == lLoginHandle)
            {
                return true;
            }
        }
    }

    return false;
}

void   CDeviceCollect::AddDevice(CBaseDevice *pDevice, long nDeviceLinkType)
{
    insert(pair<long,CBaseDevice*>(nDeviceLinkType, pDevice));
}

long CDeviceCollect::GetNetlinkType(int devType)
{
    std::map<long, DEVINFO>::iterator itor = m_mapLinkType.find(devType);
    if (itor == m_mapLinkType.end())
    {
        return -1;
    }

    return itor->second.nNetLinkType;
}

CBaseDevice* AddA(CBaseDevice* pDevice,  CDeviceCollect *pDeviceCollect, long nDeviceLinkType, long nDataType)
{
    if(pDevice != NULL && pDeviceCollect != NULL)
    {
        pDevice->SetLinkType(nDeviceLinkType, nDataType);
        pDeviceCollect->AddDevice(pDevice,nDeviceLinkType);
        return pDevice;
    }
    else
    {
        if (pDevice != NULL)
        {
            delete pDevice;
            pDevice = NULL;
        }
        return NULL;
    }
}
