CC          := gcc
CXX         := g++

DIST_INC    := -I../inc/common
IPCAM_INC   := -I../inc/rtprtcp
INC_PATH    := $(DIST_INC)  $(IPCAM_INC)

DIST_LIB    := -L../build
LIB_PATH    := $(DIST_LIB) 
LIB	    := -lpthread -lm


MODULE_DEFINES :=-fPIC -fvisibility=hidden  -DLINUX -D_GNU_SOURCE -DRV_CFLAG_EPOLL -D_RV_LINUX_API_HIDDEN
CFLAGS	    := $(MODULE_DEFINES)  -Wall -g -o  

TARGET      := ../build/librvcommon.a  #so
SRCC	    := $(wildcard *.c)
COBJ	    := $(SRCC:.c=.o)
SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(COBJ) $(CXXOBJ)

$(TARGET):$(OBJ)
	#$(CXX) $(INC_PATH) $(LIB_PATH) $(CFLAGS) $@ $(OBJ) $(LIB) 
	ar rcs $(TARGET) $(OBJ) 

$(COBJ):%.o:%.c 
	$(CC) $(INC_PATH) $(CFLAGS) $@ -c $<

$(CXXOBJ):%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

.PHONY:clean
clean:
	rm -f *~ *.o  #$(TARGET) 
