include ../profile

BIN:=../../pub/$(TARGET_DIR)
TARGET:=$(BIN)/libxt_xml.a
TARG:=$(RELEASE_DIR)/$(notdir $(TARGET))

INC_PATH += -I.

AR_FLAGS += -r 

MODULE_DEFINES :=-fPIC
CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o 


SRCC	    := $(wildcard *.c)
              
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp tinyxml/*.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
CXOBJ       := $(notdir $(CXXOBJ))

OBJ	    := $(COBJ) $(CXOBJ)

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo ar rcs $@ "*.o" 
	@ar rcs  $@ $^
	@cp $(TARG) $(TARGET)
	#@cp xtXml.h ../../include

$(RELEASE_DIR)/%.o:%.c 
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp 
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $< 

$(RELEASE_DIR)/%.o:tinyxml/%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $< 

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)


