// InfoMrg.cpp: implementation of the CInfoMrg class.
//
//////////////////////////////////////////////////////////////////////
#include "InfoMrg.h"
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

//////////////////////////////////////////////////////////////////////
// CDeviceList_DH 登陆设备信息管理类

CDeviceList_DH::CDeviceList_DH()
{
    //m_DeviceArray.SetSize(0,MAX_LOGON_AMOUNT);
}

CDeviceList_DH::~CDeviceList_DH()
{
}

/*获取未使用(空)编号*/
unsigned int CDeviceList_DH::GetNoUsedIndex()
{
   unsigned i;
   for(i=0; i<(m_DeviceArray.size()); i++)
    {
        if(m_DeviceArray[i].m_nID == -1)
            break;
    }
    return i;
}

/*要加入的设备信息(结构)*/
long CDeviceList_DH::AddLogInDevice(SDeviceInfo_DH &NewDevice)
{
    unsigned int nIndex = GetNoUsedIndex();
	if(nIndex >= m_DeviceArray.size()){
		NewDevice.lLoginHandle = nIndex;//add 8/11
		m_DeviceArray.push_back(NewDevice);
	}
    else
        m_DeviceArray[nIndex] = NewDevice;
    return nIndex;
}
/*测试用*/
long CDeviceList_DH::AddLogInDevice(long nIndex, SDeviceInfo_DH &NewDevice)
{
    if(nIndex >= (long)m_DeviceArray.size())
        m_DeviceArray.push_back(NewDevice);
    else
        m_DeviceArray[nIndex] = NewDevice;
    return nIndex;
}

/*以设备ID编号释放已注销设备*/
long CDeviceList_DH::DeletLogOutDevice(long nID)
{
    SDeviceInfo_DH * sDeleteDevice = GetDevice(nID);
    if(NULL == sDeleteDevice)
        return -1;
    StructInit(*sDeleteDevice);
    return 0;
}

/*以设备IP地址释放已注销设备*/
long CDeviceList_DH::DeletLogOutDevice(char* strIP)
{
    SDeviceInfo_DH * sDeleteDevice = SearchDevice(strIP);
    if(NULL == sDeleteDevice)
        return -1;
    StructInit(*sDeleteDevice);
    return 0;
}

/*以设备ID编号查找设备信息*/
SDeviceInfo_DH * CDeviceList_DH::SearchDevice(DWORD dwIP)
{
    for(unsigned int i=0; i<(m_DeviceArray.size()); i++)
    {
        if(m_DeviceArray[i].m_dwIP == dwIP)
            return &m_DeviceArray[i];
    }
    return NULL;
}

/*以设备IP地址查找设备信息*/
SDeviceInfo_DH * CDeviceList_DH::SearchDevice(const char* strIP)
{
    DWORD dwIP = inet_addr(strIP);
    for(unsigned int i=0; i<(m_DeviceArray.size()); i++)
    {
        if(m_DeviceArray[i].m_dwIP == dwIP)
            return &m_DeviceArray[i];
    }
    return NULL;
}

/**/
SDeviceInfo_DH * CDeviceList_DH::SearchDeviceByAudioHandle(long hAudioHandle)
{
    for(unsigned int i=0; i<(m_DeviceArray.size()); i++)
    {
        if(m_DeviceArray[i].m_nAudioHandle == hAudioHandle)
            return &m_DeviceArray[i];
    }
    return NULL;
}

//
SDeviceInfo_DH * CDeviceList_DH::SearchDeviceByLogInID(long nLogInID)
{
    for(unsigned int i=0; i<(m_DeviceArray.size()); i++)
    {
        if(m_DeviceArray[i].lLoginHandle == nLogInID) //m_nHandle
            return &m_DeviceArray[i];
    }
    return NULL;
}

/*获取指定编号设备信息*/
SDeviceInfo_DH * CDeviceList_DH::GetDevice(long nIndex)
{
    if(nIndex>=0 && nIndex<(long)(m_DeviceArray.size()))
    {
        if(0 != m_DeviceArray[nIndex].m_dwIP)
            return &m_DeviceArray[nIndex];
    }
    return NULL;
}

long CDeviceList_DH::SelectLoginID(char* strIP)
{
    DWORD dwIP = inet_addr(strIP);
    for(unsigned int i=0; i<(m_DeviceArray.size()); i++)
    {
        if(m_DeviceArray[i].m_dwIP == dwIP)
            return i;
    }
    return -1;
}

/*获取数组现有长度*/
long CDeviceList_DH::GetArraySize()
{
    return m_DeviceArray.size();
}


//////////////////////////////////////////////////////////////////////
// CChannelList_DH 播放通道信息管理类
CChannelList_DH::CChannelList_DH()
{
    //m_ChannelArray.SetSize(0,MAX_CHANNEL_AMOUNT);
}

CChannelList_DH::~CChannelList_DH()
{
}

/*获取未使用(空)通道*/
long CChannelList_DH::GetNoUsedIndex()
{
    unsigned int i;
    for(i=0; i<(m_ChannelArray.size()); i++)
    {
        if(m_ChannelArray[i].m_nID == -1)
            break;
    }
    return i;
}

/*添加新设备播放通道信息*/
long CChannelList_DH::AddPlayChannel(SChannelInfo_DH &NewChannel)
{
    long nIndex = GetNoUsedIndex();
    if(nIndex >= (long)m_ChannelArray.size())
        m_ChannelArray.push_back(NewChannel);
    else
        m_ChannelArray[nIndex] = NewChannel;
    return nIndex;
}

/*以通道Index编号释放以空闲通道*/
long CChannelList_DH::DeletStopChannel(long nIndex)
{
    SChannelInfo_DH * sDeleteChannel = GetChannel(nIndex);
    if(NULL == sDeleteChannel)
        return -1;
    StructInit(*sDeleteChannel);
    return 0;
}
/*以指定设备通道释放以空闲通道*/
long CChannelList_DH::DeletStopChannel(char* strIP, long nChID)
{
    SChannelInfo_DH * sDeleteChannel = SearchChannel(strIP, nChID);
    if(NULL == sDeleteChannel)
        return -1;
    StructInit(*sDeleteChannel);
    return 0;
}

/*释放某设备所有通道*/
long CChannelList_DH::DeletStopChannel(char* strIP)
{
    DWORD dwIP = inet_addr(strIP);
    for(unsigned int i=0; i<(m_ChannelArray.size()); i++)
    {
        if(m_ChannelArray[i].m_dwIP == dwIP)
            StructInit(m_ChannelArray[i]);
    }
    return 0;
}

/*以指定设备通道查找通道信息*/
SChannelInfo_DH * CChannelList_DH::SearchChannel(char* strIP,long nChID)
{
    DWORD dwIP = inet_addr(strIP);
    for(unsigned int i=0; i<(m_ChannelArray.size()); i++)
    {
        if(m_ChannelArray[i].m_nID == nChID && m_ChannelArray[i].m_dwIP == dwIP)
            return &m_ChannelArray[i];
    }
    return NULL;
}

//以播放窗体查找通道信息
SChannelInfo_DH * CChannelList_DH::SearchChannel(HWND hWnd)
{
   for(unsigned int i=0; i<(m_ChannelArray.size()); i++)
    {
        if(m_ChannelArray[i].m_hWnd == hWnd)
            return &m_ChannelArray[i];
    }
    return NULL;
}

//以设备IP地址查找通道操作序号
long CChannelList_DH::GetChannelIndex(char* strIP, long nChID)
{
    DWORD dwIP = inet_addr(strIP);
    for(unsigned int i=0; i<(m_ChannelArray.size()); i++)
    {
        if(m_ChannelArray[i].m_nID == nChID && m_ChannelArray[i].m_dwIP == dwIP)
            return i;
    }
    return -1;
}
//以播放窗体查找通道操作序号
long CChannelList_DH::GetChannelIndex(HWND hWnd)
{
    for(unsigned int i=0; i<(m_ChannelArray.size()); i++)
    {
        if(m_ChannelArray[i].m_hWnd == hWnd)
            return i;
    }
    return -1;
}
//
long CChannelList_DH::GetChannelIndex(DWORD nIPv4, long nChID)
{
    for(unsigned int i=0; i<(m_ChannelArray.size()); i++)
    {
        if(m_ChannelArray[i].m_dwIP == nIPv4 && m_ChannelArray[i].m_nID == nChID)
            return i;
    }
    return -1;
}

/*以通道Index编号获取通道信息*/
SChannelInfo_DH * CChannelList_DH::GetChannel(long nIndex)
{
    if(nIndex>=0 && nIndex<(long)(m_ChannelArray.size()))
    {
        if(0 != m_ChannelArray[nIndex].m_dwIP)
            return &m_ChannelArray[nIndex];
    }
    return NULL;
}

/*播放句柄转换(封装前后)*/
long CChannelList_DH::FixHandle(long lRealHandle)
{
    for(unsigned int i=0; i<(m_ChannelArray.size()); i++)
    {
        //TRACE("cbHandle:%d infHandle:%d\n", lRealHandle, m_ChannelArray[i].m_nHandle);
        if(m_ChannelArray[i].m_nHandle == lRealHandle)
            return i;
    }
    return -1;
}

/*获取数组现有长度*/
long CChannelList_DH::GetArraySize()
{
    return m_ChannelArray.size();
}

/*检查窗体是否使用中*/
long CChannelList_DH::CheckFormUsed(HWND hWnd)
{
    for(unsigned int i=0; i<m_ChannelArray.size(); i++)
    {
        if(m_ChannelArray[i].m_hWnd == hWnd && hWnd != NULL)
            return i;
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
// CBackFileList_DH 回放录像文件信息管理类
CBackFileList_DH::CBackFileList_DH()
{
    CleanFileInfo();
}

CBackFileList_DH::~CBackFileList_DH()
{
}

/*初始化指定文件信息*/
long CBackFileList_DH::InitFileInfo(long nIndex)
{
    if(0<nIndex && MAX_BACKFILE_INFO_LENGTH>nIndex)
    {
        m_SelectFileList[nIndex].ch = 0;
        strcpy(m_SelectFileList[nIndex].filename,"");
        m_SelectFileList[nIndex].size = 0;
        m_SelectFileList[nIndex].starttime.dwYear = 0;
        m_SelectFileList[nIndex].starttime.dwMonth = 0;
        m_SelectFileList[nIndex].starttime.dwDay = 0;
        m_SelectFileList[nIndex].starttime.dwHour = 0;
        m_SelectFileList[nIndex].starttime.dwMinute = 0;
        m_SelectFileList[nIndex].starttime.dwSecond = 0;
        m_SelectFileList[nIndex].endtime.dwYear = 0;
        m_SelectFileList[nIndex].endtime.dwMonth = 0;
        m_SelectFileList[nIndex].endtime.dwDay = 0;
        m_SelectFileList[nIndex].endtime.dwHour = 0;
        m_SelectFileList[nIndex].endtime.dwMinute = 0;
        m_SelectFileList[nIndex].endtime.dwSecond = 0;
        m_SelectFileList[nIndex].driveno = 0;
        m_SelectFileList[nIndex].startcluster = 0;
        //        m_hDownloadHandle[nIndex] = 0;
        return 0;
    }
    return -1;
}

/*清空文件记录缓冲区*/
void CBackFileList_DH::CleanFileInfo()
{
    for(long i=0; i<MAX_BACKFILE_INFO_LENGTH; i++)
    {
        InitFileInfo(i);
    }
}

/*获取为使用缓冲区编号*/
long CBackFileList_DH::GetNoUsedIndex()
{
    for(long i=0; i<MAX_BACKFILE_INFO_LENGTH; i++)
    {
        if(m_SelectFileList[i].starttime.dwYear != 0 && m_SelectFileList[i].endtime.dwYear != 0)
            return i;
    }
    return -1;
}

long CBackFileList_DH::AddFileInfo(NET_RECORDFILE_INFO SelectFiles)
{
    long nIndex = GetNoUsedIndex();
    if(-1 == nIndex)
        return -1;
    m_SelectFileList[nIndex] = SelectFiles;
    return nIndex;
}

/*添加查找到的文件信息数组(带清空)*/
long CBackFileList_DH::AddFileInfoX(NET_RECORDFILE_INFO *SelectFiles, long nFileCount)
{
    if(nFileCount>MAX_BACKFILE_INFO_LENGTH)
        return -1;
    CleanFileInfo();
    for(long i=0; i<nFileCount; i++)
    {
        m_SelectFileList[i] = SelectFiles[i];
    }
    return 0;
}

/*以文件名查找文件信息*/
NET_RECORDFILE_INFO * CBackFileList_DH::SearchFileInfo(long nChID, char *strFileName, long *nIndex)
{
    for(long i=0; i<MAX_BACKFILE_INFO_LENGTH; i++)
    {
        if(m_SelectFileList[i].ch==(DWORD)nChID && strcmp(m_SelectFileList[i].filename,strFileName)==0)
        {
            *nIndex = i;
            return &m_SelectFileList[i];
        }
    }
    return NULL;
}

/*获取指定编号文件信息*/
NET_RECORDFILE_INFO * CBackFileList_DH::GetFileInfo(long nIndex)
{
    if(nIndex>=0 && nIndex<MAX_BACKFILE_INFO_LENGTH)
        return &m_SelectFileList[nIndex];
    else
        return NULL;
}
