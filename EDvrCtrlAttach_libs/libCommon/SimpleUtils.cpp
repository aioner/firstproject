#include "stdafx.h"
#include ".\SimpleUtils.h"

#ifdef __cplusplus
extern "C"{
#endif

int InitUtilsAttrib(SUtilsAttrib *pInput)
{
    memset(pInput, 0, sizeof(SUtilsAttrib));
    return 0;
}

int InitUtilsElement(SUtilsElement *pInput)
{
    memset(pInput, 0, sizeof(SUtilsElement));
    //ArrayList_Init(&pInput->lstAttribs, sizeof(SUtilsAttrib), (fFreeData)InitUtilsAttrib, NULL);
    return 0;
}

char *_Utils_ReadHead(char* szUtilsString, char* szOutput)
{
    char *szCursor = szUtilsString;
    int nLength = strlen(szUtilsString);

    szCursor = strchr(szCursor, '<');
    if(szCursor != NULL)
        strncpy(szOutput, szUtilsString, szCursor - szUtilsString);

    return szCursor;
}

char *_Utils_ReadOneElement(char* szUtilsString, SUtilsElement* pOutput)
{
    SUtilsElement tmpElement = {0};
    char *szBracketL = NULL, *szBracketR = NULL;
    char *szCursor = szUtilsString;
    int nLength = strlen(szUtilsString);

    //find begin '<'
    szCursor = strchr(szCursor, '<');
    if(szCursor == NULL)
        return NULL;
    szBracketL = szCursor;

    //if(szCursor[1] == '/')
    //    continue;

    //find begin '>'
    szCursor = strchr(szCursor, '>');
    if(szCursor == NULL)
        return NULL;
    szBracketR = szCursor;

    if(szCursor[-1] == '/')
    {// the Element over
        strncpy(tmpElement.szName, szBracketL+1, szBracketR - szBracketL - 3);
        return szCursor+1;
    }

    strncpy(tmpElement.szName, szBracketL+1, szBracketR - szBracketL - 1);

    //find end '<'
    szCursor = strchr(szCursor, '<');
    if(szCursor == NULL)
        return NULL;
    szBracketL = szCursor;
    strncpy(tmpElement.szValue, szBracketR+1, szBracketL - szBracketR - 1);

    szCursor = strchr(szCursor, '>');
    if(szCursor == NULL)
        return NULL;
    szBracketR = szCursor;
    if(strncmp(tmpElement.szName, szBracketL+2, strlen(tmpElement.szName)))
        return NULL;

    strcpy(pOutput->szName, tmpElement.szName);
    strcpy(pOutput->szValue, tmpElement.szValue);

    return szCursor+1;
}

//////////////////////////////////////////////////////////////////////////

HUtils Utils_Create()
{
    SUtils *pNewUtils = NULL;
    pNewUtils = (SUtils *)malloc(sizeof(SUtils));
    if(!pNewUtils)
        return NULL;

    memset(pNewUtils, 0, sizeof(SUtils));
    ArrayList_Init(&pNewUtils->lstElements, sizeof(SUtilsElement), (fFreeData)InitUtilsElement, NULL);
    return pNewUtils;
}

void Utils_Release(HUtils hUtils)
{
    Utils_Reset(hUtils);
    ArrayListDestroy(&hUtils->lstElements, FALSE);
    free(hUtils);
}

void Utils_Reset(HUtils hUtils)
{
    ArrayListRemoveAll(&hUtils->lstElements);
}

int Utils_InputString(HUtils hUtils, char *szUtilsText)
{
    SUtilsElement newElement = {0};
    char *pString = NULL;

    pString = _Utils_ReadHead(szUtilsText, hUtils->szHead);
    if(pString == NULL)
        return -1;

    do 
    {
        pString = _Utils_ReadOneElement(pString, &newElement);
        if(pString == NULL)
            break;
        ArrayListAddAt( &hUtils->lstElements, &newElement);
    } while (pString < szUtilsText+strlen(szUtilsText));

    return ArrayListGetCount(&hUtils->lstElements);
}

int Utils_GetParam(HUtils hUtils, char* szName, char* szOutValue, int nMaxLength, char* szDefault)
{
    int i;
    SUtilsElement *pElement = NULL;
    for(i=0; i<(signed)ArrayListGetCount(&hUtils->lstElements); i++)
    {
        pElement = (SUtilsElement *)ArrayListGetData(&hUtils->lstElements, i);
        if(!pElement || strcmp(pElement->szName, szName))
            continue;
        
        strncpy(szOutValue, pElement->szValue, nMaxLength);
        return i;
    }
    if(szDefault)
        strcpy(szOutValue, szDefault);
    return -1;
}

int Utils_GetParamToInt(HUtils hUtils, char* szName, int* nOutValue, int nDefault)
{
    int i;
    SUtilsElement *pElement = NULL;
    for(i=0; i<(signed)ArrayListGetCount(&hUtils->lstElements); i++)
    {
        pElement = (SUtilsElement *)ArrayListGetData(&hUtils->lstElements, i);
        if(!pElement || strcmp(pElement->szName, szName))
            continue;

        *nOutValue = atoi(pElement->szValue);
        return 0;
    }
    *nOutValue = nDefault;
    return -1;
}

int Utils_GetParamToStruct(HUtils hUtils, char* szName, BYTE* pOutValue, int nStructSize)
{
    int i, nCRC, nTemp, nDataLength;
    SUtilsElement *pElement = NULL;

    for(i=0; i<(signed)ArrayListGetCount(&hUtils->lstElements); i++)
    {
        pElement = (SUtilsElement *)ArrayListGetData(&hUtils->lstElements, i);
        if(!pElement || strcmp(pElement->szName, szName))
            continue;
        else
            break;
    }
    if(!pElement)
        return -1;

    nDataLength = strlen(pElement->szValue)/2 - 1;
    if(nDataLength != nStructSize)
    {//数据已经损坏
        return -1;
    }

    for(i=0, nCRC=0; i<nDataLength; i++)
    {
        sscanf(pElement->szValue+i*2, "%02x", &nTemp);
        pOutValue[i] = nTemp&0xff;
        nCRC += nTemp;
    }
    sscanf(pElement->szValue+i*2, "%02x", &nTemp);
    if((nCRC&0xff) != nTemp)
    {
        return -1;
    }
    return nDataLength;
}


int Utils_SetHead(HUtils hUtils, char* szHead)
{
    strcpy(hUtils->szHead, szHead);
    return 0;
}

int Utils_AddParam(HUtils hUtils, char* szName, char* szValue)
{
    SUtilsElement newElement;
    
    InitUtilsElement(&newElement);
    strcpy(newElement.szName, szName);
    strcpy(newElement.szValue, szValue);
    return ArrayListAddAt(&hUtils->lstElements, &newElement);
}

int Utils_AddParamInt(HUtils hUtils, char* szName, int nValue)
{
    SUtilsElement newElement;

    InitUtilsElement(&newElement);
    strcpy(newElement.szName, szName);
    sprintf(newElement.szValue, "%d", nValue);
    return ArrayListAddAt(&hUtils->lstElements, &newElement);
}

int Utils_AddParamStruct(HUtils hUtils, char* szName, BYTE* pValue, int nStructSize)
{
    SUtilsElement newElement;
    int i, nCRC;

    InitUtilsElement(&newElement);
    strcpy(newElement.szName, szName);

    for(i=0, nCRC=0; i<nStructSize; i++)
    {
        sprintf(newElement.szValue+i*2, "%02x", pValue[i]);
        nCRC += pValue[i];
    }
    sprintf(newElement.szValue+i*2, "%02x", nCRC&0xff);

    return ArrayListAddAt(&hUtils->lstElements, &newElement);
}

int Utils_GetUtilsString(HUtils hUtils, char* szOutValue, int nMaxLength)
{
    int i;
    SUtilsElement *pElement = NULL;
    char *szTmp = (char*)malloc(0x10000);
    char *szCursor = szTmp;

    assert(szTmp!=NULL);
    szCursor += sprintf(szCursor, "%s", hUtils->szHead);
    for(i=0; i<(signed)ArrayListGetCount(&hUtils->lstElements); i++)
    {
        pElement = (SUtilsElement *)ArrayListGetData(&hUtils->lstElements, i);
        if(!pElement)
            continue;

        szCursor += sprintf(szCursor, "<%s>", pElement->szName);
        szCursor += sprintf(szCursor, "%s", pElement->szValue);
        szCursor += sprintf(szCursor, "</%s>", pElement->szName);
    }

    strncpy(szOutValue, szTmp, nMaxLength);

    free(szTmp);
    return strlen(szOutValue);
}

#ifdef __cplusplus
};
#endif

CSimpleUtils::CSimpleUtils(void)
{
    ArrayList_Init(&lstElements, sizeof(SUtilsElement), (fFreeData)InitUtilsElement, NULL);
}

CSimpleUtils::~CSimpleUtils(void)
{
    Reset();
    ArrayListDestroy(&lstElements, FALSE);
}

void CSimpleUtils::Reset()
{
    Utils_Reset(this);
}

int CSimpleUtils::InputString(char *szUtilsText)
{
    return Utils_InputString(this, szUtilsText);
}

int CSimpleUtils::GetParam(char* szName, char* szOutValue, int nMaxLength, char* szDefault)
{
    return Utils_GetParam(this, szName, szOutValue, nMaxLength, szDefault);
}

int CSimpleUtils::GetParamToInt(char* szName, int* nOutValue, int nDefault)
{
    return Utils_GetParamToInt(this, szName, nOutValue, nDefault);
}

int CSimpleUtils::GetParamToStruct(char* szName, BYTE* pOutValue, int nStructSize)
{
    return Utils_GetParamToStruct(this, szName, pOutValue, nStructSize);
}

int CSimpleUtils::SetHead(char* szHead)
{
    return Utils_SetHead(this, szHead);
}

int CSimpleUtils::AddParam(char* szName, char* szValue)
{
    return Utils_AddParam(this, szName, szValue);
}

int CSimpleUtils::AddParamInt(char* szName, int nValue)
{
    return Utils_AddParamInt(this, szName, nValue);
}

int CSimpleUtils::AddParamStruct(char* szName, BYTE* pValue, int nStructSize)
{
    return Utils_AddParamStruct(this, szName, pValue, nStructSize);
}

int CSimpleUtils::GetUtilsString(char* szOutValue, int nMaxLength)
{
    return Utils_GetUtilsString(this, szOutValue, nMaxLength);
}
