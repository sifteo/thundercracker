SUBDIRS = \
	hello text gfxdemo chroma nyan sensors \
	scroller-bg0 scroller-bg1 sprites stars tiltShake \
	pairing word

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
