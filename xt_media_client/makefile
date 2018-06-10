include ../profile

BIN         := ../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/dll/xt/libxt_media_client.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I../ -I../include -I../xt_mp_sink -I../xt_rtsp_client -I../udp_session/udp_session_client \
			-I../RvRtsp/RvRtspClient/include/rtsp -I../RvRtsp/RvRtspClient/include/common -I$(BOOST_INC) 

LIB_PATH    :=  -L$(BIN) -L$(BIN)/dll/xt/ -L$(BOOST_LIB)
LIB	    := -lpthread -lm -lxt_mp_sink -ludp_session_client -ltcp_session_client -lxt_rtsp_client -lxt_tcp -lrvrtspclient -lrvrtsp_client -lrvsdp_rtspclient -lrvcommon_rtspclient \
		 -lboost_thread$(BOOST_MT) -lboost_filesystem$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_date_time$(BOOST_MT)

MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE -DUSE_POST_TASK
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o  



SRCC	    := $(wildcard *.c)
              
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)

OBJ	    := $(COBJ) $(CXXOBJ)

#PUB_INC	:= ../../include/xt_media_client

.PHONY:release build $(TARG) clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo CXX $@ "*.o" -I* -L* -l*
	@$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
	@$(STRIPCMD)
	@cp $(TARG) $(TARGET)
#	@cp xt_media_client_types.h $(PUB_INC)
#	@cp xt_media_client.h $(PUB_INC)	

$(RELEASE_DIR)/%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) $(TARG) #$(TARGET)

