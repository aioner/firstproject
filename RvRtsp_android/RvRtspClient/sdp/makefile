CC          := gcc
CXX         := g++

INC_PATH    := -I../include/common -I../include/rtsp

LIB_PATH    := -L../../../../bin
LIB	    := -lpthread -lm -lrvcommon_rtspclient

MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE
CFLAGS	    := $(MODULE_DEFINES) -Wall -g -o  

TARGET      := ../../../../bin/librvsdp_rtspclient.so

SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)

OBJ	    := $(COBJ) $(CXXOBJ)

$(TARGET):$(OBJ)
	$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $(OBJ) $(LIB) 

$(COBJ):%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(CXXOBJ):%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

.PHONY:clean
clean:
	rm -f $(OBJ)  # (TARGET) $(OBJ)
