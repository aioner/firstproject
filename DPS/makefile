include ../profile
BIN :=../../pub/$(TARGET_DIR)
TARGET:=$(BIN)/DPS
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

INC_PATH:= -I. -I../include -I../xt_xml -I../xt_sdp -I../device_access/2_0/Mediadevice -I../xt_media_server -I$(BOOST_INC) -I../../third_party/jk

LIB:= -lpthread -lrt -ldl -lxt_media_server  -lxt_mp_caster -lrv_adapter -lxt_sdp -lMediaDevice2.0 -lLinkComm2.0 -lxt_sip

LIB_PATH :=  -L$(BIN) -L$(BOOST_LIB) -L../../third_party/jk -L../../third_party/boost_1_55_0/lib/gcc_64

LIB+= -lboost_thread$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_filesystem$(BOOST_MT) -lboost_date_time$(BOOST_MT) -lJKMainClient -lJKMessageClient -lMsgLink -lxt_udx -lxt_log -lboost_exception -lboost_program_options

MODULE_DEFINES :=-g3 -O0 -DLINUX -D_GNU_SOURCE -DBOOST_THREAD_USES_MOVE
CFLAGS         := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -fPIC -o
	   
SRCC	    := $(wildcard *.c ./framework/*.c) 
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp ./framework/*.cpp) 
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
	
$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/%.o:./framework/%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<
clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
