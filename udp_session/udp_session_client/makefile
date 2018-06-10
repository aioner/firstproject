include ../../profile

BIN         := ../../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/libudp_session_client.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I../common -I../$(BOOST_INC)

LIB_PATH    := -L$(BIN) -L../$(BOOST_LIB)
LIB	        := -lpthread -lm -lboost_thread$(BOOST_MT) -lboost_date_time$(BOOST_MT) -lboost_system$(BOOST_MT)

MODULE_DEFINES :=-fPIC -fvisibility=hidden -DLINUX -D_GNU_SOURCE 
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o  


SRCC	    := $(wildcard *.c)
              
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp ../common/*.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)

OBJ	    := $(COBJ) $(notdir $(CXXOBJ))

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
#	@cp  xt_udp_session_client.h ../../include

$(RELEASE_DIR)/%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $< 
	
$(RELEASE_DIR)/%.o:../common/%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $< 

clean:
	rm -rf $(RELEASE_DIR) $(TARG) #$(TARGET)
	
print:
	@echo $(SRCXX) $(SRCC)

