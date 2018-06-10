include ../../../profile

BIN         :=../../../../pub/$(TARGET_DIR)
TARGET      :=$(BIN)/libHCDvrDevice2.0.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    :=-I../include -I../LinkComm -I../../../../third_party/device_libs/hc/inc_linux

HC_DIR :=
ifeq ($(TYPE),linux_32)
HC_DIR := gcc32
endif
ifeq ($(TYPE),linux_64)
HC_DIR := gcc64
endif
DIST_LIB    :=-L$(BIN) -L../../$(THIRD_PATH)/device_libs/hc/lib/$(HC_DIR) -L../../$(THIRD_PATH)/device_libs/hc/lib/$(HC_DIR)/HCNetSDKCom
LIB_PATH    := $(DIST_LIB) 
LIB	        := -lpthread -ldl -lrt -lLinkComm2.0 -lhcnetsdk -lHCCore -lhpr -lanalyzedata
#-Wl,-rpath=../../$(THIRD_PATH)/device_libs/hc/lib/$(HC_DIR)/HCNetSDKCom
MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE
CFLAGS         := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o

SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(notdir $(COBJ)) $(notdir $(CXXOBJ))

.PHONY:release $(TARG) clean

release:$(RELEASE_DIR)/. $(TARG)
	@###
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo CXX -o $(TARGET) *.o -L* -I* -l*
	@$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
	$(STRIPCMD)
	@cp $(TARG) $(TARGET)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<
$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<
clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)