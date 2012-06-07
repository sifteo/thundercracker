# Top-level makefile for running all unit tests.
#
# These tests all use different systems and run in different environments, so
# we drive everything with per-test makefiles.
#
# To run an individual test, do "make <test-name>", where <test-name> is the
# same string found in the TESTS variable:

TC_DIR := ..
include $(TC_DIR)/Makefile.platform

#############################################################

# Cross-platform tests
TESTS = \
	sdk/array \
	sdk/compatibility \
	sdk/crc \
	sdk/math \
	sdk/filesystem \
	sdk/bg0rom \
	sdk/bg1 \
	sdk/sprites \
	sdk/tilebuffer \
	sdk/scripting \
	sdk/stir

# Mac-only tests
ifeq ($(BUILD_PLATFORM), Darwin)
    TESTS += sdk/mac-open
endif

# Tests that are broken on Windows currently
ifneq ($(BUILD_PLATFORM), windows32)
    TESTS += \
	firmware/cube \
	firmware/master \
	sdk/adpcm \
	sdk/tracker
endif

#############################################################

TC_DIR := $(abspath ..)
SDK_DIR := $(TC_DIR)/sdk

.PHONY: clean _clean tests $(TESTS)

tests: $(TESTS)

$(TESTS):
	@PATH="$(SDK_DIR)/bin:/bin:/usr/bin:/usr/local/bin" TC_DIR="$(TC_DIR)" SDK_DIR="$(SDK_DIR)" $(MAKE) -C $@

clean:
	@PATH="$(SDK_DIR)/bin:/bin:/usr/bin:/usr/local/bin" TC_DIR="$(TC_DIR)" SDK_DIR="$(SDK_DIR)" $(MAKE) _clean

# Internal target for 'clean', with environment vars set up. I couldn't
# see a better way to set up environment vars and do the 'for' loop in one step.
_clean:
	@for dir in $(TESTS); do $(MAKE) -C $$dir clean; done
