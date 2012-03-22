SUBDIRS = \
	betterflow \
	buddies \
	chroma \
	gfxdemo \
	hello \
	membrane \
	monster \
	nyan \
	pairing \
    scroller-bg0 \
	scroller-bg1 \
	sensors \
	sprites \
	stars \
	text \
	tiltshake \

# XXX Disabled for now due to errors: peano, sandwich, word

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
