include ../profile

BIN :=../../pub/$(TARGET_DIR)
TARGET:=$(BIN)/webserver
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

INC_PATH	:= -I./

DIST_LIB	:= -L$(BIN)
LIB_PATH	:= $(DIST_LIB) 
LIB		= -lpthread -lrt

MODULE_DEFINES	:=-g3 -O0 -DLINUX -D_GNU_SOURCE
CFLAGS		:= $(COMPILE_OPTIONS) $(MODULE_DEFINES) -std=c++0x -Wall -o

SRCC		:= $(wildcard *.c)
COBJ		:= $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ		:= $(SRCXX:.cpp=.o)
OBJ		:= $(COBJ) $(CXXOBJ)

#PUB_INC		:= ../include

.PHONY:release build $(TARG) clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo CXX $(TARGET) *.o -I* -L* -l*
	@$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
#rm -fR $(PUB_INC)
#	mkdir -p $(PUB_INC)
	$(STRIPCMD)
	@cp $(TARG) $(TARGET)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) $(TARG) #$(TARGET)
