// InfoMrg.h: interface for the CInfoMrg class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INFOMRG_H__CA1413DC_F028_47A3_833F_6409923978A4__INCLUDED_)
#define AFX_INFOMRG_H__CA1413DC_F028_47A3_833F_6409923978A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Common.h"
#include <vector>
using namespace std;

//登陆设备信息管理类
class CDeviceList_DH
{
private:
    vector<SDeviceInfo_DH> m_DeviceArray;
public:
    CDeviceList_DH();
    virtual ~CDeviceList_DH();
    unsigned int GetNoUsedIndex();								//获取未使用(空)编号
    long AddLogInDevice(SDeviceInfo_DH &NewDevice); 	//添加新登陆设备
    long DeletLogOutDevice(long nID);					//以设备ID编号释放已注销设备
    long DeletLogOutDevice(char* strIP);				//以设备IP地址释放已注销设备
    SDeviceInfo_DH * SearchDevice(DWORD dwIP);			//以设备ID编号查找设备信息
    SDeviceInfo_DH * SearchDevice(const char* strIP);	//以设备IP地址查找设备信息
    SDeviceInfo_DH * SearchDeviceByAudioHandle(long hAudioHandle);
    SDeviceInfo_DH * SearchDeviceByLogInID(long nLogInID);
    SDeviceInfo_DH * GetDevice(long nIndex);			//获取指定编号设备信息
    long SelectLoginID(char* strIP);
    long GetArraySize();								//获取现有数组长度
    
    long AddLogInDevice(long nIndex, SDeviceInfo_DH &NewDevice);//添加新登陆设备到指定序号(测试用)
};
//数据通道信息管理类
class CChannelList_DH
{
private:
    vector<SChannelInfo_DH> m_ChannelArray;
public:
    CChannelList_DH();
    virtual ~CChannelList_DH();
    long GetNoUsedIndex();								//获取未使用(空)通道
    long AddPlayChannel(SChannelInfo_DH &NewChannel);	//添加新设备数据通道信息
    long DeletStopChannel(long nIndex); 				//以通道ID编号释放以空闲通道
    long DeletStopChannel(char* strIP, long nChID); 	//释放以空闲通道
    long DeletStopChannel(char* strIP); 				//释放某设备所有通道
    SChannelInfo_DH * SearchChannel(char* strIP, long nChID);//以设备IP地址查找通道信息
    SChannelInfo_DH * SearchChannel(HWND hWnd); 		//以播放窗体查找通道信息
    long GetChannelIndex(char* strIP, long nChID);		//以设备IP地址查找通道操作序号
    long GetChannelIndex(HWND hWnd);					//以播放窗体查找通道操作序号
    long GetChannelIndex(DWORD nIPv4, long nChID);      //查询操作序号
    SChannelInfo_DH * GetChannel(long nIndex);			//获取指定编号通道信息
    long FixHandle(long lRealHandle);					//播放句柄转换(封装前后)
    long GetArraySize();								//获取现有数组长度
    long CheckFormUsed(HWND hWnd);						//检查窗体是否使用中，0有、-1无
};

//回放录像文件信息管理类
class CBackFileList_DH
{
private:
    NET_RECORDFILE_INFO m_SelectFileList[ MAX_BACKFILE_INFO_LENGTH ];
public:
    long m_hDownloadHandle[MAX_BACKFILE_INFO_LENGTH];
    CBackFileList_DH();
    virtual ~CBackFileList_DH();
    long InitFileInfo(long nIndex); 					//初始化指定文件信息
    void CleanFileInfo();								//清空文件记录缓冲区
    long GetNoUsedIndex();								//获取为使用缓冲区编号
    long AddFileInfo(NET_RECORDFILE_INFO SelectFiles);	//添加查找到的文件信息
    long AddFileInfoX(NET_RECORDFILE_INFO *SelectFiles, long nFileCount);//添加查找到的文件信息数组(带清空)
    NET_RECORDFILE_INFO * SearchFileInfo(long nChID, char *strFileName, long *nIndex);//以文件名查找文件信息
    NET_RECORDFILE_INFO * GetFileInfo(long nIndex); 	//获取指定编号文件信息
};


#endif // !defined(AFX_INFOMRG_H__CA1413DC_F028_47A3_833F_6409923978A4__INCLUDED_)
