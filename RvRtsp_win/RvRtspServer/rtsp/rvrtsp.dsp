# Microsoft Developer Studio Project File - Name="rvrtsp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=rvrtsp - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rvrtsp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rvrtsp.mak" CFG="rvrtsp - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rvrtsp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rvrtsp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rvrtsp - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\binaries\windows\release\bin"
# PROP Intermediate_Dir "..\binaries\windows\release\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RVRTSP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O2 /I "messages" /I "client" /I "transport" /I "utils" /I "rtspcommon" /I "..\common\ccore\netdrivers" /I "..\common\ccore\memdrivers" /I "..\common\cbase" /I "..\common\cutils" /I "..\common\ccore" /I "..\common\config\os" /I "..\common\config" /I "..\common\config\arch" /I "..\common\config\tool" /I "..\common\ares" /I "..\common\sctp" /I "..\common\adapters" /I "..\common\adapters\windows" /D "RV_RELEASE" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_USRDLL" /D "RVRTSP_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:".\..\binaries\windows\release\bin\rvrtsp_server.dll"

!ELSEIF  "$(CFG)" == "rvrtsp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\debug\bin"
# PROP Intermediate_Dir "..\binaries\windows\debug\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RVRTSP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "messages" /I "client" /I "transport" /I "utils" /I "rtspcommon" /I "..\common\ccore\netdrivers" /I "..\common\ccore\memdrivers" /I "..\common\cbase" /I "..\common\cutils" /I "..\common\ccore" /I "..\common\config\os" /I "..\common\config" /I "..\common\config\arch" /I "..\common\config\tool" /I "../common/cutils" /I "..\common\ares" /I "..\common\sctp" /I "..\common\adapters" /I "..\common\adapters\windows" /I "..\common\cbase\netdrivers" /D "RV_DEBUG" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_WINDOWS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:".\..\binaries\windows\debug\bin\rvrtsp_server.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "rvrtsp - Win32 Release"
# Name "rvrtsp - Win32 Debug"
# Begin Group "transport"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\transport\RtspTransport.c
# End Source File
# Begin Source File

SOURCE=.\transport\RtspTransport.h
# End Source File
# End Group
# Begin Group "messages"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\messages\RtspFirstLine.c
# End Source File
# Begin Source File

SOURCE=.\messages\RtspFirstLine.h
# End Source File
# Begin Source File

SOURCE=.\messages\RtspHeaders.c
# End Source File
# Begin Source File

SOURCE=.\messages\RtspHeaders.h
# End Source File
# Begin Source File

SOURCE=.\messages\RtspMessages.c
# End Source File
# Begin Source File

SOURCE=.\messages\RtspMessages.h
# End Source File
# Begin Source File

SOURCE=.\messages\RtspMsgInternal.h
# End Source File
# Begin Source File

SOURCE=.\messages\RvRtspFirstLineTypes.h
# End Source File
# Begin Source File

SOURCE=.\messages\RvRtspHeaderTypes.h
# End Source File
# Begin Source File

SOURCE=.\messages\RvRtspMessageTypes.h
# End Source File
# Begin Source File

SOURCE=.\messages\RvRtspMsg.c
# End Source File
# Begin Source File

SOURCE=.\messages\RvRtspMsg.h
# End Source File
# End Group
# Begin Group "rtspcommon"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\rtspcommon\RtspObject.h
# End Source File
# Begin Source File

SOURCE=.\rtspcommon\RvRtspCommonTypes.h
# End Source File
# Begin Source File

SOURCE=.\rtspcommon\RvRtspUsrConfig.h
# End Source File
# End Group
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\utils\copybits.h
# End Source File
# Begin Source File

SOURCE=.\utils\ra.c
# End Source File
# Begin Source File

SOURCE=.\utils\ra.h
# End Source File
# Begin Source File

SOURCE=.\utils\rpool.c
# End Source File
# Begin Source File

SOURCE=.\utils\rpool.h
# End Source File
# Begin Source File

SOURCE=.\utils\RtspUtilsInternal.c
# End Source File
# Begin Source File

SOURCE=.\utils\RtspUtilsInternal.h
# End Source File
# Begin Source File

SOURCE=.\utils\RvRtspUtils.c
# End Source File
# Begin Source File

SOURCE=.\utils\RvRtspUtils.h
# End Source File
# End Group
# End Target
# End Project
