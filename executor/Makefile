HEADERS = config.hpp
SOURCES := quobyte-mesos-executor.cpp executor.cpp
BINARY = quobyte-mesos-executor

CXX = g++
CXXFLAGS = -g -O2 -pthread -std=c++11
LDFLAGS += -lmesos -lprotobuf
CXXCOMPILE = $(CXX) $(INCLUDE_DIRS) $(INCLUDES) $(CXXFLAGS) -c $< 
CXXLINK = $(CXX) $(LINK_DIRS) $(LDFLAGS) -o $(BINARY)

OBJS := $(SOURCES:.cpp=.o)
default: all
all: $(BINARY)

executor.tar.gz: $(BINARY)
	./gather_libs.sh
	tar cvf executor.tar.gz $(BINARY) .libs

$(BINARY): $(OBJS)
	$(CXXLINK) $(OBJS) ../common/libquobyteproto.a
	
.cpp.o: $(HEADERS)
	$(CXXCOMPILE)
	
.cc.o: $(HEADERS)
	$(CXXCOMPILE)
	
clean:
	(rm -f quobyte-mesos $(OBJS))
