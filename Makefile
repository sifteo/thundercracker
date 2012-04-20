TC_DIR := .
include Makefile.platform

TOOLS := emulator stir vm firmware
SUBDIRS := $(TOOLS) docs/doxygen sdk/examples

.PHONY: clean subdirs $(SUBDIRS)

all: sdk-deps $(SUBDIRS)

subdirs: $(SUBDIRS)

tools: $(TOOLS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean: sdk-deps-clean
	rm -Rf sdk/doc/*
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

include Makefile.sdk-deps
