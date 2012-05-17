TC_DIR := .
SDK_DIR := $(abspath $(TC_DIR)/sdk)
include Makefile.platform

USERSPACE_DEPS := vm stir firmware
TEST_DEPS := emulator
DOCS := docs/doxygen
USERSPACE := launcher sdk/examples extras
TESTS := test

# Build order matters
ALL_SUBDIRS := $(USERSPACE_DEPS) $(DOCS) $(USERSPACE) $(TEST_DEPS) $(TESTS)
NONUSER_SUBDIRS := $(USERSPACE_DEPS) $(DOCS) $(TEST_DEPS) $(TESTS)

.PHONY: clean _userspace_clean $(ALL_SUBDIRS)

all: sdk-deps $(ALL_SUBDIRS)

# Set up SDK environment vars for userspace code
$(USERSPACE):
	@PATH="$(SDK_DIR)/bin:$(PATH)" SDK_DIR="$(SDK_DIR)" make -C $@

# All other subdirs are a normal make invocation
$(NONUSER_SUBDIRS):
	@$(MAKE) -C $@

clean: sdk-deps-clean
	rm -Rf sdk/doc/*
	@for dir in $(NONUSER_SUBDIRS); do $(MAKE) -C $$dir clean; done
	@PATH="$(SDK_DIR)/bin:$(PATH)" SDK_DIR="$(SDK_DIR)" make _userspace_clean

# Internal target for 'clean', with userspace environment vars set up. I couldn't
# see a better way to set up environment vars and do the 'for' loop in one step.
_userspace_clean:
	@for dir in $(USERSPACE); do $(MAKE) -C $$dir clean; done

include Makefile.sdk-deps
