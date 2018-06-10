

/***********************************************************************************************************************

Notice:
This document contains information that is proprietary to RADVISION LTD..
No part of this publication may be reproduced in any form whatsoever without
written prior approval by RADVISION LTD..

RADVISION LTD. reserves the right to revise this publication and make changes
without obligation to notify any person of such revisions or changes.

*************************************************************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "rvsocket.h"
#include "rvcbase.h"

#include "TRTSP_init.h"
#include "TRTSP_general.h"

#ifdef USE_TCL
static int tclThreadId; /* ThreadId of the thread that is running the TCL interp */

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#define TCL_COMMAND (WM_USER+200) /* Event used to update the GUI */
static HWND tclEventsWindow = NULL; /* Window that handles the TCL events for updating the GUI */

#else
#include <unistd.h>

int tclPipefd[2]; /* Pipe to use for synchronization of GUI events */
#endif /* RV_OS_TYPE_WIN32 */
#endif /* USE_TCL */

/****************************************************************************
 *                        TYPE DEFINITIONS                                  *
 ****************************************************************************/

#define APP_SPLIT_MAX_ARGC 40

/****************************************************************************
 *                           GLOBALS                                        *
 ****************************************************************************/

RvSocket splitSock;

/********************************************************************************************
 *                                                                                          *
 *                                  Private functions                                       *
 *                                                                                          *
 ********************************************************************************************/

#ifdef USE_TCL
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)

/* GUI handling window */
static LRESULT CALLBACK appTclGuiWinFunc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_CLOSE:
        {
            /* Closing the window - just kill the damn timer and the window */
            KillTimer(hWnd, 1);
            DestroyWindow(hWnd);
            break;
        }

        case TCL_COMMAND:
        {
            /* Execute the TCL command and deallocate the memory it used */
            char *cmd = (char *)wParam;


            if (Tcl_GlobalEval(gInterp, cmd) != TCL_OK)
                PutError("ERROR: GlobalEval", cmd);
            free(cmd);
            break;
        }

        default:
            return DefWindowProc(hWnd,uMsg,wParam,lParam);
    }

    return 0L;
}

#else
/* We use a pipe event for GUI synchronization */

/* GUI handling window */
static void appTclGuiFunc(IN ClientData clientData, IN int mask)
{
    char* message;

    RV_UNUSED_ARG(clientData);
    RV_UNUSED_ARG(mask);

    /* Read message from the pipe */
    if (read(tclPipefd[0], &message, sizeof(message)) == sizeof(message))
    {
        if (Tcl_GlobalEval(gInterp, message) != TCL_OK)
        {
            PutError("ERROR: GlobalEval", message);
        }
        free(message);
    }
}
#endif /* RV_OS_TYPE_WIN32 */
#endif /* USE_TCL */

/****************************************************************************
 *                        MODULE FUNCTIONS                                  *
 ****************************************************************************/

#ifdef USE_TCL

/******************************************************************************************
 * TclInit
 * purpose : Initialize the TCL interface
 * input   : none
 * output  : none
 * return  : TCL_OK - the command was invoked successfully.
 ******************************************************************************************/
int TclInit(void)
{
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    /* Create Window that we will use to synchronize Tcl commands */
    WNDCLASS wc;
    memset (&wc, 0, sizeof (WNDCLASS));
    wc.lpfnWndProc    = appTclGuiWinFunc;
    wc.hInstance      = NULL;
    wc.lpszClassName  = "appTclGui";

    RegisterClass(&wc);
    tclEventsWindow = CreateWindow("appTclGui", NULL,WS_OVERLAPPED  | WS_MINIMIZE,
        0, 0, 0, 0, NULL, NULL, NULL, NULL);

#else
    /* Use TCL for events generation through a pipe */
    pipe(tclPipefd);

    Tcl_CreateFileHandler(tclPipefd[0], TCL_READABLE, appTclGuiFunc, NULL);

#endif

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    tclThreadId = (int)GetCurrentThreadId();

#elif (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || (RV_OS_TYPE == RV_OS_TYPE_LINUX)
    tclThreadId = (int)pthread_self();

#else
/*#error Implement for OS!*/
#endif

    return TCL_OK;
}

/******************************************************************************************
 * TclEnd
 * purpose : Deinitialize the TCL interface
 * input   : none
 * output  : none
 * return  : TCL_OK - the command was invoked successfully.
 ******************************************************************************************/
int TclEnd(void)
{
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    /* Kill the associated window if there is one */
    if (tclEventsWindow != NULL)
    {
        SendMessage(tclEventsWindow, WM_CLOSE, (WPARAM)0, (LPARAM)0);
        tclEventsWindow = NULL;
    }

    UnregisterClass("appTclGui", NULL);

#else
    close(tclPipefd[0]);
    close(tclPipefd[1]);

#endif

    return RV_OK;
}
#endif /* USE_TCL */

/****************************************************************************
 * TclExecute
 * -------------------------------------------------------------------------
 * General: 
 *  Execute a command in tcl
 * 
 * Arguments:
 * Input:   cmd     - the command that is going to be executed
 * Output:  None
 * 
 * Return Value:  RV_OK - the command was invoked successfully.
 ****************************************************************************/
int TclExecute(const char* cmd, ...)
{
    va_list v;
    char ptr[2048];

    va_start(v, cmd);
    vsprintf(ptr, cmd, v);
    va_end(v);

#ifdef USE_TCL
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    if (tclThreadId == (int)GetCurrentThreadId())
#elif (RV_OS_TYPE == RV_OS_TYPE_SOLARIS) || (RV_OS_TYPE == RV_OS_TYPE_LINUX)
    if (tclThreadId == (int)pthread_self())
#endif
    {
        if (Tcl_GlobalEval(gInterp, ptr) != TCL_OK)
        {
            if ((strlen(ptr) + strlen(Tcl_GetStringResult(gInterp))) < sizeof(ptr))
                sprintf(ptr + strlen(ptr), ": %s", Tcl_GetStringResult(gInterp));
            PutError("ERROR: GlobalEval", ptr);
        }
    }
    else
    {
        /* Seems like we're not in the right thread... */
        char *message;
        RvSize_t len = strlen(ptr);
        message = (char *)malloc(len + 1);
        if (message != NULL)
            memcpy(message, ptr, len + 1);

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32) || (RV_OS_TYPE == RV_OS_TYPE_WINCE)
        PostMessage(tclEventsWindow, TCL_COMMAND, (WPARAM)message, (LPARAM)0);

#else
        write(tclPipefd[1], &message, sizeof(message));
#endif
    }
#endif

#ifdef SPLIT_APP
    rtspSplitAppTclExecute(ptr);
#endif

    return RV_OK;
}

#ifdef USE_TCL
/******************************************************************************************
 * TclGetVariable
 * purpose : get variable from tcl
 * input   : varName - the name of the variable that is going to be imported
 * input   : none
 * output  : none
 * return  : The variable's string value
 ******************************************************************************************/
char* TclGetVariable(const char* varName)
{
    char arrayName[25];
    char indexName[128];
    char* token;
    char* result;

    /* See if it's an array variable */
    token = (char *)strchr(varName, '(');

    if (token == NULL)
        result = Tcl_GetVar(gInterp, (char *)varName, TCL_GLOBAL_ONLY);
    else
    {
        /* Array - let's first split it into array name and index name */
        RvSize_t arrayLen = (RvSize_t)(token - varName);
        RvSize_t indexLen = strlen(token+1)-1;

        if ((arrayLen >= sizeof(arrayName)) || (indexLen >= sizeof(indexName)) ||
            (arrayLen > strlen(varName)))
        {
            PutError(varName, "Length of array name or index out of bounds in GetVar");
            return (char *)"-unknown-";
        }

        memcpy(arrayName, varName, arrayLen);
        arrayName[arrayLen] = '\0';
        memcpy(indexName, token+1, indexLen);
        indexName[indexLen] = '\0';
        result = Tcl_GetVar2(gInterp, arrayName, indexName, TCL_GLOBAL_ONLY);
    }

    if (result == NULL)
    {
        PutError(varName, Tcl_GetStringResult(gInterp));
        return (char *)"-unknown-";
    }

    return result;
}
#endif

/******************************************************************************
 *                      SPLIT APPLICATION FUNCTIONS                           *
 ******************************************************************************/

#ifdef SPLIT_APP
/******************************************************************************
 * rtspSplitAppParseCommand
 * ----------------------------------------------------------------------------
 * General: 
 *  Parse a command line based on arguments surrounded by {}.
 *
 * Arguments:
 * Input:  cmd     - Command buffer.
 *         cmdLen  - the command length.
 * Output: argv    - Array of arguments of size SPLIT_APP_MAX_ARGC.
 *
 * Return Value: The number of arguments parsed.
 *****************************************************************************/
static RvUint rtspSplitAppParseCommand(
    IN  char *              cmd,
    IN  RvUint              cmdLen,
    OUT char *              argv[APP_SPLIT_MAX_ARGC])
{
    RvUint argc = 0, brackCount = 0, cmdInd;

    for (cmdInd = 0; cmdInd < cmdLen; cmdInd++)
    {
        if (cmd[cmdInd] == '{')
        {
            if (brackCount == 0)
            {
                argv[argc] = (cmd + cmdInd + 1);
            }
            brackCount++;
        }
        if (cmd[cmdInd] == '}')
        {
            if (brackCount == 0)
                return 0;
            brackCount--;
            if (brackCount == 0)
            {
                cmd[cmdInd] = '\0';
                argc++;
            }
        }
    }
    return argc;
}


/******************************************************************************
 * rtspSplitAppListen
 * ----------------------------------------------------------------------------
 * General: 
 *  To wait for an incoming connection we have to open a client. yes, I
 *  know it's silly, but that's the way it is. This should be called
 *  from the C side.
 * 
 * Arguments:
 * Input:  None
 * Output: None
 *
 * Return Value: RV_OK on success
 *               Other on failure.
 *****************************************************************************/
RvStatus rtspSplitAppListen()
{
    RvStatus status;
    RvSocket serv, s;
    RvAddress addr, fromAddr;
    RvBool done = RV_FALSE;
    char msg[1024];
    RvSize_t i = 0, len = 0, offset = 0;
    char * argv[APP_SPLIT_MAX_ARGC];
    RvUint argc;
    RvUint16 guiPort;
    RvUint nesting;

    guiPort = 3049;
	
    status = RvSocketConstruct(RV_ADDRESS_TYPE_IPV4, RvSocketProtocolTcp, NULL, &serv);	
    if (status != RV_OK)
    {
        printf("RvSocketConstruct failed, status (%d) \n", status);
        return status;
    }

    RvAddressConstruct(RV_ADDRESS_TYPE_IPV4, &addr);
    RvAddressSetIpPort(&addr, guiPort);

    status = RvSocketBind(&serv, &addr, NULL, NULL);

    if (status == RV_OK)
        status = RvSocketListen(&serv, 16, NULL);

    if (status != RV_OK)
    {
        RvSocketDestruct(&serv, RV_TRUE, NULL, NULL);
        printf("Could not bind on listen on port (%d)\n", guiPort);
        return status;
    }

    printf("Listening for GUI...\n");

    RvSocketSetBlocking(&serv, RV_TRUE, NULL);
    RvSocketAccept(&serv, NULL, &s, &fromAddr);
    RvSocketDestruct(&serv, RV_TRUE, NULL, NULL);

    puts("\n---===<<<Received connection from TCL part>>>===---\n");
    splitSock = s;

    TclExecute("test:Log {Connection established};update");

    while (!done)
    {
        if (i == len)
        {
            status = RvSocketReceiveBuffer(&s, (RvUint8*)msg+offset, sizeof(msg)-offset, NULL, &len, &fromAddr);
            if (status == RV_SOCKET_ERROR_EOF)
            {
                puts("\n---===<<<Lost connection to TCL part>>>===---\n");
                break;
            }
            if ((status != RV_OK) || (len == 0))
            {
                continue;
            }
            len += offset;
        }
        
        nesting = 0;
        for (i=0;i<len;i++)
        {
            if (msg[i] == '{')
                nesting++;
            else if (msg[i] == '}')
                nesting--;
            else if ((nesting == 0) && (msg[i] == '\n'))
                break;
            else if (msg[i] == '\r')
                msg[i] = ' ';
        }
        if (i == len)
        {
            offset = i;
            i = len = 0;
            continue;
        }
        msg[i] = '\0';
        puts(msg);

        /* check command type */
        if (strncmp(msg, "varUpd", 6) == 0)
        {
            argc = rtspSplitAppParseCommand(msg+7, strlen(msg+7), argv);
            if (argc == 2)
            {
                rtspUpdateVariable(argv[0], argv[1]);
            }
        }
        else if (strncmp(msg, "callCmd", 7) == 0)
        {
            argc = rtspSplitAppParseCommand(msg+8, strlen(msg+8), argv);
            if (argc > 0)
            {
                if (strcmp(argv[0], "test.Quit") == 0)
                {
                    done = RV_TRUE;
                   
                }
                rtspCallFunction(argv[0], argc, argv);
            }
        }

        if (i < (len - 1))
        {
            len -= (i + 1);
            memmove(msg, msg+i+1, len);
            i = 0;
        }
        else
        {
            i = offset = len = 0;
        }
    }
    RvSocketDestruct(&s, RV_TRUE, NULL, NULL);
	RvCBaseEnd();

    return RV_OK;
}


/******************************************************************************
 * rtspSplitAppTclExecute
 * ----------------------------------------------------------------------------
 * General: 
 *  Send a variable update command from the TCL side to the C side.
 *
 * Arguments:
 * Input:  cmd          - The TCL command string.
 * Output: None.
 *
 * Return Value: RV_OK on success, other on failure.
 *****************************************************************************/
RvStatus rtspSplitAppTclExecute(
    IN  char *              cmd)
{
    RvSocketSendBuffer(&splitSock, (RvUint8*)cmd, strlen(cmd), NULL, NULL, NULL);
    RvSocketSendBuffer(&splitSock, (RvUint8*)"\n", 1, NULL, NULL, NULL);
    return RV_OK;
}
#endif /* SPLIT_APP */



#ifdef __cplusplus
}
#endif

