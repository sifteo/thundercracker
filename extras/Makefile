SUBDIRS = \
	nyan \
	pantera \
	monster \
	loopy \
	abort \
	kersplody \
	thrasher \
	trackycat \
	tinytracker \
	lfsmunch \
	numbers \
	drumkit

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
