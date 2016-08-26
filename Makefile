ROOT_DIR=$(realpath .)
MESOS_LIB_DIR=$(ROOT_DIR)/thirdparty/mesos/src/.libs/
export MESOS_LIB_DIR
export MESOS_VERSION

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

