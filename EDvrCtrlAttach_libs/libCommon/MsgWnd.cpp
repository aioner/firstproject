#include "stdafx.h"
#include "MsgWnd.h"
#include "WriteLog.h"

static HMsgWndMrg g_pWndMrg = NULL;

#ifdef __cplusplus
extern "C" {
#endif

INT_PTR WINAPI MsgWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR MsgWnd_Process(SMsgWnd *pMsgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MsgWnd_Command(SMsgWnd *pMsgWnd, WORD wCtlNotifyCode, WORD wCtlID, HWND hwndCtl);
void MsgWnd_Paint(SMsgWnd *pMsgWnd);
BOOL MsgWnd_InitDialog(SMsgWnd *pMsgWnd, HWND hwndCtlFocus);
INT_PTR MsgWnd_UserMsg(SMsgWnd *pMsgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void MsgWnd_Destroy(SMsgWnd *pMsgWnd);
void MsgWnd_Close(SMsgWnd *pMsgWnd);
void MsgWnd_Timer(SMsgWnd *pMsgWnd, int wTimerID, TIMERPROC tmprc);
INT_PTR MsgWnd_Notify(SMsgWnd *pMsgWnd, int idCtrl, LPNMHDR pnmh);
BOOL MsgWnd_BnClicked(SMsgWnd *pMsgWnd, WORD wCtlID, HWND hwndCtl);
BOOL MsgWnd_CopyData(SMsgWnd *pMsgWnd, WPARAM hData, COPYDATASTRUCT *pCopyData);
void MsgWnd_OnMove(SMsgWnd *pMsgWnd, int xPos, int yPos);
void MsgWnd_OnSize(SMsgWnd *pMsgWnd, UINT nType, int cx, int cy);

//////////////////////////////////////////////////////////////////////////
// Process MsgWndMrg

size_t MsgWndMrg_SelectHwnd(HMsgWndMrg hManage, HWND hWnd, SMsgWnd **ppInfo)
{
    size_t i;
    SMsgWnd *pNode = NULL;
    if(!hManage)
        return -1;
    for(i=0; i<ArrayListGetSize(hManage); i++)
    {
        pNode = MsgWndMrg_GetInfo(hManage, i);
        if(pNode == NULL || pNode->m_hWnd != hWnd)
            continue;
        if(ppInfo != NULL)
            *ppInfo = pNode;
        return i;
    }
    return -1;
}
size_t MsgWndMrg_SelectObject(HMsgWndMrg hManage, SMsgWnd *pInfo)
{
    size_t i;
    SMsgWnd *pNode = NULL;
    for(i=0; i<ArrayListGetSize(hManage); i++)
    {
        pNode = MsgWndMrg_GetInfo(hManage, i);
        if(pNode == NULL || pNode->m_pThis != pInfo)
            continue;
        return i;
    }
    return -1;
}


int InitMsgWnd(SMsgWnd *pInput)
{
    memset(pInput, 0, sizeof(SMsgWnd));
    pInput->m_nWndID = -1;
    return 0;
}

int MsgWndInit(SMsgWnd **pInput)
{
    *pInput = NULL;
    return 0;
}

//////////////////////////////////////////////////////////////////////
// Class CMsgWnd

INT_PTR WINAPI MsgWndProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SMsgWnd *pMsgWnd = NULL;

    if(uMsg == WM_INITDIALOG)
    {
        pMsgWnd = (SMsgWnd*)lParam;
        pMsgWnd->m_hWnd = hwndDlg;
    }
    else
    {
        MsgWndMrg_SelectHwnd(g_pWndMrg, hwndDlg, &pMsgWnd);
    }

    if (pMsgWnd && pMsgWnd->fpMsgWndProcess && pMsgWnd == pMsgWnd->m_pThis)
    {
        return pMsgWnd->fpMsgWndProcess(pMsgWnd, uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
    }
}

//////////////////////////////////////////////////////////////////////

SMsgWnd* MsgWnd_FromWnd(HWND hWnd)
{
    int nWndID = -1;
    SMsgWnd *pMsgWnd = NULL;
    if(!g_pWndMrg)
        return NULL;
    nWndID = MsgWndMrg_SelectHwnd(g_pWndMrg,  hWnd, &pMsgWnd);
    return pMsgWnd;
}

SMsgWnd *MsgWnd_Create(HWND hwndParent)
{
    SMsgWnd *pMsgWnd = NULL;
    if(!g_pWndMrg)
        g_pWndMrg = MsgWndMrg_Create();

    pMsgWnd = (SMsgWnd *)malloc(sizeof(SMsgWnd));
    assert(pMsgWnd!=NULL);
    InitMsgWnd(pMsgWnd);
    pMsgWnd->m_hwndParent = hwndParent;
    pMsgWnd->m_pThis = pMsgWnd;
    if(pMsgWnd->fpMsgWndProcess == NULL)
        pMsgWnd->fpMsgWndProcess = MsgWnd_Process;
    pMsgWnd->m_nWndID = MsgWndMrg_AddInfo(g_pWndMrg, pMsgWnd);
    return pMsgWnd;
}

int MsgWnd_AddWnd(SMsgWnd *pMsgWnd)
{
    if(!g_pWndMrg)
        g_pWndMrg = MsgWndMrg_Create();
    pMsgWnd->m_pThis = pMsgWnd;
    if(pMsgWnd->fpMsgWndProcess == NULL)
        pMsgWnd->fpMsgWndProcess = MsgWnd_Process;
    pMsgWnd->m_nWndID = MsgWndMrg_AddInfo(g_pWndMrg, pMsgWnd);

    return pMsgWnd->m_nWndID;
}

int MsgWnd_Release(SMsgWnd *pMsgWnd)
{
    if (IsWindow(pMsgWnd->m_hWnd))
        DestroyWindow(pMsgWnd->m_hWnd);

    pMsgWnd->m_pThis = NULL;

    if(g_pWndMrg == NULL)
        return 0;

    MsgWndMrg_DeleteInfo(g_pWndMrg, pMsgWnd->m_nWndID);

    if(MsgWndMrg_GetCount(g_pWndMrg) <= 0)
    {
        MsgWndMrg_Release(g_pWndMrg);
        g_pWndMrg = NULL;
    }
    return 0;
}

HWND MsgWnd_CreateWindow(SMsgWnd *pMsgWnd, HINSTANCE hInstance, WORD nResourceID, WORD nIconID)
{
    pMsgWnd->m_hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(nIconID));
    pMsgWnd->m_hWnd = CreateDialogParam(hInstance, MAKEINTRESOURCE(nResourceID),
        pMsgWnd->m_hwndParent, (DLGPROC)MsgWndProc, (LPARAM)pMsgWnd);
    return pMsgWnd->m_hWnd;
}

INT_PTR MsgWnd_DoModule(SMsgWnd *pMsgWnd, HINSTANCE hInstance, WORD nResourceID, WORD nIconID)
{
    pMsgWnd->m_hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(nIconID));
    return DialogBoxParam(hInstance, MAKEINTRESOURCE(nResourceID),
        pMsgWnd->m_hwndParent, (DLGPROC)MsgWndProc, (LPARAM)pMsgWnd);
}

void MsgWnd_Move(SMsgWnd *pMsgWnd, int nLeft, int nTop, int nWidth, int nHeight)
{
    MoveWindow(pMsgWnd->m_hWnd, nLeft, nTop, nWidth, nHeight, TRUE);
}

void MsgWnd_MoveLe(SMsgWnd *pMsgWnd, int nLeft, int nTop)
{
    RECT rc;
    GetWindowRect(pMsgWnd->m_hWnd, &rc);
    rc.right -= rc.left;
    rc.bottom -= rc.top;
    MoveWindow(pMsgWnd->m_hWnd, nLeft, nTop, rc.right, rc.bottom, TRUE);
}

//////////////////////////////////////////////////////////////////////
INT_PTR MsgWnd_Process(SMsgWnd *pMsgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY: MsgWnd_Destroy(pMsgWnd); break;
    case WM_MOVE: MsgWnd_OnMove(pMsgWnd, (short)LOWORD(lParam), (short)HIWORD(lParam)); break;
    case WM_SIZE: MsgWnd_OnSize(pMsgWnd, wParam, (short)LOWORD(lParam), (short)HIWORD(lParam)); break;
    case WM_PAINT: MsgWnd_Paint(pMsgWnd); break;
    case WM_CLOSE: MsgWnd_Close(pMsgWnd); break;
    case WM_COPYDATA: return MsgWnd_CopyData(pMsgWnd, wParam, (COPYDATASTRUCT*)lParam);
    case WM_NOTIFY: return MsgWnd_Notify(pMsgWnd, (int)wParam, (LPNMHDR)lParam);
    case WM_INITDIALOG: return MsgWnd_InitDialog(pMsgWnd, (HWND)wParam);
    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case BN_CLICKED: MsgWnd_BnClicked(pMsgWnd, LOWORD(wParam), (HWND)lParam); break;
        default: break;
        }
        MsgWnd_Command(pMsgWnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam); break;
    case WM_TIMER: MsgWnd_Timer(pMsgWnd, (int)wParam, (TIMERPROC)lParam); break;
    default: return MsgWnd_UserMsg(pMsgWnd, uMsg, wParam, lParam);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////

BOOL MsgWnd_InitDialog(SMsgWnd *pMsgWnd, HWND hwndCtlFocus)
{
    if(pMsgWnd->m_hIcon != NULL)
    {
        SendMessage(pMsgWnd->m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)pMsgWnd->m_hIcon);
        SendMessage(pMsgWnd->m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)pMsgWnd->m_hIcon);
    }
    if(pMsgWnd->fpOnInitDialog)
        return pMsgWnd->fpOnInitDialog(pMsgWnd, hwndCtlFocus);
    else
        return FALSE;
}

void MsgWnd_Paint(SMsgWnd *pMsgWnd)
{
    HDC hdc = NULL;
    PAINTSTRUCT ps;

    hdc = BeginPaint(pMsgWnd->m_hWnd, &ps);
    // TODO: Add any drawing code here...
    if(pMsgWnd->fpOnPaint)
        pMsgWnd->fpOnPaint(pMsgWnd, hdc);
    // end TODO
    EndPaint(pMsgWnd->m_hWnd, &ps);
}

void MsgWnd_Command(SMsgWnd *pMsgWnd, WORD wCtlNotifyCode, WORD wCtlID, HWND hwndCtl)
{
    if(pMsgWnd->fpOnCommand)
        pMsgWnd->fpOnCommand(pMsgWnd, wCtlNotifyCode, wCtlID, hwndCtl);
}

INT_PTR MsgWnd_UserMsg(SMsgWnd *pMsgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if(pMsgWnd->fpOnUserMsg)
        return pMsgWnd->fpOnUserMsg(pMsgWnd, uMsg, wParam, lParam);
    else
        return 0;
}

void MsgWnd_Destroy(SMsgWnd *pMsgWnd)
{
    if(pMsgWnd->fpOnDestroy)
        pMsgWnd->fpOnDestroy(pMsgWnd);
    pMsgWnd->m_hWnd = NULL;
    //PostQuitMessage(0);
}

void MsgWnd_Close(SMsgWnd *pMsgWnd)
{
    if(pMsgWnd->fpOnClose)
        pMsgWnd->fpOnClose(pMsgWnd);
    EndDialog(pMsgWnd->m_hWnd, IDCANCEL);
}

void MsgWnd_Timer(SMsgWnd *pMsgWnd, int wTimerID, TIMERPROC tmprc)
{
    if(pMsgWnd->fpOnTimer)
        pMsgWnd->fpOnTimer(pMsgWnd, wTimerID, tmprc);

    if(tmprc)
        tmprc(pMsgWnd->m_hWnd, WM_TIMER, wTimerID, GetTickCount());
}

INT_PTR MsgWnd_Notify(SMsgWnd *pMsgWnd, int idCtrl, LPNMHDR pnmh)
{
    if(pMsgWnd->fpOnNotify)
        return pMsgWnd->fpOnNotify(pMsgWnd, idCtrl, pnmh);
    else
        return 0;
}

BOOL MsgWnd_BnClicked(SMsgWnd *pMsgWnd, WORD wCtlID, HWND hwndCtl)
{
    if(pMsgWnd->fpOnBnClicked)
    {
        pMsgWnd->fpOnBnClicked(pMsgWnd, wCtlID, hwndCtl);
        return TRUE;
    }
    else
        return FALSE;
}

BOOL MsgWnd_CopyData(SMsgWnd *pMsgWnd, WPARAM hData, COPYDATASTRUCT *pCopyData)
{
    if(pMsgWnd->fpOnCopyData)
    {
        pMsgWnd->fpOnCopyData(pMsgWnd, hData, pCopyData);
        return TRUE;
    }
    else
        return FALSE;
}

void MsgWnd_OnMove(SMsgWnd *pMsgWnd, int xPos, int yPos)
{
    if(pMsgWnd->fpOnMove)
    {
        pMsgWnd->fpOnMove(pMsgWnd, xPos, yPos);
    }
}

void MsgWnd_OnSize(SMsgWnd *pMsgWnd, UINT nType, int cx, int cy)
{
    if(pMsgWnd->fpOnSize)
    {
        pMsgWnd->fpOnSize(pMsgWnd, nType, cx, cy);
    }
}

//////////////////////////////////////////////////////////////////////////

LRESULT ComboBox_GetSelectData(HWND hwndCtl)
{
    return ComboBox_GetItemData(hwndCtl, ComboBox_GetCurSel(hwndCtl));
}

int ComboBox_SetSelectByData(HWND hwndCtl, LRESULT theData)
{
    int i;
    for(i=0; i<ComboBox_GetCount(hwndCtl); i++)
    {
        if(ComboBox_GetItemData(hwndCtl, i) == theData)
            return i;
    }
    return -1;
}

LRESULT ListBox_GetSelectData(HWND hwndCtl)
{
    return ListBox_GetItemData(hwndCtl, ListBox_GetCurSel(hwndCtl));
}

int ListBox_SetSelectByData(HWND hwndCtl, LRESULT theData)
{
    int i;
    for(i=0; i<ListBox_GetCount(hwndCtl); i++)
    {
        if(ListBox_GetItemData(hwndCtl, i) == theData)
            return i;
    }
    return -1;
}

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////
// class CMsgWnd

INT_PTR CMsgWnd::OnMsgWndProcess(SMsgWnd *pMsgWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMsgWnd *pThis = (CMsgWnd *)pMsgWnd;
    HDC hdc = NULL;
    PAINTSTRUCT ps;

    switch (uMsg)
    {
    case WM_DESTROY: pThis->OnDestroy(); break;
    case WM_MOVE: pThis->OnMove((short)LOWORD(lParam), (short)HIWORD(lParam)); break;
    case WM_SIZE: pThis->OnSize(wParam, (short)LOWORD(lParam), (short)HIWORD(lParam)); break;
    case WM_PAINT:
        hdc = BeginPaint(pThis->m_hWnd, &ps);
        pThis->OnPaint(hdc);
        EndPaint(pThis->m_hWnd, &ps);
        break;
    case WM_CLOSE: pThis->OnClose(); break;
    case WM_COPYDATA: return pThis->OnCopyData(wParam, (COPYDATASTRUCT*)lParam);
    case WM_NOTIFY:return pThis->OnNotify((int)wParam, (LPNMHDR)lParam);
    case WM_INITDIALOG: return pThis->OnInitDialog((HWND)wParam);
    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case BN_CLICKED: pThis->OnBnClicked(LOWORD(wParam), (HWND)lParam); break;
        default: break;
        }
        pThis->OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam); break;
    case WM_TIMER: pThis->OnTimer((int)wParam, (TIMERPROC)lParam); break;
    default: return pThis->OnUserMsg(uMsg, wParam, lParam);
    }
    return 0;
}

CMsgWnd* CMsgWnd::FromWnd(HWND hWnd)
{
    return (CMsgWnd*)MsgWnd_FromWnd(hWnd);
}

void CMsgWnd::InitCommCtrl()
{
    //InitCommonControls();
}

CMsgWnd::CMsgWnd(HWND hwndParent/* = NULL*/)
{
    InitMsgWnd(this);
    m_hwndParent = hwndParent;
    fpMsgWndProcess = OnMsgWndProcess;
    MsgWnd_AddWnd(this);
}

CMsgWnd::~CMsgWnd(void)
{
    MsgWnd_Release(this);
}

HWND CMsgWnd::CreateWnd(HINSTANCE hInstance, WORD nResourceID, WORD nIconID)
{
    return MsgWnd_CreateWindow(this, hInstance, nResourceID, nIconID);
}

INT_PTR CMsgWnd::ModuleWnd(HINSTANCE hInstance, WORD nResourceID, WORD nIconID)
{
    return MsgWnd_DoModule(this, hInstance, nResourceID, nIconID);
}

CMsgWnd::operator HWND()
{
    return m_hWnd;
}

void CMsgWnd::MoveWindow(int nLeft, int nTop)
{
    MsgWnd_MoveLe(this, nLeft, nTop);
}

void CMsgWnd::MoveWindow(int nLeft, int nTop, int nWidth, int nHeight)
{
    MsgWnd_Move(this, nLeft, nTop, nWidth, nHeight);
}

HWND CMsgWnd::GetDlgItem(int nIDDlgItem)
{
    return ::GetDlgItem(m_hWnd, nIDDlgItem);
}

UINT CMsgWnd::GetDlgItemInt(int nIDDlgItem, BOOL bSigned)
{
    return ::GetDlgItemInt(m_hWnd, nIDDlgItem, NULL, bSigned);
}

BOOL CMsgWnd::SetDlgItemInt(int nIDDlgItem, UINT nValue, BOOL bSigned)
{
    return ::SetDlgItemInt(m_hWnd, nIDDlgItem, nValue, bSigned);
}

UINT CMsgWnd::GetDlgItemText(int nIDDlgItem, LPSTR lpString, int nMaxCount)
{
    return ::GetDlgItemText(m_hWnd, nIDDlgItem, lpString, nMaxCount);
}

BOOL CMsgWnd::SetDlgItemText(int nIDDlgItem, LPCSTR lpString)
{
    return ::SetDlgItemText(m_hWnd, nIDDlgItem, lpString);
}

//////////////////////////////////////////////////////////////////////////

void CMsgWnd::OnCommand(WORD wCtlNotifyCode, WORD wCtlID, HWND hwndCtl){}

BOOL CMsgWnd::OnInitDialog(HWND hwndCtlFocus)
{
    if(m_hIcon != NULL)
    {
        SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
        SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIcon);
    }
    return FALSE;
}

void CMsgWnd::OnPaint(HDC hDC){}

INT_PTR CMsgWnd::OnUserMsg(UINT uMsg, WPARAM wParam, LPARAM lParam){ return 0; }

void CMsgWnd::OnDestroy()
{
    m_hWnd = NULL;
    //PostQuitMessage(0);
}
void CMsgWnd::OnMove(int xPos, int yPos){};
void CMsgWnd::OnSize(UINT nType, int cx, int cy){};
void CMsgWnd::OnClose(){ EndDialog(m_hWnd, IDCANCEL); }

void CMsgWnd::OnTimer(int wTimerID, TIMERPROC tmprc)
{
    if(tmprc) tmprc(m_hWnd, WM_TIMER, wTimerID, GetTickCount());
}

INT_PTR CMsgWnd::OnNotify(int idCtrl, LPNMHDR pnmh){ return 0; }

void CMsgWnd::OnBnClicked(WORD wCtlID, HWND hwndCtl){}

BOOL CMsgWnd::OnCopyData(WPARAM hData, COPYDATASTRUCT *pCopyData){ return FALSE; }
