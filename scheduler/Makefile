HEADERS = scheduler.hpp config.hpp
SOURCES := scheduler.cpp quobyte-mesos.cpp http_server.cpp
BINARY = quobyte-mesos

CXX = g++
CXXFLAGS = -g -pthread -std=c++11
LDFLAGS += $(LIBRARY_DIRS) -lmesos -lpthread -lprotobuf -lgflags -lmicrohttpd
CXXCOMPILE = $(CXX) $(INCLUDE_DIRS) $(INCLUDES) $(CXXFLAGS) -c $<
CXXLINK = $(CXX) $(LINK_DIRS) $(LDFLAGS) -o $(BINARY)

OBJS := $(SOURCES:.cpp=.o)
default: all
all: $(BINARY)

$(BINARY): $(OBJS)
	$(CXXLINK) $(OBJS) ../common/libquobyteproto.a
	
.cpp.o: $(HEADERS)
	$(CXXCOMPILE)
	
.cc.o: $(HEADERS)
	$(CXXCOMPILE)
	
quobyte.pb.o: quobyte.proto
	protoc quobyte.proto --cpp_out=.
	
clean:
	(rm -f quobyte-mesos $(OBJS))
