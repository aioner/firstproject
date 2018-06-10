/*
>*FileName:InfoTypeDef.h
>*Des:交换输出信息类型定义
>*CtreateTime：2014-7-18
>*Auther：SongLei
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

// LT_TCP_TCP_PRI = 0,          //私有tcp会话以及私有tcp流
// LT_TCP_UDP_PRI = 1,          //私有tcp会话以及私有udp流
// LT_TCP_UDP_MULTI = 2,        //私有tcp会话以及私有udp流多播
// LT_TCP_RTP_PRI = 3,          //私有tcp会话以及私有rtp混合流
// LT_TCP_RTP_MULTI = 4,        //私有tcp会话以及私有rtp混合流多播
// LT_TCP_RTP_DEMUX_PRI = 5,    //私有tcp会话以及私有rtp混合端口复用流
// LT_RTSP_RTP_STD = 6,         //标准rtsp会话以及标准rtp流
// LT_RTSP_RTP_DEMUX = 7,       //标准rtsp会话以及复用rtp流
// LT_RTSP_RTP_PRI = 8          //标准rtsp会话以及私有rtp流
namespace info_mgr {

    typedef unsigned short US_PROTOCOL_TYPE;
    const US_PROTOCOL_TYPE TCP_MODE      =  1;  //TCP模式
    const US_PROTOCOL_TYPE MUTICAST_MODE = 3;   //UDP多播模式
    const US_PROTOCOL_TYPE RTP_UDP       = 4;   //RTP单播
    const US_PROTOCOL_TYPE RTP_MUL       = 5;   //RTP多播模式
    const US_PROTOCOL_TYPE RTP_DEMUX     = 6;   //RTP复用
    const US_PROTOCOL_TYPE RTP_SID_RTSP  = 7;   //RSTP会话RTP传
    const US_PROTOCOL_TYPE RTP_DEMUX_RTSP = 9;  //RSTP会话RTP复用传 
    const US_PROTOCOL_TYPE INVALID_MODE  = -1; //无效


    //结果类型
    typedef  unsigned short RESULT_TYPE;
    const RESULT_TYPE  RESULT_ERROR   = 0;  //错误
    const RESULT_TYPE  RESULT_WARRING = 1;  //警告
    const RESULT_TYPE  RESULT_INFO    = 2;  //信息
    const RESULT_TYPE  RESULT_NA      = -1; //无效

    //信息级别
    typedef unsigned short INFO_LEVEL_TYPE;
    const INFO_LEVEL_TYPE INFO_LEVEL_FATAL   = 0;   //严重
    const INFO_LEVEL_TYPE INFO_LEVEL_GENERAL = 1;   //一般
    const INFO_LEVEL_TYPE INFO_LEVEL_HINT    = 2;   //提示
    const INFO_LEVEL_TYPE INFO_LEVEL_NA      = -1;  //无效

    //<!--*********************************信息类结果****************************************-->//
    //信息类型（111 - 555）
    typedef unsigned long INFO_TYPE;
    const INFO_TYPE INFO_NA                               = -1;      // 无效
    const INFO_TYPE INFO_PLAY_COUNT                       = 111;     // 111-： 点播信息及统计
    const INFO_TYPE INFO_GLOBAL_COUNT                     = 112;     // 112-： 全局信息统计
    //const INFO_TYPE INFO_LOGIN_CENTER_EVENT           = 113;       // 113-： 登录中心事件
    //const INFO_TYPE INFO_LOGOUT_CENTER_EVENT          = 114;       // 114-： 注销中心事件
    const INFO_TYPE INFO_SYS_TIME_CHENG_TIME              = 115;     // 115-： 系统时间改变时间
    const INFO_TYPE INFO_SYS_START_TIME                   = 116;     // 116-： 系统启动记录
    const INFO_TYPE INFO_SYS_EXIT_TIME                    = 117;     // 117-： 系统退出记录
    const INFO_TYPE INFO_LINK_STAT_EVENT                  = 118;     // 118-： 线路状态事件
    const INFO_TYPE INFO_TRANS_COUNT                      = 119;     // 119-:  转发信息统计
    const INFO_TYPE INFO_CENTER_DB_COMMAND_EVENT          = 120;     // 120-:  点播事件  
    const INFO_TYPE INFO_LINK_SEVER_EVENT                 = 121;     // 121-:  LinkServer事件
    const INFO_TYPE INFO_RESPONSE_TO_CENTER_EVET_ID       = 122;     // 122-:  向中心反馈事件信息类型
    const INFO_TYPE INFO_LOGIN_OR_LOGOUT_CENTER_EVNT_ID   = 123;     // 123-： 登录登出中心事件类型
    const INFO_TYPE INFO_SIGNALING_EXEC_RESULT_ID         = 124;     // 124-:  信令执行结果

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
    //显示信息基类
    typedef struct _struct_info_base
    {
    private:
        INFO_TYPE  m_ulInfoType;  //信息类型
    public:
        RESULT_TYPE m_usResult;    //结果类型
        INFO_LEVEL_TYPE m_usInfoLevel; //信息级别	
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


    //转发信息结构类型
    typedef struct _struct_trans_info : INFOBASE
    {
        //Base_Info
        /////////////////////////////////////
        char m_pszCreateTime[MAX_STR_SIZE];//创建时间
        char m_pszSrcIDS[MAX_STR_SIZE];    //源IDS
        char m_pszSrcIP[MAX_STR_SIZE];     //源IP
        char m_pszDestIP[MAX_STR_SIZE];    //目标IP
        int srcno;
        long m_lChID;                     //服务通道
        long m_lDestPort;                 //目标端口
        long m_lSendPort;                  //源端口
        int m_usProtocol;    //协议  
        unsigned int m_uiSsrc;            //SSRC
        /////////////////////////////////////

        //RR_Info
        /////////////////////////////////////
        unsigned int    m_uiFractionLost;	  //丢包率
        unsigned int    m_uiCumulativeLost;   //累计丢包
        unsigned int    m_uiSequenceNumber;   //Sequence number that was received from the source specified by SSRC.
        unsigned int    m_uiJitter;	          //网络抖动	
        unsigned int    m_uiLSR;			  //The middle 32 bits of the NTP timestamp received. 
        unsigned int    m_uiDlSR;			  //Delay since the last SR. 
		unsigned int    m_urtt;               //round trip time
        ///////////////////////////////////

        //SR_Info
        ///////////////////////////////////
        unsigned int    m_uiMNTPtimestamp;	 //Most significant 32bit of NTP timestamp 
        unsigned int    m_uiLNTPtimestamp;	 //Least significant 32bit of NTP timestamp
        unsigned int    m_uiTimestamp;		 //RTP timestamp
        unsigned int    m_uiPackets;         //累计发包量 
        unsigned int    m_uiOctets;          //累计发送字节数
        ////////////////////////////////

        bool			m_bDestMultiplex;	     // 复用
        unsigned int	m_uiDestMultid;		     // 复用id

        bool			m_bSendMultiplex;	     // 复用
        unsigned int	m_uiSendMultid;		     // 复用id

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

    //线路状态事件类型
    typedef struct _struct_link_stat_event_info : INFOBASE
    {
        char m_strSrcIP[MAX_STR_SIZE];   //链路源IP
        char m_strDestIP[MAX_STR_SIZE];  //链路目的IP
        char m_strCfgTime[MAX_STR_SIZE]; //链路配置时间
        char m_strState[MAX_STR_SIZE];   //链路状态
        char m_ChangeTime[MAX_STR_SIZE]; //状态改变时间点

        _struct_link_stat_event_info()
        {
            SetInfoType(INFO_LINK_STAT_EVENT);
        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            os<<" 链路源IP:"<<m_strSrcIP
                <<" 链路目的IP:"<<m_strDestIP
                <<" 链路配置时间:"<<m_strCfgTime
                <<" 链路状态:"<<m_strState
                <<" 状态改变时间点:"<<m_ChangeTime; 
            return os.str();
        }
    }INFO_LINKSTATEVENT,*PINFO_LINKSTATEVENT;

    //点播信息结构类型
    typedef struct _struct_play_info : INFOBASE
    {
        std::string m_strServerIP;      //服务器IP
        std::string m_strServerMAC;     //服务器MAC
        std::string m_sDevIDS;        //设备IDS
        long		m_lDevCh;			//设备通道号
        long		m_lDevStrmTyp;		//设备码流类型
        std::string m_sDBUrl;			//点播url
        long		m_lDBCh;			//点播通道
        std::string m_strStartBiuldTime;//开始建立时间
        std::string m_strBiuldTime;     //立成功时间
        long m_lPort;                   //端口
        long m_lState;                  //状态	
        long m_lRCVByte;                //接收字节总数（单位KB/MB)
        long m_lRCVBytePS;              //每秒接收字节数（单位kbps)
        long m_lLost;                   //丢包总数
        long m_lDelay;                  //延迟时间
        US_PROTOCOL_TYPE m_usProtocol;  //协议
        int m_iType;                    //类型
        int m_Srcno;
        long m_Dev_handle;
        int			m_lStreamId;		//流标识
        int			m_iTracknum;		//流数量
        long         m_iTransChannel;    //转发服务通道
        long		m_lKey;				 //系统头
        unsigned long m_lFrames;		 //接收帧数
        int         m_iTransType;       //转发类型
        int         m_iTransState;      //转发状态 0：无信号 1：信号弱 2：信号强 3：信号一般

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

    //全局信息统计结构类型
    typedef struct _struct_global_info : INFOBASE
    {
        std::string m_strNetCardIP;       //网卡IP
        std::string m_strMAC;             //网卡MAC
        std::string m_strGatewayIP;       //网卡网关地址
        std::string m_strNetCardMask;     //网卡掩码
        std::string m_strNetCardDNS;      //网卡DNS
        unsigned long m_ulCurJoinNum;     //当前接入数量
        unsigned long m_ulMaxJoinNum;     //最大接入数量
        unsigned long m_ulCurTransNum;    //当前转发数量
        unsigned long m_ulMaxTransNum;    //最大转发数量
        unsigned long m_ulRcvNum;         //接收指定总条数
        unsigned long m_ulPlayCount;      //点播总次数
        unsigned long m_ulClientCount;    //客户端访问总次数
        unsigned long m_ulErrorCount;     //错误总数
        unsigned long m_ulWaringCount;    //警告总数
        double m_dMeanBandwidth;          //平均网络带宽(MB)
        double m_dMaxBandWidethPS;        //每秒最高网络带宽
        double m_dMinBandWidethPS;        //每秒最低网络带宽
        double m_dMeanMeM;                //平均内存占用
        double m_dMaxMemPS;               //每秒最高内存占用
        double m_dMinMemPS;               //每秒最低内存占用
        double m_dMeanCPU;                //平均CPU占用
        double m_dMaxCPUPS;               //每秒最高CPU占用

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

    //事件
    //////////////////////////////////////////////////////////////////////////

    //信令执行结果结构
    typedef struct _struct_signaling_exec_result_info :INFOBASE
    {
        std::string     m_strTime;              //执行时间
        std::string     m_strDevIds;            //设备IDS
        std::string		m_strDbIP;			    //点播IP
        std::string     m_strCtrlName;          //操作名称
        std::string     m_strNewIDS;            //新IDS
        long            m_lNewChid;             //新通道号
        long            m_lCtrlID;              //操作指令ID
        long		    m_lDevChianId;			//设备通道号
        long		    m_lDevStrmType;		    //设备码流类型
        long            m_lTransChId;           //转发服务通道		
        long		    m_lDbChanid;			//点播通道
        long		    m_lDbType;			    //点播类型
        long            m_lLinkType;            //连接类型
        long            m_lServerType;          //交换服务器模式（默认为：9）
        long            m_lExecRet;             //执行结果（小于0-：失败）

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
            AppenOsByCondition(os,"执行时间",m_strTime,!m_strTime.empty());
            AppenOsByCondition(os,"操作名称",m_strCtrlName,!m_strCtrlName.empty());		
            AppenOsByCondition(os,"点播IP",m_strDbIP,!m_strDbIP.empty());
            AppenOsByCondition(os,"设备IDS",m_strDevIds,!m_strDevIds.empty());		
            AppenOsByCondition(os,"操作指令ID",m_lCtrlID,(VALUE_NA != m_lCtrlID));
            AppenOsByCondition(os,"设备通道号",m_lDevChianId,(VALUE_NA != m_lDevChianId));
            AppenOsByCondition(os,"设备码流类型",m_lDevStrmType,(VALUE_NA != m_lDevStrmType));
            AppenOsByCondition(os,"转发服务通道",m_lTransChId,(VALUE_NA != m_lTransChId));
            AppenOsByCondition(os,"点播通道",m_lDbChanid,(VALUE_NA != m_lDbChanid));
            AppenOsByCondition(os,"点播类型",m_lDbType, (VALUE_NA != m_lDbType));
            AppenOsByCondition(os,"连接类型",m_lLinkType,(VALUE_NA != m_lLinkType));
            AppenOsByCondition(os,"交换服务器模式",m_lServerType,(VALUE_NA != m_lServerType));
            AppenOsByCondition(os,"执行结果",m_lExecRet,(VALUE_NA != m_lExecRet));
            return os.str();
        }

    }INFO_SIGNALINGEXECRESULT,*PINFO_SIGNALINGEXECRESULT;


    //向中心反馈事件信息结构
    typedef struct _struct_response_to_center : INFOBASE
    {
        std::string     m_strTime;              //反馈时间
        std::string     m_strDevIds;            //设备IDS
        std::string		m_strDbIP;			    //点播IP
        long		    m_lDevChianId;			//设备通道号
        long		    m_lDevStrmType;		    //设备码流类型
        long            m_lTransChId;           //转发服务通道		
        long		    m_lDbChanid;			//点播通道
        long		    m_lDbType;			    //点播类型
        long            m_lLinkType;            //连接类型
        long            m_lServerType;          //交换服务器模式（默认为：9）
        long            m_lCtrlID;              //操作指令ID
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
            AppenOsByCondition(os,"反馈时间",m_strTime,!m_strTime.empty());
            AppenOsByCondition(os,"点播IP",m_strDbIP,!m_strDbIP.empty());
            AppenOsByCondition(os,"设备IDS",m_strDevIds,!m_strDevIds.empty());		
            AppenOsByCondition(os,"设备通道号",m_lDevChianId,( VALUE_NA != m_lDevChianId));
            AppenOsByCondition(os,"设备码流类型",m_lDevStrmType,( VALUE_NA != m_lDevStrmType));
            AppenOsByCondition(os,"转发服务通道",m_lTransChId,( VALUE_NA != m_lTransChId));
            AppenOsByCondition(os,"点播通道",m_lDbType,( VALUE_NA != m_lDbType));
            AppenOsByCondition(os,"点播类型",m_lDevChianId,( VALUE_NA != m_lDevChianId));
            AppenOsByCondition(os,"连接类型",m_lLinkType,( VALUE_NA != m_lLinkType));
            AppenOsByCondition(os,"交换服务器模式",m_lServerType,( VALUE_NA != m_lServerType));
            AppenOsByCondition(os,"操作指令ID",m_lCtrlID,( VALUE_NA != m_lCtrlID));
            return os.str();
        }

    }INFO_RESPONSETOCENTER,*PINFO_RESPONSETOCENTER;


    //LinkServer事件结构
    typedef struct _struct_link_server_event_info : INFOBASE
    {
        std::string m_strTime; //产生时间
        long m_lsNum;
        long m_lbz;


        virtual std::string GetStr()
        {
            std::ostringstream os;
            AppenOsByCondition(os,"产生时间",m_strTime,!m_strTime.empty());
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

    //点播事件信息
    typedef struct _struct_center_db_command_event_info : INFOBASE
    {
        long m_lCtrl;				// 中心指令		
        long m_lDevChid;			// 设备通道号
        long m_lDevStrmtype;	    // 设备码流类型
        long m_lDbChanid ;			// 点播通道	
        long m_lDevType ;		    // 设备类型
        long m_lChanid;		        // 指定转发服务通道号	
        long m_lDevChidNew;		    // 设备新通道号

        std::string m_DevIDS;		// 设备IDS
        std::string m_DevIDSNew;	// 设备新IDS
        std::string m_DbIp;			// 点播IP 
        std::string m_strTime;      // 事件产生时间
        std::string m_strDes;       //事件描述

        virtual std::string GetStr()
        {
            std::ostringstream os;
            AppenOsByCondition(os,"事件产生时间",m_strTime,!m_strTime.empty());
            AppenOsByCondition(os,"命令名称",m_strDes,!m_strDes.empty());		
            AppenOsByCondition(os,"中心指令",m_lCtrl,( VALUE_NA != m_lCtrl));
            AppenOsByCondition(os,"点播IP",m_DbIp,!m_DbIp.empty());
            AppenOsByCondition(os,"设备IDS",m_DevIDS,!m_DevIDS.empty());
            AppenOsByCondition(os,"设备新IDS",m_DevIDSNew,!m_DevIDSNew.empty());
            AppenOsByCondition(os,"设备新通道号",m_lDevChidNew,( VALUE_NA != m_lDevChidNew));
            AppenOsByCondition(os,"指定转发服务通道号",m_lChanid,( VALUE_NA != m_lChanid));
            AppenOsByCondition(os,"设备类型",m_lDevType,( VALUE_NA != m_lDevType));
            AppenOsByCondition(os,"点播通道",m_lDbChanid,( VALUE_NA != m_lDbChanid));
            AppenOsByCondition(os,"设备码流类型",m_lDevStrmtype,( VALUE_NA != m_lDevStrmtype));
            AppenOsByCondition(os,"设备通道号",m_lDevChid,( VALUE_NA != m_lDevChid));
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

    //登录登出中心事件结构
    typedef struct _struct_login_or_logout_center_event_info : INFOBASE
    {
        std::string m_strTime;   //发生时间
        std::string m_strIDS;    //IDS
        std::string m_strName;   //名称
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
            AppenOsByCondition(os,"发生时间",m_strTime,!m_strTime.empty());
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

    //系统时间改变时间结构类型
    typedef struct _struct_sys_time_cheng_time_info : INFOBASE
    {
        std::string m_strOldTime;    //原时间
        std::string m_NewTime;       //更改后时间

        _struct_sys_time_cheng_time_info()
        {
            SetInfoType(INFO_SYS_TIME_CHENG_TIME);
        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            os<<" 原时间:"<<m_strOldTime
                <<" 更改后时间:"<<m_NewTime;

            return os.str();
        }


    }INFO_SYSTIMECHENGTIME,*PINFO_SYSTIMECHENGTIME;

    //系统启动记录结构类型
    typedef struct _struct_sys_start_time_info : INFOBASE
    {
        std::string m_strTime;  //时间
        std::string m_strCause; //原因
        _struct_sys_start_time_info()
        {
            SetInfoType(INFO_SYS_START_TIME);
        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            os<<" 时间:"<<m_strTime
                <<" 原因:"<<m_strCause;

            return os.str();
        }

    }INFO_SYSSTARTTIME,*PINFO_SYSSTARTTIME;


    //系统退出记录结构类型
    typedef struct _struct_sys_exit_time_info :INFOBASE
    {
        std::string m_strTime;  //时间
        std::string m_strCause; //原因

        _struct_sys_exit_time_info()
        {
            SetInfoType(INFO_SYS_EXIT_TIME);
        }

        virtual std::string GetStr()
        {
            std::ostringstream os;
            os<<" 时间:"<<m_strTime
                <<" 原因:"<<m_strCause;
            return os.str();
        }
    }INFO_SYSEIXTTIME,*PINFO_SYSEIXTTIME;

    // <!--*********************************警告类结果****************************************-->//
    // <!--
    // 警告类型（666-999）
    // 666-：输入流量过低警告
    // 667-：转发流量过低警告
    // 668-：系统内存过低警告
    // 669-：网络占用过高警告
    // 670-：CPU占用过高警告
    // 671-：丢包率过高警告
    // 672-：延时过高警告
    // 673-：无转发客户端警告
    // -->

    // <Info Type="666" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>输入流量过低警告</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SrcIDS>源IDS</SrcIDS>
    // <SrcChID>通道</SrcChID>
    // <SrcPort>源端口</SrcPort>
    // <SrcType>类型</SrcType>
    // <Protocol>协议</Protocol>
    // <CurRcvBytPS>当前秒接收字节总数</CurRcvBytPS>
    // <WaringTime>告警时间</WaringTime>       
    // </Info>
    // 
    // <Info Type="667" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>转发流量过低警告</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SinkIP>客户端IP</SinkIP>
    // <SinkPort>客户端端口</SinkPort>
    // <Protocol>协议</Protocol>
    // <CurRcvBytPS>当前秒接收字节总数</CurRcvBytPS>
    // <WaringTime>告警时间</WaringTime>       
    // </Info>
    // 
    // <Info Type="668" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>系统内存过低警告</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <PhysicalMem>系统物理内存</PhysicalMem>
    // <UsableMem>系统可用内存</UsableMem>
    // <VirtualMem>系统虚拟内存使用数</VirtualMem>
    // <CurRcvBytPS>交换使用内存数</CurRcvBytPS>
    // <WaringTime>告警时间</WaringTime>       
    // </Info>  
    // 
    // <Info Type="669" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>网络占用过高警告</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <NetCrad>网卡名称</NetCrad>
    // <NetType>网卡类型</NetType>
    // <ServiceRate>网络单位时间内平均占用率</ServiceRate>
    // <WaringTime>告警时间</WaringTime>       
    // </Info>
    // 
    // <Info Type="670" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>CPU占用过高警告</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <CPU>处理器名称</CPU>
    // <ServiceRate>CPU单位时间内平均占用率</ServiceRate>
    // <ServiceRateOfRouter>交换单位时间内CPU平均占用率</ServiceRateOfRouter>
    // <WaringTime>告警时间</WaringTime>       
    // </Info>
    // 
    // <Info Type="671" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>丢包率过高警告</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SrcIDS>源IDS</SrcIDS>
    // <SrcChID>通道</SrcChID>
    // <SrcIP>源IP</SrcIP>
    // <SrcPort>源端口</SrcPort>
    // <SrcType>类型</SrcType>
    // <SrcProtocol>协议</SrcProtocol>
    // <SinkIP>客户端IP</SinkIP>
    // <SinkPort>客户端端口</SinkPort>
    // <SinkProtocol>协议类型</SinkProtocol>
    // <TransTime>转发开始时间</TransTime>
    // <Lost>丢包总数</Lost>
    // <LostPS>单位时间内丢包数</LostPS>
    // <WaringTime>告警时间</WaringTime>       
    // </Info>
    // 
    // <Info Type="672" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>延时过高警告</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SrcIDS>源IDS</SrcIDS>
    // <SrcChID>通道</SrcChID>
    // <SrcIP>源IP</SrcIP>
    // <SrcPort>源端口</SrcPort>
    // <SrcType>类型</SrcType>
    // <SrcProtocol>协议</SrcProtocol>
    // <SinkIP>客户端IP</SinkIP>
    // <SinkPort>客户端端口</SinkPort>
    // <SinkProtocol>协议类型</SinkProtocol>
    // <TransTime>转发开始时间</TransTime>
    // <Delay>平均延时时间（ms）</Delay>
    // <DelayPS>单位时间内延时时间（ms）</DelayPS>
    // <WaringTime>告警时间</WaringTime>       
    // </Info>
    // 
    // <Info Type="673" Level="3" Time="yy-mm-dd-hh-mm-ss-msms">       
    // <Des>无转发客户端警告</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SrcIDS>源IDS</SrcIDS>
    // <SrcChID>通道</SrcChID>
    // <SrcIP>源IP</SrcIP>
    // <SrcPort>源端口</SrcPort>
    // <SrcType>类型</SrcType>
    // <SrcProtocol>协议</SrcProtocol>
    // <TransPort>转发端口</TransPort>
    // <WaringTime>告警时间</WaringTime>       
    // </Info>


    // <!--*********************************错误类结果****************************************-->//
    // <!--
    // 错误类型（1111-9999）
    // 1111-: 建立连接失败错误
    // 1112-: 建立转发失败错误
    // 1113-: 输入缓冲区溢出错误
    // 1114-: 转发缓冲区溢出错误
    // 1115-: 程序崩溃错误
    // 1116-: 连接异常断开错误
    // 1117-: 转发异常断开错误
    // -->

    // <Info Type="1111" Level="2" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>建立连接失败错误</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SrcIDS>源IDS</SrcIDS>
    // <SrcChID>通道</SrcChID>
    // <SrcIP>源IP</SrcIP>
    // <SrcPort>源端口</SrcPort>
    // <SrcType>类型</SrcType>
    // <SrcProtocol>协议</SrcProtocol>
    // <StartTime>建立时间</StartTime>
    // <Cause>失败原因</Cause>       
    // </Info>
    // 
    // <Info Type="1112" Level="2" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>建立转发失败错误</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SinkIP>客户端IP</SinkIP>
    // <SinkPort>客户端端口</SinkPort>
    // <SrcIP>源IP</SrcIP>
    // <SrcProtocol>协议类型</SrcProtocol>
    // <StartTime>建立时间</StartTime>
    // <Cause>失败原因</Cause>       
    // </Info>
    // 
    // <Info Type="1113" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>输入缓冲区溢出错误</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <SinkIP>客户端IP</SinkIP>
    // <SinkPort>客户端端口</SinkPort>
    // <SrcIP>源IP</SrcIP>
    // <SrcProtocol>协议类型</SrcProtocol>
    // <Time>溢出时间</Time>
    // <BufferSize>当前缓冲区大小</BufferSize>
    // <BufferName>缓冲区名称</BufferName>      
    // </Info>
    // 
    // <Info Type="1114" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>转发缓冲区溢出错误</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SrcIDS>源IDS</SrcIDS>
    // <SrcChID>通道</SrcChID>
    // <SrcIP>源IP</SrcIP>
    // <SrcPort>源端口</SrcPort>
    // <SrcType>类型</SrcType>
    // <SrcProtocol>协议</SrcProtocol>
    // <Time>溢出时间</Time>
    // <BufferSize>当前缓冲区大小</BufferSize>
    // <BufferName>缓冲区名称</BufferName>      
    // </Info>
    // 
    // <Info Type="1115" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>程序崩溃错误</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <Mode>程序崩溃模块</Mode>
    // <Time>程序崩溃时间</Time>
    // <DumpPath>程序崩溃dump文件路径</DumpPath>   
    // </Info>
    // 
    // <Info Type="1116" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>连接异常断开错误</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <ServerMac>服务器MAC</ServerMac>
    // <SrcIDS>源IDS</SrcIDS>
    // <SrcChID>通道</SrcChID>
    // <SrcIP>源IP</SrcIP>
    // <SrcPort>源端口</SrcPort>
    // <SrcType>类型</SrcType>
    // <SrcProtocol>协议</SrcProtocol>
    // <CreateTime>建立时间</CreateTime>
    // <BreakTime>断开时间</BreakTime>
    // <Cause>断开原因</Cause>      
    // </Info>
    // 
    // <Info Type="1117" Level="1" Time="yy-mm-dd-hh-mm-ss-msms">
    // <Des>输入缓冲区溢出错误</Des> 
    // <ServerIP>服务器IP</ServerIP>
    // <SinkIP>客户端IP</SinkIP>
    // <SinkPort>客户端端口</SinkPort>
    // <SrcIP>源IP</SrcIP>
    // <SrcProtocol>协议类型</SrcProtocol>
    // <CreateTime>建立时间</CreateTime>
    // <BreakTime>断开时间</BreakTime>
    // <Cause>断开原因</Cause>       
    // </Info>

    //公共函数
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

    //获取当时时间microsec_级
    inline InfoTimeT GetCurTimeMicrosecValue()
    {
        return boost::posix_time::microsec_clock::local_time();
    }

    //获取当时时间Second_级
    inline InfoTimeT GetCurTimeSecondValue()
    {
        return boost::posix_time::second_clock::local_time();
    }

    //获取当前时间字符串
    inline std::string GetCurTime()
    {
        //获取创创时间
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


    //任意类型转string
    template <typename T>
    inline std::string Type2Str(T& Src)
    {
        std::ostringstream os;
        os<<Src;
        return os.str();
    }

    //转换基类指针为子类对像
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