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
 *                               <TRTSP_init.h>
 *
 * This file include functions that initiate the stack and the tcl.
 *
 *******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include<stdio.h>

#include "TRTSP_init.h"
#include "TRTSP_general.h"
#include "TRTSP_testapp.h"

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
#include <process.h>
#else
#include<pthread.h>
#include<unistd.h>
#endif


/*-----------------------------------------------------------------------*/
/*                          MACRO DEFINITIONS                            */
/*-----------------------------------------------------------------------*/

#define TCL_FILENAME    "TRTSP_testapp.tcl"
#define PROCESS_EVENTS_INTERVAL (50)
#define TCL_LIBPATH     "lib/tcl8.3"
#define TK_LIBPATH      "lib/tk8.3"

#ifdef USE_TCL
Tcl_Interp *gInterp;

void UpdateTclVariable(void);

typedef int (*rtspTclProc)(int argc, char *argv[]);

typedef struct
{
    rtspTclProc  pProc; /* Procedure to execute */
    char *      fName; /* Name of the function */
} TclWrapData;


#if (RV_LOGMASK != RV_LOGLEVEL_NONE)

static RvChar * levels[] = { "exp", "err", "wrn", "inf", "dbg", "ent", "lve", "syn", "" };
static RvChar * sources[] = { "ALLOC", "APP", "ARES", "CLOCK", "EHD", "EMA", "EPP",
    "HOST", "LDAP", "LOCK", "RTSP", "MEMORY", "MUTEX", "PORT", "QUEUE", "RA",
    "RCACHE", "RTSP", "SDP", "SCTP", "SELECT", "SEMA4", "SDP", "SNMP", "SOCKET",
    "THREAD", "TIMER", "TIMERMGR", "TIMESTAMP", "TLS", "TM", "" };

#endif

#endif


/********************************************************************************************
 *                                                                                          *
 *                                  Private functions                                       *
 *                                                                                          *
 ********************************************************************************************/


/******************************************************************************
 * AppAllocateResourceId
 * ----------------------------------------------------------------------------
 * General: Request user to allocate a resource identifier for an object.
 *
 * Return Value: Allocated resource on success, negative value on failure.
 * ----------------------------------------------------------------------------
 * Arguments:
 * Input:  rtspApp      - RTSP Client object to use.
 *         resourceType - Type of resource we need id for.
 * Output: None.
 *****************************************************************************/
RvInt32 AppAllocateResourceId(
    IN TestRtspStackStruct *   rtspApp,
    IN RtspClientResourceType  resourceType)
{
    static RvInt32 resCount[3] = {0, 0};
    RvInt32 resValue = 0;

    RV_UNUSED_ARG(rtspApp);

    switch (resourceType)
    {
    case AppResourceSession:
        TclExecute("incr tmp(record,cnt,session)");
        resCount[(int)resourceType]++;
        resValue = resCount[(int)resourceType];
        break;

    case AppResourceConnection:
        TclExecute("incr tmp(record,cnt,connection)");
        resCount[(int)resourceType]++;
        resValue = resCount[(int)resourceType];
        break;

    case AppResourceTransaction:
        TclExecute("incr tmp(record,cnt,transaction)");
        resCount[(int)resourceType]++;
        resValue = resCount[(int)resourceType];
        break;
    }

    return resValue;
}


#ifdef USE_TCL


/********************************************************************************************
 * WrapperFunc
 * purpose : wrapping stack functions - this function is called whenever we wrap a
 *           function for the scripts' use
 * input   : clientData - used for creating new command in tcl
 *           interp - interpreter for tcl commands
 *           argc - number of parameters entered to the new command
 *           argv - the parameters entered to the tcl command
 * output  : none
 * return  : TCL_OK - the command was invoked successfully or TCL_ERROR if not.
 ********************************************************************************************/
int WrapperFunc(ClientData clientData, Tcl_Interp *interp,int argc, char *argv[])
{
    TclWrapData  *wData = (TclWrapData *)clientData;
    int          result = TCL_ERROR;

    if (wData->pProc != NULL)
        result = wData->pProc(argc, argv);

    RV_UNUSED_ARG(interp);
    return result;
}

#define WRAPPER_COMMAND(tclName, cName) {static TclWrapData _data; _data.pProc = cName; _data.fName = tclName; Tcl_CreateCommand(interp, (char *)tclName, WrapperFunc, (ClientData)&_data, (Tcl_CmdDeleteProc *)NULL); }

#define CREATE_COMMAND(tclName, cName) Tcl_CreateCommand(gInterp, (char *)tclName, cName, (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL)

#endif

int test_Start(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    if(gbStackInited == RV_TRUE)
    {
        TclExecute("test:Log {Illegal Operation:Stack already started};update");
        return RV_OK;
    }
    else
    {
        TestStackInitAndStart(argv);
        TclExecute("set app(config,reqSessionId) {}");
    }

    return RV_OK;
}

int test_Stop(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);
    RV_UNUSED_ARG(argv);

    if(gbStackInited == RV_FALSE)
    {
        TclExecute("test:Log {Illegal Operation: Rtsp Stack not started};update");
    }
    else
    {
        TclExecute("test:Log {Stopping Rtsp Stack...};update");
#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
        Sleep(1000);
#else
        sleep(1);
#endif
        gbStopStack   = RV_TRUE;
        gbStackInited = RV_FALSE;
    }
    TclExecute("set app(config,reqSessionId) {}");

    return RV_OK;
}

int test_Restart(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);
    RV_UNUSED_ARG(argv);

    if(gbStackInited == RV_FALSE)
    {
        TclExecute("test:Log {Stack not started:Starting RTSP Stack now};update");
    }
    else
    {
        TclExecute("test:Log {Stopping Rtsp Stack...};update");
        gbStopStack   = RV_TRUE;
        gbStackInited = RV_FALSE;
    }

#if (RV_OS_TYPE == RV_OS_TYPE_WIN32)
    Sleep(1000);
#else
    sleep(1);
#endif
    TestStackInitAndStart(argv);
    TclExecute("set app(config,reqSessionId) {}");

    return RV_OK;
}

int test_SendResponse(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    if(gbStackInited == RV_FALSE)
    {
        TclExecute("test:Log {Illegal Operation: Stack not started};update");
        }
        else
        {
#ifdef USE_TCL
            UpdateTclVariable();
#endif
            TestClientSendResponse(argv);
        }

        return RV_OK;
}

int test_SendDescRequest(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    if(gbStackInited == RV_FALSE)
    {
        TclExecute("test:Log {Illegal Operation: Stack not started};update");
    }
    else
    {
#ifdef USE_TCL
        UpdateTclVariable();
#endif
        TestClientSendDescRequest(argv);
    }
    return RV_OK;
}

int test_ConstructAndSendReq(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    if(gbStackInited == RV_FALSE)
    {
        TclExecute("test:Log {Illegal Operation: Stack not started};update");
    }
    else
    {
#ifdef USE_TCL
        UpdateTclVariable();
#endif
        TestClientSendDescRequest(argv);
    }

    return RV_OK;
}

int test_TestConstructConn(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    if(gbStackInited == RV_FALSE)
    {
        TclExecute("test:Log {Illegal Operation: Stack not started};update");
    }
    else
    {
#ifdef USE_TCL
        UpdateTclVariable();
#endif
        TestClientConstructConn(argv[1]);
    }

    return RV_OK;
}

int test_SendRequest(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    if(gbStackInited == RV_FALSE)
    {
        TclExecute("test:Log {Illegal Operation: Stack not started};update");
    }
    else
    {
#ifdef USE_TCL
        UpdateTclVariable();
#endif
        TestClientSendRequest(argv);
    }

    return RV_OK;
}

int test_DestructConn(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    TestClientConnectionDestruct(atoi(argv[1]));

    return RV_OK;
}

int test_SetUpSession(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

#ifdef USE_TCL
    UpdateTclVariable();
#endif

    TestClientSendSetup(argv);

    return RV_OK;
}

int test_PlaySession(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);
 
    TestClientSessionPlay(atoi(argv[1]), argv[2]);

    return RV_OK;
}

int test_PauseSession(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    TestClientSessionPause(atoi(argv[1]), argv[2]);

    return RV_OK;
}

int test_TeardownSession(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    TestClientSessionTeardown(atoi(argv[1]), argv[2]);

    return RV_OK;
}

int test_UpdateSessList(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);

    TestClientUpdateSessionList(atoi(argv[1]));

    return RV_OK;
}

RvBool bGuiStopped = RV_FALSE;

int test_Quit(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);
    RV_UNUSED_ARG(argv);

    TclExecute("test:Log {Shutting down the application};update");
    TclExecute("init:SaveOptions 0");

    doneTest();

#ifdef USE_TCL
    TclEnd();
    Tcl_DeleteInterp(gInterp);
    exit(0);
#endif

    bGuiStopped = RV_TRUE;
    return RV_OK;
}

RvBool IsGuiStopped(void)
{
    return bGuiStopped;
}


int test_ResetLog(int argc, char *argv[])
{
    RV_UNUSED_ARG(argc);
    RV_UNUSED_ARG(argv);

    return LogFileReset();
}


#ifdef USE_TCL
/********************************************************************************************
 * CreateTclCommands
 * purpose : Create all tcl commands that was written in c language.
 * input   : interp - interpreter for tcl commands
 * output  : none
 * return  : none
 ********************************************************************************************/
void CreateTclCommands(Tcl_Interp* interp)
{
    /********
     * TOOLS
     ********/
    WRAPPER_COMMAND("test.Start", test_Start);
    WRAPPER_COMMAND("test.SendResponse", test_SendResponse);
    WRAPPER_COMMAND("test.SendDescRequest", test_SendDescRequest);
    WRAPPER_COMMAND("test.ConstructAndSendReq", test_ConstructAndSendReq);
    WRAPPER_COMMAND("test.TestConstructConn", test_TestConstructConn);
    WRAPPER_COMMAND("test.SendRequest", test_SendRequest);
    WRAPPER_COMMAND("test.SetUpSession", test_SetUpSession);
    WRAPPER_COMMAND("test.PlaySession", test_PlaySession);
    WRAPPER_COMMAND("test.PauseSession", test_PauseSession);
    WRAPPER_COMMAND("test.TeardownSession", test_TeardownSession);
    WRAPPER_COMMAND("test.Restart", test_Restart);
    WRAPPER_COMMAND("test.Stop", test_Stop);
    WRAPPER_COMMAND("test.updateSessionList", test_UpdateSessList);
    WRAPPER_COMMAND("test.DestructConnection", test_DestructConn);
    WRAPPER_COMMAND("test.Quit", test_Quit);
    WRAPPER_COMMAND("test.ResetLog", test_ResetLog);
}

/********************************************************************************************
 * PutError
 * purpose : Notify the user about errors that occured
 * input   : title  - Title of the error
 *           reason - Reason that caused the error
 * output  : none
 * return  : none
 ********************************************************************************************/
void PutError(const char* title, const char* reason)
{
    static RvBool TRTSP_inError = RV_FALSE;
    if ((reason == NULL) || (strlen(reason) == 0))
        reason = "Undefined error was encountered";

    fprintf(stderr, "%s: %s\n", title, reason);

    /* Make sure we don't get into an endless loop over this one */
    if (TRTSP_inError) return;
    TRTSP_inError = RV_TRUE;

#ifdef USE_TCL

#if defined (WIN32) && defined (_WINDOWS)
    if (gInterp == NULL)
        MessageBox(NULL, reason, title, MB_OK | MB_ICONERROR);
#endif  /* WIN32 */

    if (gInterp != NULL)
#endif /* USE_TCL */
    {
        TclExecute("test:Log {%s: %s}", title, reason);
        TclExecute("update;msgbox {%s} picExclamation {%s} {{Ok -1 <Key-Return>}}", title, reason);
    }
    TRTSP_inError = RV_FALSE;
}

/********************************************************************************************
 * tclGetFile
 * purpose : This function is automatically generated with the tcl scripts array in
 *           TMGC_scripts.tcl.
 *           It returns the buffer holding the .tcl file information
 * input   : name   - Name of file to load
 * output  : none
 * return  : The file's buffer if found
 *           NULL if file wasn't found
 ********************************************************************************************/
char* tclGetFile(IN char* name);


int test_File(ClientData clientData, Tcl_Interp *interp,int argc, char *argv[])
{
    int i, retCode;
    Tcl_DString str;

    RV_UNUSED_ARG(clientData);

    if ((argc == 3) && (strncmp(argv[1], "exis", 4)) == 0)
    {
        /* "file exist" command - overloaded... */
        if (tclGetFile(argv[2]) != NULL)
        {
            Tcl_SetResult(interp, (char *)"1", TCL_STATIC);
            return TCL_OK;
        }
    }

    /* Continue executing the real "file" command */
    Tcl_DStringInit(&str);
    Tcl_DStringAppendElement(&str, "fileOverloaded");
    for(i = 1; i < argc; i++)
        Tcl_DStringAppendElement(&str, argv[i]);
    retCode = Tcl_Eval(interp, Tcl_DStringValue(&str));
    Tcl_DStringFree(&str);
    return retCode;
}

int test_Source(ClientData clientData, Tcl_Interp *interp,int argc, char *argv[])
{
    FILE* exists;
    char* fileBuf;

    RV_UNUSED_ARG(clientData);

    if (argc != 2)
    {
        Tcl_SetResult(interp, (char *)"wrong # args: should be \"source <filename>\"", TCL_STATIC);
        return TCL_ERROR;
    }

    /* First see if we've got such a file on the disk */
    exists = fopen(argv[1], "r");
    if (exists == NULL)
    {
        /* File doesn't exist - get from compiled array */
        fileBuf = tclGetFile(argv[1]);
        if (fileBuf == NULL)
        {
            /* No such luck - we don't have a file to show */
            char error[300];
            sprintf(error, "file %s not found", argv[1]);
            Tcl_SetResult(interp, error, TCL_VOLATILE);
            return TCL_ERROR;
        }
        else
        {
            /* Found! */
            int retCode;
            retCode = Tcl_Eval(interp, fileBuf);
            if (retCode == TCL_ERROR)
            {
                char error[300];
                sprintf(error, "\n    (file \"%s\" line %d)", argv[1], interp->errorLine);
                Tcl_AddErrorInfo(interp, error);
            }
            return retCode;
        }
    }

    /* Close this file - we're going to read it */
    fclose(exists);

    /* File exists - evaluate from the file itself */
    return Tcl_EvalFile(interp, argv[1]);
}
#endif /* USE_TCL */


/*-----------------------------------------------------------------------*/
/*                           MODULE FUNCTIONS                            */
/*-----------------------------------------------------------------------*/

/******************************************************************************
 * rtspUpdateVariable
 * ----------------------------------------------------------------------------
 * General: 
 *  Update a variable in the Test object.
 *
 * Arguments:
 * Input:  varname          - String name of the variable.
 *         value            - String value of the variable.
 * Output: None.
 *
 * Return Value: none.
 *****************************************************************************/
void rtspUpdateVariable(char * varName, char * value)
{
    TestSetStackVariables(varName, value);
}


/******************************************************************************
 * rtspCallFunction
 * ----------------------------------------------------------------------------
 * General:
 *  Call a function as if called from the TCL.
 *
 * Arguments:
 * Input:  funcName    - String name of the function to call.
 *         argc        - Number of arguments.
 *         argv        - Arguments vector.
 * Output: None
 *
 * Return Value:  None
 *****************************************************************************/
void rtspCallFunction(char * funcName, int argc, char *argv[])
{
    RvStatus                   result;
    RV_UNUSED_ARG(argc);

    result  = RV_OK;

    if (strcmp(funcName, "test.Init") == 0)
    {
       TclExecute("set app(config,reqSessionId) {}");
    }
    else if (strcmp(funcName, "test.Start") == 0)
    {
        test_Start(argc, argv);
    }
    else if (strcmp(funcName, "test.SendResponse") == 0)
    {
        test_SendResponse(argc, argv);
    } 
    else if (strcmp(funcName, "test.SendDescRequest") == 0)
    {
        test_SendDescRequest(argc, argv);
    }
    else if (strcmp(funcName, "test.ConstructAndSendReq") == 0)
    {
        test_ConstructAndSendReq(argc, argv);
    }
    else if (strcmp(funcName, "test.TestConstructConn") == 0)
    {
        test_TestConstructConn(argc, argv);
    }
    else if (strcmp(funcName, "test.SendRequest") == 0)
    {
        test_SendRequest(argc, argv);
    }
    else if (strcmp(funcName, "test.SetUpSession") == 0)
    {
        test_SetUpSession(argc, argv);
    }
    else if (strcmp(funcName, "test.PlaySession") == 0)
    {
        test_PlaySession(argc, argv);
    }
    else if (strcmp(funcName, "test.PauseSession") == 0)
    {
        test_PauseSession(argc, argv);
    }
    else if (strcmp(funcName, "test.TeardownSession") == 0)
    {
        test_TeardownSession(argc, argv);
    }
    else if (strcmp(funcName, "test.Restart") == 0)
    {
        test_Restart(argc, argv);
    }
    else if (strcmp(funcName, "test.ResetLog") == 0)
    {
        test_ResetLog(argc, argv);
    }
    else if (strcmp(funcName, "test.Stop") == 0)
    {
        test_Stop(argc, argv);
    }
    else if (strcmp(funcName, "test.updateSessionList") == 0)
    {
        test_UpdateSessList(argc, argv);
    }
    else if (strcmp(funcName, "test.DestructConnection") == 0)
    {
        test_DestructConn(argc, argv);
    }
    else if (strcmp(funcName, "test.Quit") == 0)
    {
    }
}

#ifdef SPLIT_APP
/******************************************************************************
 * InitSplitAppNetwork
 * ----------------------------------------------------------------------------
 * General: 
 *  Initialize the split application network - connect to the TCL side.
 *
 * Arguments:
 * Input:   None
 * Output:  None
 *
 * Return Value:  Non-negative value on success
 *                Negative value on failure
 ******************************************************************************/
int InitSplitAppNetwork(void)
{
    rtspSplitAppListen();
    return 0;
}
#endif

#ifdef USE_TCL
Tcl_Interp* InitTcl(const char* executable, char* versionString, char** reason)
{
    static char strBuf[1024];
    int retCode;

    /* Find TCL executable and create an interpreter */
    Tcl_FindExecutable(executable);
    gInterp = Tcl_CreateInterp();

    if (gInterp == NULL)
    {
        *reason = (char*)"Failed to create Tcl interpreter";
        return NULL;
    }

    TclInit();

    /* Overload file and source commands */
    TclExecute("rename file fileOverloaded");
    CREATE_COMMAND("file", test_File);
    CREATE_COMMAND("source", test_Source);

    /* Reroute tcl libraries - we'll need this one later */
    /*TclSetVariable("tcl_library", TCL_LIBPATH);
    TclSetVariable("env(TCL_LIBRARY)", TCL_LIBPATH);
    TclSetVariable("tk_library", TK_LIBPATH);
    TclSetVariable("env(TK_LIBRARY)", TK_LIBPATH);*/

    /* Initialize TCL */
    retCode = Tcl_Init(gInterp);
    if (retCode != TCL_OK)
    {
        sprintf(strBuf, "Error in Tcl_Init: %s", Tcl_GetStringResult(gInterp));
        *reason = strBuf;
        Tcl_DeleteInterp(gInterp);
        return NULL;
    }

    /* Initialize TK */
    retCode = Tk_Init(gInterp);
    if (retCode != TCL_OK)
    {
        sprintf(strBuf, "Error in Tk_Init: %s", Tcl_GetStringResult(gInterp));
        *reason = strBuf;
        Tcl_DeleteInterp(gInterp);
        return NULL;
    }

    /* Set argc and argv parameters for the script.
       This allows us to work with C in the scripts. */
#if defined(RV_RELEASE)
    retCode = TclExecute("set tmp(version) {%s RTSP Client Test Application (Release)}", versionString);
#elif defined(RV_DEBUG)
    retCode = TclExecute("set tmp(version) {%s RTSP Client Test Application (Debug)}", versionString);
#else
#error RV_RELEASE or RV_DEBUG must be defined!
#endif
    if (retCode != TCL_OK)
    {
        *reason = (char*)"Error setting stack's version for test application";
        return gInterp;
    }

    /* Create new commands that are used in the tcl script */
    CreateTclCommands(gInterp);

    /* Evaluate the Tcl script of the test application */
    retCode = Tcl_Eval(gInterp, (char*)"source " TCL_FILENAME);
    if (retCode != TCL_OK)
    {
        sprintf(strBuf, "Error reading testapp script (line %d): %s\n", gInterp->errorLine, Tcl_GetStringResult(gInterp));
        *reason = strBuf;
        return NULL;
    }

    /* Return the created interpreter */
    *reason = NULL;
    return gInterp;
}

char *tclVarUpdate(
    IN ClientData   clientData,
    IN Tcl_Interp   *interp,
    IN char         *name1,
    IN char         *name2,
    IN int          flags)
{
    char varName[64];
    char * val;

    RV_UNUSED_ARG(clientData);
    RV_UNUSED_ARG(interp);
    RV_UNUSED_ARG(name1);
    RV_UNUSED_ARG(flags);

    sprintf(varName, "app(%s)", name2);
    val = TclGetVariable(varName);
    rtspUpdateVariable(name2, val);
    return NULL;
}

static void TraceTclVariable(IN const char    *varName)
{
    Tcl_TraceVar2(gInterp, (char*)"app", (char*)varName, TCL_TRACE_WRITES, tclVarUpdate, NULL);

    TclExecute("if {[lsearch $tmp(tvars) %s] < 0} {lappend tmp(tvars) %s}", varName, varName);

    /* Make sure we get the initial value here */
    tclVarUpdate(NULL, gInterp, (char*)"app", (char*)varName, TCL_TRACE_WRITES);
}

/********************************************************************************************
 * InitApp
 * purpose : Initialize the test application
 *           This includes parts as RTP/RTCP support, etc.
 * input   : none
 * output  : none
 * return  : Non-negative value on success
 *           Negative value on failure
 ********************************************************************************************/
int InitApp(void)
{
    /* Set tracing on specific variables */
    TraceTclVariable("config,autoAnswer");
    TraceTclVariable("config,maxConnections");
    TraceTclVariable("config,maxRequests");
    TraceTclVariable("config,maxResponses");
    TraceTclVariable("config,memoryElementSize");
    TraceTclVariable("config,memoryElementNum");
    TraceTclVariable("config,maxHeaders");
    TraceTclVariable("config,dnsAddr");
    return 0;
}


void UpdateTclVariable(void)
{
    TraceTclVariable("config,autoAnswer");
    TraceTclVariable("config,maxSession");
    TraceTclVariable("config,maxDescReq");
    TraceTclVariable("config,transmitQSize");
    TraceTclVariable("config,maxUri");
    TraceTclVariable("config,dnsMaxResults");
    TraceTclVariable("config,descTimeout");
    TraceTclVariable("config,responseTimeout");
    TraceTclVariable("config,pingTimeout");
}

/********************************************************************************************
 * InitApp
 * purpose : Initialize the stack
 *
 * input   : none
 * output  : none
 * return  : Non-negative value on success
 *           Negative value on failure
 ********************************************************************************************/
int InitStack(void)
{
#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    RvChar varname[32];
    int i, j;
#endif

    TestStackInit();

#if (RV_LOGMASK != RV_LOGLEVEL_NONE)
    for (i = 0; sources[i][0] != '\0'; i++)
    {
        for (j = 0; levels[j][0] != '\0'; j++)
        {
            sprintf(varname, "logFilter,%s,%s", sources[i], levels[j]);
            TraceTclVariable(varname);
        }
    }
    TraceTclVariable("options,catchStackLog"); 
#endif

    TestStackStart();

    return RV_OK;
}

#endif /* USE_TCL */

#ifdef __cplusplus
}
#endif

