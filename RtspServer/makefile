include ../profile

BIN         :=../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/libxt_rtsp_server.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I$(BOOST_INC) -I../RvRtsp/RvRtspServer/include -I../RvRtsp/RvRtspServer/include/common -I../include

LIB_PATH     := -L$(BIN) -L$(BOOST_LIB)
LIB	        := -lpthread -lm -lrvrtspserver -lrvrtsp_server -lrvsdp_rtspserver -lrvcommon_rtspserver

MODULE_DEFINES :=-fPIC  -fvisibility=hidden -DLINUX -D_GNU_SOURCE -DXT_TCP_EXPORTS -DBOOST_ASIO_DISABLE_EPOLL -DOS_RAM
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o  

SRCC	    := $(wildcard *.c)
              
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)

OBJ	    := $(COBJ) $(CXXOBJ)

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*


$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo ar rcs $@ *.o
	@ar rcs $@ $^
	@cp $(TARG) $(TARGET)
#	@cp ./h_rtspserver.h ../../include

$(RELEASE_DIR)/%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	-$(RM) -rf $(RELEASE_DIR) #$(TARGET)
