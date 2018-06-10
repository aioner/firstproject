include ../profile

BIN         := ../../pub/$(TARGET_DIR)
TARGET      := $(BIN)/libtghelper.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

#DIST_INC    := -I../../third_library
INC_PATH    := -I$(BOOST_INC)

#DIST_LIB    := -L../../bin/linux
#DIST_LIB    := -L../../bin/$(TARGET_DIR)

#LIB_PATH    := $(DIST_LIB) 
LIB_PATH    := -L$(BOOST_LIB)
LIB         := -lpthread -lm -lrt

MODULE_DEFINES :=-fPIC -fvisibility=hidden
CFLAGS      := $(COMPILE_OPTIONS) $(MODULE_DEFINES)  -g -Wall -o 

SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	        := $(COBJ) $(CXXOBJ)

#PUB_INC	:= ../../include/tghelper

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@#$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $^ $(LIB)
	@echo ar rcs $@ "*.o"
	@ar rcs $@ $^
	@cp $(TARG) $(TARGET)
#	rm -fR $(PUB_INC)
#	mkdir -p $(PUB_INC)
#	@cp -R *.h -t $(PUB_INC)

$(RELEASE_DIR)/%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)
