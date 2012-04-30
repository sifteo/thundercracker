TC_DIR := .
SDK_DIR := $(abspath $(TC_DIR)/sdk)
include Makefile.platform

TOOLS := emulator stir vm firmware
DOCS := docs/doxygen
EXAMPLES := sdk/examples
TESTS := test

.PHONY: clean subdirs $(TOOLS) $(DOCS) $(TESTS) $(EXAMPLES)

all: sdk-deps $(TOOLS) $(DOCS) $(TESTS) $(EXAMPLES)

subdirs: $(TOOLS) $(DOCS) $(TESTS) $(EXAMPLES)

tools: $(TOOLS)

# Set up environment vars before building examples
$(EXAMPLES):
	PATH="$(SDK_DIR)/bin:$(PATH)" SDK_DIR="$(SDK_DIR)" make -C $@

$(TOOLS) $(DOCS) $(TESTS):
	@$(MAKE) -C $@

clean: sdk-deps-clean
	rm -Rf sdk/doc/*
	@for dir in $(TOOLS) $(DOCS); do $(MAKE) -C $$dir clean; done
	PATH="$(SDK_DIR)/bin:$(PATH)" SDK_DIR="$(SDK_DIR)" make -C $(EXAMPLES) clean

include Makefile.sdk-deps
