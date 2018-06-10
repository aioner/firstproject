include ../../../profile

BIN         := ../../../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/librvcommon_rtspclient.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

DIST_INC    := -I../include/common
IPCAM_INC   := -I../include/rtsp
INC_PATH    := $(DIST_INC)  $(IPCAM_INC)

DIST_LIB    := -L../build
LIB_PATH    := $(DIST_LIB) 
LIB	    := -lpthread -lm

#MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL
MODULE_DEFINES :=-fPIC -fvisibility=hidden  -DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL -D_RV_LINUX_API_HIDDEN
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES)  -Wall -g -o  

SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(COBJ) $(CXXOBJ)

PUB_INC     :=../../../../include/RvRtspClient/common 

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*
	

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@#$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
	@echo ar rcs $@ "*.o"
	@ar rcs $@ $^
	@cp $(TARG) $(TARGET)
	@rm -fR $(PUB_INC)
	@mkdir -p $(PUB_INC)
	@cp  ../include/common/*.h $(PUB_INC)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
