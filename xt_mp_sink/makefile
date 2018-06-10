include ../profile

BIN        := ../../pub/$(TARGET_DIR)
TARGET		:= $(BIN)/dll/xt/libxt_mp_sink.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH	:= -I../ -I../include -I../rv_adapter -I../xt_xml -I$(BOOST_INC) -I$(AV_CHECK_SN_INC)


DIST_LIB	:= -L$(BIN)  -L$(BOOST_LIB)
LIB_PATH	:= $(DIST_LIB) 
LIB		= -lxt_xml -lpthread -lm -lz -lrv_adapter -lrv32rtp -lrvcommon -ltghelper  $(AV_CHECK_SN_LIB) \
		-lboost_thread$(BOOST_MT) -lboost_filesystem$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_date_time$(BOOST_MT)

MODULE_DEFINES	:=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE 
CFLAGS		:= $(COMPILE_OPTIONS) $(MODULE_DEFINES) -g -Wall -o 

SRCC		:= $(wildcard *.c)
COBJ		:= $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ		:= $(SRCXX:.cpp=.o)
OBJ		:= $(COBJ) $(CXXOBJ)

PUB_INC		:= ../../include/xt_mp_sink

.PHONY:release build $(TARG) clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo CXX $(TARGET) *.o -I* -L* -l*
	@$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
#rm -fR $(PUB_INC)
#	mkdir -p $(PUB_INC)
	$(STRIPCMD)
	@cp $(TARG) $(TARGET)
#	@cp xt_mp_sink_api.h $(PUB_INC)
#	@cp xt_mp_sink_def.h $(PUB_INC)
#	@cp ../../share/xt_mp_def.h $(PUB_INC)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) $(TARG) #$(TARGET)
