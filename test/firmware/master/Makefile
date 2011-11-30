SUBDIRS = cubetest radiotest

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

tools: $(TOOLS)

$(SUBDIRS):
	@$(MAKE) -C $@

run:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir run; done

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
