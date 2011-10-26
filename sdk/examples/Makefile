SUBDIRS = \
	hello monster text gfxdemo chroma neighbor megademo \
	scroller-bg0 scroller-bg1 sprites stars

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
