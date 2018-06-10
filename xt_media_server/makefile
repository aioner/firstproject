include ../profile

BIN :=../../pub/$(TARGET_DIR)
TARGET  := $(BIN)/libxt_media_server.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH +=-I.. -I../xt_sdp -I../xt_xml -I../include -I../xt_mp_caster -I../xt_session_server \
			 -I../rv_adapter -I$(THIRD_PATH)/xt_sip/include -I$(BOOST_INC)

MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o 


LIB_PATH := -L$(BIN) -L$(BOOST_LIB)
LIB  := -lpthread -lm -lxt_session_server -lxt_rtsp_server -lrvrtspserver -lrvrtsp_server -lrvsdp_rtspserver -lrvcommon_rtspserver \
		-ludp_session_server -ltcp_session_server -lxt_tcp -lxt_mp_caster -lrv_adapter -lrvcommon -ltghelper -lxt_sdp -lxt_sip -lxt_xml \
		-lboost_thread$(BOOST_MT) -lboost_filesystem$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_date_time$(BOOST_MT) -lboost_atomic$(BOOST_MT)

SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	        := $(COBJ) $(CXXOBJ)

.PHONY:release $(TARG) clean

release:$(RELEASE_DIR)/. $(TARG)
	@###
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo CXX -o $@ *.o -I* -L* -l*
	@$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
	@$(STRIPCMD)
	@cp $(TARG) $(TARGET)
	#@cp h_xtmediaserver.h ../../pub/include

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<
clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)

