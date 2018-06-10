/************************************************************************
        Copyright (c) 2003 RADVISION Inc. and RADVISION Ltd.
************************************************************************
NOTICE:
This document contains information that is confidential and proprietary
to RADVISION Inc. and RADVISION Ltd.. No part of this document may be
reproduced in any form whatsoever without written prior approval by
RADVISION Inc. or RADVISION Ltd..

RADVISION Inc. and RADVISION Ltd. reserve the right to revise this
publication and make changes without obligation to notify any person of
such revisions or changes.
***********************************************************************/

/********************************************************************************************
 *                                osDependency.c
 *
 * Operating Systems dependent code needed for test applications.
 * This code allows better execution of our test applications in all the OSs we support
 * without making them too big to handle.
 *
 *
 *
 ********************************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

#include "osDependency.h"

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#include <process.h>
#endif

/*-----------------------------------------------------------------------*/
/*                        STATIC FUNCTIONS PROTOTYPES                    */
/*-----------------------------------------------------------------------*/

/* The RtpSessionMain() function should be defined by the running application itself */
int RtpSessionMain(int argc, char* argv[]);







/*-----------------------------------------------------------------------*/
/*                        OS-SPECIFIC FUNCTIONS                          */
/*-----------------------------------------------------------------------*/


#if (RV_OS_TYPE == RV_OS_TYPE_WINCE)
/*---------------------------- Windows CE -------------------------------*/

static HWND hWnd; /* window to print messages to */

/* New print function - used instead of printf() since we have no terminal */
int ceprintf(const char* string, ...)
{
    static int line = 0;
    va_list v;
    static char message[256];
    static unsigned short wMessage[256];
    int i, j;

    memset(message, 0, sizeof(message));

    va_start(v, string);
    vsprintf(message, string, v);
    va_end(v);

    for (i=0; message[i]; i+=j)
    {
        for (j=0; ;j++)
        {
            if ((message[i+j] == '\n') || (message[i+j] == '\0'))
            {
                memset(wMessage, 0, sizeof(wMessage));
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (message+i), j, wMessage, 2048);
                SendMessage(hWnd, LB_ADDSTRING, (WPARAM)0, (LPARAM)wMessage);
                if (line > 20)
                    SendMessage(hWnd, LB_DELETESTRING, 0, 0);
                else
                    line++;
                if (message[i+j] == '\n')
                    j++;
                break;
            }
        }
    }

    UpdateWindow(hWnd);

    return 1;
}


WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    int status;

    /* Make sure we create the window for our printf messages */
    hWnd = CreateWindow(TEXT("LISTBOX"), TEXT("TestApp"), WS_SYSMENU|WS_VISIBLE|LBS_NOSEL ,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    status = RtpSessionMain(0, NULL);

    /* WinCE - we should probably wait until user closes the application on his own */
    {
        MSG msg;
        BOOL bRet;

        while( (bRet = GetMessage( &msg, NULL, 0, 0 )) != 0)
        {
            if (bRet == -1)
                break;
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return status;
}




#else
/*---------------------------- Windows -------------------------------*/

/*---------------------------- Others -------------------------------*/
/* Operating Systems supporting main() as their initial function */
#if (RV_OS_TYPE != RV_OS_TYPE_VXWORKS && !RV_ANDROID && RV_OS_TYPE != RV_OS_TYPE_IPHONE)
int main(int argc, char* argv[])
{
    return RtpSessionMain(argc, argv);
}
#endif /* (RV_OS_TYPE != RV_OS_TYPE_VXWORKS) */
#endif /* (RV_OS_TYPE == RV_OS_TYPE_WINCE) */

/***************************************************************************
* advancedIceSample
* ------------------------------------------------------------------------
* General: The main function for Android & iOS. 
***************************************************************************/
void rtpSession(int bStart)
{
    if(bStart)
    {
        RtpSessionMain(0, NULL);
    }
}

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)

RvBool goThread = RV_TRUE;

unsigned __stdcall keyboardInputThread(void * argv)
{
    int c;
    RvRtpSeliCallback cb = (RvRtpSeliCallback)argv;

    while (goThread)
    {
        if ((c = getc(stdin)) != EOF)
        {
            ungetc(c, stdin);
            cb(0, (RvRtpSeliEvents)0, RV_FALSE);
        }
    }
    _endthreadex(0);
    return 0;
}

#endif /* (RV_OS_TYPE == RV_OS_TYPE_WIN32) */

void setKeyboardInput(RvRtpSeliCallback cb)
{
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    DWORD keyboardInputThreadId;
    _beginthreadex(NULL,0, keyboardInputThread, (void *)cb, 0, &keyboardInputThreadId);
#else
    RvRtpSeliCallOn(0, RvRtpSeliEvRead, cb);
#endif
}

void endKeyboardInput(void)
{
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    goThread = RV_FALSE;
#else
    RvRtpSeliCallOn(0, (RvRtpSeliEvents)0, NULL);
#endif
}

#ifdef __cplusplus
}
#endif

