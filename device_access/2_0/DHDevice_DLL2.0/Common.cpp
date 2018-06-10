// Common.cpp: implementation of the CCommon class.
//
//////////////////////////////////////////////////////////////////////
#include "Common.h"
//log
bool g_bLog = true;
const char g_szDirectory[] = "D:\\XtLog";

void EnableLog(bool bEnable)
{
    g_bLog = bEnable?true:false;
}

void WriteLog(const char* szFlag, const char* szLogFmt, ...)
{
    if(!g_bLog) return;
#ifdef _WIN32
    SYSTEMTIME tm = {0};
    GetLocalTime(&tm);
    char szFileName[256]={0};
    char szTmp[2048]={0};
    sprintf(szFileName, "%s\\%02d%02d%02d_%02d_%s.log",g_szDirectory,
        tm.wYear, tm.wMonth, tm.wDay, tm.wHour, szFlag);

    FILE* pFile;
    pFile = fopen(szFileName, "a");
    if(pFile)
    {
        va_list v;

        va_start(v, szLogFmt);
        vsprintf(szTmp, szLogFmt, v);
        va_end(v);

        fprintf(pFile, "%02d%02d%02d_%02d%02d%02d", tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond);

        fprintf(pFile, "%s\n", szTmp);

        fclose(pFile);
    }
#else
#endif //#ifdef _WIN32


}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction

void StructInit(SDeviceInfo_DH& rInput)
{
    //ZeroMemory(&rInput, sizeof(SDeviceInfo_DH));
    memset(&rInput, 0, sizeof(SDeviceInfo_DH));
    rInput.m_nID = -1;
    rInput.m_wPost = 37777;
    strcpy(rInput.m_strUserName, "666666");
    strcpy(rInput.m_strPassWord, "666666");
    rInput.m_nID = (WORD)-1;
}

void StructInit(SChannelInfo_DH& rInput)
{
    memset(&rInput, 0, sizeof(SChannelInfo_DH));
    rInput.m_nID = -1;
    rInput.m_nData = -1;
}

void StructInit(SReOnlineInfo& rInput)
{
    memset(&rInput, 0, sizeof(SReOnlineInfo));
}

void StructInit(SReLinkInfo& rInput)
{
    memset(&rInput, 0, sizeof(SReLinkInfo));
}

void BitSet(DWORD &nInput, long nBitCount, BYTE bFlag)
{
    if(nBitCount<1 || nBitCount>32)
        return;
    DWORD nSet = 1<<nBitCount;
    if(bFlag)
    {
        nInput |= nSet;
    }
    else
    {
        nInput ^= nSet;
    }
}

void FixComCfgStruct(SComCfg& rOutput, const DH_COMM_PROP& rInput)
{
    // 串口配置信息
    // 数据位；0：5，1：6，2：7，3：8
    // 停止位；0：1位，1：1.5位，2：2位
    // 校验位；0：无校验，1：奇校验；2：偶校验
    // 波特率；0：300，1：600，2：1200，3：2400，4：4800，5：9600，6：19200，7：38400，8：57600，9：115200
    
    // 透明传输信息
    // 数据位 4~8表示4位~8位
    // 停止位 1表示1位, 2表示1.5位 ,3表示2位
    // 检验位 1 odd奇校验, 2 even偶校验, 3 无校验
    // 波特率 1~8分别表示1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
    
    rOutput.nDataBit = rInput.byDataBit+5;
    rOutput.nStopBit = rInput.byStopBit+1;
    rOutput.nParity = (rInput.byParity==0)?3:rInput.byParity;
    rOutput.nBaudRate = rInput.byBaudRate-1;
}

void FixComCfgStruct(DH_COMM_PROP& rOutput, const SComCfg& rInput)
{
    // 串口配置信息
    // 数据位；0：5，1：6，2：7，3：8
    // 停止位；0：1位，1：1.5位，2：2位
    // 校验位；0：无校验，1：奇校验；2：偶校验
    // 波特率；0：300，1：600，2：1200，3：2400，4：4800，5：9600，6：19200，7：38400，8：57600，9：115200
    
    // 透明传输信息
    // 数据位 4~8表示4位~8位
    // 停止位 1表示1位, 2表示1.5位 ,3表示2位
    // 检验位 1 odd奇校验, 2 even偶校验, 3 无校验
    // 波特率 1~8分别表示1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
    
    rOutput.byDataBit = rInput.nDataBit-5;
    rOutput.byStopBit = rInput.nStopBit-1;
    rOutput.byParity = (rInput.nParity==3)?0:rInput.nParity;
    rOutput.byBaudRate = rInput.nBaudRate+1;
}

DH_RealPlayType FixMediaType(int nMediaType)
{
    switch(nMediaType)
    {
    case 0: return DH_RType_Realplay_0;
    case 1: return DH_RType_Realplay_1;
    case 2: return DH_RType_Realplay_2;
    case 3: return DH_RType_Realplay_3;
    default: return DH_RType_Realplay;
    }
}
