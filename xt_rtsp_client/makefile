include ../profile

BIN         := ../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/libxt_rtsp_client.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I$(BOOST_INC) -I../RvRtsp/RvRtspClient/include/common -I../RvRtsp/RvRtspClient/include/rtsp -I../include

LIB_PATH    := -L$(BIN) -L$(BOOST_LIB)
LIB	    := -lrvrtspclient -lrvrtsp_client -lrvsdp_rtspclient -lrvcommon_rtspclient \
		    -lboost_thread$(BOOST_MT) -lboost_filesystem$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_date_time$(BOOST_MT) -lpthread -lm
			
#MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden -DBOOST_THREAD_USES_MOVE -DLINUX -D_GNU_SOURCE
MODULE_DEFINES :=-fPIC -fvisibility=hidden -DBOOST_THREAD_USES_MOVE -DLINUX -D_GNU_SOURCE
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o  
#TARGET      := ../../bin/$(TARGET_DIR)/libxt_rtsp_client.a

SRCC	    := $(wildcard *.c)
              
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)

OBJ	    := $(COBJ) $(CXXOBJ)

PUB_INC	:= ../../include/xt_rtsp_client

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo ar rcs $@ "*.o"
	@ar rcs $@ $^
	@cp $(TARG) $(TARGET)
#	@cp xt_rtsp_client_types.h $(PUB_INC)
#	@cp xt_rtsp_client.h $(PUB_INC)

$(RELEASE_DIR)/%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) $(TARG) #$(TARGET)