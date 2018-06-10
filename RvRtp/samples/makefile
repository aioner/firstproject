CC          := gcc
CXX         := g++

DIST_INC    := -I/home/kongfd/RvRtp/inc/common
IPCAM_INC		:= -I/home/kongfd/RvRtp/inc/rtprtcp
INC_PATH    := $(DIST_INC)  $(IPCAM_INC)

DIST_LIB   	:= -L/home/kongfd/RvRtp/build
LIB_PATH    := $(DIST_LIB) 
LIB	    		:= -lpthread -lm -lrvcommon -lrv32rtp

MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE
CFLAGS	    := $(MODULE_DEFINES)  -Wall -O2 -o 

TARGET      := sample
SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(COBJ) $(CXXOBJ)

$(TARGET):$(OBJ)
	$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $(OBJ) $(LIB) 

$(COBJ):%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(CXXOBJ):%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

.PHONY:clean
clean:
	rm -f $(TARGET) *~ *.o
