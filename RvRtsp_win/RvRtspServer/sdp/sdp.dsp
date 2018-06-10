# Microsoft Developer Studio Project File - Name="sdp" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sdp - Win32 DebugIpv6DnsTls
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "sdp.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "sdp.mak" CFG="sdp - Win32 DebugIpv6DnsTls"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "sdp - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sdp - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sdp - Win32 DebugTls" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sdp - Win32 DebugDns" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sdp - Win32 DebugIpv6DnsTls" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sdp - Win32 ReleaseTls" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "sdp - Win32 ReleaseDns" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sdp - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GX /O2 /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /I "../common/adapters" /I "../common/adapters/windows" /I "../common/ceutils" /I "../sdp" /I "../sdp/codecs" /D "_WINDOWS" /D "_USRDLL" /D "SDP_EXPORTS" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "NDEBUG" /D "WIN32" /D "_MBCS" /FD /D /ALIGN:8192 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\release\bin/rvsdp_rtspserver.dll" /ALIGN:8192
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "sdp - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\Debug\lib"
# PROP Intermediate_Dir "..\binaries\windows\Debug\obj\sdp"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /I "../common/adapters" /I "../common/adapters/windows" /I "../common/ceutils" /I "../sdp" /I "../sdp/codecs" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /D "RV_DEBUG" /D "_DEBUG" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\Debug\bin/rvsdp_rtspserver.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "sdp - Win32 DebugTls"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sdp___Win32_DebugTls"
# PROP BASE Intermediate_Dir "sdp___Win32_DebugTls"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\DebugTls\lib"
# PROP Intermediate_Dir "..\binaries\windows\DebugTls\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\sdpparser" /I ".\util" /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /D "RV_DEBUG" /D "_DEBUG" /D "WIN32" /D "RV_TLS_ON" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /I "../common/adapters" /I "../common/adapters/windows" /I "../common/ceutils" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /D "RV_DEBUG" /D "RV_CFLAG_TLS" /D "_DEBUG" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\Debug\bin/rvsdp.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\DebugTls\bin/rvsdp.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "sdp - Win32 DebugDns"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sdp___Win32_DebugDns"
# PROP BASE Intermediate_Dir "sdp___Win32_DebugDns"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\DebugDns\lib"
# PROP Intermediate_Dir "..\binaries\windows\DebugDns\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\sdpparser" /I ".\util" /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /D "RV_DEBUG" /D "_DEBUG" /D "WIN32" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /I "../common/adapters" /I "../common/adapters/windows" /I "../common/ceutils" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /D "RV_DEBUG" /D "_DEBUG" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\Debug\bin/rvsdp.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\DebugDns\bin/rvsdp.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "sdp - Win32 DebugIpv6DnsTls"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sdp___Win32_DebugIpv6DnsTls"
# PROP BASE Intermediate_Dir "sdp___Win32_DebugIpv6DnsTls"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\binaries\windows\DebugIpv6DnsTls\lib"
# PROP Intermediate_Dir "..\binaries\windows\DebugIpv6DnsTls\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\sdpparser" /I ".\util" /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /D "RV_DEBUG" /D "_DEBUG" /D "WIN32" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GX /ZI /Od /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /I "../common/adapters" /I "../common/adapters/windows" /I "../common/ceutils" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SDP_EXPORTS" /D "RV_DEBUG" /D "_DEBUG" /D "WIN32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\DebugTlsQos\bin/rvsdp.dll" /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"..\binaries\windows\DebugIpv6DnsTls\bin/rvsdp.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "sdp - Win32 ReleaseTls"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sdp___Win32_ReleaseTls"
# PROP BASE Intermediate_Dir "sdp___Win32_ReleaseTls"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\binaries\windows\releaseTls\lib"
# PROP Intermediate_Dir "..\binaries\windows\releaseTls\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I ".\sdpparser" /I ".\util" /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /D "_WINDOWS" /D "_USRDLL" /D "SDP_EXPORTS" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "NDEBUG" /D "WIN32" /D "_MBCS" /YX /FD /D /ALIGN:8192" " /c
# ADD CPP /nologo /MD /W4 /GX /O1 /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /I "../common/adapters" /I "../common/adapters/windows" /I "../common/ceutils" /D "_WINDOWS" /D "_USRDLL" /D "SDP_EXPORTS" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_MBCS" /D "RV_CFLAG_TLS" /D "NDEBUG" /D "WIN32" /FD /D /ALIGN:8192" " /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\release\bin/rvsdp.dll" /ALIGN:8192
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\releaseTls\bin/rvsdp.dll" /ALIGN:8192
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "sdp - Win32 ReleaseDns"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sdp___Win32_ReleaseDns"
# PROP BASE Intermediate_Dir "sdp___Win32_ReleaseDns"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\binaries\windows\releaseDns\lib"
# PROP Intermediate_Dir "..\binaries\windows\releaseDns\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O1 /I ".\sdpparser" /I ".\util" /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /D "_WINDOWS" /D "_USRDLL" /D "SDP_EXPORTS" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "NDEBUG" /D "WIN32" /D "_MBCS" /YX /FD /D /ALIGN:8192" " /c
# ADD CPP /nologo /MD /W4 /GX /O1 /I "..\ads" /I "../common/ccore/memdrivers" /I "../common/ccore/netdrivers" /I "../common/cbase" /I "../common/ccore" /I "../common/config" /I "../common/config/arch" /I "../common/config/os" /I "../common/config/tool" /I "../common/cutils" /I "../common/cbase/netdrivers" /I "../common/adapters" /I "../common/adapters/windows" /I "../common/ceutils" /D "_WINDOWS" /D "_USRDLL" /D "SDP_EXPORTS" /D RV_SDP_DECLSPEC=__declspec(dllexport) /D "_MBCS" /D "NDEBUG" /D "WIN32" /D "RV_DNS_ENHANCED_FEATURES_SUPPORT" /FD /D /ALIGN:8192" " /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\release\bin/rvsdp.dll" /ALIGN:8192
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"..\binaries\windows\releaseDns\bin/rvsdp.dll" /ALIGN:8192
# SUBTRACT LINK32 /pdb:none /debug

!ENDIF 

# Begin Target

# Name "sdp - Win32 Release"
# Name "sdp - Win32 Debug"
# Name "sdp - Win32 DebugTls"
# Name "sdp - Win32 DebugDns"
# Name "sdp - Win32 DebugIpv6DnsTls"
# Name "sdp - Win32 ReleaseTls"
# Name "sdp - Win32 ReleaseDns"
# Begin Group "Sources (sdpparser)"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\rvsdpattr.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpbadsyntax.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpbandwidth.c
# End Source File
# Begin Source File

SOURCE=.\codecs\rvsdpcodecsenums.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpconnection.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpcrypto.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpemail.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpglobals.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpinit.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpkey.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpkeymgmt.c
# End Source File
# Begin Source File

SOURCE=.\rvsdplist.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpmedia.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpmsg.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpmsgparse.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpmsgreparse.c
# End Source File
# Begin Source File

SOURCE=.\rvsdporigin.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpphone.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpprsutils.c
# End Source File
# Begin Source File

SOURCE=.\rvsdprtpmap.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpsesstime.c
# End Source File
# Begin Source File

SOURCE=.\rvsdpstrings.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\rvsdp.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpapiprivate.h
# End Source File
# Begin Source File

SOURCE=.\codecs\rvsdpcodecparseutils.h
# End Source File
# Begin Source File

SOURCE=.\codecs\rvsdpcodecs.h
# End Source File
# Begin Source File

SOURCE=.\codecs\rvsdpcodecsconfig.h
# End Source File
# Begin Source File

SOURCE=.\codecs\rvsdpcodecsenums.h
# End Source File
# Begin Source File

SOURCE=.\codecs\rvsdpcodecsinternal.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpconfig.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpdataprivate.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpdatastruct.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpenc.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpenums.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpglobals.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpglobalsinternal.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpinit.h
# End Source File
# Begin Source File

SOURCE=.\rvsdplist.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpmedia.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpmsg.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpobjs.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpprivate.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpprsaux.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpprsutils.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpstrings.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpsymb.h
# End Source File
# Begin Source File

SOURCE=.\rvsdptypes.h
# End Source File
# Begin Source File

SOURCE=.\rvsdpversion.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\rvsdpVersion.rc
# End Source File
# End Target
# End Project
