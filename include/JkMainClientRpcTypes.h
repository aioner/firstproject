#ifndef _JK_MAIN_CLIENT_OCX_RPC_TYPES_H_INCLUDED
#define _JK_MAIN_CLIENT_OCX_RPC_TYPES_H_INCLUDED

#include "rpc/rpc_types.h"
#include "rpc/rpc_serialize.h"
#include "rpc/rpc_control.h"
#include "rpc/rpc_closure.h"

class SetServerInfo_RequestType
{
public:
    SetServerInfo_RequestType()
        :m_sNum(0),
        m_strIPS(),
        m_lPort(0)
    {}

    short get_num() const
    {
        return m_sNum;
    }
    void set_num(short num)
    {
        m_sNum = num;
    }

    const std::string& get_IPS() const
    {
        return m_strIPS;
    }
    void set_IPS(const char *ips)
    {
        m_strIPS.assign(ips);
    }

    long get_port() const
    {
        return m_lPort;
    }
    void set_port(long port)
    {
        m_lPort = port;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_sNum & m_strIPS & m_lPort;
    }
private:
    short m_sNum;
    std::string m_strIPS;
    long m_lPort;
};

class NoReturnResponsType
{
public:
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {}
};

class NoParamResponsType
{
public:
    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {}
};

typedef NoReturnResponsType SetServerInfo_ResponseType;

class SetMainHwnd_RequestType
{
public:
    SetMainHwnd_RequestType()
        :m_lHwnd(0)
    {}

    long get_hwnd() const
    {
        return m_lHwnd;
    }

    void set_hwnd(long hwnd)
    {
        m_lHwnd = hwnd;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_lHwnd;
    }

private:
    long m_lHwnd;
};

typedef NoReturnResponsType SetMainHwnd_ReponseType;

class StartLinkServer_RequestType
{
public:
    StartLinkServer_RequestType()
        :m_sNum(0)
    {}

    short get_num() const
    {
        return m_sNum;
    }

    void set_num(short num)
    {
        m_sNum = num;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_sNum;
    }

private:
    short m_sNum;
};

typedef NoReturnResponsType StartLinkServer_ResponseType;


class StopLinkServer_RequestType
{
public:
    StopLinkServer_RequestType()
        :m_sNum(0)
    {}

    short get_num() const
    {
        return m_sNum;
    }

    void set_num(short num)
    {
        m_sNum = num;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_sNum;
    }

private:
    short m_sNum;
};

typedef NoReturnResponsType StopLinkServer_ResponseType;

class NewSetLocalType_RequestType
{
public:
    NewSetLocalType_RequestType()
        :m_lType(0)
    {}

    long get_type() const
    {
        return m_lType;
    }

    void set_type(long type)
    {
        m_lType = type;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_lType;
    }

private:
    long m_lType;
};

typedef NoReturnResponsType NewSetLocalType_ReponseType;

class CheckPassword_RequestType
{
public:
    CheckPassword_RequestType()
        :m_lType(0),
        m_strName(),
        m_strPwd(),
        m_strRes1(),
        m_lRes2(0)
    {}

    long get_type() const
    {
        return m_lType;
    }
    void set_type(long type)
    {
        m_lType = type;
    }

    const std::string& get_name() const
    {
        return m_strName;
    }
    void set_name(const char *name)
    {
        m_strName.assign(name);
    }

    const std::string& get_pwd() const
    {
        return m_strPwd;
    }
    void set_pwd(const char *pwd)
    {
        m_strPwd.assign(pwd);
    }

    const std::string& get_res1() const
    {
        return m_strRes1;
    }
    void set_res1(const char *res1)
    {
        m_strRes1.assign(res1);
    }

    long get_res2() const
    {
        return m_lRes2;
    }
    void set_res2(long res2)
    {
        m_lRes2 = res2;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_lType & m_strName & m_strPwd & m_strRes1 & m_lRes2;
    }

private:
    long m_lType;
    std::string m_strName;
    std::string m_strPwd;
    std::string m_strRes1;
    long m_lRes2;
};

class CheckPassword_ResponseType
{
public:
    CheckPassword_ResponseType()
        :m_lResult(0)
    {}

    long get_result() const
    {
        return m_lResult;
    }
    void set_result(long result)
    {
        m_lResult = result;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_lResult;
    }

private:
    long m_lResult;
};

class SendInfo_RequestType
{
public:
    SendInfo_RequestType()
        :m_strName(),
        m_strIPS()
    {}

    const std::string& get_name() const
    {
        return m_strName;
    }
    void set_name(const char *name)
    {
        m_strName.assign(name);
    }

    const std::string& get_IPS() const
    {
        return m_strIPS;
    }
    void set_IPS(const char *ips)
    {
        m_strIPS.assign(ips);
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strName & m_strIPS;
    }

private:
    std::string m_strName;
    std::string m_strIPS;
};

typedef NoReturnResponsType SendInfo_ResponseType;

class GetLoginInfo_RequestType
{
public:
    GetLoginInfo_RequestType()
        :m_strIDS()
    {}

    const std::string& get_ids() const
    {
        return m_strIDS;
    }
    void set_ids(const char *ids)
    {
        m_strIDS.assign(ids);
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strIDS;
    }
private:
    std::string m_strIDS;
};

class GetLoginInfo_ResponseType
{
public:
    GetLoginInfo_ResponseType()
        :m_strName(),
        m_strPwd(),
        m_lPort(0),
        m_strRes1(),
        m_lRes2(0)
    {}

    const std::string& get_name() const
    {
        return m_strName;
    }
    void set_name(const char *name)
    {
        m_strName.assign(name);
    }
    void set_name(const std::string& name)
    {
        m_strName = name;
    }

    const std::string& get_pwd() const
    {
        return m_strPwd;
    }
    void set_pwd(const char *pwd)
    {
        m_strPwd.assign(pwd);
    }
    void set_pwd(const std::string& pwd)
    {
        m_strPwd = pwd;
    }

    long get_port() const
    {
        return m_lPort;
    }
    void set_port(long port)
    {
        m_lPort = port;
    }

    const std::string& get_res1() const
    {
        return m_strRes1;
    }
    void set_res1(const char *res1)
    {
        m_strRes1.assign(res1);
    }
    void set_res1(const std::string& res1)
    {
        m_strRes1 = res1;
    }

    long get_res2() const
    {
        return m_lRes2;
    }
    void set_res2(long res2)
    {
        m_lRes2 = res2;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strName & m_strPwd & m_lPort & m_strRes1 & m_lRes2;
    }

private:
    std::string m_strName;
    std::string m_strPwd;
    long m_lPort;
    std::string m_strRes1;
    long m_lRes2;
};

class SendDARReq_RequestType
{
public:
    SendDARReq_RequestType()
        :m_strIDS(),
        m_lCH(0),
        m_lCodeType(0),
        m_lRes1(0),
        m_strRes2()
    {}

    const std::string& get_ids() const
    {
        return m_strIDS;
    }
    void set_ids(const char *ids)
    {
        m_strIDS.assign(ids);
    }

    long get_ch() const
    {
        return m_lCH;
    }
    void set_ch(long ch)
    {
        m_lCH = ch;
    }

    long get_code_type() const
    {
        return m_lCodeType;
    }
    void set_code_type(long type)
    {
        m_lCodeType = type;
    }

    long get_res1() const
    {
        return m_lRes1;
    }
    void set_res1(long res1)
    {
        m_lRes1 = res1;
    }

    const std::string& get_res2() const
    {
        return m_strRes2;
    }
    void set_res2(const char *res2)
    {
        m_strRes2.assign(res2);
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strIDS & m_lCH & m_lCodeType & m_lRes1 & m_strRes2;
    }
private:
    std::string m_strIDS;
    long m_lCH;
    long m_lCodeType;
    long m_lRes1;
    std::string m_strRes2;
};

typedef NoReturnResponsType SendDARReq_ResponseType;

class SendTransparentCommand_RequestType
{
public:
    SendTransparentCommand_RequestType()
        :m_strIDS(),
        m_strIPS(),
        m_strCmds()
    {}

    const std::string& get_ids() const
    {
        return m_strIDS;
    }
    void set_ids(const char *ids)
    {
        m_strIDS.assign(ids);
    }

    const std::string& get_ips() const
    {
        return m_strIPS;
    }
    void set_ips(const char *ips)
    {
        m_strIPS.assign(ips);
    }

    const std::string& get_cmds() const
    {
        return m_strCmds;
    }
    void set_cmds(const char *cmds)
    {
        m_strCmds.assign(cmds);
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strIDS & m_strIPS & m_strCmds;
    }

private:
    std::string m_strIDS;
    std::string m_strIPS;
    std::string m_strCmds;
};

class SendTransparentCommand_ResponseType
{
public:
    SendTransparentCommand_ResponseType()
        :m_lResult(0)
    {}

    long get_result() const
    {
        return m_lResult;
    }
    void set_result(long result)
    {
        m_lResult = result;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_lResult;
    }

private:
    long m_lResult;
};

class SetVideoCenterPlayID_RequestType
{
public:
    SetVideoCenterPlayID_RequestType()
        :m_strIDS(),
        m_lDvrChID(0),
        m_lVCenterChID(0),
        m_lRes1(0),
        m_strRes1()
    {}

    const std::string& get_ids() const
    {
        return m_strIDS;
    }
    void set_ids(const char *ids)
    {
        m_strIDS.assign(ids);
    }

    long get_dvr_chid() const
    {
        return m_lDvrChID;
    }
    void set_dvr_chid(long id)
    {
        m_lDvrChID = id;
    }

    long get_vcenter_chid() const
    {
        return m_lVCenterChID;
    }
    void set_vcenter_chid(long id)
    {
        m_lVCenterChID = id;
    }

    long get_res1() const
    {
        return m_lRes1;
    }
    void set_res1(long res1)
    {
        m_lRes1 = res1;
    }

    const std::string& get_str_res1() const
    {
        return m_strRes1;
    }
    void set_str_res1(const char *str_res1)
    {
        m_strRes1.assign(str_res1);
    }
    const std::string& get_sExInfo()const
    {
        return m_sExInfo;
    }
    void set_sExInfo(const char* sExInfo)
    {
        m_sExInfo.assign(sExInfo);
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strIDS & m_lDvrChID & m_lVCenterChID & m_lRes1 & m_strRes1;
    }

private:
    std::string m_strIDS;
    long m_lDvrChID;
    long m_lVCenterChID;
    long m_lRes1;
    std::string m_strRes1;
    std::string m_sExInfo;
};

typedef NoReturnResponsType SetVideoCenterPlayID_ResponseType;

class SetVideoCenterPlayIDEx_RequestType
{
public:
    SetVideoCenterPlayIDEx_RequestType()
        :m_strIDS(),
        m_lDvrChID(0),
        m_lVCenterChID(0),
        m_lRes1(0),
        m_strRes1()
    {}

    const std::string& get_ids() const
    {
        return m_strIDS;
    }
    void set_ids(const char *ids)
    {
        m_strIDS.assign(ids);
    }

    long get_dvr_chid() const
    {
        return m_lDvrChID;
    }
    void set_dvr_chid(long id)
    {
        m_lDvrChID = id;
    }

    long get_vcenter_chid() const
    {
        return m_lVCenterChID;
    }
    void set_vcenter_chid(long id)
    {
        m_lVCenterChID = id;
    }

    long get_res1() const
    {
        return m_lRes1;
    }
    void set_res1(long res1)
    {
        m_lRes1 = res1;
    }

    const std::string& get_str_res1() const
    {
        return m_strRes1;
    }
    void set_str_res1(const char *str_res1)
    {
        m_strRes1.assign(str_res1);
    }
    const std::string& get_sExInfo()const
    {
        return m_sExInfo;
    }
    void set_sExInfo(const char* sExInfo)
    {
        m_sExInfo.assign(sExInfo);
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strIDS & m_lDvrChID & m_lVCenterChID & m_lRes1 & m_strRes1 & m_sExInfo;
    }

private:
    std::string m_strIDS;
    long m_lDvrChID;
    long m_lVCenterChID;
    long m_lRes1;
    std::string m_strRes1;
    std::string m_sExInfo;
};

typedef NoReturnResponsType SetVideoCenterPlayIDEx_ResponseType;

class LinkServerEvent_RequestType
{
public:
    LinkServerEvent_RequestType()
        :m_lNum(0),
        m_lFlag(0)
    {}

    long get_num() const
    {
        return m_lNum;
    }
    void set_num(long num)
    {
        m_lNum = num;
    }

    long get_flag() const
    {
        return m_lFlag;
    }
    void set_flag(long flag)
    {
        m_lFlag = flag;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_lNum & m_lFlag;
    }

private:
    long m_lNum;
    long m_lFlag;
};

class UserLoginOrLogoutEvent_RequestType
{
public:
    UserLoginOrLogoutEvent_RequestType()
        :m_strIDS(),
        m_strName(),
        m_lType(0),
        m_strIPS(),
        m_lFlag(0),
        m_lRes1(0),
        m_lRes2(0),
        m_strRes1(),
        m_strRes2()
    {}

    const std::string& get_ids() const
    {
        return m_strIDS;
    }
    void set_ids(const char *ids)
    {
        m_strIDS.assign(ids);
    }

    const std::string& get_name() const
    {
        return m_strName;
    }
    void set_name(const char *name)
    {
        m_strName.assign(name);
    }

    long get_type() const
    {
        return m_lType;
    }
    void set_type(long type)
    {
        m_lType = type;
    }

    const std::string& get_ips() const
    {
        return m_strIPS;
    }
    void set_ips(const char *ips)
    {
        m_strIPS.assign(ips);
    }

    long get_flag() const
    {
        return m_lFlag;
    }
    void set_flag(long flag)
    {
        m_lFlag = flag;
    }

    long get_res1() const
    {
        return m_lRes1;
    }
    void set_res1(long res1)
    {
        m_lRes1 = res1;
    }

    long get_res2() const
    {
        return m_lRes2;
    }
    void set_res2(long res2)
    {
        m_lRes2 = res2;
    }

    const std::string& get_str_res1() const
    {
        return m_strRes1;
    }
    void set_str_res1(const char *res1)
    {
        m_strRes1.assign(res1);
    }

    const std::string& get_str_res2() const
    {
        return m_strRes2;
    }
    void set_str_res2(const char *res2)
    {
        m_strRes2.assign(res2);
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strIDS & m_strName & m_lType & m_strIPS & m_lFlag & m_lRes1 & m_lRes2 & m_strRes1 & m_strRes2;
    }

private:
    std::string m_strIDS;
    std::string m_strName;
    long m_lType;
    std::string m_strIPS;
    long m_lFlag;
    long m_lRes1;
    long m_lRes2;
    std::string m_strRes1;
    std::string m_strRes2;
};

class DBImageCenterEvent_RequestType
{
public:
    DBImageCenterEvent_RequestType()
        :m_strIDS(),
        m_lCH(0),
        m_lFlag(0),
        m_strfIDS(),
        m_strfIPS(),
        m_lDbMode(0),
        m_lLocalVPort(0),
        m_lLocalAPort(0),
        m_lDestVPort(0),
        m_lDestAPort(0),
        m_lRes1(0),
        m_strRes1()
    {}

    const std::string& get_ids() const
    {
        return m_strIDS;
    }
    void set_ids(const char *ids)
    {
        m_strIDS.assign(ids);
    }

    long get_ch() const
    {
        return m_lCH;
    }
    void set_ch(long ch)
    {
        m_lCH = ch;
    }

    long get_flag() const
    {
        return m_lFlag;
    }
    void set_flag(long flag)
    {
        m_lFlag = flag;
    }

    const std::string& get_fids() const
    {
        return m_strfIDS;
    }
    void set_fids(const char *fids)
    {
        m_strfIDS.assign(fids);
    }

    const std::string& get_ips() const
    {
        return m_strfIPS;
    }
    void set_ips(const char *ips)
    {
        m_strfIPS.assign(ips);
    }

    long get_dbmode() const
    {
        return m_lDbMode;
    }
    void set_dbmode(long mode)
    {
        m_lDbMode = mode;
    }

    long get_local_vport() const
    {
        return m_lLocalVPort;
    }
    void set_local_vport(long port)
    {
        m_lLocalVPort = port;
    }

    long get_local_aport() const
    {
        return m_lLocalAPort;
    }
    void set_local_aport(long port)
    {
        m_lLocalAPort = port;
    }

    long get_dest_vport() const
    {
        return m_lDestVPort;
    }
    void set_dest_vport(long port)
    {
        m_lDestVPort = port;
    }

    long get_dest_aport() const
    {
        return m_lDestAPort;
    }
    void set_dest_aport(long port)
    {
        m_lDestAPort = port;
    }

    long get_res1() const
    {
        return m_lRes1;
    }
    void set_res1(long res1)
    {
        m_lRes1 = res1;
    }

    const std::string& get_str_res1() const
    {
        return m_strRes1;
    }
    void set_str_res1(const char *res1)
    {
        m_strRes1.assign(res1);
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strIDS & m_lCH & m_lFlag & m_strfIDS & m_strfIPS & m_lDbMode & m_lLocalVPort & m_lLocalAPort & m_lDestVPort & m_lDestAPort & m_lRes1 & m_strRes1;
    }

private:
    std::string m_strIDS;	// 设备IDS
    long m_lCH;				// 设备通道号
    long m_lFlag;			// 指令
    std::string m_strfIDS;	// 保留
    std::string m_strfIPS;	// 点播IP
    long m_lDbMode;			// 点播模式  0-TCP  1-UDP  2-UDP组播 3-RTP 4-RTP组播
    long m_lLocalVPort;		// 设备类型
    long m_lLocalAPort;		// 保留
    long m_lDestVPort;		// 指定转发服务通道号
    long m_lDestAPort;		// 设备码流类型
    long m_lRes1;			// 点播通道号(与点播IP对应)[在IDS更新表示新设备通道号(通道+码流类型)]
    std::string m_strRes1;	// 在IDS更新表示新设备IDS
};
/*
sIDS , sCH 点播的DVR IDS和通道，
bz点播标志  =0 表明停止点播   =1 表明点播
DBMode点播模式  0-TCP  1-UDP  2-组播
fIP点播使用IP
iRes1 点播使用的通道
LocalVPort  目前模式下，代表点播图像的类型 1—工控型  2—EDVR型
*/
//收到点播信息 "STOPALL" bz=0 停止所有

class OnGetMsgEvent_RequestType
{
public:
    OnGetMsgEvent_RequestType()
        :m_strSrcIDS(),
        m_strInfo(),
        m_lInfoSize(0),
        m_nType(0)
    {}

    const std::string& get_src_ids() const
    {
        return m_strSrcIDS;
    }
    void set_src_ids(const char *ids)
    {
        m_strSrcIDS.assign(ids);
    }

    const std::string& get_info() const
    {
        return m_strInfo;
    }
    void set_info(const char *info)
    {
        m_strInfo.assign(info);
    }

    long get_info_size() const
    {
        return m_lInfoSize;
    }
    void set_info_size(long size)
    {
        m_lInfoSize = size;
    }

    long get_type() const
    {
        return m_nType;
    }
    void set_type(long type)
    {
        m_nType = type;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strSrcIDS & m_strInfo & m_lInfoSize & m_nType;
    }

private:
    std::string m_strSrcIDS;
    std::string m_strInfo;
    long m_lInfoSize;
    long m_nType;
};

class OnTellLocalIDSEvent_RequestType
{
public:
    OnTellLocalIDSEvent_RequestType()
        :m_strLocalIDS(),
        m_strsRes1(),
        m_iRes1(0)
    {}

    const std::string& get_local_ids() const
    {
        return m_strLocalIDS;
    }
    void set_local_ids(const char *ids)
    {
        m_strLocalIDS.assign(ids);
    }

    const std::string& get_str_res1() const
    {
        return m_strsRes1;
    }
    void set_str_res1(const char *ids)
    {
        m_strsRes1.assign(ids);
    }

    long get_ires1() const
    {
        return m_iRes1;
    }
    void set_ires1(long iRes1)
    {
        m_iRes1 = iRes1;
    }

    template<typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & m_strLocalIDS & m_strsRes1 & m_iRes1;
    }

private:
    std::string m_strLocalIDS;
    std::string m_strsRes1;
    long m_iRes1;
};

//add by songlei 20150518
class OnTransparentCommandEvent_RequestType
{
public:

	void set_ids(const char* ids)
	{
		ids_.assign(ids);

	}
	
	void set_command(const char* ctx)
	{
		command_ctx_.assign(ctx);
	}

	const std::string& get_ids() const
	{
		return ids_;
	}
	const std::string& get_command() const
	{
		return command_ctx_;
	}
	template<typename Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & ids_ & command_ctx_;
	}
private:
	std::string ids_;
	std::string command_ctx_;
};

template<typename DrivedT>
class JkMainClientOcxRpcInterfaces
{
public:
    bool SetServerInfo(rpc::control_t&, SetServerInfo_RequestType *, SetServerInfo_ResponseType *, rpc::closure_t *);
    bool SetMainHwnd(rpc::control_t&, SetMainHwnd_RequestType *, SetMainHwnd_ReponseType *, rpc::closure_t *);
    bool StartLinkServer(rpc::control_t&, StartLinkServer_RequestType *, StartLinkServer_ResponseType *, rpc::closure_t *);
    bool StopLinkServer(rpc::control_t&, StopLinkServer_RequestType *, StopLinkServer_ResponseType *, rpc::closure_t *);
    bool NewSetLocalType(rpc::control_t&, NewSetLocalType_RequestType *, NewSetLocalType_ReponseType *, rpc::closure_t *);
    bool CheckPassword(rpc::control_t&, CheckPassword_RequestType *, CheckPassword_ResponseType *, rpc::closure_t *);
    bool SendInfo(rpc::control_t&, SendInfo_RequestType *, SendInfo_ResponseType *, rpc::closure_t *);
    bool GetLoginInfo(rpc::control_t&, GetLoginInfo_RequestType *, GetLoginInfo_ResponseType *, rpc::closure_t *);
    bool SendDARReq(rpc::control_t&, SendDARReq_RequestType *, SendDARReq_ResponseType *, rpc::closure_t *);
    bool SendTransparentCommand(rpc::control_t&, SendTransparentCommand_RequestType *, SendTransparentCommand_ResponseType *, rpc::closure_t *);
    bool SetVideoCenterPlayID(rpc::control_t&, SetVideoCenterPlayID_RequestType *, SetVideoCenterPlayID_ResponseType *, rpc::closure_t *);
    bool SetVideoCenterPlayIDEx(rpc::control_t&, SetVideoCenterPlayIDEx_RequestType *, SetVideoCenterPlayIDEx_ResponseType *, rpc::closure_t *);

    void OnLinkServerEvent(rpc::control_t&, LinkServerEvent_RequestType *);
    void OnUserLoginOrLogoutEvent(rpc::control_t&, UserLoginOrLogoutEvent_RequestType *);
    void OnDBImageCenterEvent(rpc::control_t&, DBImageCenterEvent_RequestType *);
    void OnGetMsgEvent(rpc::control_t&, OnGetMsgEvent_RequestType *);
    void OnTellLocalIDSEvent(rpc::control_t&, OnTellLocalIDSEvent_RequestType *);
    void OnTransparentCommandEvent(rpc::control_t&,OnTransparentCommandEvent_RequestType *);

    BEGIN_RPC_FUNC(1, JkMainClientOcxRpcInterfaces, DrivedT)
        RPC_FUNC2(1, SetServerInfo, SetServerInfo_RequestType, SetServerInfo_ResponseType)
        RPC_FUNC2(2, SetMainHwnd, SetMainHwnd_RequestType, SetMainHwnd_ReponseType)
        RPC_FUNC2(3, StartLinkServer, StartLinkServer_RequestType, StartLinkServer_ResponseType)
        RPC_FUNC2(4, StopLinkServer, StopLinkServer_RequestType, StopLinkServer_ResponseType)
        RPC_FUNC2(5, NewSetLocalType, NewSetLocalType_RequestType, NewSetLocalType_ReponseType)
        RPC_FUNC2(6, CheckPassword, CheckPassword_RequestType, CheckPassword_ResponseType)
        RPC_FUNC2(7, SendInfo, SendInfo_RequestType, SendInfo_ResponseType)
        RPC_FUNC2(8, GetLoginInfo, GetLoginInfo_RequestType, GetLoginInfo_ResponseType)
        RPC_FUNC2(9, SendDARReq, SendDARReq_RequestType, SendDARReq_ResponseType)
        RPC_FUNC2(10, SendTransparentCommand, SendTransparentCommand_RequestType, SendTransparentCommand_ResponseType)
        RPC_FUNC2(11, SetVideoCenterPlayID, SetVideoCenterPlayID_RequestType, SetVideoCenterPlayID_ResponseType)
        RPC_FUNC2(12, SetVideoCenterPlayIDEx, SetVideoCenterPlayIDEx_RequestType, SetVideoCenterPlayIDEx_ResponseType)
        RPC_EVENT(13, OnLinkServerEvent, LinkServerEvent_RequestType)
        RPC_EVENT(14, OnUserLoginOrLogoutEvent, UserLoginOrLogoutEvent_RequestType)
        RPC_EVENT(15, OnDBImageCenterEvent, DBImageCenterEvent_RequestType)
        RPC_EVENT(16, OnGetMsgEvent, OnGetMsgEvent_RequestType)
        RPC_EVENT(17, OnTellLocalIDSEvent, OnTellLocalIDSEvent_RequestType)
        RPC_EVENT(18, OnTransparentCommandEvent,OnTransparentCommandEvent_RequestType)
    END_RPC_FUNC()
};

#endif //_JK_MAIN_CLIENT_OCX_RPC_TYPES_H_INCLUDED
