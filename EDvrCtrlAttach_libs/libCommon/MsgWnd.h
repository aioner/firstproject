#ifndef _MSG_WND_H__INCLUDE_
#define _MSG_WND_H__INCLUDE_
#pragma once

#include <commdlg.h>
#include <commctrl.h>
#include "ArrayList.h"

#if (_MSC_VER > 1000)
#pragma comment( lib, "comctl32" )
#endif

typedef HArrayList HMsgWndMrg;
typedef struct _SMsgWnd SMsgWnd;

typedef INT_PTR (*fMsgWndProcess)(SMsgWnd *pMsgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
typedef void (*fMsgWndDestroy)(SMsgWnd *pMsgWnd);
typedef void (*fMsgWndMove)(SMsgWnd *pMsgWnd, int xPos, int yPos);
typedef void (*fMsgWndSize)(SMsgWnd *pMsgWnd, UINT nType, int cx, int cy);
typedef void (*fMsgWndPaint)(SMsgWnd *pMsgWnd, HDC hDC);
typedef void (*fMsgWndClose)(SMsgWnd *pMsgWnd);
typedef BOOL (*fMsgWndCopyData)(SMsgWnd *pMsgWnd, WPARAM hData, COPYDATASTRUCT *pCopyData);
typedef INT_PTR (*fMsgWndNotify)(SMsgWnd *pMsgWnd, int idCtrl, LPNMHDR pnmh);
typedef BOOL (*fMsgWndInitDialog)(SMsgWnd *pMsgWnd, HWND hwndCtlFocus);
typedef void (*fMsgWndCommand)(SMsgWnd *pMsgWnd, WORD wCtlNotifyCode, WORD wCtlID, HWND hwndCtl);
typedef void (*fMsgWndTimer)(SMsgWnd *pMsgWnd, int wTimerID, TIMERPROC tmprc);

typedef void (*fMsgWndBnClicked)(SMsgWnd *pMsgWnd, WORD wCtlID, HWND hwndCtl);

#define MSGWND_MEMBER \
    HWND m_hWnd;\
    HWND m_hwndParent;\
    HICON m_hIcon;\
    SMsgWnd *m_pThis;\
    int m_nWndID;\
    fMsgWndProcess fpMsgWndProcess;\
    fMsgWndProcess fpOnUserMsg;\
    fMsgWndDestroy fpOnDestroy;\
    fMsgWndMove fpOnMove;\
    fMsgWndSize fpOnSize;\
    fMsgWndPaint fpOnPaint;\
    fMsgWndClose fpOnClose;\
    fMsgWndCopyData fpOnCopyData;\
    fMsgWndNotify fpOnNotify;\
    fMsgWndInitDialog fpOnInitDialog;\
    fMsgWndCommand fpOnCommand;\
    fMsgWndTimer fpOnTimer;\
    fMsgWndBnClicked fpOnBnClicked

struct _SMsgWnd
{
    //MSGWND_MEMBER;
    HWND m_hWnd;
    HWND m_hwndParent;
    HICON m_hIcon;
    SMsgWnd *m_pThis;
    int m_nWndID;
    fMsgWndProcess fpMsgWndProcess;
    fMsgWndProcess fpOnUserMsg;
    fMsgWndDestroy fpOnDestroy;
    fMsgWndMove fpOnMove;
    fMsgWndSize fpOnSize;
    fMsgWndPaint fpOnPaint;
    fMsgWndClose fpOnClose;
    fMsgWndCopyData fpOnCopyData;
    fMsgWndNotify fpOnNotify;
    fMsgWndInitDialog fpOnInitDialog;
    fMsgWndCommand fpOnCommand;
    fMsgWndTimer fpOnTimer;
    fMsgWndBnClicked fpOnBnClicked;
};

#ifdef __cplusplus
extern "C" {
#endif

#define MsgWndMrg_Create() ArrayListCreateByProc(sizeof(SMsgWnd*), (fFreeData)MsgWndInit, NULL)
#define MsgWndMrg_Release(hManage) ArrayListRelease(hManage)

#define MsgWndMrg_GetCount(hManage) ArrayListGetCount(hManage)
#define MsgWndMrg_GetSize(hManage) ArrayListGetSize(hManage)

#define MsgWndMrg_AddInfo(hManage, pInfo) ArrayListAddAt(hManage, &pInfo)
#define MsgWndMrg_SetInfo(hManage, pInfo, nIndex) ArrayListSetAt(hManage, &pInfo, nIndex)
#define MsgWndMrg_DeleteInfo(hManage, nIndex) ArrayListEraseAt(hManage, nIndex)

#define MsgWndMrg_GetInfo(hManage, nIndex) \
    (NULL == ArrayListGetAt(hManage, nIndex)? \
    NULL:*(SMsgWnd**)ArrayListGetData(hManage, nIndex))

size_t MsgWndMrg_SelectHwnd(HMsgWndMrg hManage, HWND hWnd, SMsgWnd **ppInfo);
size_t MsgWndMrg_SelectObject(HMsgWndMrg hManage, SMsgWnd *pInfo);

//////////////////////////////////////////////////////////////////////////

int InitMsgWnd(SMsgWnd *pInput);
int MsgWndInit(SMsgWnd **pInput);

SMsgWnd* MsgWnd_FromWnd(HWND hWnd);
SMsgWnd *MsgWnd_Create(HWND hwndParent);
int MsgWnd_AddWnd(SMsgWnd *pMsgWnd);

int MsgWnd_Release(SMsgWnd *pMsgWnd);

HWND MsgWnd_CreateWindow(SMsgWnd *pMsgWnd, HINSTANCE hInstance, WORD nResourceID, WORD nIconID);
INT_PTR MsgWnd_DoModule(SMsgWnd *pMsgWnd, HINSTANCE hInstance, WORD nResourceID, WORD nIconID);

void MsgWnd_Move(SMsgWnd *pMsgWnd, int nLeft, int nTop, int nWidth, int nHeight);
void MsgWnd_MoveLe(SMsgWnd *pMsgWnd, int nLeft, int nTop);

//////////////////////////////////////////////////////////////////////////
LRESULT ComboBox_GetSelectData(HWND hwndCtl);
int ComboBox_SetSelectByData(HWND hwndCtl, LRESULT theData);

LRESULT ListBox_GetSelectData(HWND hwndCtl);
int ListBox_SetSelectByData(HWND hwndCtl, LRESULT theData);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class CMsgWnd : public _SMsgWnd
{
protected:
    static INT_PTR OnMsgWndProcess(SMsgWnd *pMsgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual INT_PTR OnUserMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void OnDestroy();
    virtual void OnMove(int xPos, int yPos);
    virtual void OnSize(UINT nType, int cx, int cy);
    virtual void OnPaint(HDC hDC);
    virtual void OnClose();
    virtual BOOL OnCopyData(WPARAM hData, COPYDATASTRUCT *pCopyData);
    virtual INT_PTR OnNotify(int idCtrl, LPNMHDR pnmh);
    virtual BOOL OnInitDialog(HWND hwndCtlFocus);
    virtual void OnCommand(WORD wCtlNotifyCode, WORD wCtlID, HWND hwndCtl);
    virtual void OnTimer(int wTimerID, TIMERPROC tmprc);

    virtual void OnBnClicked(WORD wCtlID, HWND hwndCtl);

public:
    static CMsgWnd* FromWnd(HWND hWnd);
    static void InitCommCtrl();
    CMsgWnd(HWND hwndParent = NULL);
    virtual ~CMsgWnd(void);

    HWND CreateWnd(HINSTANCE hInstance, WORD nResourceID, WORD nIconID = NULL);
    INT_PTR ModuleWnd(HINSTANCE hInstance, WORD nResourceID, WORD nIconID = NULL);
    operator HWND();

    virtual void MoveWindow(int nLeft, int nTop);
    virtual void MoveWindow(int nLeft, int nTop, int nWidth, int nHeight);

    virtual HWND GetDlgItem(int nIDDlgItem);
    virtual UINT GetDlgItemInt(int nIDDlgItem, BOOL bSigned = TRUE);
    virtual BOOL SetDlgItemInt(int nIDDlgItem, UINT nValue, BOOL bSigned = TRUE);
    virtual UINT GetDlgItemText(int nIDDlgItem, LPSTR lpString, int nMaxCount);
    virtual BOOL SetDlgItemText(int nIDDlgItem, LPCSTR lpString);
};

#endif

#endif//_MSG_WND_H__INCLUDE_

