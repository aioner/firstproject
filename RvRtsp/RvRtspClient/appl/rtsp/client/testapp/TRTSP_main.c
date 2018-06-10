/*
 ********************************************************************************
 *                                                                              *
 * NOTICE:                                                                      *
 * This document contains information that is confidential and proprietary to   *
 * RADVision LTD.. No part of this publication may be reproduced in any form    *
 * whatsoever without written prior approval by RADVision LTD..                 *
 *                                                                              *
 * RADVision LTD. reserves the right to revise this publication and make changes*
 * without obligation to notify any person of such revisions or changes.        *
 ********************************************************************************
*/

/********************************************************************************
 *                               <TRTSP_main.c>
 *
 * This file is the main file of the Test Application.
 * This file generates the GUI that was written in TCL/TK.
 * It also creates new TCL commands.
 *******************************************************************************/


/*****************************************************************
 * DEFINITIONS for ADDONS
 * ----------------------
 * USE_         -
 *****************************************************************/

#define USE_GRANULAR_SELECT 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "rvconfig.h"

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#pragma warning (push,3)
#include <windows.h>
#include <shellapi.h>
#include <locale.h>
#pragma warning (pop)
#else

#if (USE_GRANULAR_SELECT == 1)
#include <sys/time.h>
#if (RV_SELECT_TYPE == RV_SELECT_POLL)
#include <sys/poll.h>
#endif
#endif

#if (RV_OS_TYPE == RV_OS_TYPE_LINUX) 
#include <sys/resource.h>
#endif


#endif  /* WIN32 */

#include "TRTSP_general.h"
#include "TRTSP_init.h"
#include "TRTSP_testapp.h"

#ifdef USE_TCL
Tcl_Interp* interp;
#endif

/********************************************************************************************
 * main
 * -----------------------------------------------------------------------------------------
 * General: 
 *  the main function of the test application.
 *
 * Arguments: 
 * Input:  argc - number of parameters entered to main
 *         argv - parameters entered to test application
 * Output: None
 * 
 * Return Value: None.
 ********************************************************************************************/
#if (RV_OS_TYPE == RV_OS_TYPE_VXWORKS)
int rtspmain(void)
#else
int main(int argc, char *argv[])
#endif
{
#ifdef USE_TCL
#if (RV_OS_TYPE != RV_OS_TYPE_WIN32)
    RvStatus status = RV_OK;
#endif

    char verStr[60];
    char* reason = NULL;
#endif

    RV_UNUSED_ARG(argc);
    RV_UNUSED_ARG(argv);

#ifdef USE_TCL
    sprintf(verStr, "%s", "RTSP Stack Version 2.0.0.2  Copyright (c) 2006 RADVISION");
    interp = InitTcl(argv[0], verStr, &reason);

    InitApp();

    InitStack();

    TclExecute("update; wm geometry .test $app(test,size); wm deiconify .test; update");
    TclExecute("after 100 {wm deiconify .test}");
    TclExecute("after 500 {wm withdraw .; notebook:refresh test}");

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    Tk_MainLoop();
#else
    while ((status == RV_OK) && !IsGuiStopped())
    {
        RvBool notFinished = RV_TRUE;

        status = TestStackLoop();

        /* Handle GUI events until we don't have any */
        while (notFinished)
        {
            if (!Tk_DoOneEvent(TK_DONT_WAIT))
                notFinished = RV_FALSE; /* no more events */
        }
    }
#endif

#endif

#ifdef SPLIT_APP
    /* Call the network functions */
	InitSplitAppNetwork();
#endif

    return 0;
}

#if defined (WIN32) && defined (_WINDOWS)

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int nullLoc;
    int argc;
    char *cmdLine;
    char *argv[2];

    RV_UNUSED_ARG(nCmdShow);
    RV_UNUSED_ARG(hPrevInstance);
    RV_UNUSED_ARG(hInstance);

    argc = 1;
    if ((lpCmdLine != NULL) && *lpCmdLine)
        argc = 2;

    /* Find out the executable's name */
    setlocale(LC_ALL, "C");
    cmdLine = GetCommandLine();
    if (strlen(lpCmdLine) > 0)
    {
        nullLoc = strlen(cmdLine) - strlen(lpCmdLine) - 1;
        cmdLine[nullLoc] = '\0';
    }

    argv[0] = cmdLine;
    argv[1] = lpCmdLine;

    /* Deal with it as if we're running a console or Unix application */
    return main(argc, argv);
}



#endif  /* Win32 App */




#ifdef __cplusplus
}
#endif


