include ../../profile

BIN         := ../../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/librvcommon.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

DIST_INC    := -I../inc/common
IPCAM_INC   := -I../inc/rtprtcp
INC_PATH    := $(DIST_INC)  $(IPCAM_INC)

DIST_LIB    := -L../build
LIB_PATH    := $(DIST_LIB) 
LIB	    := -lpthread -lm

#MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL
MODULE_DEFINES :=-fPIC -fvisibility=hidden  -DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL -D_RV_LINUX_API_DEFAULT
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES)  -Wall -g -o  

#TARGET      := ../../../bin/$(TARGET_DIR)/librvcommon.a  
SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(COBJ) $(CXXOBJ)

PUB_INC     :=../../../include/RvRtp/common 

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

# creat default build dirs
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
	@cp  ../inc/common/*.h $(PUB_INC)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
