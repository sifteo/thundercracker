SUBDIRS = \
	hello \
	nyan \
	pantera \
	monster \
	loopy \
	thrasher \
	trackycat \
	tinytracker \
	lfsmunch

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
