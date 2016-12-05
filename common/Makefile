HEADERS = 
SOURCES := quobyte.pb.cc
BINARY = libquobyteproto.a

CXX = g++
CXXFLAGS = -g -O2 -pthread -std=c++11
LDFLAGS += -lmesos -lpthread -lprotobuf -lgflags
CXXCOMPILE = $(CXX) $(INCLUDE_DIRS) $(CXXFLAGS) -c $<
CXXLINK = $(CXX) $(LINK_DIRS) $(LDFLAGS) -o $(BINARY)
PROTOC = $(MESOS_SRC)/build/3rdparty/protobuf-2.6.1/src/protoc

OBJS := $(SOURCES:.cc=.o)
default: $(BINARY)

.cc.o: $(HEADERS)
	$(CXXCOMPILE)
	
quobyte.pb.cc: quobyte.proto
	$(PROTOC) quobyte.proto --cpp_out=.

$(BINARY): $(OBJS)
	ar rcs $@ $^
	ranlib $@

clean:
	(rm -f libquobyte.a $(OBJS) $(SOURCES))
