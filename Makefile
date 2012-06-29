TC_DIR := .
SDK_DIR := $(abspath $(TC_DIR)/sdk)
include Makefile.platform

USERSPACE_DEPS := vm stir firmware
TEST_DEPS := emulator
DOCS := docs/doxygen
USERSPACE := launcher sdk/examples extras
TESTS := test
TOOLS := tools/fwdeploy swiss

# Default parallelization for make. Override on the command line
PARALLEL := -j 4

# Build order matters
ALL_SUBDIRS := $(USERSPACE_DEPS) $(DOCS) $(TOOLS) $(USERSPACE) $(TEST_DEPS) $(TESTS)
NONUSER_SUBDIRS := $(USERSPACE_DEPS) $(DOCS) $(TOOLS) $(TEST_DEPS) $(TESTS)

.PHONY: clean _userspace_clean $(ALL_SUBDIRS)

all: sdk-deps $(ALL_SUBDIRS)

# Set up SDK environment vars for userspace code
# NOTE: We totally replace $PATH here, as well as in 'clean' below,
# and in the similar build rules in our SDK tests. This is primarily to
# work around a bug in our 'make' on Windows when parenthesis are in $PATH,
# but it also helps keep us honest about our SDK build being self-contained.
$(USERSPACE):
	@PATH="$(SDK_DIR)/bin:/bin:/usr/bin:/usr/local/bin" SDK_DIR="$(SDK_DIR)" make -C $@

# Plain subdir builds (Don't parallelize tests, docs, firmware)
$(DOCS) $(TESTS) firmware $(TOOLS):
	@$(MAKE) -C $@

# Parallelize our large builds
vm stir emulator:
	@$(MAKE) $(PARALLEL) -C $@

clean: sdk-deps-clean docs-clean nonuser-clean userspace-clean

.PHONY: sdk-deps-clean docs-clean nonuser-clean userspace-clean

docs-clean:
	rm -Rf sdk/doc/*

nonuser-clean:
	@for dir in $(NONUSER_SUBDIRS); do $(MAKE) -C $$dir clean; done

userspace-clean:
	@PATH="$(SDK_DIR)/bin:/bin:/usr/bin:/usr/local/bin" SDK_DIR="$(SDK_DIR)" make _userspace_clean

# Internal target for 'clean', with userspace environment vars set up. I couldn't
# see a better way to set up environment vars and do the 'for' loop in one step.
_userspace_clean:
	@for dir in $(USERSPACE); do $(MAKE) -C $$dir clean; done

include Makefile.sdk-deps
