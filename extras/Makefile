SUBDIRS = \
	hello \
	nyan \
	pantera \
	monster \
	loopy \
	thrasher \
	synth

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
