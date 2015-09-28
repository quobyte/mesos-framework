HEADERS = 
SOURCES := quobyte.pb.cc
BINARY = libquobyteproto.a
MESOS_SRC = ../thirdparty/mesos
CXX = g++
CXXFLAGS = -g -O2 -pthread -std=c++11
LDFLAGS += -L../thirdparty/mesos -lmesos -lpthread -lprotobuf -lgflags
CXXCOMPILE = $(CXX) $(INCLUDES) $(CXXFLAGS) -c $<
CXXLINK = $(CXX) $(LDFLAGS) -o $(BINARY)

OBJS := $(SOURCES:.cc=.o)
default: $(BINARY)

.cc.o: $(HEADERS)
	$(CXXCOMPILE)
	
quobyte.pb.cc: quobyte.proto
	protoc quobyte.proto --cpp_out=.

$(BINARY): $(OBJS)
	ar rcs $@ $^
	ranlib $@

clean:
	(rm -f libquobyte.a $(OBJS) $(SOURCES))
