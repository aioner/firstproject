# Main dependency list
ALL : INCLUDE



###################################################################################################
#
# Include Directory Part
#
###################################################################################################

# Directories
RTSPCLIENTDIR=.
INCDIR=..\..\include\.
RTSPINC=$(INCDIR)\rtsp\.

# Make sure include directory is created if necessary
INCLUDE_DIRS : INCLUDE_DIR INCLUDE_RTSP_DIR

!IF exist($(INCDIR))
INCLUDE_DIR :
!ELSE
INCLUDE_DIR :
    mkdir $(INCDIR) > NUL
!ENDIF

!IF exist($(RTSPINC))
INCLUDE_RTSP_DIR :
!ELSE
INCLUDE_RTSP_DIR :
    mkdir $(RTSPINC) > NUL
!ENDIF

INCLUDE : INCLUDE_DIRS RTSP

# RTSP related header files.

RTSP : $(RTSPINC)\RvRtspClientInc.h $(RTSPINC)\RvRtspClient.h $(RTSPINC)\RvRtspClientConnection.h $(RTSPINC)\RvRtspClientSession.h $(RTSPINC)\RvRtspClientTypes.h


$(RTSPINC)\RvRtspClient.h : $(RTSPCLIENTDIR)\RvRtspClient.h
   copy $(RTSPCLIENTDIR)\RvRtspClient.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspClientInc.h : $(RTSPCLIENTDIR)\RvRtspClientInc.h
   copy $(RTSPCLIENTDIR)\RvRtspClientInc.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspClientConnection.h : $(RTSPCLIENTDIR)\RvRtspClientConnection.h
   copy $(RTSPCLIENTDIR)\RvRtspClientConnection.h $(RTSPINC) > NUL


$(RTSPINC)\RvRtspClientSession.h : $(RTSPCLIENTDIR)\RvRtspClientSession.h
   copy $(RTSPCLIENTDIR)\RvRtspClientSession.h $(RTSPINC) > NUL

$(RTSPINC)\RvRtspClientTypes.h : $(RTSPCLIENTDIR)\RvRtspClientTypes.h
   copy $(RTSPCLIENTDIR)\RvRtspClientTypes.h $(RTSPINC) > NUL
