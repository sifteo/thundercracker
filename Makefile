TC_DIR := .
SDK_DIR := $(abspath $(TC_DIR)/sdk)
include Makefile.platform

TOOLS := emulator stir vm firmware
DOCS := docs/doxygen
USERSPACE := sdk/examples extras
TESTS := test

.PHONY: clean userspace-clean subdirs $(TOOLS) $(DOCS) $(TESTS) $(USERSPACE)

all: sdk-deps $(TOOLS) $(DOCS) $(TESTS) $(USERSPACE)

subdirs: $(TOOLS) $(DOCS) $(TESTS) $(USERSPACE)

tools: $(TOOLS)

# Set up SDK environment vars for userspace code
$(USERSPACE):
	@PATH="$(SDK_DIR)/bin:$(PATH)" SDK_DIR="$(SDK_DIR)" make -C $@

$(TOOLS) $(DOCS) $(TESTS):
	@$(MAKE) -C $@

clean: sdk-deps-clean
	rm -Rf sdk/doc/*
	@for dir in $(TOOLS) $(DOCS); do $(MAKE) -C $$dir clean; done
	@PATH="$(SDK_DIR)/bin:$(PATH)" SDK_DIR="$(SDK_DIR)" make _userspace_clean

# Internal target for 'clean', with userspace environment vars set up. I couldn't
# see a better way to set up environment vars and do the 'for' loop in one step.
_userspace_clean:
	@for dir in $(USERSPACE); do $(MAKE) -C $$dir clean; done

include Makefile.sdk-deps
