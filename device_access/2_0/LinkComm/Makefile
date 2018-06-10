include ../../../profile

BIN         :=../../../../pub/$(TARGET_DIR)
TARGET      :=$(BIN)/libLinkComm2.0.so
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -I../include

DIST_LIB    :=
LIB_PATH    := $(DIST_LIB) 
LIB	        := -lpthread -ldl

MODULE_DEFINES :=-fPIC -shared -fvisibility=hidden -DLINUX -D_GNU_SOURCE
CFLAGS         := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o

SRCC	    := $(wildcard *.c ./tinyxml/*.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp ./tinyxml/*.cpp)
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

$(RELEASE_DIR)/%.o:./tinyxml/%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<
	
$(RELEASE_DIR)/%.o:./tinyxml/%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)