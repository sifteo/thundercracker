TOOLS = emulator firmware stir speex
SUBDIRS = $(TOOLS) sdk

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

tools: $(TOOLS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
