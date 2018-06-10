include ../../../profile

BIN         :=../../../../pub/$(TARGET_DIR)
TARGET      :=$(BIN)/librvrtsp_server.a
TARG       :=$(RELEASE_DIR)/$(notdir $(TARGET))


STRIPCMD:=$(STRIP) $(TARG)
ifeq ($(STRIP),)
STRIPCMD:=
endif

INC_PATH    := -Imessages -Irtspcommon -Itransport -Iutils -I../include

LIB_PATH    := -L../../../../bin/$(TARGET_DIR)
LIB	    := -lpthread -lm -lrvcommon_rtspserver

MODULE_DEFINES :=-fPIC -shared -DLINUX -D_GNU_SOURCE

CFLAGS	    := $(COMPILE_OPTIONS) $(MODULE_DEFINES) -Wall -g -o  

SRCC	    := $(wildcard messages/*.c rtspcommon/*.c transport/*.c) \
               $(wildcard utils/*.c)
COBJ	    := $(SRCC:.c=.o)

SRCXX	    := $(wildcard *.cpp)
CXXOBJ      := $(SRCXX:.cpp=.o)
OBJ	    := $(notdir $(COBJ)) $(CXXOBJ)

.PHONY:release build clean

release:$(RELEASE_DIR)/. $(TARG)
	@###

build:
%/.:
	mkdir -m 777 -p $*

$(TARG):$(addprefix $(RELEASE_DIR)/,$(OBJ))
	@echo ar rcs $@ *.o
	@ar rcs $@ $^
	@cp $(TARG) $(TARGET)

$(RELEASE_DIR)/%.o:messages/%.c
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:rtspcommon/%.c
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:transport/%.c
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:utils/%.c
	$(CC)  $(INC_PATH) $(CFLAGS) $@ -c $<

$(RELEASE_DIR)/%.o:%.cpp
	$(CXX) $(INC_PATH) $(CFLAGS) $@ -c $<

clean:
	rm -rf $(RELEASE_DIR) #$(TARGET)


