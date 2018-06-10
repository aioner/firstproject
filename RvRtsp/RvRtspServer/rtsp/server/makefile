include ../../../../profile

BIN         :=../../../../../pub/$(TARGET_DIR)
TARGET      :=$(BIN)/librvrtspserver.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I./ -I../messages -I../rtspcommon -I../transport -I../utils \
               -I../../include

LIB_PATH    := -L$(BIN)
LIB	    := -lpthread -lm -lrvcommon_rtspserver -lrvrtsp_server

MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE
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
	@cp ./RvRtspServerSession.h  ../../include/
	@cp ./RvRtspServerTypes.h  ../../include/
	@cp ./RvRtspServerInc.h  ../../include/
	@cp ./RvRtspServerConnection.h  ../../include/
	@cp ./RvRtspServer.h ../../include/

$(RELEASE_DIR)/%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
