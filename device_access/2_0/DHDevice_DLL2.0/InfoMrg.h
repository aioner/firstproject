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

//��½�豸��Ϣ������
class CDeviceList_DH
{
private:
    vector<SDeviceInfo_DH> m_DeviceArray;
public:
    CDeviceList_DH();
    virtual ~CDeviceList_DH();
    unsigned int GetNoUsedIndex();								//��ȡδʹ��(��)���
    long AddLogInDevice(SDeviceInfo_DH &NewDevice); 	//����µ�½�豸
    long DeletLogOutDevice(long nID);					//���豸ID����ͷ���ע���豸
    long DeletLogOutDevice(char* strIP);				//���豸IP��ַ�ͷ���ע���豸
    SDeviceInfo_DH * SearchDevice(DWORD dwIP);			//���豸ID��Ų����豸��Ϣ
    SDeviceInfo_DH * SearchDevice(const char* strIP);	//���豸IP��ַ�����豸��Ϣ
    SDeviceInfo_DH * SearchDeviceByAudioHandle(long hAudioHandle);
    SDeviceInfo_DH * SearchDeviceByLogInID(long nLogInID);
    SDeviceInfo_DH * GetDevice(long nIndex);			//��ȡָ������豸��Ϣ
    long SelectLoginID(char* strIP);
    long GetArraySize();								//��ȡ�������鳤��
    
    long AddLogInDevice(long nIndex, SDeviceInfo_DH &NewDevice);//����µ�½�豸��ָ�����(������)
};
//����ͨ����Ϣ������
class CChannelList_DH
{
private:
    vector<SChannelInfo_DH> m_ChannelArray;
public:
    CChannelList_DH();
    virtual ~CChannelList_DH();
    long GetNoUsedIndex();								//��ȡδʹ��(��)ͨ��
    long AddPlayChannel(SChannelInfo_DH &NewChannel);	//������豸����ͨ����Ϣ
    long DeletStopChannel(long nIndex); 				//��ͨ��ID����ͷ��Կ���ͨ��
    long DeletStopChannel(char* strIP, long nChID); 	//�ͷ��Կ���ͨ��
    long DeletStopChannel(char* strIP); 				//�ͷ�ĳ�豸����ͨ��
    SChannelInfo_DH * SearchChannel(char* strIP, long nChID);//���豸IP��ַ����ͨ����Ϣ
    SChannelInfo_DH * SearchChannel(HWND hWnd); 		//�Բ��Ŵ������ͨ����Ϣ
    long GetChannelIndex(char* strIP, long nChID);		//���豸IP��ַ����ͨ���������
    long GetChannelIndex(HWND hWnd);					//�Բ��Ŵ������ͨ���������
    long GetChannelIndex(DWORD nIPv4, long nChID);      //��ѯ�������
    SChannelInfo_DH * GetChannel(long nIndex);			//��ȡָ�����ͨ����Ϣ
    long FixHandle(long lRealHandle);					//���ž��ת��(��װǰ��)
    long GetArraySize();								//��ȡ�������鳤��
    long CheckFormUsed(HWND hWnd);						//��鴰���Ƿ�ʹ���У�0�С�-1��
};

//�ط�¼���ļ���Ϣ������
class CBackFileList_DH
{
private:
    NET_RECORDFILE_INFO m_SelectFileList[ MAX_BACKFILE_INFO_LENGTH ];
public:
    long m_hDownloadHandle[MAX_BACKFILE_INFO_LENGTH];
    CBackFileList_DH();
    virtual ~CBackFileList_DH();
    long InitFileInfo(long nIndex); 					//��ʼ��ָ���ļ���Ϣ
    void CleanFileInfo();								//����ļ���¼������
    long GetNoUsedIndex();								//��ȡΪʹ�û��������
    long AddFileInfo(NET_RECORDFILE_INFO SelectFiles);	//��Ӳ��ҵ����ļ���Ϣ
    long AddFileInfoX(NET_RECORDFILE_INFO *SelectFiles, long nFileCount);//��Ӳ��ҵ����ļ���Ϣ����(�����)
    NET_RECORDFILE_INFO * SearchFileInfo(long nChID, char *strFileName, long *nIndex);//���ļ��������ļ���Ϣ
    NET_RECORDFILE_INFO * GetFileInfo(long nIndex); 	//��ȡָ������ļ���Ϣ
};


#endif // !defined(AFX_INFOMRG_H__CA1413DC_F028_47A3_833F_6409923978A4__INCLUDED_)
