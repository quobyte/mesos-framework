all: scheduler common executor
.PHONY: scheduler common executor clean docker

scheduler: common executor
	$(MAKE) -C scheduler
	
common:
	$(MAKE) -C common
	
executor: common
	$(MAKE) -C executor

clean:
	$(MAKE) -C executor clean
	$(MAKE) -C common clean
	$(MAKE) -C scheduler clean

