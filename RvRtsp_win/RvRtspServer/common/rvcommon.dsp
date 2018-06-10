# Microsoft Developer Studio Project File - Name="rvcommon" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=rvcommon - Win32 DebugIpv6DnsTls
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "rvcommon.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rvcommon.mak" CFG="rvcommon - Win32 DebugIpv6DnsTls"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rvcommon - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rvcommon - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rvcommon - Win32 DebugTls" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rvcommon - Win32 DebugDns" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rvcommon - Win32 DebugIpv6DnsTls" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rvcommon - Win32 ReleaseTls" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "rvcommon - Win32 ReleaseDns" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "rvcommon - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\binaries\windows\release\lib"
# PROP Intermediate_Dir "..\binaries\windows\release\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O1 /I "cbase" /I "ccore" /I "sctp" /I "config" /I "ipsec" /I "adapters" /I "adapters/windows" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /I "ceutils" /D "RV_RELEASE" /D "NDEBUG" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /FD /D /ALIGN:8192 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\release\bin/rvcommon_rtspserver.dll" /ALIGN:8192
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "rvcommon - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\Debug\lib"
# PROP Intermediate_Dir "..\binaries\windows\Debug\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "ipsec" /I "cbase" /I "ccore" /I "sctp" /I "config" /I "adapters" /I "adapters/windows" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /I "ceutils" /D "RV_DEBUG" /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\debug\bin/rvcommon_rtspserver.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=nmake /I /f commonInclude.mak
# End Special Build Tool

!ELSEIF  "$(CFG)" == "rvcommon - Win32 DebugTls"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rvcommon___Win32_DebugTls"
# PROP BASE Intermediate_Dir "rvcommon___Win32_DebugTls"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\DebugTls\lib"
# PROP Intermediate_Dir "..\binaries\windows\DebugTls\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "cbase" /I "ccore" /I "config" /I "sctp" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "RV_DEBUG" /D "_DEBUG" /D "WIN32" /D "RV_TLS_ON" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "cbase" /I "ccore" /I "sctp" /I "config" /I "ipsec" /I "adapters" /I "adapters/windows" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /I "ceutils" /D "RV_DEBUG" /D "RV_CFLAG_TLS" /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib ssleay32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\debug\bin/rvcommon.dll" /pdbtype:sept
# ADD LINK32 libeay32.lib ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib ssleay32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\DebugTls\bin/rvcommon.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "rvcommon - Win32 DebugDns"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rvcommon___Win32_DebugDns"
# PROP BASE Intermediate_Dir "rvcommon___Win32_DebugDns"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\DebugDns\lib"
# PROP Intermediate_Dir "..\binaries\windows\DebugDns\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "cbase" /I "ccore" /I "config" /I "sctp" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "RV_DEBUG" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "RV_CFLAG_NTP_TIME" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "cbase" /I "ccore" /I "sctp" /I "config" /I "ipsec" /I "adapters" /I "adapters/windows" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /I "ceutils" /D "RV_DEBUG" /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib ssleay32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\debug\bin/rvcommon.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\debugDns\bin/rvcommon.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "rvcommon - Win32 DebugIpv6DnsTls"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "rvcommon___Win32_DebugIpv6DnsTls"
# PROP BASE Intermediate_Dir "rvcommon___Win32_DebugIpv6DnsTls"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\DebugIpv6DnsTls\lib"
# PROP Intermediate_Dir "..\binaries\windows\DebugIpv6DnsTls\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "cbase" /I "ccore" /I "config" /I "sctp" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /D "RV_DEBUG" /D "_DEBUG" /D "RV_CFLAG_TLS" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /D "RV_CFLAG_NTP_TIME" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "cbase" /I "ccore" /I "sctp" /I "config" /I "ipsec" /I "adapters" /I "adapters/windows" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /I "ceutils" /D "RV_CFLAG_TLS" /D "RV_CFLAG_IPV6" /D "RV_DEBUG" /D "_DEBUG" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib ssleay32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\DebugTlsQos\bin/rvcommon.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib ssleay32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\DebugIpv6DnsTls\bin/rvcommon.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "rvcommon - Win32 ReleaseTls"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "rvcommon___Win32_ReleaseTls"
# PROP BASE Intermediate_Dir "rvcommon___Win32_ReleaseTls"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\binaries\windows\releaseTls\lib"
# PROP Intermediate_Dir "..\binaries\windows\releaseTls\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /GX /O1 /I "cbase" /I "ccore" /I "sctp" /I "config" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /D "RV_RELEASE" /D "NDEBUG" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /D "RV_CFLAG_NTP_TIME" /FD /D /ALIGN:8192" " /c
# ADD CPP /nologo /MD /W4 /GX /O1 /I "cbase" /I "ccore" /I "sctp" /I "config" /I "ipsec" /I "adapters" /I "adapters/windows" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /I "ceutils" /D "RV_RELEASE" /D "RV_CFLAG_TLS" /D "NDEBUG" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /FD /D /ALIGN:8192" " /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\release\bin/rvcommon.dll" /ALIGN:8192
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 libeay32.lib ssleay32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\releaseTls\bin/rvcommon.dll" /ALIGN:8192
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "rvcommon - Win32 ReleaseDns"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "rvcommon___Win32_ReleaseDns"
# PROP BASE Intermediate_Dir "rvcommon___Win32_ReleaseDns"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\binaries\windows\releaseDns\lib"
# PROP Intermediate_Dir "..\binaries\windows\releaseDns\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /GX /O1 /I "cbase" /I "ccore" /I "sctp" /I "config" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /D "RV_RELEASE" /D "NDEBUG" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /D "RV_CFLAG_NTP_TIME" /FD /D /ALIGN:8192" " /c
# ADD CPP /nologo /MD /W4 /GX /O1 /I "cbase" /I "ccore" /I "sctp" /I "config" /I "ipsec" /I "adapters" /I "adapters/windows" /I "config/arch" /I "config/os" /I "config/tool" /I "cutils" /I "ccore/memdrivers" /I "ccore/netdrivers" /I "cbase/netdrivers" /I "ares" /I "ceutils" /D "RV_RELEASE" /D "NDEBUG" /D "RV_DNS_ENHANCED_FEATURES_SUPPORT" /D "_MBCS" /D "_USRDLL" /D "RVCORE_EXPORTS" /D "_WINDOWS" /D "WIN32" /FD /D /ALIGN:8192" " /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\release\bin/rvcommon.dll" /ALIGN:8192
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\releaseDns\bin/rvcommon.dll" /ALIGN:8192
# SUBTRACT LINK32 /pdb:none /debug

!ENDIF 

# Begin Target

# Name "rvcommon - Win32 Release"
# Name "rvcommon - Win32 Debug"
# Name "rvcommon - Win32 DebugTls"
# Name "rvcommon - Win32 DebugDns"
# Name "rvcommon - Win32 DebugIpv6DnsTls"
# Name "rvcommon - Win32 ReleaseTls"
# Name "rvcommon - Win32 ReleaseDns"
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Base Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cbase\rvcbase.c
# End Source File
# Begin Source File

SOURCE=.\cbase\rvcbase.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvcbaseconfig.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvcbasedefs.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvevent.c
# End Source File
# Begin Source File

SOURCE=.\cbase\rvevent.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvglobalindexes.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvglobals.c
# End Source File
# Begin Source File

SOURCE=.\cbase\rvglobals.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvqueue.c
# End Source File
# Begin Source File

SOURCE=.\cbase\rvqueue.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvsmq.c
# End Source File
# Begin Source File

SOURCE=.\cbase\rvsmq.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvtimer.c
# End Source File
# Begin Source File

SOURCE=.\cbase\rvtimer.h
# End Source File
# Begin Source File

SOURCE=.\cbase\rvtimerengine.c
# End Source File
# Begin Source File

SOURCE=.\cbase\rvtimerengine.h
# End Source File
# End Group
# Begin Group "Utils Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\cutils\rv64bits.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rv64bits.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvaddress.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvaddress.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvansi.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvansi.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvbase64.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvbase64.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvema.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvema.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rverror.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvhashu.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvhashu.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvnet2host.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvnet2host.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvntptime.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvntptime.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvobjlist.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvobjlist.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvobjpool.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvobjpool.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvpqueue.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvpqueue.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvra.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvra.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvrandomgenerator.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvrandomgenerator.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvresource.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvresource.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvsqrt.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvsqrt.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvstrutils.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvstrutils.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvtime.c
# End Source File
# Begin Source File

SOURCE=.\cutils\rvtime.h
# End Source File
# Begin Source File

SOURCE=.\cutils\rvtypes.h
# End Source File
# End Group
# Begin Group "Configuration Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\config\arch\rvarchconfig.h
# End Source File
# Begin Source File

SOURCE=.\config\arch\rvarchdefs.h
# End Source File
# Begin Source File

SOURCE=.\config\rvcflags.h
# End Source File
# Begin Source File

SOURCE=.\config\rvconfig.h
# End Source File
# Begin Source File

SOURCE=.\config\rvdependencies.h
# End Source File
# Begin Source File

SOURCE=.\config\rvinterfacesdefs.h
# End Source File
# Begin Source File

SOURCE=.\config\tool\rvmsvc.h
# End Source File
# Begin Source File

SOURCE=.\config\os\rvosconfig.h
# End Source File
# Begin Source File

SOURCE=.\config\os\rvosdefs.h
# End Source File
# Begin Source File

SOURCE=.\config\tool\rvtoolconfig.h
# End Source File
# Begin Source File

SOURCE=.\config\tool\rvtooldefs.h
# End Source File
# Begin Source File

SOURCE=.\config\rvusrconfig.h
# End Source File
# Begin Source File

SOURCE=.\config\os\rvwin32.h
# End Source File
# End Group
# Begin Group "Core Files"

# PROP Default_Filter ""
# Begin Group "memdrivers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ccore\memdrivers\rvosmem.c
# End Source File
# Begin Source File

SOURCE=.\ccore\memdrivers\rvosmem.h
# End Source File
# Begin Source File

SOURCE=.\ccore\memdrivers\rvpoolmem.c
# End Source File
# Begin Source File

SOURCE=.\ccore\memdrivers\rvpoolmem.h
# End Source File
# End Group
# Begin Group "netdrivers"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ccore\netdrivers\rvhost.c
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvhost.h
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvportrange.c
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvportrange.h
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvselect.c
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvselect.h
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvselectinternal.h
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvsocket.c
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvsocket.h
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvtls.c
# End Source File
# Begin Source File

SOURCE=.\ccore\netdrivers\rvtls.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\ccore\rv64ascii.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rv64ascii.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvassert.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccore.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccore.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccoreconfig.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccoredefs.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccoreglobals.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccoreglobals.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccorestrings.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccoreversion.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvccoreversion.rc
# End Source File
# Begin Source File

SOURCE=.\ccore\rvclock.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvclock.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvlock.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvlock.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvlog.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvlog.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvloginternal.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvloglistener.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvloglistener.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvmemory.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvmemory.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvmutex.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvmutex.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvsemaphore.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvsemaphore.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvstdio.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvstdio.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvthread.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvthread.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvthreadtls.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvthreadtls.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvtimestamp.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvtimestamp.h
# End Source File
# Begin Source File

SOURCE=.\ccore\rvtm.c
# End Source File
# Begin Source File

SOURCE=.\ccore\rvtm.h
# End Source File
# End Group
# Begin Group "Ares"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ares\ares.h
# End Source File
# Begin Source File

SOURCE=.\ares\ares__read_line.c
# End Source File
# Begin Source File

SOURCE=.\ares\ares_dns.h
# End Source File
# Begin Source File

SOURCE=.\ares\ares_expand_name.c
# End Source File
# Begin Source File

SOURCE=.\ares\ares_init.c
# End Source File
# Begin Source File

SOURCE=.\ares\ares_private.h
# End Source File
# Begin Source File

SOURCE=.\ares\ares_process.c
# End Source File
# Begin Source File

SOURCE=.\ares\ares_query.c
# End Source File
# Begin Source File

SOURCE=.\ares\rvares.c
# End Source File
# Begin Source File

SOURCE=.\ares\rvares.h
# End Source File
# Begin Source File

SOURCE=.\ares\rvarescache.c
# End Source File
# Begin Source File

SOURCE=.\ares\rvarescache.h
# End Source File
# Begin Source File

SOURCE=.\ares\rvarescached.h
# End Source File
# Begin Source File

SOURCE=.\ares\rvehd.c
# End Source File
# Begin Source File

SOURCE=.\ares\rvehd.h
# End Source File
# Begin Source File

SOURCE=.\ares\rvoscomp.c
# End Source File
# Begin Source File

SOURCE=.\ares\rvoscomp.h
# End Source File
# End Group
# Begin Group "Adapters"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\adapters\windows\rvadclock.c
# End Source File
# Begin Source File

SOURCE=.\adapters\rvadclock.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadlock.c
# End Source File
# Begin Source File

SOURCE=.\adapters\rvadlock.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadlock_t.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadmutex.c
# End Source File
# Begin Source File

SOURCE=.\adapters\rvadmutex.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadmutex_t.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadsema4.c
# End Source File
# Begin Source File

SOURCE=.\adapters\rvadsema4.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadsema4_t.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadthread.c
# End Source File
# Begin Source File

SOURCE=.\adapters\rvadthread.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadthread_t.h
# End Source File
# Begin Source File

SOURCE=.\adapters\windows\rvadtstamp.c
# End Source File
# Begin Source File

SOURCE=.\adapters\rvadtstamp.h
# End Source File
# End Group
# Begin Group "CEUtils Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ceutils\rvalloc.c
# End Source File
# Begin Source File

SOURCE=.\ceutils\rvalloc.h
# End Source File
# Begin Source File

SOURCE=.\ceutils\rvhash.h
# End Source File
# Begin Source File

SOURCE=.\ceutils\rvlist.h
# End Source File
# Begin Source File

SOURCE=.\ceutils\rvptrvector.c
# End Source File
# Begin Source File

SOURCE=.\ceutils\rvptrvector.h
# End Source File
# Begin Source File

SOURCE=.\ceutils\rvvector.h
# End Source File
# End Group
# End Target
# End Project
