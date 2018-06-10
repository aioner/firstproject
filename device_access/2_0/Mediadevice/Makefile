include ../../../profile

BIN         :=../../../../pub/$(TARGET_DIR)
TARGET      :=$(BIN)/libMediaDevice2.0.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I../include -I../LinkComm -I../../$(BOOST_INC)

DIST_LIB    :=-L$(BIN)
LIB_PATH    := $(DIST_LIB) -L../../$(BOOST_LIB)
LIB	        := -lpthread -ldl -lLinkComm2.0 -lboost_thread$(BOOST_MT) -lboost_system$(BOOST_MT)

MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE
CFLAGS         :=$(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o

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
