include ../profile

BIN :=../../pub/$(TARGET_DIR)
TARGET  := $(BIN)/JKMainClientProxy
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH +=-I../xt_xml -I../include -I$(THIRD_PATH) -I../xt_boost_log -I$(BOOST_INC)

MODULE_DEFINES :=-w -g -std=c++0x  -o
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES)


LIB_PATH := -L$(BIN) -L$(BOOST_LIB)
LIB := -lpthread -lrt -ldl -lc\
	 -lJKMessageClient -lJKMainClient  -lMsgLink -lxt_udx -lxt_xml -lxt_boost_log -lxt_log\
	 -lboost_thread$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_filesystem$(BOOST_MT) -lboost_date_time$(BOOST_MT) -lboost_serialization$(BOOST_MT) -lboost_serialization$(BOOST_MT)

SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(COBJ) $(CXXOBJ)

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

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
