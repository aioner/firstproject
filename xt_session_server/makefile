include ../profile
BIN         :=../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/libxt_session_server.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    :=  -I$(BOOST_INC) -I../include -I../tcp_session_server -I../RtspServer -I../udp_session/udp_session_server

LIB_PATH    := -L$(BIN) 
LIB	        := -lxt_rtsp_server -ltcp_session_server -ludp_session_server -lpthread -lm 
MODULE_DEFINES :=-fPIC  -fvisibility=hidden -DLINUX -D_GNU_SOURCE -D_USE_XT_TCP_SESSION_SERVER -D_USE_XT_RTSP_SESSION_SERVER
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
	@echo ar rcs $@ "*.o"
	@ar rcs $@ $^
	@cp $(TARG) $(TARGET)
#	@cp ./h_xtsessionserver.h ../../include

$(RELEASE_DIR)/%.o:%.c
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $< 

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
	
print:
	@echo $(SRCXX) $(SRCC)

