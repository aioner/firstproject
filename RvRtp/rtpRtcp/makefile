include ../../profile

BIN         := ../../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/librv32rtp.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

DIST_INC    := -I../inc/common
IPCAM_INC   := -I../inc/rtprtcp
INC_PATH    := $(DIST_INC)  $(IPCAM_INC)

#MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE -D_RV_LINUX_API_DEFAULT
MODULE_DEFINES :=-fPIC -fvisibility=hidden -DLINUX -D_GNU_SOURCE -D_RV_LINUX_API_DEFAULT
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES)  -Wall -g -o 

#TARGET      := ../../../bin/$(TARGET_DIR)/librv32rtp.a
SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(COBJ) $(CXXOBJ)

PUB_INC     := ../../../include/RvRtp/rtprtcp

.PHONY:release clean build

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo ar rcs $@ "*.o"
	@ar rcs $@ $^
	@cp $(TARG) $(TARGET)
	@rm -fR $(PUB_INC)
	@mkdir -p $(PUB_INC)
	@cp  ../inc/rtprtcp/*.h $(PUB_INC)
	
$(RELEASE_DIR)/%.o:%.c
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $^

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $^

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
