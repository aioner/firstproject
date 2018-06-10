include ../profile

BIN:=../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/libxt_media_device.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I../include -I../xt_media_client -I../device_access/2_0/LinkComm -I../device_access/2_0/include -I$(BOOST_INC) -I../xt_boost_log


DIST_LIB    := -L$(BIN) -L$(BOOST_LIB)
LIB_PATH    := $(DIST_LIB) 
LIB	    := -lxt_media_client -lxt_boost_log -lpthread -lm -lboost_thread$(BOOST_MT) -lboost_system$(BOOST_MT) -lrt

#MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL
#MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden  -DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL -D_RV_LINUX_API_HIDDEN
MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden  -DLINUX -D_GNU_SOURCE
CFLAGS	    := $(MODULE_DEFINES)  $(COMPILE_OPTIONS) -Wall -g -o  

#TARGET      := $(BIN)/libxt_media_device.so
SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	        := $(COBJ) $(CXXOBJ)

.PHONY:release build clean

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
	

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) $(TARG) #$(TARGET)

