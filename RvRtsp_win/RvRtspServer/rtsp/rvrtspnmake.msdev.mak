# Main dependency list
ALL : INCLUDE



###################################################################################################
#
# Include Directory Part
#
###################################################################################################

# Directories
COMMONDIR=..\common
RTSPDIR=.
INCDIR=..\include\.
COMMONINC=$(INCDIR)\common\.
RTSPINC=$(INCDIR)\rtsp\.

# Make sure include directory is created if necessary
INCLUDE_DIRS : INCLUDE_DIR INCLUDE_COMMON_DIR INCLUDE_RTSP_DIR

!IF exist($(INCDIR))
INCLUDE_DIR :
!ELSE
INCLUDE_DIR :
    mkdir $(INCDIR) > NUL
!ENDIF

!IF exist($(COMMONINC))
INCLUDE_COMMON_DIR :
!ELSE
INCLUDE_COMMON_DIR :
    mkdir $(COMMONINC) > NUL
!ENDIF

!IF exist($(RTSPINC))
INCLUDE_RTSP_DIR :
!ELSE
INCLUDE_RTSP_DIR :
    mkdir $(RTSPINC) > NUL
!ENDIF



!IF DEFINED(UNDER_CE)
OS_INCLUDE = $(COMMONINC)\rvemvc.h $(COMMONINC)\rvwince.h
!ELSE
OS_INCLUDE = $(COMMONINC)\rvmsvc.h $(COMMONINC)\rvwin32.h
!ENDIF

INCLUDE : INCLUDE_DIRS COMMONCORE RTSP

# Common Core related header files.
COMMONCORE : $(COMMONINC)\rvtypes.h $(COMMONINC)\rverror.h $(COMMONINC)\rvconfig.h $(COMMONINC)\rvcflags.h $(COMMONINC)\rvdependencies.h $(COMMONINC)\rvusrconfig.h $(COMMONINC)\rvinterfacesdefs.h $(COMMONINC)\rvarchconfig.h $(COMMONINC)\rvarchdefs.h $(COMMONINC)\rvosconfig.h $(COMMONINC)\rvosdefs.h $(COMMONINC)\rvtoolconfig.h $(COMMONINC)\rvtooldefs.h $(COMMONINC)\rvccoredefs.h $(COMMONINC)\rvsocket.h $(COMMONINC)\rvcbase.h $(COMMONINC)\rvccoreconfig.h $(OS_INCLUDE)

$(COMMONINC)\rvtypes.h : $(COMMONDIR)\cutils\rvtypes.h
   copy $(COMMONDIR)\cutils\rvtypes.h $(COMMONINC) > NUL

$(COMMONINC)\rverror.h : $(COMMONDIR)\cutils\rverror.h
   copy $(COMMONDIR)\cutils\rverror.h $(COMMONINC) > NUL

$(COMMONINC)\rvconfig.h : $(COMMONDIR)\config\rvconfig.h
   copy $(COMMONDIR)\config\rvconfig.h $(COMMONINC) > NUL

$(COMMONINC)\rvcflags.h : $(COMMONDIR)\config\rvcflags.h
   copy $(COMMONDIR)\config\rvcflags.h $(COMMONINC) > NUL

$(COMMONINC)\rvdependencies.h : $(COMMONDIR)\config\rvdependencies.h
   copy $(COMMONDIR)\config\rvdependencies.h $(COMMONINC) > NUL

$(COMMONINC)\rvusrconfig.h : $(COMMONDIR)\config\rvusrconfig.h
   copy $(COMMONDIR)\config\rvusrconfig.h $(COMMONINC) > NUL

$(COMMONINC)\rvinterfacesdefs.h : $(COMMONDIR)\config\rvinterfacesdefs.h
   copy $(COMMONDIR)\config\rvinterfacesdefs.h $(COMMONINC) > NUL

$(COMMONINC)\rvarchconfig.h : $(COMMONDIR)\config\arch\rvarchconfig.h
   copy $(COMMONDIR)\config\arch\rvarchconfig.h $(COMMONINC) > NUL

$(COMMONINC)\rvarchdefs.h : $(COMMONDIR)\config\arch\rvarchdefs.h
   copy $(COMMONDIR)\config\arch\rvarchdefs.h $(COMMONINC) > NUL

$(COMMONINC)\rvosconfig.h : $(COMMONDIR)\config\os\rvosconfig.h
   copy $(COMMONDIR)\config\os\rvosconfig.h $(COMMONINC) > NUL

$(COMMONINC)\rvosdefs.h : $(COMMONDIR)\config\os\rvosdefs.h
   copy $(COMMONDIR)\config\os\rvosdefs.h $(COMMONINC) > NUL

$(COMMONINC)\rvwin32.h : $(COMMONDIR)\config\os\rvwin32.h
   copy $(COMMONDIR)\config\os\rvwin32.h $(COMMONINC) > NUL

$(COMMONINC)\rvwince.h : $(COMMONDIR)\config\os\rvwince.h
   copy $(COMMONDIR)\config\os\rvwince.h $(COMMONINC) > NUL

$(COMMONINC)\rvtoolconfig.h : $(COMMONDIR)\config\tool\rvtoolconfig.h
   copy $(COMMONDIR)\config\tool\rvtoolconfig.h $(COMMONINC) > NUL

$(COMMONINC)\rvtooldefs.h : $(COMMONDIR)\config\tool\rvtooldefs.h
   copy $(COMMONDIR)\config\tool\rvtooldefs.h $(COMMONINC) > NUL

$(COMMONINC)\rvccoredefs.h : $(COMMONDIR)\ccore\rvccoredefs.h
   copy $(COMMONDIR)\ccore\rvccoredefs.h $(COMMONINC) > NUL

$(COMMONINC)\rvsocket.h : $(COMMONDIR)\ccore\netdrivers\rvsocket.h
   copy $(COMMONDIR)\ccore\netdrivers\rvsocket.h $(COMMONINC) > NUL

$(COMMONINC)\rvccoreconfig.h : $(COMMONDIR)\ccore\rvccoreconfig.h
   copy $(COMMONDIR)\ccore\rvccoreconfig.h $(COMMONINC) > NUL

$(COMMONINC)\rvmsvc.h : $(COMMONDIR)\config\tool\rvmsvc.h
   copy $(COMMONDIR)\config\tool\rvmsvc.h $(COMMONINC) > NUL

$(COMMONINC)\rvemvc.h : $(COMMONDIR)\config\tool\rvemvc.h
   copy $(COMMONDIR)\config\tool\rvemvc.h $(COMMONINC) > NUL

$(COMMONINC)\rvcbase.h : $(COMMONDIR)\cbase\rvcbase.h
   copy $(COMMONDIR)\cbase\rvcbase.h $(COMMONINC) > NUL


# RTSP related header files.

RTSP : $(RTSPINC)\RvRtspFirstLineTypes.h $(RTSPINC)\RvRtspHeaderTypes.h $(RTSPINC)\RvRtspMessageTypes.h $(RTSPINC)\RvRtspMsg.h $(RTSPINC)\RvRtspUtils.h $(RTSPINC)\RvRtspCommonTypes.h $(RTSPINC)\RvRtspUsrConfig.h

$(RTSPINC)\RvRtspFirstLineTypes.h : $(RTSPDIR)\messages\RvRtspFirstLineTypes.h
   copy $(RTSPDIR)\messages\RvRtspFirstLineTypes.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspHeaderTypes.h : $(RTSPDIR)\messages\RvRtspHeaderTypes.h
   copy $(RTSPDIR)\messages\RvRtspHeaderTypes.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspMessageTypes.h : $(RTSPDIR)\messages\RvRtspMessageTypes.h
   copy $(RTSPDIR)\messages\RvRtspMessageTypes.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspMsg.h : $(RTSPDIR)\messages\RvRtspMsg.h
   copy $(RTSPDIR)\messages\RvRtspMsg.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspUtils.h : $(RTSPDIR)\utils\RvRtspUtils.h
   copy $(RTSPDIR)\utils\RvRtspUtils.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspCommonTypes.h : $(RTSPDIR)\rtspcommon\RvRtspCommonTypes.h
   copy $(RTSPDIR)\rtspcommon\RvRtspCommonTypes.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspUsrConfig.h : $(RTSPDIR)\rtspcommon\RvRtspUsrConfig.h
   copy $(RTSPDIR)\rtspcommon\RvRtspUsrConfig.h $(RTSPINC) > NUL

