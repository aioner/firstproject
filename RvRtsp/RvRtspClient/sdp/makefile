include ../../../profile

BIN         := ../../../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/librvsdp_rtspclient.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I../include/common -I../include/rtsp

LIB_PATH    := -L../../../../bin/$(TARGET_DIR)
LIB	    := -lpthread -lm -lrvcommon_rtspclient

MODULE_DEFINES :=-fPIC -fvisibility=hidden -DLINUX -D_GNU_SOURCE
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
#	$(CXX) $(INC_PATH)  $(CFLAGS) $@ $^ $(LIB)
	@echo ar rcs $@ "*.o"
	@ar rcs $@ $^
	@cp $(TARG) $(TARGET)

$(RELEASE_DIR)/%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
