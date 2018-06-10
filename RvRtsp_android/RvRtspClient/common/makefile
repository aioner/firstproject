CC          := gcc
CXX         := g++

INC_PATH    := -I../include/common

LIB_PATH    := -L../../../../bin
LIB	    := -lpthread -lm

#-fvisibility=hidden && in rvlinux.h: #define RVAPI __attribute__((visibility("default")))  
MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE  
CFLAGS	    := $(MODULE_DEFINES) -Wall -g -o

TARGET      := ../../../../bin/librvcommon_rtspclient.so

SRCC	    := $(wildcard ./adapters/windows/*.c ./ares/*.c ./cbase/*.c ./ccore/*.c) \
               $(wildcard ./ccore/memdrivers/*.c ./ccore/netdrivers/*.c) \
               $(wildcard ./ceutils/*.c ./cutils/*.c)
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)

OBJ	    := $(COBJ) $(CXXOBJ)

PUB_INC     :=../../../../include/RvRtspClient/common

$(TARGET):$(OBJ)
	$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $(OBJ) $(LIB) 
	rm -fR $(PUB_INC)
	mkdir -p $(PUB_INC)
	cp  ../include/common/*.h $(PUB_INC)

$(COBJ):%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(CXXOBJ):%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

.PHONY:clean
clean:
	rm -f $(OBJ)  # (TARGET) $(OBJ)
