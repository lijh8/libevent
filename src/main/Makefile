# build dynamic library with -fPIC -shared
CFLAGS   = -g # -O3 -fPIC
CXXFLAGS = -g # -O3 -fPIC
CPPFLAGS = -MMD -MP # -I../foo -DNDEBUG
LDFLAGS  = # -L../foo -shared -static
LDLIBS   = -levent

ifneq ($(wildcard *.cpp),)
CC       = $(CXX) # link with CXX for .cpp
endif

all: server client

# target name is basename of one of the source files
# main: $(addsuffix .o,$(basename $(wildcard *.c *.cpp)))
server: $(addsuffix .o,$(basename $(wildcard server.cpp)))
client: $(addsuffix .o,$(basename $(wildcard client.cpp)))
-include *.d
clean: ; rm -fr *.o *.d server client
.PHONY: clean
