include ../profile

BIN        := ../../pub/$(TARGET_DIR)
TARGET		:= $(BIN)/libxt_boost_log.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH	:= -I../ -I$(BOOST_INC)


DIST_LIB	:= -L$(BIN)  -L$(BOOST_LIB)
LIB_PATH	:= $(DIST_LIB) 
LIB		=  -lboost_thread$(BOOST_MT) -lboost_filesystem$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_date_time$(BOOST_MT) -lboost_log$(BOOST_MT)

MODULE_DEFINES	:=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE 
CFLAGS		:= $(COMPILE_OPTIONS) $(MODULE_DEFINES) -g -Wall -o 

SRCC		:= $(wildcard *.c)
COBJ		:= $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ		:= $(SRCXX:.cpp=.o)
OBJ		:= $(COBJ) $(CXXOBJ)

PUB_INC		:= ../../include/xt_boost_log

.PHONY:release build $(TARG) clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo CXX $(TARGET) *.o -I* -L* -l*
	@$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)

	$(STRIPCMD)
	@cp $(TARG) $(TARGET)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) $(TARG) #$(TARGET)
