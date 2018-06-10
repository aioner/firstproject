include ../../../profile

BIN         := ../../../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/librvrtsp_client.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I../include/common -I../include/rtsp

LIB_PATH    := -L../../../../bin/$(TARGET_DIR)
LIB	    := -lpthread -lm #-lrvcommon_rtspclient

MODULE_DEFINES :=-fPIC -fvisibility=hidden -DLINUX -D_GNU_SOURCE
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o  


SRCC	    := $(wildcard messages/*.c transport/*.c utils/*.c)
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)

OBJ	    := $(notdir $(COBJ)) $(notdir $(CXXOBJ))

PUB_INC     :=../../../../include/RvRtspClient/rtsp

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*
	

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@#$(CXX) $(INC_PATH) $(CFLAGS) $@ $(OBJ) $(LIB)
	@echo ar rcs $@ "*.o"
	ar rcs $@ $^
	@cp $(TARG) $(TARGET)
	@rm -fR $(PUB_INC)
	@mkdir -p $(PUB_INC)
	@cp  ../include/rtsp/*.h $(PUB_INC)
	
$(RELEASE_DIR)/%.o:messages/%.c
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/%.o:transport/%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/%.o:utils/%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
