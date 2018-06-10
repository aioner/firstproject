include ../profile
	
BIN :=../../pub/$(TARGET_DIR)
TARGET:=$(BIN)/XTRouter
TARG:=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif
CLB :=
ifeq ($(TYPE),linux_32)
CLB :=gcc32
XLIB :=x86
endif

ifeq ($(TYPE),linux_64)
CLB :=gcc64
XLIB :=x64
endif

INC_PATH:= -I./framework -I./framework/detail -I./mml -I../include -I../xt_xml -I../xt_sdp -I../device_access/2_0/Mediadevice -I../xt_media_server \
			-I$(BOOST_INC) -I$(THIRD_PATH)/xt_sip/include  -I$(THIRD_PATH)/XmppGlooxApply -I$(THIRD_PATH)/xtlog_public \
			-I$(THIRD_PATH)/web_srv/include -I$(THIRD_PATH)/snmp -I$(THIRD_PATH)/snmp/include -I$(THIRD_PATH)/CommunicationLib -I../xt_boost_log

LIB:= -lpthread -lrt -ldl -lz -lxt_media_server  -lxt_mp_caster -lrv_adapter -lxt_boost_log -lxt_log -lxt_sdp -lxt_sip -lMediaDevice2.0 -lLinkComm2.0 -lXmppGlooxApply -lgloox \
		-lDevicePerformance -lStatusSlaveInterface -lShareMemoryDeal -lxtxkWebSrv -lCommunicationLib -luuid

LIB_PATH :=  -L$(BIN) -L$(BIN)/dll/xt -L$(BIN)/dll/trans_server -L$(BOOST_LIB) 

ifneq ($(TYPE),arm_A8)
COMPILE_OPTIONS+= -D_USE_WEB_SRV_
INC_PATH += -I$(THIRD_PATH)/web_srv/include
LIB_PATH += -L$(THIRD_PATH)/web_srv/$(CLB)
LIB += -lxtxkWebSrv

#COMPILE_OPTIONS+= -D_USE_XMPP_FUNC_
#INC_PATH += -I$(THIRD_PATH)/XmppGlooxApply
#LIB_PATH += -L$(THIRD_PATH)/XmppGlooxApply/$(XLIB)
#LIB+= -lXmppGlooxApply

#COMPILE_OPTIONS+= -D_USE_COMMON_CTRL_MSG_
#INC_PATH += -I$(THIRD_PATH)/CommunicationLib
#LIB_PATH += -L$(THIRD_PATH)/CommunicationLib/lib/$(CLB)
#LIB+= -lCommunicationLib -luuid

#COMPILE_OPTIONS += -DUSE_SNMP_
#INC_PATH += -I$(THIRD_PATH)/snmp -I$(THIRD_PATH)/snmp/include 
#LIB_PATH +=
#LIB+= -lDevicePerformance -lStatusSlaveInterface -lShareMemoryDeal
endif

LIB+= -lboost_thread$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_filesystem$(BOOST_MT) -lboost_date_time$(BOOST_MT) -lboost_serialization$(BOOST_MT)

MODULE_DEFINES :=-g3 -O0 -DLINUX -D_GNU_SOURCE
CFLAGS         := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -fPIC -o
	   
SRCC	    := $(wildcard *.c ./framework/*.c ./framework/detail/*.c ./mml/*.c) 
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp ./framework/*.cpp ./framework/detail/*.cpp ./mml/*.cpp) 
SRCXX       += $(wildcard ../include/base64/base64.cpp ../include/xt_ping/icmp_ping.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(notdir $(COBJ)) $(notdir $(CXXOBJ))

.PHONY:release $(TARG) clean

release:$(RELEASE_DIR)/. $(TARG)
	@###
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo CXX -o $@ *.o -I* -L* -l*
	@$(CXX) -v  $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
	@$(STRIPCMD)
	@cp $(TARG) $(TARGET)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/%.o:./framework/%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:./mml/%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:./framework/detail/%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/%.o:./framework/%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:./mml/%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:./framework/detail/%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/base64.o:../include/base64/base64.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/icmp_ping.o:../include/xt_ping/icmp_ping.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
	
