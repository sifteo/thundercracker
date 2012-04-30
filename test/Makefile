TC_DIR := ..
SDK_DIR := $(abspath $(TC_DIR)/sdk)

SUBDIRS = \
	firmware/cube

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@PATH="$(SDK_DIR)/bin:$(PATH)" SDK_DIR="$(SDK_DIR)" $(MAKE) -C $@

clean:
	@PATH="$(SDK_DIR)/bin:$(PATH)" SDK_DIR="$(SDK_DIR)" $(MAKE) -C $@ clean
