ROOT_DIR=$(realpath .)
MESOS_LIB_DIR=$(ROOT_DIR)/thirdparty/mesos/src/.libs/
export MESOS_LIB_DIR

MESOS_SRC=$(ROOT_DIR)/thirdparty/mesos
export MESOS_SRC

MESOS_VERSION=1.0.0
export MESOS_VERSION

INCLUDE_DIRS = -I../common \
    -I$(MESOS_SRC)/src \
    -I$(MESOS_SRC)/build/include \
    -I$(MESOS_SRC)/include/mesos \
    -I$(MESOS_SRC)/include \
    -I$(MESOS_SRC)/build/src \
    -I$(MESOS_SRC)/build/3rdparty/protobuf-2.6.1/src \
    -I$(MESOS_SRC)/build/3rdparty/libprocess/3rdparty/glog-0.3.3/src/ \
    -I$(MESOS_SRC)/build/3rdparty/zookeeper-3.4.8/src/c/include/ \
    -I$(MESOS_SRC)/build/3rdparty/zookeeper-3.4.8/src/c/generated/ \
    -I$(MESOS_SRC)/3rdparty/stout/include/ \
    -I$(MESOS_SRC)/3rdparty/libprocess/include \
    -I$(MESOS_SRC)/3rdparty/libprocess/3rdparty/picojson-1.3.0/

#    -I$(MESOS_SRC)/build/3rdparty/libprocess/3rdparty/picojson-4f93734/
export INCLUDE_DIRS

LINK_DIRS = -L$(MESOS_SRC)/build/3rdparty/protobuf-2.6.1/src/.libs -L$(MESOS_LIB_DIR)
export LINK_DIRS

all: scheduler common executor
.PHONY: scheduler common executor clean

scheduler: common executor
	$(MAKE) -C scheduler
	
common:
	$(MAKE) -C common
	
executor: common
	$(MAKE) -C executor

clean:
	$(MAKE) -C common clean
	$(MAKE) -C scheduler clean
	$(MAKE) -C executor clean

