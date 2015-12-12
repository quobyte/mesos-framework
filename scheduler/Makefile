HEADERS = scheduler.hpp config.hpp
SOURCES := scheduler.cpp quobyte-mesos.cpp http_server.cpp
BINARY = quobyte-mesos
MESOS_SRC = ../thirdparty/mesos
INCLUDE_DIRS = -I../common -I$(MESOS_SRC)/src -I$(MESOS_SRC)/build/include \
    -I$(MESOS_SRC)/include -I$(MESOS_SRC)/build/src \
    -I$(MESOS_SRC)/build/3rdparty/zookeeper-3.4.5/src/c/include/ -I$(MESOS_SRC)/build/3rdparty/zookeeper-3.4.5/src/c/generated/ \
    -I$(MESOS_SRC)/build/3rdparty/libprocess/3rdparty/glog-0.3.3/src/ \
    -I$(MESOS_SRC)/build/3rdparty/libprocess/3rdparty/picojson-4f93734/

CXX = g++
CXXFLAGS = -g -pthread -std=c++11
LDFLAGS += -L../thirdparty/mesos -L../common -lmesos -lpthread -lprotobuf -lgflags -lmicrohttpd
CXXCOMPILE = $(CXX) $(INCLUDE_DIRS) $(INCLUDES) $(CXXFLAGS) -c $<
CXXLINK = $(CXX) $(LDFLAGS) -o $(BINARY)

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
