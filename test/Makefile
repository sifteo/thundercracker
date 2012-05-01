# Top-level makefile for running all unit tests.
#
# These tests all use different systems and run in different environments, so
# we drive everything with per-test makefiles.
#
# To run an individual test, do "make <test-name>", where <test-name> is the
# same string found in the TESTS variable:

TESTS = \
	firmware/cube \
	sdk/math \
	sdk/bg1 \
	sdk/sprites

####################

TC_DIR := $(abspath ..)
SDK_DIR := $(TC_DIR)/sdk

.PHONY: clean _clean tests $(TESTS)

tests: $(TESTS)

$(TESTS):
	@PATH="$(SDK_DIR)/bin:$(PATH)" TC_DIR="$(TC_DIR)" SDK_DIR="$(SDK_DIR)" $(MAKE) -C $@

clean:
	@PATH="$(SDK_DIR)/bin:$(PATH)" TC_DIR="$(TC_DIR)" SDK_DIR="$(SDK_DIR)" $(MAKE) _clean

# Internal target for 'clean', with environment vars set up. I couldn't
# see a better way to set up environment vars and do the 'for' loop in one step.
_clean:
	@for dir in $(TESTS); do $(MAKE) -C $$dir clean; done
