TC_DIR := .
include Makefile.platform

TOOLS = emulator stir vm firmware
SUBDIRS = $(TOOLS) docs/doxygen sdk/examples

# List of LLVM binaries to stage with the SDK
LLVM_SDK_BINS := \
	$(LLVM_BIN)/arm-clang$(BIN_EXT)

.PHONY: clean subdirs sdk-deps $(SUBDIRS)

all: sdk-deps $(SUBDIRS)

subdirs: $(SUBDIRS)

tools: $(TOOLS)

$(SUBDIRS):
	@$(MAKE) -C $@

# Stage the platform-specific SDK dependencies
sdk-deps:
	install -s $(LLVM_SDK_BINS) sdk/bin

clean:
	rm -Rf sdk/doc/*
	rm -Rf sdk/bin/*
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
