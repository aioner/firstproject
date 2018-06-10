include ../profile


BIN:=../../pub/$(TARGET_DIR)
TARGET := $(BIN)/libxt_sdp.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))

STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

AR := ar

INC_PATH :=-I./
#CXXFLAGS := -g3 -O0 -Wall -fPIC -fvisibility=hidden 
CFLAGS :=$(COMPILE_OPTIONS) -g -o0 -Wall -fPIC -fvisibility=hidden 

LIB_PATH :=
LIBS :=
LD_FLAGS := 


CXX_SRCS := $(wildcard *.cpp)
CXX_OBJS := $(CXX_SRCS:%.cpp=%.o)

OBJ := $(CXX_OBJS)

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo ar rcs $@ "*.o"
	@$(AR) rcs $@ $^
	@cp $(TARG) $(TARGET)

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) -c $(CFLAGS)  $< -o $@ $(INC_PATH)


clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)






