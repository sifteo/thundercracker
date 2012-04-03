SUBDIRS = \
	betterflow \
	chroma \
	hello \
	membrane \
	monster \
	nyan \
	pairing \
	peano \
	sandwich \
	scroller-bg0 \
	scroller-bg1 \
	sensors \
	sprites \
	stars \
	text \
	word \

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
