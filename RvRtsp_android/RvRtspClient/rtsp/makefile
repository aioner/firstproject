CC          := gcc
CXX         := g++

INC_PATH    := -I../include/common -I../include/rtsp

LIB_PATH    := -L../../../../bin
LIB	    := -lpthread -lm -lrvcommon_rtspclient

MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE
CFLAGS	    := $(MODULE_DEFINES) -Wall -g -o  

TARGET      := ../../../../bin/librvrtsp_client.so

SRCC	    := $(wildcard messages/*.c transport/*.c utils/*.c)
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)

OBJ	    := $(COBJ) $(CXXOBJ)

PUB_INC     :=../../../../include/RvRtspClient/rtsp

$(TARGET):$(OBJ)
	$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $(OBJ) $(LIB) 
	rm -fR $(PUB_INC)
	mkdir -p $(PUB_INC)
	cp  ../include/rtsp/*.h $(PUB_INC)
	
$(COBJ):%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(CXXOBJ):%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

.PHONY:clean
clean:
	rm -f $(OBJ)  # (TARGET) $(OBJ)
