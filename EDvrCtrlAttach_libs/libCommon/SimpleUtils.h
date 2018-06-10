#pragma once
#include "ArrayList.h"

typedef struct _SUtilsAttrib
{
    char szName[128];
    char szValue[256];
}SUtilsAttrib;

typedef struct _SUtilsElement
{
    char szName[128];
    char szValue[1024];

    //SArrayListRoot lstAttribs;
}SUtilsElement;

typedef struct _SUtils
{
    SArrayListRoot lstElements;
    char szHead[64];
}SUtils, *HUtils;

#ifdef __cplusplus
extern "C"{
#endif

int InitUtilsAttrib(SUtilsAttrib *pInput);
int InitUtilsElement(SUtilsElement *pInput);

//   SONYRET<CmdIndex>1</CmdIndex><Ret>0</Ret>
//   SONYCMD<CmdIndex>1</CmdIndex><InterfaceIndex>3</InterfaceIndex><IP>127.0.0.1</IP><Port>1331</Port><User>admin</User><Password>admin</Password><SubType>1</SubType>"
HUtils Utils_Create();
void Utils_Release(HUtils hUtils);
void Utils_Reset(HUtils hUtils);

int Utils_InputString(HUtils hUtils, char *szUtilsText);
int Utils_GetParam(HUtils hUtils, char* szName, char* szOutValue, int nMaxLength, char* szDefault);
int Utils_GetParamToInt(HUtils hUtils, char* szName, int* nOutValue, int nDefault);
int Utils_GetParamToStruct(HUtils hUtils, char* szName, BYTE* pOutValue, int nStructSize);

int Utils_SetHead(HUtils hUtils, char* szHead);
int Utils_AddParam(HUtils hUtils, char* szName, char* szValue);
int Utils_AddParamInt(HUtils hUtils, char* szName, int nValue);
int Utils_AddParamStruct(HUtils hUtils, char* szName, BYTE* pValue, int nStructSize);
int Utils_GetUtilsString(HUtils hUtils, char* szOutValue, int nMaxLength);

#ifdef __cplusplus
};

class CSimpleUtils : public SUtils
{
public:
    CSimpleUtils(void);
    ~CSimpleUtils(void);

    void Reset();

    int InputString(char *szUtilsText);
    int GetParam(char* szName, char* szOutValue, int nMaxLength, char* szDefault);
    int GetParamToInt(char* szName, int* nOutValue, int nDefault);
    int GetParamToStruct(char* szName, BYTE* pOutValue, int nStructSize);

    int SetHead(char* szHead);
    int AddParam(char* szName, char* szValue);
    int AddParamInt(char* szName, int nValue);
    int AddParamStruct(char* szName, BYTE* pValue, int nStructSize);
    int GetUtilsString(char* szOutValue, int nMaxLength);
};

#endif //__cplusplus
