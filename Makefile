all: scheduler common executor
.PHONY: scheduler common executor

scheduler: common executor
	$(MAKE) -C scheduler
	
common:
	$(MAKE) -C common
	
executor: common
	$(MAKE) -C executor
