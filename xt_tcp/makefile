include ../profile

BIN         :=	../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/libxt_tcp.a
TARG        := $(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I$(BOOST_INC)

LIB_PATH    := -L../../pub/$(TARGET_DIR) -L$(BOOST_LIB)
LIB	    := -lpthread -lm -lboost_thread -lboost_filesystem -lboost_system -lboost_date_time

MODULE_DEFINES :=-fPIC -fvisibility=hidden -DLINUX -D_GNU_SOURCE -DXT_TCP_EXPORTS #-DBOOST_ASIO_DISABLE_EPOLL
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
	#@cp ./xt_tcp.h ../../include/xt_tcp
	#@cp ./xt_tcp_wrapper.h ../../include/xt_tcp

$(RELEASE_DIR)/%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
