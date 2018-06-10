CXX := g++
INCLUDED_DIRS := -I../../third_library

LIB_DIRS := -L../../bin
LIBS := 

CFLAGS := -fPIC -fvisibility=hidden -g -O0 -DXT_CONFIG_EXPORTS
LDFLAGS := -shared

BIN := ../../bin/libxt_config.so

CXX_SRCS := $(wildcard *.cpp)
CXX_OBJS := $(CXX_SRCS:%.cpp=%.o)

$(BIN):$(CXX_OBJS)
	$(CXX) $(LDFLAGS) $^ $(LIB_DIRS) $(LIBS) -o $@

$(CXX_OBJS):%.o:%.cpp
	$(CXX) $(CFLAGS) -c $< $(INCLUDED_DIRS) -o $@

.PHONY:clean
clean:
	-rm -f *.o $(BIN)
