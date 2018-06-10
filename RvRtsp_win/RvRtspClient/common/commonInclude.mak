#*********************************************************************************
#*                              <commonInclude.mak>                                                              
#* 
#* Fill the include dir with application API header files.
#*
#*    Author                         Date
#*    ------                        ------
#*    Rafi Kiel                     Nov 2006
#*********************************************************************************
DEBUG = off

!IF "$(DEBUG)" == "on"
DEBUG_PARAM =
!ELSE
DEBUG_PARAM = @
!ENDIF

ROOT = ..
CORE_ROOT = $(ROOT)\common
CORE_CUTILS_ROOT = $(CORE_ROOT)\cutils
CORE_CEUTILS_ROOT = $(CORE_ROOT)\ceutils
CORE_CCORE_ROOT  = $(CORE_ROOT)\ccore
CORE_NETDRIVERS_ROOT  = $(CORE_ROOT)\ccore\netdrivers
CORE_CCONFIG_ROOT  = $(CORE_ROOT)\config

INCLUDE_ROOT		= $(ROOT)\include
INCLUDE_COMMON_ROOT = $(INCLUDE_ROOT)\common

# Module directories - must have relative path from this makefile

# common core files
COMMON_FILES = \
			   "$(CORE_CUTILS_ROOT)\rvtypes.h" \
			   "$(CORE_CUTILS_ROOT)\rverror.h" \
			   "$(CORE_CUTILS_ROOT)\rv64bits.h" \
			   "$(CORE_CCONFIG_ROOT)\rvcflags.h" \
			   "$(CORE_CCONFIG_ROOT)\rvconfig.h" \
			   "$(CORE_CCONFIG_ROOT)\rvdependencies.h" \
			   "$(CORE_CCONFIG_ROOT)\rvusrconfig.h" \
			   "$(CORE_CCONFIG_ROOT)\rvinterfacesdefs.h" \
			   "$(CORE_CEUTILS_ROOT)\rvlist.h" \
			   "$(CORE_CEUTILS_ROOT)\rvalloc.h" \
			   "$(CORE_CEUTILS_ROOT)\rvvector.h" \
 			   "$(CORE_CCONFIG_ROOT)\arch\rvarchconfig.h" \
			   "$(CORE_CCONFIG_ROOT)\arch\rvarchdefs.h" \
			   "$(CORE_CCONFIG_ROOT)\os\rvosconfig.h" \
			   "$(CORE_CCONFIG_ROOT)\os\rvosdefs.h" \
			   "$(CORE_CCONFIG_ROOT)\os\rvwin32.h" \
			   "$(CORE_CCONFIG_ROOT)\tool\rvtoolconfig.h" \
			   "$(CORE_CCONFIG_ROOT)\tool\rvtooldefs.h" \
			   "$(CORE_CCONFIG_ROOT)\tool\rvmsvc.h" \
			   "$(CORE_CCORE_ROOT)\rvccoreconfig.h" \
			   "$(CORE_CCORE_ROOT)\rvccoredefs.h"  \
               "$(CORE_NETDRIVERS_ROOT)\rvnettypes.h"  \

ALL : INCLUDEAPI COMMONAPI


INCLUDEAPI:
	@echo --------------------- Updating Include API... ---------------------
	@-mkdir $(INCLUDE_ROOT)

#*********************************************************************************
#*                           C OM M O N   A P I 
#*********************************************************************************

COMMONAPI: COMMONSTART $(COMMON_FILES)


COMMONSTART: 
	@echo --------------------- Updating Common API... ---------------------
	@-mkdir $(INCLUDE_COMMON_ROOT)

$(COMMON_FILES): PHONEY
	@copy $@ $(INCLUDE_COMMON_ROOT)\

PHONEY: