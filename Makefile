TC_DIR := .
include Makefile.platform

TOOLS = emulator stir vm firmware
SUBDIRS = $(TOOLS) docs/doxygen sdk/examples

.PHONY: clean subdirs sdk-deps $(SUBDIRS)

all: sdk-deps $(SUBDIRS)

subdirs: $(SUBDIRS)

tools: $(TOOLS)

$(SUBDIRS):
	@$(MAKE) -C $@

# Stage the platform-specific SDK dependencies
sdk-deps: sdk/bin/arm-clang$(BIN_EXT)

sdk/bin/arm-clang$(BIN_EXT): $(LLVM_BIN)/arm-clang$(BIN_EXT)
	install -s $< $@

clean:
	rm -Rf sdk/doc/*
	rm -Rf sdk/bin/*
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
