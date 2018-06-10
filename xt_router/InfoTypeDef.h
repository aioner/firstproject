/*
>*FileName:InfoTypeDef.h
>*Des:���������Ϣ���Ͷ���
>*CtreateTime��2014-7-18
>*Auther��SongLei
*/
#ifndef INFOTYPEDEF_H
#define INFOTYPEDEF_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#define  VALUE_NA -2
#define  MAX_STR_SIZE 512

#ifndef	_WIN32
#define sscanf_s sscanf
#endif

// LT_TCP_TCP_PRI = 0,          //˽��tcp�Ự�Լ�˽��tcp��
// LT_TCP_UDP_PRI = 1,          //˽��tcp�Ự�Լ�˽��udp��
// LT_TCP_UDP_MULTI = 2,        //˽��tcp�Ự�Լ�˽��udp���ಥ
// LT_TCP_RTP_PRI = 3,          //˽��tcp�Ự�Լ�˽��rtp�����
// LT_TCP_RTP_MULTI = 4,        //˽��tcp�Ự�Լ�˽��rtp������ಥ
// LT_TCP_RTP_DEMUX_PRI = 5,    //˽��tcp�Ự�Լ�˽��rtp��϶˿ڸ�����
// LT_RTSP_RTP_STD = 6,         //��׼rtsp�Ự�Լ���׼rtp��
// LT_RTSP_RTP_DEMUX = 7,       //��׼rtsp�Ự�Լ�����rtp��
// LT_RTSP_RTP_PRI = 8          //��׼rtsp�Ự�Լ�˽��rtp��
namespace info_mgr {

    typedef unsigned short US_PROTOCOL_TYPE;
    const US_PROTOCOL_TYPE TCP_MODE      =  1;  //TCPģʽ
    const US_PROTOCOL_TYPE MUTICAST_MODE = 3;   //UDP�ಥģʽ
    const US_PROTOCOL_TYPE RTP_UDP       = 4;   //RTP����
    const US_PROTOCOL_TYPE RTP_MUL       = 5;   //RTP�ಥģʽ
    const US_PROTOCOL_TYPE RTP_DEMUX     = 6;   //RTP����
    const US_PROTOCOL_TYPE RTP_SID_RTSP  = 7;   //RSTP�ỰRTP��
    const US_PROTOCOL_TYPE RTP_DEMUX_RTSP = 9;  //RSTP�ỰRTP���ô� 
    const US_PROTOCOL_TYPE INVALID_MODE  = -1; //��Ч


    //�������
    typedef  unsigned short RESULT_TYPE;
    const RESULT_TYPE  RESULT_ERROR   = 0;  //����
    const RESULT_TYPE  RESULT_WARRING = 1;  //����
    const RESULT_TYPE  RESULT_INFO    = 2;  //��Ϣ
    const RESULT_TYPE  RESULT_NA      = -1; //��Ч

    //��Ϣ����
    typedef unsigned short INFO_LEVEL_TYPE;
    const INFO_LEVEL_TYPE INFO_LEVEL_FATAL   = 0;   //����
    const INFO_LEVEL_TYPE INFO_LEVEL_GENERAL = 1;   //һ��
    const INFO_LEVEL_TYPE INFO_LEVEL_HINT    = 2;   //��ʾ
    const INFO_LEVEL_TYPE INFO_LEVEL_NA      = -1;  //��Ч

    //<!--*********************************��Ϣ����****************************************-->//
    //��Ϣ���ͣ�111 - 555��
    typedef unsigned long INFO_TYPE;
    const INFO_TYPE INFO_NA                               = -1;      // ��Ч
    const INFO_TYPE INFO_PLAY_COUNT                       = 111;     // 111-�� �㲥��Ϣ��ͳ��
    const INFO_TYPE INFO_GLOBAL_COUNT                     = 112;     // 112-�� ȫ����Ϣͳ��
    //const INFO_TYPE INFO_LOGIN_CENTER_EVENT           = 113;       // 113-�� ��¼�����¼�
    //const INFO_TYPE INFO_LOGOUT_CENTER_EVENT          = 114;       // 114-�� ע�������¼�
    const INFO_TYPE INFO_SYS_TIME_CHENG_TIME              = 115;     // 115-�� ϵͳʱ��ı�ʱ��
    const INFO_TYPE INFO_SYS_START_TIME                   = 116;     // 116-�� ϵͳ������¼
    const INFO_TYPE INFO_SYS_EXIT_TIME                    = 117;     // 117-�� ϵͳ�˳���¼
    const INFO_TYPE INFO_LINK_STAT_EVENT                  = 118;     // 118-�� ��·״̬�¼�
    const INFO_TYPE INFO_TRANS_COUNT                      = 119;     // 119-:  ת����Ϣͳ��
    const INFO_TYPE INFO_CENTER_DB_COMMAND_EVENT          = 120;     // 120-:  �㲥�¼�  
    const INFO_TYPE INFO_LINK_SEVER_EVENT                 = 121;     // 121-:  LinkServer�¼�
    const INFO_TYPE INFO_RESPONSE_TO_CENTER_EVET_ID       = 122;     // 122-:  �����ķ����¼���Ϣ����
    const INFO_TYPE INFO_LOGIN_OR_LOGOUT_CENTER_EVNT_ID   = 123;     // 123-�� ��¼�ǳ������¼�����
    const INFO_TYPE INFO_SIGNALING_EXEC_RESULT_ID         = 124;     // 124-:  ����ִ�н��

    template <typename T>
    void AppenOsByCondition(std::ostringstream &os,
        const std::string strFieldName,
        const T& Value ,
        const bool bCondition)
    {
        if (bCondition)
        {
            os<<strFieldName<<"["<<Value<<"]"<<"  ";
        }
    }
    //��ʾ��Ϣ����
    typedef struct _struct_info_base
    {
    private:
        INFO_TYPE  m_ulInfoType;  //��Ϣ����
    public:
        RESULT_TYPE m_usResult;    //�������
        INFO_LEVEL_TYPE m_usInfoLevel; //��Ϣ����	
        _struct_info_base()
        {
            m_usResult = RESULT_NA;
            m_usInfoLevel = INFO_LEVEL_NA;
            m_ulInfoType = INFO_NA;
        }

        virtual INFO_TYPE GetInfoType()
        {
            return m_ulInfoType;
        }

        virtual std::string GetStr()=0;

    protected:
        virtual void SetInfoType(const INFO_TYPE& usInfoType)
        {
            m_ulInfoType = usInfoType;
        }

    }INFOBASE,*PINFOBASE;


    //ת����Ϣ�ṹ����
    typedef struct _struct_trans_info : INFOBASE
    {
        //Base_Info
        /////////////////////////////////////
        char m_pszCreateTime[MAX_STR_SIZE];//����ʱ��
        char m_pszSrcIDS[MAX_STR_SIZE];    //ԴIDS
        char m_pszSrcIP[MAX_STR_SIZE];     //ԴIP
        char m_pszDestIP[MAX_STR_SIZE];    //Ŀ��IP
        int srcno;
        long m_lChID;                     //����ͨ��
        long m_lDestPort;                 //Ŀ��˿�
        long m_lSendPort;                  //Դ�˿�
        int m_usProtocol;    //Э��  
        unsigned int m_uiSsrc;            //SSRC
        /////////////////////////////////////

        //RR_Info
        /////////////////////////////////////
        unsigned int    m_uiFractionLost;	  //������
        unsigned int    m_uiCumulativeLost;   //�ۼƶ���
        unsigned int    m_uiSequenceNumber;   //Sequence number that was received from the source specified by SSRC.
        unsigned int    m_uiJitter;	          //���綶��	
        unsigned int    m_uiLSR;			  //The middle 32 bits of the NTP timestamp received. 
        unsigned int    m_uiDlSR;			  //Delay since the last SR. 
		unsigned int    m_urtt;               //round trip time
        ///////////////////////////////////

        //SR_Info
        ///////////////////////////////////
        unsigned int    m_uiMNTPtimestamp;	 //Most significant 32bit of NTP timestamp 
        unsigned int    m_uiLNTPtimestamp;	 //Least significant 32bit of NTP timestamp
        unsigned int    m_uiTimestamp;		 //RTP timestamp
        unsigned int    m_uiPackets;         //�ۼƷ����� 
        unsigned int    m_uiOctets;          //�ۼƷ����ֽ���
        ////////////////////////////////

        bool			m_bDestMultiplex;	     // ����
        unsigned int	m_uiDestMultid;		     // ����id

        bool			m_bSendMultiplex;	     // ����
        unsigned int	m_uiSendMultid;		     // ����id

        virtual std::string GetStr()
        {
            std::ostringstream os;

            return os.str();
        }

        _struct_trans_info()
        {
            SetInfoType(INFO_PLAY_COUNT);
            clear();
        }

        void clear()
        {		
            ::memset(m_pszCreateTime,0,MAX_STR_SIZE);
            ::memset(m_pszSrcIDS,0,MAX_STR_SIZE);
            ::memset(m_pszDestIP,0,MAX_STR_SIZE);
            ::memset(m_pszSrcIP,0,MAX_STR_SIZE);
            srcno = -1;
            m_lChID = -1;
            m_lDestPort = -1;
            m_lSendPort=-1;
            m_usProtocol = INVALID_MODE;
            m_uiSsrc=-1;


            m_uiFractionLost = 0;
            m_uiCumulativeLost=0;
            m_uiSequenceNumber=0;
            m_uiJitter=0;
            m_uiLSR=0;
            m_uiDlSR=0;

            m_uiMNTPtimestamp=0;
            m_uiLNTPtimestamp=0;
            m_uiTimestamp=0;
            m_uiPackets=0;
            m_uiOctets=0;

            m_bDestMultiplex = false;
            m_uiDestMultid = 0;


            m_bSendMultiplex = false;
            m_uiSendMultid = 0;
        }

    }INFO_TRANS,*PINFO_TRANS;

    //��·״̬�¼�����
    typedef struct _struct_link_stat_event_info : INFOBASE
    {
        char m_strSrcIP[MAX_STR_SIZE];   //��·ԴIP
        char m_strDestIP[MAX_STR_SIZE];  //��·Ŀ��IP
        char m_strCfgTime[MAX_STR_SIZE]; //��·����ʱ��
        char m_strState[MAX_STR_SIZE];   //��·״̬
        char m_ChangeTime[MAX_STR_SIZE]; //״̬�ı�ʱ���

        _struct_link_stat_event_info()
        {
            SetInfoType(INFO_LINK_STAT_EVENT);
        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            os<<" ��·ԴIP:"<<m_strSrcIP
                <<" ��·Ŀ��IP:"<<m_strDestIP
                <<" ��·����ʱ��:"<<m_strCfgTime
                <<" ��·״̬:"<<m_strState
                <<" ״̬�ı�ʱ���:"<<m_ChangeTime; 
            return os.str();
        }
    }INFO_LINKSTATEVENT,*PINFO_LINKSTATEVENT;

    //�㲥��Ϣ�ṹ����
    typedef struct _struct_play_info : INFOBASE
    {
        std::string m_strServerIP;      //������IP
        std::string m_strServerMAC;     //������MAC
        std::string m_sDevIDS;        //�豸IDS
        long		m_lDevCh;			//�豸ͨ����
        long		m_lDevStrmTyp;		//�豸��������
        std::string m_sDBUrl;			//�㲥url
        long		m_lDBCh;			//�㲥ͨ��
        std::string m_strStartBiuldTime;//��ʼ����ʱ��
        std::string m_strBiuldTime;     //���ɹ�ʱ��
        long m_lPort;                   //�˿�
        long m_lState;                  //״̬	
        long m_lRCVByte;                //�����ֽ���������λKB/MB)
        long m_lRCVBytePS;              //ÿ������ֽ�������λkbps)
        long m_lLost;                   //��������
        long m_lDelay;                  //�ӳ�ʱ��
        US_PROTOCOL_TYPE m_usProtocol;  //Э��
        int m_iType;                    //����
        int m_Srcno;
        long m_Dev_handle;
        int			m_lStreamId;		//����ʶ
        int			m_iTracknum;		//������
        long         m_iTransChannel;    //ת������ͨ��
        long		m_lKey;				 //ϵͳͷ
        unsigned long m_lFrames;		 //����֡��
        int         m_iTransType;       //ת������
        int         m_iTransState;      //ת��״̬ 0�����ź� 1���ź��� 2���ź�ǿ 3���ź�һ��

        virtual std::string GetStr()
        {
            std::ostringstream os;

            return os.str();
        }

        _struct_play_info()
        {
            m_usProtocol = INVALID_MODE;
            m_lPort =-1;
            m_lState =-1;
            m_lRCVByte =0;
            m_lRCVBytePS =0;
            m_lDevCh=0;
            m_lDevStrmTyp=0;
            m_lDBCh=0;
            m_lLost = 0;
            m_lDelay = 0;
            m_iType = -1;
            m_lStreamId = -1;
            m_iTracknum = 0;
            m_lKey = 0;
            m_lFrames = 0;
            m_iTransChannel= -1;
            m_iTransType = -1;
            m_iTransState = -1;
            m_Srcno=-1;
            m_Dev_handle=-1;
            SetInfoType(INFO_PLAY_COUNT);
        }

    }INFO_PLAY,*PINFO_PLAY;

    //ȫ����Ϣͳ�ƽṹ����
    typedef struct _struct_global_info : INFOBASE
    {
        std::string m_strNetCardIP;       //����IP
        std::string m_strMAC;             //����MAC
        std::string m_strGatewayIP;       //�������ص�ַ
        std::string m_strNetCardMask;     //��������
        std::string m_strNetCardDNS;      //����DNS
        unsigned long m_ulCurJoinNum;     //��ǰ��������
        unsigned long m_ulMaxJoinNum;     //����������
        unsigned long m_ulCurTransNum;    //��ǰת������
        unsigned long m_ulMaxTransNum;    //���ת������
        unsigned long m_ulRcvNum;         //����ָ��������
        unsigned long m_ulPlayCount;      //�㲥�ܴ���
        unsigned long m_ulClientCount;    //�ͻ��˷����ܴ���
        unsigned long m_ulErrorCount;     //��������
        unsigned long m_ulWaringCount;    //��������
        double m_dMeanBandwidth;          //ƽ���������(MB)
        double m_dMaxBandWidethPS;        //ÿ������������
        double m_dMinBandWidethPS;        //ÿ������������
        double m_dMeanMeM;                //ƽ���ڴ�ռ��
        double m_dMaxMemPS;               //ÿ������ڴ�ռ��
        double m_dMinMemPS;               //ÿ������ڴ�ռ��
        double m_dMeanCPU;                //ƽ��CPUռ��
        double m_dMaxCPUPS;               //ÿ�����CPUռ��

        virtual std::string GetStr()
        {
            std::ostringstream os;

            return os.str();
        }

        _struct_global_info()
        {
            SetInfoType(INFO_GLOBAL_COUNT);

            m_ulCurJoinNum = 0;     
            m_ulMaxJoinNum = 0;     
            m_ulCurTransNum = 0;    
            m_ulMaxTransNum = 0;   
            m_ulRcvNum = 0;        
            m_ulPlayCount = 0;    
            m_ulClientCount = 0;    
            m_ulErrorCount = 0;     
            m_ulWaringCount = 0;    
            m_dMeanBandwidth = 0;         
            m_dMaxBandWidethPS = 0;        
            m_dMinBandWidethPS = 0;        
            m_dMeanMeM = 0;               
            m_dMaxMemPS = 0;              
            m_dMinMemPS = 0;               
            m_dMeanCPU = 0;                
            m_dMaxCPUPS = 0;             
        }

    }IFNO_GLOBAL,*PINFO_GLOBAL;

    //�¼�
    //////////////////////////////////////////////////////////////////////////

    //����ִ�н���ṹ
    typedef struct _struct_signaling_exec_result_info :INFOBASE
    {
        std::string     m_strTime;              //ִ��ʱ��
        std::string     m_strDevIds;            //�豸IDS
        std::string		m_strDbIP;			    //�㲥IP
        std::string     m_strCtrlName;          //��������
        std::string     m_strNewIDS;            //��IDS
        long            m_lNewChid;             //��ͨ����
        long            m_lCtrlID;              //����ָ��ID
        long		    m_lDevChianId;			//�豸ͨ����
        long		    m_lDevStrmType;		    //�豸��������
        long            m_lTransChId;           //ת������ͨ��		
        long		    m_lDbChanid;			//�㲥ͨ��
        long		    m_lDbType;			    //�㲥����
        long            m_lLinkType;            //��������
        long            m_lServerType;          //����������ģʽ��Ĭ��Ϊ��9��
        long            m_lExecRet;             //ִ�н����С��0-��ʧ�ܣ�

        _struct_signaling_exec_result_info()
        {
            SetInfoType(INFO_SIGNALING_EXEC_RESULT_ID);

            m_lNewChid = VALUE_NA;
            m_lCtrlID = VALUE_NA;
            m_lDevChianId = VALUE_NA;
            m_lDevStrmType = VALUE_NA;
            m_lTransChId = VALUE_NA;
            m_lDbChanid = VALUE_NA;
            m_lDbType = VALUE_NA;
            m_lLinkType = VALUE_NA;
            m_lServerType = VALUE_NA;
            m_lExecRet = VALUE_NA;
        }

        std::string GetStr()
        {
            std::ostringstream os;
            AppenOsByCondition(os,"ִ��ʱ��",m_strTime,!m_strTime.empty());
            AppenOsByCondition(os,"��������",m_strCtrlName,!m_strCtrlName.empty());		
            AppenOsByCondition(os,"�㲥IP",m_strDbIP,!m_strDbIP.empty());
            AppenOsByCondition(os,"�豸IDS",m_strDevIds,!m_strDevIds.empty());		
            AppenOsByCondition(os,"����ָ��ID",m_lCtrlID,(VALUE_NA != m_lCtrlID));
            AppenOsByCondition(os,"�豸ͨ����",m_lDevChianId,(VALUE_NA != m_lDevChianId));
            AppenOsByCondition(os,"�豸��������",m_lDevStrmType,(VALUE_NA != m_lDevStrmType));
            AppenOsByCondition(os,"ת������ͨ��",m_lTransChId,(VALUE_NA != m_lTransChId));
            AppenOsByCondition(os,"�㲥ͨ��",m_lDbChanid,(VALUE_NA != m_lDbChanid));
            AppenOsByCondition(os,"�㲥����",m_lDbType, (VALUE_NA != m_lDbType));
            AppenOsByCondition(os,"��������",m_lLinkType,(VALUE_NA != m_lLinkType));
            AppenOsByCondition(os,"����������ģʽ",m_lServerType,(VALUE_NA != m_lServerType));
            AppenOsByCondition(os,"ִ�н��",m_lExecRet,(VALUE_NA != m_lExecRet));
            return os.str();
        }

    }INFO_SIGNALINGEXECRESULT,*PINFO_SIGNALINGEXECRESULT;


    //�����ķ����¼���Ϣ�ṹ
    typedef struct _struct_response_to_center : INFOBASE
    {
        std::string     m_strTime;              //����ʱ��
        std::string     m_strDevIds;            //�豸IDS
        std::string		m_strDbIP;			    //�㲥IP
        long		    m_lDevChianId;			//�豸ͨ����
        long		    m_lDevStrmType;		    //�豸��������
        long            m_lTransChId;           //ת������ͨ��		
        long		    m_lDbChanid;			//�㲥ͨ��
        long		    m_lDbType;			    //�㲥����
        long            m_lLinkType;            //��������
        long            m_lServerType;          //����������ģʽ��Ĭ��Ϊ��9��
        long            m_lCtrlID;              //����ָ��ID
        _struct_response_to_center()
        {
            SetInfoType(INFO_RESPONSE_TO_CENTER_EVET_ID);
            m_lDevChianId   = VALUE_NA;
            m_lDevStrmType  = VALUE_NA;
            m_lTransChId    = VALUE_NA;
            m_lDbChanid     = VALUE_NA;
            m_lDbType       = VALUE_NA;
            m_lLinkType     = VALUE_NA;
            m_lServerType   = VALUE_NA;
            m_lCtrlID       = VALUE_NA;

        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            AppenOsByCondition(os,"����ʱ��",m_strTime,!m_strTime.empty());
            AppenOsByCondition(os,"�㲥IP",m_strDbIP,!m_strDbIP.empty());
            AppenOsByCondition(os,"�豸IDS",m_strDevIds,!m_strDevIds.empty());		
            AppenOsByCondition(os,"�豸ͨ����",m_lDevChianId,( VALUE_NA != m_lDevChianId));
            AppenOsByCondition(os,"�豸��������",m_lDevStrmType,( VALUE_NA != m_lDevStrmType));
            AppenOsByCondition(os,"ת������ͨ��",m_lTransChId,( VALUE_NA != m_lTransChId));
            AppenOsByCondition(os,"�㲥ͨ��",m_lDbType,( VALUE_NA != m_lDbType));
            AppenOsByCondition(os,"�㲥����",m_lDevChianId,( VALUE_NA != m_lDevChianId));
            AppenOsByCondition(os,"��������",m_lLinkType,( VALUE_NA != m_lLinkType));
            AppenOsByCondition(os,"����������ģʽ",m_lServerType,( VALUE_NA != m_lServerType));
            AppenOsByCondition(os,"����ָ��ID",m_lCtrlID,( VALUE_NA != m_lCtrlID));
            return os.str();
        }

    }INFO_RESPONSETOCENTER,*PINFO_RESPONSETOCENTER;


    //LinkServer�¼��ṹ
    typedef struct _struct_link_server_event_info : INFOBASE
    {
        std::string m_strTime; //����ʱ��
        long m_lsNum;
        long m_lbz;


        virtual std::string GetStr()
        {
            std::ostringstream os;
            AppenOsByCondition(os,"����ʱ��",m_strTime,!m_strTime.empty());
            AppenOsByCondition(os,"m_lsNum",m_lsNum,( VALUE_NA != m_lsNum));
            AppenOsByCondition(os,"m_lbz",m_lbz,( VALUE_NA != m_lbz));
            return os.str();
        }


        _struct_link_server_event_info()
        {
            SetInfoType(INFO_LINK_SEVER_EVENT);
            m_lsNum = VALUE_NA;
            m_lbz = VALUE_NA;
        }

    }INFO_LINKSERVEREVENT,*PINFO_LINKSERVEREVENT;

    //�㲥�¼���Ϣ
    typedef struct _struct_center_db_command_event_info : INFOBASE
    {
        long m_lCtrl;				// ����ָ��		
        long m_lDevChid;			// �豸ͨ����
        long m_lDevStrmtype;	    // �豸��������
        long m_lDbChanid ;			// �㲥ͨ��	
        long m_lDevType ;		    // �豸����
        long m_lChanid;		        // ָ��ת������ͨ����	
        long m_lDevChidNew;		    // �豸��ͨ����

        std::string m_DevIDS;		// �豸IDS
        std::string m_DevIDSNew;	// �豸��IDS
        std::string m_DbIp;			// �㲥IP 
        std::string m_strTime;      // �¼�����ʱ��
        std::string m_strDes;       //�¼�����

        virtual std::string GetStr()
        {
            std::ostringstream os;
            AppenOsByCondition(os,"�¼�����ʱ��",m_strTime,!m_strTime.empty());
            AppenOsByCondition(os,"��������",m_strDes,!m_strDes.empty());		
            AppenOsByCondition(os,"����ָ��",m_lCtrl,( VALUE_NA != m_lCtrl));
            AppenOsByCondition(os,"�㲥IP",m_DbIp,!m_DbIp.empty());
            AppenOsByCondition(os,"�豸IDS",m_DevIDS,!m_DevIDS.empty());
            AppenOsByCondition(os,"�豸��IDS",m_DevIDSNew,!m_DevIDSNew.empty());
            AppenOsByCondition(os,"�豸��ͨ����",m_lDevChidNew,( VALUE_NA != m_lDevChidNew));
            AppenOsByCondition(os,"ָ��ת������ͨ����",m_lChanid,( VALUE_NA != m_lChanid));
            AppenOsByCondition(os,"�豸����",m_lDevType,( VALUE_NA != m_lDevType));
            AppenOsByCondition(os,"�㲥ͨ��",m_lDbChanid,( VALUE_NA != m_lDbChanid));
            AppenOsByCondition(os,"�豸��������",m_lDevStrmtype,( VALUE_NA != m_lDevStrmtype));
            AppenOsByCondition(os,"�豸ͨ����",m_lDevChid,( VALUE_NA != m_lDevChid));
            return os.str();
        }

        _struct_center_db_command_event_info()
        {
            SetInfoType(INFO_CENTER_DB_COMMAND_EVENT);
            m_lCtrl = VALUE_NA;
            m_lDevChid = VALUE_NA;
            m_lDevStrmtype = VALUE_NA;
            m_lDbChanid = VALUE_NA;
            m_lDevType = VALUE_NA;
            m_lChanid = VALUE_NA;
            m_lDevChidNew = VALUE_NA;
        }
    }INFO_CENTERDBCOMMANDEVENT,*PINFO_CENTERDBCOMMANDEVENT;

    //��¼�ǳ������¼��ṹ
    typedef struct _struct_login_or_logout_center_event_info : INFOBASE
    {
        std::string m_strTime;   //����ʱ��
        std::string m_strIDS;    //IDS
        std::string m_strName;   //����
        std::string m_strIPS;    //IP
        std::string m_strRes1;
        std::string m_strRes2;
        long m_lType;
        long m_lFlag;
        long m_lRes1;
        long m_lRes2;

        virtual std::string GetStr()
        {
            std::ostringstream os;
            AppenOsByCondition(os,"����ʱ��",m_strTime,!m_strTime.empty());
            AppenOsByCondition(os,"IP",m_strIPS,!m_strIPS.empty());
            AppenOsByCondition(os,"IDS",m_strIDS,!m_strIDS.empty());		
            AppenOsByCondition(os,"m_strRes1",m_strRes1,!m_strRes1.empty());
            AppenOsByCondition(os,"m_strRes2",m_strRes2,!m_strRes2.empty());
            AppenOsByCondition(os,"m_lType",m_lType,( VALUE_NA != m_lType));
            AppenOsByCondition(os,"m_lFlag",m_lFlag,( VALUE_NA != m_lFlag));
            AppenOsByCondition(os,"m_lRes1",m_lRes1,( VALUE_NA != m_lRes1));
            AppenOsByCondition(os,"m_lRes2",m_lRes2,( VALUE_NA != m_lRes2));
            return os.str();
        }

        _struct_login_or_logout_center_event_info()
        {
            SetInfoType(INFO_LOGIN_OR_LOGOUT_CENTER_EVNT_ID);
            m_lType = VALUE_NA;
            m_lFlag = VALUE_NA;
            m_lRes1 = VALUE_NA;
            m_lRes2 = VALUE_NA;
        }

    }INFO_LOGINORLOGOUTCENTEREVNT,*PINFO_LOGINORLOGOUTCENTEREVNT;

    //ϵͳʱ��ı�ʱ��ṹ����
    typedef struct _struct_sys_time_cheng_time_info : INFOBASE
    {
        std::string m_strOldTime;    //ԭʱ��
        std::string m_NewTime;       //���ĺ�ʱ��

        _struct_sys_time_cheng_time_info()
        {
            SetInfoType(INFO_SYS_TIME_CHENG_TIME);
        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            os<<" ԭʱ��:"<<m_strOldTime
                <<" ���ĺ�ʱ��:"<<m_NewTime;

            return os.str();
        }


    }INFO_SYSTIMECHENGTIME,*PINFO_SYSTIMECHENGTIME;

    //ϵͳ������¼�ṹ����
    typedef struct _struct_sys_start_time_info : INFOBASE
    {
        std::string m_strTime;  //ʱ��
        std::string m_strCause; //ԭ��
        _struct_sys_start_time_info()
        {
            SetInfoType(INFO_SYS_START_TIME);
        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            os<<" ʱ��:"<<m_strTime
                <<" ԭ��:"<<m_strCause;

            return os.str();
        }

    }INFO_SYSSTARTTIME,*PINFO_SYSSTARTTIME;


    //ϵͳ�˳���¼�ṹ����
    typedef struct _struct_sys_exit_time_info :INFOBASE
    {
        std::string m_strTime;  //ʱ��
        std::string m_strCause; //ԭ��

        _struct_sys_exit_time_info()
        {
            SetInfoType(INFO_SYS_EXIT_TIME);
        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            os<<" ʱ��:"<<m_strTime
                <<" ԭ��:"<<m_strCause;
            return os.str();
        }
    }INFO_SYSEIXTTIME,*PINFO_SYSEIXTTIME;

    // <!--*********************************��������****************************************-->//
    // <!--
    // �������ͣ�666-999��
    // 666-�������������;���
    // 667-��ת���������;���
    // 668-��ϵͳ�ڴ���;���
    // 669-������ռ�ù��߾���
    // 670-��CPUռ�ù��߾���
    // 671-�������ʹ��߾���
    // 672-����ʱ���߾���
    // 673-����ת���ͻ��˾���
    // -->

    // <Info Type="666" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>�����������;���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SrcIDS>ԴIDS</SrcIDS>
    // <SrcChID>ͨ��</SrcChID>
    // <SrcPort>Դ�˿�</SrcPort>
    // <SrcType>����</SrcType>
    // <Protocol>Э��</Protocol>
    // <CurRcvBytPS>��ǰ������ֽ�����</CurRcvBytPS>
    // <WaringTime>�澯ʱ��</WaringTime>       
    // </Info>
    // 
    // <Info Type="667" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>ת���������;���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SinkIP>�ͻ���IP</SinkIP>
    // <SinkPort>�ͻ��˶˿�</SinkPort>
    // <Protocol>Э��</Protocol>
    // <CurRcvBytPS>��ǰ������ֽ�����</CurRcvBytPS>
    // <WaringTime>�澯ʱ��</WaringTime>       
    // </Info>
    // 
    // <Info Type="668" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>ϵͳ�ڴ���;���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <PhysicalMem>ϵͳ�����ڴ�</PhysicalMem>
    // <UsableMem>ϵͳ�����ڴ�</UsableMem>
    // <VirtualMem>ϵͳ�����ڴ�ʹ����</VirtualMem>
    // <CurRcvBytPS>����ʹ���ڴ���</CurRcvBytPS>
    // <WaringTime>�澯ʱ��</WaringTime>       
    // </Info>  
    // 
    // <Info Type="669" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>����ռ�ù��߾���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <NetCrad>��������</NetCrad>
    // <NetType>��������</NetType>
    // <ServiceRate>���絥λʱ����ƽ��ռ����</ServiceRate>
    // <WaringTime>�澯ʱ��</WaringTime>       
    // </Info>
    // 
    // <Info Type="670" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>CPUռ�ù��߾���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <CPU>����������</CPU>
    // <ServiceRate>CPU��λʱ����ƽ��ռ����</ServiceRate>
    // <ServiceRateOfRouter>������λʱ����CPUƽ��ռ����</ServiceRateOfRouter>
    // <WaringTime>�澯ʱ��</WaringTime>       
    // </Info>
    // 
    // <Info Type="671" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>�����ʹ��߾���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SrcIDS>ԴIDS</SrcIDS>
    // <SrcChID>ͨ��</SrcChID>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcPort>Դ�˿�</SrcPort>
    // <SrcType>����</SrcType>
    // <SrcProtocol>Э��</SrcProtocol>
    // <SinkIP>�ͻ���IP</SinkIP>
    // <SinkPort>�ͻ��˶˿�</SinkPort>
    // <SinkProtocol>Э������</SinkProtocol>
    // <TransTime>ת����ʼʱ��</TransTime>
    // <Lost>��������</Lost>
    // <LostPS>��λʱ���ڶ�����</LostPS>
    // <WaringTime>�澯ʱ��</WaringTime>       
    // </Info>
    // 
    // <Info Type="672" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>��ʱ���߾���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SrcIDS>ԴIDS</SrcIDS>
    // <SrcChID>ͨ��</SrcChID>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcPort>Դ�˿�</SrcPort>
    // <SrcType>����</SrcType>
    // <SrcProtocol>Э��</SrcProtocol>
    // <SinkIP>�ͻ���IP</SinkIP>
    // <SinkPort>�ͻ��˶˿�</SinkPort>
    // <SinkProtocol>Э������</SinkProtocol>
    // <TransTime>ת����ʼʱ��</TransTime>
    // <Delay>ƽ����ʱʱ�䣨ms��</Delay>
    // <DelayPS>��λʱ������ʱʱ�䣨ms��</DelayPS>
    // <WaringTime>�澯ʱ��</WaringTime>       
    // </Info>
    // 
    // <Info Type="673" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>��ת���ͻ��˾���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SrcIDS>ԴIDS</SrcIDS>
    // <SrcChID>ͨ��</SrcChID>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcPort>Դ�˿�</SrcPort>
    // <SrcType>����</SrcType>
    // <SrcProtocol>Э��</SrcProtocol>
    // <TransPort>ת���˿�</TransPort>
    // <WaringTime>�澯ʱ��</WaringTime>       
    // </Info>


    // <!--*********************************��������****************************************-->//
    // <!--
    // �������ͣ�1111-9999��
    // 1111-: ��������ʧ�ܴ���
    // 1112-: ����ת��ʧ�ܴ���
    // 1113-: ���뻺�����������
    // 1114-: ת���������������
    // 1115-: �����������
    // 1116-: �����쳣�Ͽ�����
    // 1117-: ת���쳣�Ͽ�����
    // -->

    // <Info Type="1111" Level="2" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>��������ʧ�ܴ���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SrcIDS>ԴIDS</SrcIDS>
    // <SrcChID>ͨ��</SrcChID>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcPort>Դ�˿�</SrcPort>
    // <SrcType>����</SrcType>
    // <SrcProtocol>Э��</SrcProtocol>
    // <StartTime>����ʱ��</StartTime>
    // <Cause>ʧ��ԭ��</Cause>       
    // </Info>
    // 
    // <Info Type="1112" Level="2" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>����ת��ʧ�ܴ���</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SinkIP>�ͻ���IP</SinkIP>
    // <SinkPort>�ͻ��˶˿�</SinkPort>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcProtocol>Э������</SrcProtocol>
    // <StartTime>����ʱ��</StartTime>
    // <Cause>ʧ��ԭ��</Cause>       
    // </Info>
    // 
    // <Info Type="1113" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>���뻺�����������</Des> 
    // <ServerIP>������IP</ServerIP>
    // <SinkIP>�ͻ���IP</SinkIP>
    // <SinkPort>�ͻ��˶˿�</SinkPort>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcProtocol>Э������</SrcProtocol>
    // <Time>���ʱ��</Time>
    // <BufferSize>��ǰ��������С</BufferSize>
    // <BufferName>����������</BufferName>      
    // </Info>
    // 
    // <Info Type="1114" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>ת���������������</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SrcIDS>ԴIDS</SrcIDS>
    // <SrcChID>ͨ��</SrcChID>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcPort>Դ�˿�</SrcPort>
    // <SrcType>����</SrcType>
    // <SrcProtocol>Э��</SrcProtocol>
    // <Time>���ʱ��</Time>
    // <BufferSize>��ǰ��������С</BufferSize>
    // <BufferName>����������</BufferName>      
    // </Info>
    // 
    // <Info Type="1115" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>�����������</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <Mode>�������ģ��</Mode>
    // <Time>�������ʱ��</Time>
    // <DumpPath>�������dump�ļ�·��</DumpPath>   
    // </Info>
    // 
    // <Info Type="1116" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>�����쳣�Ͽ�����</Des> 
    // <ServerIP>������IP</ServerIP>
    // <ServerMac>������MAC</ServerMac>
    // <SrcIDS>ԴIDS</SrcIDS>
    // <SrcChID>ͨ��</SrcChID>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcPort>Դ�˿�</SrcPort>
    // <SrcType>����</SrcType>
    // <SrcProtocol>Э��</SrcProtocol>
    // <CreateTime>����ʱ��</CreateTime>
    // <BreakTime>�Ͽ�ʱ��</BreakTime>
    // <Cause>�Ͽ�ԭ��</Cause>      
    // </Info>
    // 
    // <Info Type="1117" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>���뻺�����������</Des> 
    // <ServerIP>������IP</ServerIP>
    // <SinkIP>�ͻ���IP</SinkIP>
    // <SinkPort>�ͻ��˶˿�</SinkPort>
    // <SrcIP>ԴIP</SrcIP>
    // <SrcProtocol>Э������</SrcProtocol>
    // <CreateTime>����ʱ��</CreateTime>
    // <BreakTime>�Ͽ�ʱ��</BreakTime>
    // <Cause>�Ͽ�ԭ��</Cause>       
    // </Info>

    //��������
    ///////////////////////////////////////////////////////////

    typedef boost::posix_time::ptime InfoTimeT;

    //%04d-%02d-%02d %02d:%02d:%02d:%d
    inline std::string ToStrMicrosecByPtime(const InfoTimeT& t)
    {
        std::string strTime = to_iso_extended_string(t);
        std::string::size_type uiPos = strTime.find("T");
        if (uiPos != std::string::npos)
        {
            strTime.replace(uiPos,1,std::string(" "));
        }

        uiPos = strTime.find(".");
        if (uiPos != std::string::npos)
        {
            strTime.replace(uiPos,1,std::string(":"));
        }

        return strTime;
    }

    //%04d-%02d-%02d %02d:%02d:%02d
    inline std::string ToStrSecondByPtime(const InfoTimeT& t)
    {
        std::string strTime = to_iso_extended_string(t);
        std::string::size_type uiPos = strTime.find("T");
        if (uiPos >= 0)
        {
            strTime.replace(uiPos,1,std::string(" "));
        }

        return strTime;
    }

    //��ȡ��ʱʱ��microsec_��
    inline InfoTimeT GetCurTimeMicrosecValue()
    {
        return boost::posix_time::microsec_clock::local_time();
    }

    //��ȡ��ʱʱ��Second_��
    inline InfoTimeT GetCurTimeSecondValue()
    {
        return boost::posix_time::second_clock::local_time();
    }

    //��ȡ��ǰʱ���ַ���
    inline std::string GetCurTime()
    {
        //��ȡ����ʱ��
        return ToStrMicrosecByPtime(GetCurTimeMicrosecValue());
    }

    //Convert string to unix timestamp by wluo
    inline time_t ToUnixTimestamp(const char *datatime)
    {
        struct tm tm_time;
        time_t unixtime = 0;
        int  year , month , day , hour , minute , second;
        sscanf_s(datatime , "%d-%d-%d %d:%d:%d", &year,&month,&day,&hour,&minute,&second);
        tm_time.tm_year  = year - 1900;
        tm_time.tm_mon   = month - 1;
        tm_time.tm_mday  = day;
        tm_time.tm_hour  = hour;
        tm_time.tm_min   = minute;
        tm_time.tm_sec   = second;
        tm_time.tm_isdst = 0;

        unixtime = mktime(&tm_time);
        return unixtime;
    }

    inline struct tm ToTMTime(const char *datatime) //wluo
    {
        struct tm tm_time;
        int  year , month , day , hour , minute , second;
        sscanf_s(datatime , "%d-%d-%d %d:%d:%d", &year,&month,&day,&hour,&minute,&second);
        tm_time.tm_year  = year - 1900;
        tm_time.tm_mon   = month - 1;
        tm_time.tm_mday  = day;
        tm_time.tm_hour  = hour;
        tm_time.tm_min   = minute;
        tm_time.tm_sec   = second;
        tm_time.tm_isdst = 0;

        return tm_time;
    }


    //��������תstring
    template <typename T>
    inline std::string Type2Str(T& Src)
    {
        std::ostringstream os;
        os<<Src;
        return os.str();
    }

    //ת������ָ��Ϊ�������
    template<typename T>
    inline T B2C(INFOBASE* pBase)
    {

        T* p = NULL;
        if (p = dynamic_cast<T*>(pBase))
        {
            return *p;

        }
        else
        {
            T t1;
            return t1;
        }
    }
    ///////////////////////////////////////////////////////////

} //End namespace info_mgr

#endif //INFOTYPEDEF_H