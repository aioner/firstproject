include ../profile

BIN         :=../../pub/$(TARGET_DIR)
TARGET      :=$(BIN)/librv_adapter.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I$(BOOST_INC) -I../ -I../RvRtp/inc/common -I../RvRtp/inc/rtprtcp

DIST_LIB    := -L$(BIN) -L$(BOOST_LIB) 
LIB_PATH    := $(DIST_LIB) 
LIB	    := -lpthread -lm -ltghelper -lrv32rtp -lrvcommon -lboost_thread$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_date_time$(BOOST_MT) -lboost_atomic$(BOOST_MT)

#LIB	    := -lpthread -lm -ltghelper -lrv32rtp -lrvcommon -lboost_thread -lboost_system -lboost_date_time -lboost_atomic
MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE
CFLAGS         := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -o 

SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(COBJ) $(CXXOBJ)

PUB_INC	:= ../../include/rv_adapter

.PHONY:release build $(TARG) clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo CXX -o $(TARGET) *.o -L* -I* -l*
	@$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
	$(STRIPCMD)
	@cp $(TARG) $(TARGET)
#rm -fR $(PUB_INC)
#	mkdir -p $(PUB_INC)
#	@cp rv_def.h $(PUB_INC)
#	@cp rv_api.h $(PUB_INC)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)

